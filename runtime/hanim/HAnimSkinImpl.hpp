// HAnimSkinImpl.hpp — reference H-Anim deformation, included by HAnimSkin.hpp.
#ifndef X3D_RUNTIME_HANIM_SKIN_IMPL_HPP
#define X3D_RUNTIME_HANIM_SKIN_IMPL_HPP

#include "HAnimSkin.hpp" // permits this implementation header's isolation check
#include "TransformSystem.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/CoordinateDouble.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimSegment.hpp"
#include "x3d/nodes/HAnimDisplacer.hpp"
#include <algorithm>
#include <any>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime::hanim {
namespace detail {
using namespace x3d::nodes;
using namespace x3d::core;
using Ptr = std::shared_ptr<X3DNode>;
inline std::vector<Ptr> nodes(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields())
    if (f.x3dName == name && f.get && f.type == X3DFieldType::MFNode)
      return std::any_cast<std::vector<Ptr>>(f.get(n));
  return {};
}
inline Ptr node(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields())
    if (f.x3dName == name && f.get && f.type == X3DFieldType::SFNode)
      return std::any_cast<Ptr>(f.get(n));
  return {};
}
inline std::vector<SFVec3f> points(const Ptr &n) {
  if (auto *c = dynamic_cast<const Coordinate *>(n.get())) return c->getPoint();
  std::vector<SFVec3f> out;
  if (auto *c = dynamic_cast<const CoordinateDouble *>(n.get()))
    for (const auto &p : c->getPoint()) out.push_back({float(p.x), float(p.y), float(p.z)});
  return out;
}
inline SFVec3f add(SFVec3f a, SFVec3f b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline SFVec3f mul(SFVec3f a, float s) { return {a.x*s,a.y*s,a.z*s}; }
inline SFVec3f unit(SFVec3f a, SFVec3f fallback) {
  float length = std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
  return length > 0 && std::isfinite(length) ? mul(a,1/length) : fallback;
}
inline SFVec3f normalByInverse(const Mat4 &m, SFVec3f n) {
  return {m.m[0]*n.x+m.m[1]*n.y+m.m[2]*n.z,
          m.m[4]*n.x+m.m[5]*n.y+m.m[6]*n.z,
          m.m[8]*n.x+m.m[9]*n.y+m.m[10]*n.z};
}
// Skin one bind-pose normal with the influences of source coordinate
// `coordinate`: normalize(sum w * (D_j^-T)3x3 * n). Used for a mesh corner,
// whose normal follows the corner's coordinate whatever its normal index.
inline SFVec3f deformNormalWithCoordinate(const SkinBinding &b,
                                          const std::vector<Mat4> &inversePalette,
                                          std::uint32_t coordinate, SFVec3f bind) {
  const std::size_t source = coordinate;
  if (source + 1 >= b.influenceOffset.size()) return bind;
  const auto first = b.influenceOffset[source];
  const auto last = b.influenceOffset[source + 1];
  if (first == last) return bind;
  SFVec3f sum{0,0,0};
  for (auto k = first; k < last; ++k) {
    const auto &in = b.influences[k];
    if (in.joint < inversePalette.size())
      sum = add(sum, mul(normalByInverse(inversePalette[in.joint], bind), in.weight));
  }
  return unit(sum, bind);
}
inline SFVec3f deformCornerNormal(const SkinBinding &b,
                                 const std::vector<Mat4> &inversePalette,
                                 std::uint32_t coordinate, std::uint32_t normal) {
  if (normal >= b.bindNormals.size()) return {};
  return deformNormalWithCoordinate(b, inversePalette, coordinate, b.bindNormals[normal]);
}
} // namespace detail

