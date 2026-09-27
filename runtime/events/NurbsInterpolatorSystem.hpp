#ifndef X3D_RUNTIME_NURBS_INTERPOLATOR_SYSTEM_HPP
#define X3D_RUNTIME_NURBS_INTERPOLATOR_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "NurbsEval.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/CoordinateDouble.hpp"
#include "x3d/nodes/NurbsOrientationInterpolator.hpp"
#include "x3d/nodes/NurbsPositionInterpolator.hpp"
#include "x3d/nodes/NurbsSurfaceInterpolator.hpp"

#include <any>
#include <algorithm>
#include <cmath>

namespace x3d::runtime {
namespace xn = x3d::nodes;
namespace nurbs = x3d::runtime::extract::nurbs;

inline std::vector<x3d::core::SFVec3f> nurbsPoints(const x3d::core::SFNode& node) {
  if (auto* c = dynamic_cast<xn::Coordinate*>(node.get())) return c->getPoint();
  if (auto* c = dynamic_cast<xn::CoordinateDouble*>(node.get())) {
    std::vector<x3d::core::SFVec3f> points;
    for (const auto& p : c->getPoint()) points.push_back({(float)p.x,(float)p.y,(float)p.z});
    return points;
  }
  return {};
}

inline bool nurbsTangent(const nurbs::CurveDef& curve, double f, x3d::core::SFVec3f& tangent) {
  x3d::core::SFVec3f a, b;
  // Central difference in the fraction. The curve points are rounded to float,
  // so keep the step well clear of float spacing at large coordinates; the
  // O(delta^2) truncation error is negligible.
  const double delta = 1e-3;
  double lo = std::max(0.0, f-delta), hi = std::min(1.0, f+delta);
  if (!(hi > lo) || !nurbs::evalCurve(curve, lo, a) || !nurbs::evalCurve(curve, hi, b)) return false;
  tangent = {b.x-a.x,b.y-a.y,b.z-a.z};
  double len = std::sqrt((double)tangent.x*tangent.x+(double)tangent.y*tangent.y+(double)tangent.z*tangent.z);
  if (!(len > 1e-12) || !std::isfinite(len)) return false;
  tangent = {(float)(tangent.x/len),(float)(tangent.y/len),(float)(tangent.z/len)};
  return true;
}

inline x3d::core::SFRotation rotationFromPositiveZ(x3d::core::SFVec3f d) {
  float dot = d.z;
  if (dot < -0.999999f) return {1,0,0,3.14159265358979323846f};
  float ax = -d.y, ay = d.x, az = 0;
  float len = std::sqrt(ax*ax+ay*ay+az*az);
  if (len < 1e-7f) return {0,0,1,0};
  return {ax/len,ay/len,az,std::acos(std::clamp(dot,-1.0f,1.0f))};
}

class NurbsPositionInterpolatorSystem : public System {
public:
  void attach(X3DNode* node, X3DExecutionContext& ctx) override {
    auto* interp=dynamic_cast<xn::NurbsPositionInterpolator*>(node); if (!interp) return;
    interp->setOnSet_fractionHandler([&ctx,interp](const SFFloat& f) {
      nurbs::CurveDef c; c.cp=nurbsPoints(interp->getControlPoint()); c.w=interp->getWeight(); c.knot=interp->getKnot(); c.order=interp->getOrder();
      SFVec3f p; if (nurbs::evalCurve(c,f,p)) ctx.postEvent(interp,"value_changed",std::any(p));
    });
  }
  void detach(X3DNode* node, X3DExecutionContext&) override {
    if (auto* n=dynamic_cast<xn::NurbsPositionInterpolator*>(node)) n->setOnSet_fractionHandler({});
  }
};

class NurbsOrientationInterpolatorSystem : public System {
public:
  void attach(X3DNode* node, X3DExecutionContext& ctx) override {
    auto* interp=dynamic_cast<xn::NurbsOrientationInterpolator*>(node); if (!interp) return;
    interp->setOnSet_fractionHandler([&ctx,interp](const SFFloat& f) {
      nurbs::CurveDef c; c.cp=nurbsPoints(interp->getControlPoint()); c.w=interp->getWeight(); c.knot=interp->getKnot(); c.order=interp->getOrder();
      SFVec3f tangent; if (!nurbsTangent(c,f,tangent)) return;
      ctx.postEvent(interp,"value_changed",std::any(rotationFromPositiveZ(tangent)));
    });
  }
  void detach(X3DNode* node, X3DExecutionContext&) override {
    if (auto* n=dynamic_cast<xn::NurbsOrientationInterpolator*>(node)) n->setOnSet_fractionHandler({});
  }
};

class NurbsSurfaceInterpolatorSystem : public System {
public:
  void attach(X3DNode* node, X3DExecutionContext& ctx) override {
    auto* interp=dynamic_cast<xn::NurbsSurfaceInterpolator*>(node); if (!interp) return;
    interp->setOnSet_fractionHandler([&ctx,interp](const SFVec2f& uv) {
      nurbs::SurfaceDef s; s.cp=nurbsPoints(interp->getControlPoint()); s.w=interp->getWeight();
      s.uDim=interp->getUDimension(); s.vDim=interp->getVDimension(); s.uOrder=interp->getUOrder(); s.vOrder=interp->getVOrder(); s.uKnot=interp->getUKnot(); s.vKnot=interp->getVKnot();
      auto prepared=nurbs::detail::prepareSurface(s);
      if (prepared.uDim < prepared.uOrder || prepared.vDim < prepared.vOrder ||
          (int)prepared.cp.size() < prepared.uDim*prepared.vDim ||
          (int)prepared.uKnot.size()!=prepared.uDim+prepared.uOrder ||
          (int)prepared.vKnot.size()!=prepared.vDim+prepared.vOrder) return;
      for (std::size_t i=0;i<prepared.uKnot.size();++i)
        if (!std::isfinite(prepared.uKnot[i]) || (i && prepared.uKnot[i]<prepared.uKnot[i-1])) return;
      for (std::size_t i=0;i<prepared.vKnot.size();++i)
        if (!std::isfinite(prepared.vKnot[i]) || (i && prepared.vKnot[i]<prepared.vKnot[i-1])) return;
      if (!std::isfinite(uv.x) || !std::isfinite(uv.y)) return;
      double u0=prepared.uKnot[prepared.uOrder-1], u1=prepared.uKnot[prepared.uDim];
      double v0=prepared.vKnot[prepared.vOrder-1], v1=prepared.vKnot[prepared.vDim];
      if (!(u1>u0) || !(v1>v0)) return;
      auto sample=nurbs::detail::evalSurface(prepared.cp,prepared.w,prepared.uKnot,prepared.vKnot,prepared.uDim,prepared.vDim,prepared.uOrder,prepared.vOrder,
          u0+std::clamp((double)uv.x,0.0,1.0)*(u1-u0), v0+std::clamp((double)uv.y,0.0,1.0)*(v1-v0), prepared.weightMode);
      if (!std::isfinite(sample.p.x)||!std::isfinite(sample.p.y)||!std::isfinite(sample.p.z)||!std::isfinite(sample.n.x)||!std::isfinite(sample.n.y)||!std::isfinite(sample.n.z)) return;
      if (sample.n.x*sample.n.x+sample.n.y*sample.n.y+sample.n.z*sample.n.z < 1e-12f) return;
      ctx.postEvent(interp,"position_changed",std::any(sample.p));
      ctx.postEvent(interp,"normal_changed",std::any(sample.n));
    });
  }
  void detach(X3DNode* node, X3DExecutionContext&) override {
    if (auto* n=dynamic_cast<xn::NurbsSurfaceInterpolator*>(node)) n->setOnSet_fractionHandler({});
  }
};

} // namespace x3d::runtime
#endif
