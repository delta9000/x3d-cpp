// ViewDependentSystem.hpp — M2e: viewer-dependent scene-graph evaluation.
// Run each tick() from the bound Viewpoint. Owns LOD level_changed tracking and
// ProximitySensor/VisibilitySensor enter/exit. Render-time selection (LOD child,
// Billboard rotation) lives in the per-path walk of SceneExtractor/PickSystem and
// uses the free helpers at the bottom of this header.
#ifndef X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP
#define X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP

#include "FieldRead.hpp"
#include "Billboard.hpp"
#include "GeometryBounds.hpp"
#include "GeoNodes.hpp"
#include "LODSelection.hpp"
#include "Mat4.hpp"
#include "RecursionLimits.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include <algorithm>
#include <cmath>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

// Number of nodes in a node's `children` MFNode slot (0 if absent).
inline std::size_t lodChildCount(const X3DNode &n) {
  for (const auto &f : n.fields())
    if (f.x3dName == "children" && f.type == X3DFieldType::MFNode) {
      FieldRef<std::vector<std::shared_ptr<X3DNode>>> c(n, f);
      return c ? c->size() : 0;
    }
  return 0;
}

class ViewDependentSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    const std::string t = node ? node->nodeTypeName() : "";
    if (t == "LOD" || t == "GeoLOD") lodLevel_.emplace(node, -1);
    else if (t == "ProximitySensor" || t == "GeoProximitySensor" ||
             t == "VisibilitySensor" || t == "TransformSensor") {
      sensorActive_.emplace(node, false);
      sensorBelowValid_ = false;
    }
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    lodLevel_.erase(node);
    sensorActive_.erase(node);
    proxState_.erase(node);
    trSensorState_.erase(node);
    motion_.erase(node);
    sensorPaths_.erase(node);
    sensorBelow_.erase(node);
    sensorBelowValid_ = false;
  }

  // A capped path walk may leave some sensor instances unevaluated this tick.
  bool budgetExceeded() const { return budgetExceeded_; }

  // Test/observer seam: invoked when an LOD's announced level changes.
  void setLevelChangedHook(std::function<void(X3DNode *, int)> h) { levelHook_ = std::move(h); }

  // Test/observer seam: (sensor, isActive, eventTime) on each edge.
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h) { sensorHook_ = std::move(h); }

  // Consumer-supplied exact view volume (frustum). VisibilitySensor tests the
  // sensor box against the six planes of the bound viewpoint's frustum; the
  // consumer supplies the camera aspect ratio (default 1.0) so a wide-aspect
  // periphery is not reported invisible (ENV-05). Renderers set this each frame
  // from their camera.
  struct ViewVolume { bool valid=false; float aspect=1.0f; };
  void setViewVolume(const ViewVolume &vv) { viewVolume_ = vv; }

  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    const SFVec3f eye = ctx.cameraWorldPosition();
    for (auto &[node, last] : lodLevel_) {
      if (node->nodeTypeName() == "GeoLOD") {
        // §25.3.4: range is measured in metres from the geospatial center.
        SFVec3f centerWorld;
        const SFVec3d authored = geombounds::getField<SFVec3d>(*node, "center", {0, 0, 0});
        if (!geo::toWorld(*node, authored, centerWorld)) continue;
        const SFVec3f eyeLocal = ctx.worldTransform(node).inverse().transformPoint(eye);
        const float range = geombounds::getField<float>(*node, "range", 10.0f);
        const int lvl = viewdep::len(viewdep::sub(eyeLocal, centerWorld)) < range ? 1 : 0;
        if (lvl != last) {
          last = lvl;
          const auto root = geombounds::getField<std::vector<std::shared_ptr<X3DNode>>>(
              *node, "rootNode", {});
          // Child-URL tiles are not loaded yet (GEOLOD-1): until they are, the
          // root tile stays the displayed content at level 1 too.
          const auto &children = root;
          ctx.postEvent(node, "children", std::any(children));
          ctx.postEvent(node, "level_changed", std::any(static_cast<SFInt32>(lvl)));
          ctx.markActiveChildChanged(node);
          if (levelHook_) levelHook_(node, lvl);
        }
        continue;
      }
      // Per-node convenience level via primary-path world transform (documented
      // per-node-event / per-path-render split, M2C-1). worldTransform(node) is
      // the first-path world matrix; identity if unknown.
      const Mat4 w = ctx.worldTransform(node);
      const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
      const SFVec3f eyeLocal = w.inverse().transformPoint(eye);
      const float d = viewdep::len(viewdep::sub(eyeLocal, center));
      int lvl = lodSelectLevel(*node, d);
      // LOD-1 §23.4.3: when there are fewer children than range bins, the last
      // child is used for all further ranges; level_changed reports the index of
      // the child actually rendered (matches the extractor's render-time clamp).
      const std::size_t childCount = lodChildCount(*node);
      if (childCount > 0 && lvl >= static_cast<int>(childCount))
        lvl = static_cast<int>(childCount) - 1;
      if (lvl != last) {
        last = lvl;
        ctx.postEvent(node, "level_changed", std::any(static_cast<SFInt32>(lvl)));
        // The rendered LOD level is computed (camera distance), not a settable
        // field, so it never reaches classifyDirty — mark the subtree dirty so
        // incremental delta() re-walks the LOD and swaps the active child.
        ctx.markActiveChildChanged(node);
        if (levelHook_) levelHook_(node, lvl);
      }
    }
    // ENV-06/SENSOR-SWITCH (ADR-0034): a sensor reachable only through a
    // non-selected Switch child or inactive LOD level is treated as removed from
    // the transformation hierarchy. Compute the active set once (roots -> active
    // children only) and deactivate any attached sensor not in it; re-enabling a
    // branch lets the per-sensor update re-fire enter on the next tick.
    sensorPaths_.clear();
    const bool gate = !ctx.sceneRoots().empty();
    budgetExceeded_ = false;
    if (gate) {
      bool structureDirty = false;
      for (const X3DNode *changed : ctx.dirtyTracker().changedNodes())
        if (ctx.dirtyTracker().flags(changed) & DirtyChildren) { structureDirty = true; break; }
      if (!sensorBelowValid_ || cachedCtx_ != &ctx ||
          cachedTransformRevision_ != ctx.transformRevision() || structureDirty)
        rebuildSensorBelow(ctx);
      collectActive(ctx);
    }
    else for (const auto &[node, active] : sensorActive_)
      sensorPaths_[node].push_back(ctx.worldTransformAny(node));
    for (auto &[node, active] : sensorActive_) {
      if (gate && !sensorPaths_.count(node)) {
        deactivateIfActive(node, active, now, ctx);
        continue;
      }
      if (node->nodeTypeName() == "ProximitySensor" ||
          node->nodeTypeName() == "GeoProximitySensor") updateProximity(node, active, now, ctx);
      else if (node->nodeTypeName() == "VisibilitySensor") updateVisibility(node, active, now, ctx);
      else if (node->nodeTypeName() == "TransformSensor") updateTransform(node, active, now, ctx);
    }
  }