inline SkinBinding compileBinding(const X3DNode &humanoid) {
  using namespace detail;
  SkinBinding b;
  b.humanoid = &humanoid;
  auto *h = dynamic_cast<const HAnimHumanoid *>(&humanoid);
  if (!h) { b.influenceOffset={0}; return b; }
  // X3D §26.3.2 / 19774-1 §6.2: authored binding sources take precedence.
  auto sourceCoord=node(*h,"skinCoord"), bindCoord=node(*h,"skinBindingCoords");
  b.bindPositions=points(bindCoord ? bindCoord : sourceCoord);
  auto sourceNormal=node(*h,"skinNormal"), bindNormal=node(*h,"skinBindingNormals");
  auto *normal=dynamic_cast<const Normal *>((bindNormal ? bindNormal : sourceNormal).get());
  if (normal) b.bindNormals=normal->getVector();
  if (bindCoord && b.bindPositions.size()!=points(sourceCoord).size())
    b.diagnostics.push_back("skinBindingCoords length mismatch");
  if (bindNormal) {
    auto *src=dynamic_cast<const Normal *>(sourceNormal.get());
    if (src && b.bindNormals.size()!=src->getVector().size())
      b.diagnostics.push_back("skinBindingNormals length mismatch");
  }
  std::unordered_set<const X3DNode *> seen;
  for (const auto &n : h->getJoints())
    if (dynamic_cast<const HAnimJoint *>(n.get()) && seen.insert(n.get()).second)
      b.joints.push_back(n.get());
  std::unordered_set<const X3DNode *> visited;
  auto walk = [&](auto &&self, const Ptr &n) -> void {
    if (!n || !visited.insert(n.get()).second) return;
    if (auto *j=dynamic_cast<const HAnimJoint *>(n.get()))
      if (!j->getSkinCoordIndex().empty() && seen.insert(n.get()).second) b.joints.push_back(n.get());
    for (const auto &c : nodes(*n,"children")) self(self,c);
  };
  for (const auto &n : h->getSkeleton()) walk(walk,n);
  const auto &bp=h->getJointBindingPositions();
  const auto &br=h->getJointBindingRotations();
  const auto &bs=h->getJointBindingScales();
  const bool authoredBinding=!bp.empty() || !br.empty() || !bs.empty();
  // 19774-1 §6.2: the binding values are applied "to the corresponding Joint
  // objects", i.e. they replace each Joint's own translation/rotation/scale, so
  // a Joint's bind matrix composes with its skeleton parents' bind matrices. A
  // single value applies to all; otherwise by position in the joints list.
  auto localBind=[&](const X3DNode *n) {
    auto *joint=dynamic_cast<const HAnimJoint *>(n);
    if (!joint) return TransformSystem::isTransform(n) ? TransformSystem::localMatrix(n) : Mat4::identity();
    auto it=std::find_if(h->getJoints().begin(),h->getJoints().end(),
                         [&](const auto &p){return p.get()==n;});
    size_t slot=it==h->getJoints().end() ? h->getJoints().size() : size_t(it-h->getJoints().begin());
    auto value=[slot](const auto &a, const auto &fallback) {
      return a.empty() ? fallback : a.size()==1 ? a.front() : slot<a.size() ? a[slot] : fallback;
    };
    return transformMatrix(value(bp,SFVec3f{0,0,0}),value(br,SFRotation{0,0,1,0}),
                           value(bs,SFVec3f{1,1,1}),joint->getCenter(),joint->getScaleOrientation());
  };
  std::unordered_map<const X3DNode *,Mat4> bindWorld;
  if (authoredBinding) {
    std::unordered_set<const X3DNode *> bound;
    auto bindWalk=[&](auto &&self,const Ptr &n,const Mat4 &parent)->void {
      if (!n || !bound.insert(n.get()).second) return;
      Mat4 m=parent*localBind(n.get());
      bindWorld.emplace(n.get(),m);
      for (const auto &c : nodes(*n,"children")) self(self,c,m);
    };
    for (const auto &n : h->getSkeleton()) bindWalk(bindWalk,n,Mat4::identity());
  }
  for (const auto *joint : b.joints) {
    if (!authoredBinding) { b.inverseBind.push_back(Mat4::identity()); continue; }
    auto it=bindWorld.find(joint);
    b.inverseBind.push_back((it==bindWorld.end() ? localBind(joint) : it->second).inverse());
  }
  for (const auto &[name,count] : {std::pair{"jointBindingPositions",bp.size()},
                                   {"jointBindingRotations",br.size()},
                                   {"jointBindingScales",bs.size()}})
    if (count>1 && count!=h->getJoints().size())
      b.diagnostics.push_back(std::string(name)+" length mismatch");
  std::vector<std::vector<Influence>> perVertex(b.bindPositions.size());
  for (size_t j=0;j<b.joints.size();++j) {
    auto *joint=dynamic_cast<const HAnimJoint *>(b.joints[j]);
    const auto &ids=joint->getSkinCoordIndex();
    const auto &weights=joint->getSkinCoordWeight();
    if (ids.size()!=weights.size()) b.diagnostics.push_back("skinCoordIndex/skinCoordWeight length mismatch");
    for (size_t k=0;k<std::min(ids.size(),weights.size());++k) {
      if (ids[k]<0 || size_t(ids[k])>=perVertex.size()) {
        b.diagnostics.push_back("skinCoordIndex out of range"); continue;
      }
      if (weights[k]<0 || !std::isfinite(weights[k]))
        b.diagnostics.push_back("invalid skinCoordWeight");
      perVertex[ids[k]].push_back({std::uint32_t(j),weights[k]});
    }
    for (const auto &n : joint->getDisplacers()) {
      auto *d=dynamic_cast<const HAnimDisplacer *>(n.get());
      if (!d) continue;
      JointDisplacer out; out.node=n.get(); out.joint=std::uint32_t(j);
      const auto &di=d->getCoordIndex();
      const auto &ds=d->getDisplacements();
      if (di.size()!=ds.size()) b.diagnostics.push_back("displacer index/displacement length mismatch");
      for (size_t k=0;k<std::min(di.size(),ds.size());++k) {
        if (di[k]<0 || size_t(di[k])>=perVertex.size()) {
          b.diagnostics.push_back("displacer index out of range"); continue;
        }
        out.index.push_back(std::uint32_t(di[k])); out.displacement.push_back(ds[k]);
      }
      b.displacers.push_back(std::move(out));
    }
  }
  b.influenceOffset.push_back(0);
  for (auto &v : perVertex) {
    if (!v.empty()) {
      float sum=0; for (const auto &in : v) sum+=in.weight;
      if (!(sum>0) || !std::isfinite(sum)) {
        b.diagnostics.push_back("non-positive influence sum"); v.clear();
      } else for (auto &in : v) in.weight/=sum;
    }
    b.influences.insert(b.influences.end(),v.begin(),v.end());
    b.influenceOffset.push_back(std::uint32_t(b.influences.size()));
  }
  return b;
}