private:
  std::function<void(X3DNode *, int)> levelHook_;
  std::function<void(X3DNode *, bool, double)> sensorHook_;

  // Cache potential sensor descendants across ticks. Include unselected children:
  // Switch/LOD selection is applied only by the active per-path walk.
  void rebuildSensorBelow(X3DExecutionContext &ctx) {
    sensorBelow_.clear();
    std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> parents;
    std::unordered_set<const X3DNode *> seen;
    std::vector<const X3DNode *> pending(ctx.sceneRoots().begin(), ctx.sceneRoots().end());
    while (!pending.empty()) {
      const X3DNode *n = pending.back(); pending.pop_back();
      if (!n || !seen.insert(n).second) continue;
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        if (c) {
          parents[c.get()].push_back(n);
          pending.push_back(c.get());
        }
      });
    }
    pending.clear();
    for (const X3DNode *n : seen)
      if (sensorActive_.count(const_cast<X3DNode *>(n)) && sensorBelow_.insert(n).second)
        pending.push_back(n);
    while (!pending.empty()) {
      const X3DNode *n = pending.back(); pending.pop_back();
      for (const X3DNode *parent : parents[n])
        if (sensorBelow_.insert(parent).second) pending.push_back(parent);
    }
    cachedCtx_ = &ctx;
    cachedTransformRevision_ = ctx.transformRevision();
    sensorBelowValid_ = true;
  }

  // §22.4.1: disabling an active sensor clears isActive without an exitTime.
  // A path removal still reports the exit edge (ADR-0034).
  void deactivateIfActive(X3DNode *node, bool &last, double now,
                          X3DExecutionContext &ctx, bool exitTime = true) {
    if (!last) return;
    last = false;
    motion_.erase(node); // ENV-08: stale viewer motion; next enter reports its own tick
    ctx.postEvent(node, "isActive", std::any(false));
    if (exitTime) ctx.postEvent(node, "exitTime", std::any(static_cast<SFTime>(now)));
    if (sensorHook_) sensorHook_(node, false, now);
  }

  // The `children` MFNode slot of a grouping node (empty if absent). Matches the
  // extractor's child selection (SceneExtractor::childrenOf).
  static const std::vector<std::shared_ptr<X3DNode>> &childrenOf(const X3DNode &n) {
    static const std::vector<std::shared_ptr<X3DNode>> kNone;
    for (const auto &f : n.fields())
      if (f.x3dName == "children" && f.type == X3DFieldType::MFNode)
        if (const auto *c = fieldPtr<std::vector<std::shared_ptr<X3DNode>>>(n, f))
          return *c;
    return kNone;
  }

  // ENV-06/SENSOR-SWITCH: collect every node on an ACTIVE transformation path
  // from the scene roots, mirroring the extractor's render-time cull — a Switch
  // descends only children[whichChoice], a LOD only the level the extractor would
  // choose (camera distance in the LOD's local frame). Union over DEF/USE.
  void collectActive(X3DExecutionContext &ctx) {
    std::unordered_set<const X3DNode *> seen;
    // Sensor walks can run more than once per tick during a route cascade.
    WalkBudget budget(100'000);
    for (const X3DNode *r : ctx.sceneRoots())
      collectActiveFrom(r, ctx, Mat4::identity(), seen, budget);
    budgetExceeded_ = budget.tripped;
  }
  void collectActiveFrom(const X3DNode *n, X3DExecutionContext &ctx,
                         Mat4 world, std::unordered_set<const X3DNode *> &seen,
                         WalkBudget &budget) {
    if (!n || !sensorBelow_.count(n) || !budget.spend()) return;
    if (seen.size() >= kMaxNestingDepth || !seen.insert(n).second) return;
    if (TransformSystem::isTransform(n)) world = world * TransformSystem::localMatrix(n);
    if (sensorActive_.count(const_cast<X3DNode *>(n))) {
      auto &paths = sensorPaths_[n];
      if (paths.empty() || paths.back().m != world.m) paths.push_back(world);
    }
    const std::string t = n->nodeTypeName();
    if (t == "Switch") {
      const int which = geombounds::getField<int>(*n, "whichChoice", -1);
      const auto &kids = childrenOf(*n);
      if (which >= 0 && which < static_cast<int>(kids.size()) && kids[which])
        collectActiveFrom(kids[which].get(), ctx, world, seen, budget);
    } else if (t == "LOD") {
      const auto &kids = childrenOf(*n);
      if (!kids.empty()) {
        const SFVec3f center = geombounds::getField<SFVec3f>(*n, "center", {0, 0, 0});
        const SFVec3f eyeLocal = world.inverse().transformPoint(ctx.cameraWorldPosition());
        const float d = viewdep::len(viewdep::sub(eyeLocal, center));
        int lvl = lodSelectLevel(*n, d);
        if (lvl >= static_cast<int>(kids.size())) lvl = static_cast<int>(kids.size()) - 1;
        if (kids[lvl]) collectActiveFrom(kids[lvl].get(), ctx, world, seen, budget);
      }
    } else {
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        collectActiveFrom(c.get(), ctx, world, seen, budget);
      });
    }
    seen.erase(n);
  }

  // ENV-08: fraction (0..1) along the straight world-space segment p0->p1 at
  // which the viewer crosses into (entering) or out of (exiting) the sensor box
  // [center ± size/2] in the sensor's local frame. Falls back to 0 (=> edge at
  // `now`) when the segment misses the box (teleport) or is degenerate.
  static float segmentBoxFraction(const SFVec3f &p0w, const SFVec3f &p1w,
                                  const Mat4 &inv, const SFVec3f &center,
                                  const SFVec3f &size, bool entering) {
    const SFVec3f p0 = inv.transformPoint(p0w);
    const SFVec3f p1 = inv.transformPoint(p1w);
    const SFVec3f lo{center.x - size.x * 0.5f, center.y - size.y * 0.5f,
                     center.z - size.z * 0.5f};
    const SFVec3f hi{center.x + size.x * 0.5f, center.y + size.y * 0.5f,
                     center.z + size.z * 0.5f};
    const float o[3] = {p0.x, p0.y, p0.z};
    const float e[3] = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    const float mn[3] = {lo.x, lo.y, lo.z};
    const float mx[3] = {hi.x, hi.y, hi.z};
    float tmin = 0.0f, tmax = 1.0f;
    for (int a = 0; a < 3; ++a) {
      if (std::fabs(e[a]) < 1e-9f) {
        if (o[a] < mn[a] || o[a] > mx[a]) return 0.0f; // parallel & outside slab
        continue;
      }
      float ta = (mn[a] - o[a]) / e[a];
      float tb = (mx[a] - o[a]) / e[a];
      if (ta > tb) std::swap(ta, tb);
      tmin = std::max(tmin, ta);
      tmax = std::min(tmax, tb);
      if (tmin > tmax) return 0.0f;
    }
    const float t = entering ? tmin : tmax;
    return (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
  }

  // ENV-05/ENV-09 (§22.4.3): visible iff the sensor's box — center/size transformed
  // to world by `w` (its full world matrix, so ancestor scale counts) — is not
  // entirely outside any of the six view-frustum planes built from the bound
  // viewpoint's fieldOfView (the smaller of the horizontal/vertical angles,
  // §23.4.6) and the consumer aspect.
  bool boxInFrustum(const Mat4 &w, const SFVec3f &center, const SFVec3f &size,
                    X3DExecutionContext &ctx) const {
    const SFVec3f eye = ctx.cameraWorldPosition();
    const Mat4 cam = ctx.viewMatrix().inverse();
    const SFVec3f f = viewdep::norm(cam.transformDirection(SFVec3f{0, 0, -1}));
    const SFVec3f u = viewdep::norm(ctx.cameraWorldUp());
    SFVec3f r = viewdep::norm(viewdep::cross(f, u));
    if (viewdep::len(r) < 1e-6f) r = viewdep::norm(viewdep::cross(f, SFVec3f{0, 0, 1}));
    X3DNode *vp = ctx.boundViewpoint();
    const float fov = vp ? geombounds::getField<float>(*vp, "fieldOfView", 0.7854f) : 0.7854f;
    float aspect = (viewVolume_.valid && viewVolume_.aspect > 0.0f) ? viewVolume_.aspect : 1.0f;
    const float half = fov * 0.5f;
    float tanH, tanV;
    if (aspect >= 1.0f) { tanV = std::tan(half); tanH = aspect * tanV; }
    else { tanH = std::tan(half); tanV = tanH / aspect; }
    // Inward normals of the five lateral/near planes (top/bottom/left/right/near).
    const SFVec3f planes[5] = {
        f,
        {f.x * tanH - r.x, f.y * tanH - r.y, f.z * tanH - r.z}, // right
        {f.x * tanH + r.x, f.y * tanH + r.y, f.z * tanH + r.z}, // left
        {f.x * tanV - u.x, f.y * tanV - u.y, f.z * tanV - u.z}, // top
        {f.x * tanV + u.x, f.y * tanV + u.y, f.z * tanV + u.z}, // bottom
    };
    const SFVec3f h{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
    for (const SFVec3f &p : planes) {
      bool allOutside = true;
      for (int i = 0; i < 8 && allOutside; ++i) {
        const SFVec3f lc{center.x + ((i & 1) ? h.x : -h.x),
                         center.y + ((i & 2) ? h.y : -h.y),
                         center.z + ((i & 4) ? h.z : -h.z)};
        const SFVec3f a = viewdep::sub(w.transformPoint(lc), eye);
        if (viewdep::dot(a, p) >= 0.0f) allOutside = false;
      }
      if (allOutside) return false; // box entirely outside this plane => invisible
    }
    return true;
  }

  static bool vecEq(const SFVec3f &a, const SFVec3f &b) {
    return std::fabs(a.x - b.x) < 1e-6f && std::fabs(a.y - b.y) < 1e-6f &&
           std::fabs(a.z - b.z) < 1e-6f;
  }
  static bool rotEq(const SFRotation &a, const SFRotation &b) {
    return std::fabs(a.x - b.x) < 1e-6f && std::fabs(a.y - b.y) < 1e-6f &&
           std::fabs(a.z - b.z) < 1e-6f && std::fabs(a.angle - b.angle) < 1e-6f;
  }

  static bool insideBox(const SFVec3f &p, const SFVec3f &center, const SFVec3f &size) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) return false; // zero-volume => inert (§22.4.1)
    return std::fabs(p.x - center.x) <= size.x * 0.5f &&
           std::fabs(p.y - center.y) <= size.y * 0.5f &&
           std::fabs(p.z - center.z) <= size.z * 0.5f;
  }

  // §22.4.1 orientation_changed: axis-angle that orients local -Z along `fwd`
  // and local +Y along `up`, in the sensor's coordinate system.
  static SFRotation lookAtRotation(const SFVec3f &fwd, const SFVec3f &up) {
    using namespace viewdep;
    const SFVec3f f = norm(fwd);
    if (len(f) < 1e-9f) return SFRotation{0, 0, 1, 0};
    const SFVec3f zAxis = {-f.x, -f.y, -f.z}; // camera looks down -Z, so +Z = -fwd
    SFVec3f xAxis = cross(up, zAxis);
    if (len(xAxis) < 1e-9f) xAxis = cross(SFVec3f{0, 1, 0}, zAxis);
    xAxis = norm(xAxis);
    const SFVec3f yAxis = cross(zAxis, xAxis);
    // Rotation-matrix (columns xAxis,yAxis,zAxis) -> axis-angle.
    const float trace = xAxis.x + yAxis.y + zAxis.z;
    const float cosT = std::max(-1.0f, std::min(1.0f, (trace - 1.0f) * 0.5f));
    const float angle = std::acos(cosT);
    if (angle < 1e-6f) return SFRotation{0, 0, 1, 0};
    SFVec3f ax{yAxis.z - zAxis.y, zAxis.x - xAxis.z, xAxis.y - yAxis.x};
    if (len(ax) < 1e-9f) return SFRotation{0, 1, 0, angle};
    ax = norm(ax);
    return SFRotation{ax.x, ax.y, ax.z, angle};
  }

  static SFVec3f viewMatrixForward(X3DExecutionContext &ctx) {
    return ctx.viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1});
  }

  void updateProximity(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) {
      deactivateIfActive(node, last, now, ctx, false);
      return;
    }
    const SFVec3f eyeWorld = ctx.cameraWorldPosition();
    const bool geographic = node->nodeTypeName() == "GeoProximitySensor";
    Mat4 tangent = Mat4::identity();
    if (geographic) {
      // §25.3.8: the sensor box is tangent to the ellipsoid at geoCenter.
      const SFVec3d geoCenter = geombounds::getField<SFVec3d>(*node, "geoCenter", {0, 0, 0});
      if (!geo::tangentFrameOf(*node, geoCenter, tangent)) {
        deactivateIfActive(node, last, now, ctx);
        return;
      }
    }
    const SFVec3f center = geographic ? SFVec3f{0, 0, 0} :
        geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    auto &pst = proxState_[node];
    // §22.4.1: DEF/USE boxes form a union; ADR-0034 limits it to active paths.
    bool inside = false;
    Mat4 w = pst.world;
    const auto &paths = sensorPaths_[node];
    for (std::size_t i = 0; i < paths.size(); ++i) {
      const Mat4 path = geographic ? paths[i] * tangent : paths[i];
      if (insideBox(path.inverse().transformPoint(eyeWorld), center, size)) {
        w = path;
        pst.pathIndex = i;
        inside = true;
        break;
      }
    }
    if (!inside && !paths.empty()) {
      w = paths[std::min(pst.pathIndex, paths.size() - 1)];
      if (geographic) w = w * tangent;
    }
    const Mat4 inv = w.inverse();
    const SFVec3f eyeLocal = inv.transformPoint(eyeWorld);
    if (inside || last) {
      // §22.4.1: pose outputs include the exit instant.
      const SFVec3f fwdLocal = viewdep::norm(inv.transformDirection(viewMatrixForward(ctx)));
      const SFVec3f upLocal = viewdep::norm(inv.transformDirection(ctx.cameraWorldUp()));
      const SFRotation ori = lookAtRotation(fwdLocal, upLocal);
      // ENV-04: emit only when the viewer pose actually changes, not every tick.
      if (!pst.has || !vecEq(eyeLocal, pst.pos)) {
        ctx.postEvent(node, "position_changed", std::any(eyeLocal));
        if (geographic) {
          // §25.3.8: pair each position_changed with its geoSystem coordinate.
          SFVec3d coords;
          // Geographic coordinates live in the sensor's local frame (ADR-0053):
          // ancestor transforms place the geo content, so undo them first.
          const Mat4 parent = w * tangent.inverse();
          if (geo::fromWorld(*node, parent.inverse().transformPoint(eyeWorld), coords))
            ctx.postEvent(node, "geoCoord_changed", std::any(coords));
        }
      }
      if (!pst.has || !rotEq(ori, pst.ori))
        ctx.postEvent(node, "orientation_changed", std::any(ori));
      pst.has = true;
      pst.pos = eyeLocal;
      pst.ori = ori;
      // §22.4.1: centerOfRotation_changed is available only with LOOKAT navigation.
      X3DNode *nav = ctx.boundNavigationInfo();
      const auto types = nav ? geombounds::getField<std::vector<std::string>>(*nav, "type", {})
                             : std::vector<std::string>{};
      if (std::find(types.begin(), types.end(), "LOOKAT") != types.end()) {
        X3DNode *vp = ctx.boundViewpoint();
        SFVec3f cor{0, 0, 0};
        if (vp && vp->nodeTypeName() == "GeoViewpoint")
          geo::toWorld(*vp, geo::fieldOf<SFVec3d>(*vp, "centerOfRotation", {0, 0, 0}), cor);
        else if (vp)
          cor = geombounds::getField<SFVec3f>(*vp, "centerOfRotation", {0, 0, 0});
        const SFVec3f corWorld = vp ? ctx.worldOf(vp).transformPoint(cor) : cor;
        const SFVec3f corLocal = inv.transformPoint(corWorld);
        if (!pst.corHas || !vecEq(corLocal, pst.cor))
          ctx.postEvent(node, "centerOfRotation_changed", std::any(corLocal));
        pst.corHas = true;
        pst.cor = corLocal;
      } else pst.corHas = false;
      pst.world = w;
    }
    // ENV-08 (§22.4.1): report the boundary-crossing time of the viewer's
    // straight-line motion between the two ticks, not merely the tick `now`.
    auto &ms = motion_[node];
    if (inside != last) {
      double edge = now;
      if (ms.has && now > ms.t)
        edge = ms.t + segmentBoxFraction(ms.eyeWorld, eyeWorld, inv, center, size, inside) *
                          (now - ms.t);
      last = inside;
      ctx.postEvent(node, "isActive", std::any(inside));
      ctx.postEvent(node, inside ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(edge)));
      if (sensorHook_) sensorHook_(node, inside, edge);
    }
    ms.has = true;
    ms.eyeWorld = eyeWorld;
    ms.t = now;
  }

  // §22.4.3 VisibilitySensor: visible iff the size-box (in the sensor's frame)
  // may intersect the viewing volume, tested against the six view-frustum planes
  // of the bound viewpoint (ENV-05) with the box transformed to world by its full
  // world matrix so ancestor scale counts (ENV-09). ENV-08: enter/exitTime use
  // the tick `now` — a visibility region has no single geometric boundary the
  // viewer crosses, unlike ProximitySensor's box.
  void updateVisibility(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) {
      deactivateIfActive(node, last, now, ctx, false);
      return;
    }
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    bool visible = false;
    if (size.x > 0 && size.y > 0 && size.z > 0) {
      const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
      // §22.4.3: visibility is the union of active DEF/USE instances.
      for (const Mat4 &w : sensorPaths_[node])
        if (boxInFrustum(w, center, size, ctx)) { visible = true; break; }
    }
    if (visible != last) {
      last = visible;
      ctx.postEvent(node, "isActive", std::any(visible));
      ctx.postEvent(node, visible ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(now)));
      if (sensorHook_) sensorHook_(node, visible, now);
    }
  }

  // §22.4.2 TransformSensor: tracks targetObject's world AABB against a
  // sensor-local box (center + size); fires isActive/enterTime/exitTime on
  // boundary transitions; while inside, fires position_changed /
  // orientation_changed relative to center in the sensor's local frame.
  void updateTransform(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) {
      deactivateIfActive(node, last, now, ctx); // ENV-07: disable deactivates
      return;
    }
    SFNode target = geombounds::getField<SFNode>(*node, "targetObject", nullptr);
    if (!target) {
      deactivateIfActive(node, last, now, ctx); // null target -> inert
      return;
    }
    const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) {
      deactivateIfActive(node, last, now, ctx); // zero-volume box -> inert
      return;
    }
    const Mat4 sw = ctx.worldTransform(node);
    const Mat4 swInv = sw.inverse();
    // worldTransformAny walks UP via TransformSystem's parent index, so a target
    // shared between targetObject (non-Transform path) and a Transform ancestor
    // (Transform-children path) still resolves through the Transform ancestor.
    const Mat4 tw = ctx.worldTransformAny(target.get());
    const Aabb tb = ctx.localBounds(target.get()).transformed(tw);
    const SFVec3f tc{(tb.min.x + tb.max.x) * 0.5f,
                     (tb.min.y + tb.max.y) * 0.5f,
                     (tb.min.z + tb.max.z) * 0.5f};
    const SFVec3f th{(tb.max.x - tb.min.x) * 0.5f,
                     (tb.max.y - tb.min.y) * 0.5f,
                     (tb.max.z - tb.min.z) * 0.5f};
    const SFVec3f sc = sw.transformPoint(center);
    const SFVec3f sh{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
    // Overlap test (AABB-vs-AABB in world): active iff the target's box
    // intersects the sensor's box (§22.4.2 "enters ... a region in space").
    const bool inside =
        std::fabs(tc.x - sc.x) <= sh.x + th.x &&
        std::fabs(tc.y - sc.y) <= sh.y + th.y &&
        std::fabs(tc.z - sc.z) <= sh.z + th.z;
    if (inside) {
      // Position: target's AABB center, in sensor's local frame, relative to center.
      const SFVec3f tcl = swInv.transformPoint(tc);
      const SFVec3f posLocal{tcl.x - center.x, tcl.y - center.y, tcl.z - center.z};
      // Orientation: target world transform expressed in sensor's local frame.
      Mat4 rel = swInv * tw;
      // Change-gate: emit only when pose actually changes (ENV-04 mirror).
      auto &st = trSensorState_[node];
      // The relative basis can contain scale and shear from either hierarchy.
      // A degenerate basis has no defined orientation, so preserve the last
      // valid value (identity before the first valid sample).
      const SFRotation oriLocal = orthonormalizeRotation(rel)
          ? rotationFromMatrix(rel) : (st.has ? st.ori : SFRotation{});
      if (!st.has || !vecEq(posLocal, st.pos))
        ctx.postEvent(node, "position_changed", std::any(posLocal));
      if (!st.has || !rotEq(oriLocal, st.ori))
        ctx.postEvent(node, "orientation_changed", std::any(oriLocal));
      st.has = true;
      st.pos = posLocal;
      st.ori = oriLocal;
    }
    if (inside != last) {
      last = inside;
      ctx.postEvent(node, "isActive", std::any(inside));
      ctx.postEvent(node, inside ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(now)));
      if (sensorHook_) sensorHook_(node, inside, now);
    }
  }

  // ENV-04/ENV-03: last emitted ProximitySensor pose and centerOfRotation.
  struct ProxState {
    bool has = false; SFVec3f pos{}; SFRotation ori{};
    bool corHas = false; SFVec3f cor{};
    Mat4 world = Mat4::identity();
    std::size_t pathIndex = 0;
  };

  // ENV-08: previous tick's viewer world position + time, for interpolating the
  // boundary-crossing enter/exit time.
  struct MotionState { bool has = false; SFVec3f eyeWorld{}; double t = 0.0; };

  // ENV-01: last emitted TransformSensor pose, for change-gating.
  struct TransformSensorState { bool has = false; SFVec3f pos{}; SFRotation ori{}; };

  ViewVolume viewVolume_{};
  std::unordered_map<X3DNode *, int> lodLevel_;      // last announced level
  std::unordered_map<X3DNode *, bool> sensorActive_; // last isActive
  std::unordered_map<X3DNode *, ProxState> proxState_;
  std::unordered_map<X3DNode *, TransformSensorState> trSensorState_;
  std::unordered_map<X3DNode *, MotionState> motion_;
  std::unordered_map<const X3DNode *, std::vector<Mat4>> sensorPaths_;
  std::unordered_set<const X3DNode *> sensorBelow_;
  const X3DExecutionContext *cachedCtx_ = nullptr;
  std::uint64_t cachedTransformRevision_ = 0;
  bool sensorBelowValid_ = false;
  bool budgetExceeded_ = false;
};

} // namespace x3d::runtime
#endif