inline SkinPose evaluatePose(const SkinBinding &b) {
  using namespace detail;
  SkinPose pose;
  std::unordered_map<const X3DNode *,Mat4> matrices;
  std::unordered_set<const X3DNode *> visited;
  auto walk=[&](auto &&self,const Ptr &n,const Mat4 &parent)->void {
    if (!n || !visited.insert(n.get()).second) return;
    Mat4 m=TransformSystem::isTransform(n.get()) ? parent*TransformSystem::localMatrix(n.get()) : parent;
    matrices.emplace(n.get(),m);
    for (const auto &c : nodes(*n,"children")) self(self,c,m);
  };
  if (b.humanoid) for (const auto &n : nodes(*b.humanoid,"skeleton"))
    walk(walk,n,Mat4::identity()); // Humanoid's own TRS is excluded.
  for (size_t j=0;j<b.joints.size();++j) {
    auto it=matrices.find(b.joints[j]);
    Mat4 m=it==matrices.end() ? TransformSystem::localMatrix(b.joints[j]) : it->second;
    pose.jointMatrix.push_back(m);
    pose.palette.push_back(m*b.inverseBind[j]);
  }
  for (const auto &d : b.displacers) {
    auto *n=dynamic_cast<const HAnimDisplacer *>(d.node);
    pose.displacerWeight.push_back(n ? n->getWeight() : 0);
  }
  return pose;
}

inline void deform(const SkinBinding &b,const SkinPose &pose,std::vector<SFVec3f> &positions,
                   std::vector<SFVec3f> *normals) {
  using namespace detail;
  positions=b.bindPositions;
  std::vector<Mat4> inverse;
  if (normals) {
    *normals=b.bindNormals;
    for (const auto &m : pose.palette) inverse.push_back(m.inverse());
  }
  for (size_t i=0;i<positions.size() && i+1<b.influenceOffset.size();++i) {
    auto first=b.influenceOffset[i], last=b.influenceOffset[i+1];
    if (first==last) continue;
    SFVec3f p{0,0,0},n{0,0,0};
    for (size_t k=first;k<last;++k) {
      const auto &in=b.influences[k];
      if (in.joint>=pose.palette.size()) continue;
      p=add(p,mul(pose.palette[in.joint].transformPoint(b.bindPositions[i]),in.weight));
      if (normals && i<normals->size())
        n=add(n,mul(normalByInverse(inverse[in.joint],b.bindNormals[i]),in.weight));
    }
    positions[i]=p;
    if (normals && i<normals->size()) (*normals)[i]=unit(n,b.bindNormals[i]);
  }
  // 19774-1 §6.6: Joint offsets follow skinning and use joint-local directions.
  for (size_t d=0;d<b.displacers.size() && d<pose.displacerWeight.size();++d) {
    const auto &item=b.displacers[d];
    if (item.joint>=pose.jointMatrix.size()) continue;
    for (size_t k=0;k<item.index.size();++k)
      if (item.index[k]<positions.size())
        positions[item.index[k]]=add(positions[item.index[k]],
          mul(pose.jointMatrix[item.joint].transformDirection(item.displacement[k]),pose.displacerWeight[d]));
  }
}

inline bool displaceSegmentPoints(const X3DNode &segmentNode, const X3DNode &coordNode,
                                  std::vector<SFVec3f> &points) {
  using namespace detail;
  auto *segment=dynamic_cast<const HAnimSegment *>(&segmentNode);
  if (!segment || segment->getCoord().get()!=&coordNode) return false;
  bool changed=false;
  // §26.3.1: Segment displacements are in Segment coordinates.
  for (const auto &n : segment->getDisplacers()) {
    auto *d=dynamic_cast<const HAnimDisplacer *>(n.get());
    if (!d || d->getWeight()==0) continue;
    const auto &ids=d->getCoordIndex();
    const auto &ds=d->getDisplacements();
    for (size_t k=0;k<std::min(ids.size(),ds.size());++k)
      if (ids[k]>=0 && size_t(ids[k])<points.size()) {
        points[ids[k]]=add(points[ids[k]],mul(ds[k],d->getWeight())); changed=true;
      }
  }
  return changed;
}
} // namespace x3d::runtime::hanim
#endif
