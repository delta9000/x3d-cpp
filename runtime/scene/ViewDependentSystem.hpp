// ViewDependentSystem.hpp — M2e: viewer-dependent scene-graph evaluation.
// Run each tick() from the bound Viewpoint. Owns LOD level_changed tracking and
// ProximitySensor/VisibilitySensor enter/exit. Render-time selection (LOD child,
// Billboard rotation) lives in the per-path walk of SceneExtractor/PickSystem and
// uses the free helpers at the bottom of this header.
#ifndef X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP
#define X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP

#include "Billboard.hpp"
#include "GeometryBounds.hpp"
#include "Mat4.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>

namespace x3d::runtime {
using namespace x3d::core;

// Number of nodes in a node's `children` MFNode slot (0 if absent).
inline std::size_t lodChildCount(const X3DNode &n) {
  for (const auto &f : n.fields())
    if (f.x3dName == std::string("children") && f.get &&
        f.type == X3DFieldType::MFNode)
      return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(n)).size();
  return 0;
}

// §23.4.3 LOD step function L(d): level = number of ranges d meets-or-exceeds.
inline int lodSelectLevel(const X3DNode &lod, float distToCenter) {
  const auto range = geombounds::getField<std::vector<float>>(lod, "range", {});
  if (range.empty()) return 0;                 // empty -> browser choice: highest detail
  int level = 0;
  for (float r : range) { if (distToCenter >= r) ++level; else break; }
  return level;
}

class ViewDependentSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    const std::string t = node ? node->nodeTypeName() : "";
    if (t == "LOD") lodLevel_.emplace(node, -1);
    else if (t == "ProximitySensor" || t == "VisibilitySensor" || t == "TransformSensor")
      sensorActive_.emplace(node, false);
  }

  // Test/observer seam: invoked when an LOD's announced level changes.
  void setLevelChangedHook(std::function<void(X3DNode *, int)> h) { levelHook_ = std::move(h); }

  // Test/observer seam: (sensor, isActive, eventTime) on each edge.
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h) { sensorHook_ = std::move(h); }

  // Consumer-supplied exact view volume (frustum). When valid, VisibilitySensor
  // widens the cone's effective horizontal half-angle by `aspect` (the minimal
  // exact-aspect improvement); a full 6-plane frustum test is deferred (see the
  // design's frustum-seam note). Renderers set this each frame from their camera.
  struct ViewVolume { bool valid=false; float aspect=1.0f; /* extend: planes */ };
  void setViewVolume(const ViewVolume &vv) { viewVolume_ = vv; }

  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    const SFVec3f eye = ctx.cameraWorldPosition();
    for (auto &[node, last] : lodLevel_) {
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
    for (auto &[node, active] : sensorActive_) {
      if (node->nodeTypeName() == "ProximitySensor") updateProximity(node, active, now, ctx);
      else if (node->nodeTypeName() == "VisibilitySensor") updateVisibility(node, active, now, ctx);
      else if (node->nodeTypeName() == "TransformSensor") updateTransform(node, active, now, ctx);
    }
  }

private:
  std::function<void(X3DNode *, int)> levelHook_;
  std::function<void(X3DNode *, bool, double)> sensorHook_;

  // ENV-07: on the disable edge of an active sensor, fire isActive=FALSE/exitTime
  // (a disabled sensor is no longer active) and reset so a later re-enable re-fires
  // enter. Idempotent once already inactive.
  void deactivateIfActive(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!last) return;
    last = false;
    ctx.postEvent(node, "isActive", std::any(false));
    ctx.postEvent(node, "exitTime", std::any(static_cast<SFTime>(now)));
    if (sensorHook_) sensorHook_(node, false, now);
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
      deactivateIfActive(node, last, now, ctx); // ENV-07: disable deactivates (fires exit)
      return;
    }
    const Mat4 w = ctx.worldTransform(node);
    const Mat4 inv = w.inverse();
    const SFVec3f eyeLocal = inv.transformPoint(ctx.cameraWorldPosition());
    const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    const bool inside = insideBox(eyeLocal, center, size);
    if (inside) {
      // position/orientation in sensor coordinate system (§22.4.1).
      const SFVec3f fwdLocal = viewdep::norm(inv.transformDirection(viewMatrixForward(ctx)));
      const SFVec3f upLocal = viewdep::norm(inv.transformDirection(ctx.cameraWorldUp()));
      const SFRotation ori = lookAtRotation(fwdLocal, upLocal);
      // ENV-04: emit only when the viewer pose actually changes, not every tick.
      auto &pst = proxState_[node];
      if (!pst.has || !vecEq(eyeLocal, pst.pos))
        ctx.postEvent(node, "position_changed", std::any(eyeLocal));
      if (!pst.has || !rotEq(ori, pst.ori))
        ctx.postEvent(node, "orientation_changed", std::any(ori));
      pst.has = true;
      pst.pos = eyeLocal;
      pst.ori = ori;
    }
    if (inside != last) {
      last = inside;
      ctx.postEvent(node, "isActive", std::any(inside));
      ctx.postEvent(node, inside ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(now)));
      if (sensorHook_) sensorHook_(node, inside, now);
    }
  }

  // §22.4.3 VisibilitySensor: visible iff the size-box (in the sensor's frame)
  // may intersect the viewing volume. Default volume is a forward cone with
  // half-angle = Viewpoint.fieldOfView/2; a consumer-supplied ViewVolume widens
  // the effective half-angle by its aspect (exact-frustum is deferred).
  void updateVisibility(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) {
      deactivateIfActive(node, last, now, ctx); // ENV-07
      return;
    }
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    bool visible = false;
    if (size.x > 0 && size.y > 0 && size.z > 0) {
      const Mat4 w = ctx.worldTransform(node);
      const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
      const SFVec3f boxWorld = w.transformPoint(center);
      const SFVec3f eye = ctx.cameraWorldPosition();
      const SFVec3f fwd = viewdep::norm(
          ctx.viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1}));
      const SFVec3f toBox = viewdep::sub(boxWorld, eye);
      // conservative bounding radius of the box in world (half-diagonal, unscaled approx)
      const float radius = 0.5f * viewdep::len(size);
      // cone half-angle from fieldOfView (min viewing angle, §23.3.1)
      X3DNode *vp = ctx.boundViewpoint();
      const float fov = vp ? geombounds::getField<float>(*vp, "fieldOfView", 0.7854f) : 0.7854f;
      float halfAngle = fov * 0.5f;
      // Frustum seam: widen the effective half-angle by the consumer aspect.
      if (viewVolume_.valid && viewVolume_.aspect > 1.0f) halfAngle *= viewVolume_.aspect;
      const float along = viewdep::dot(toBox, fwd);
      const float dist = viewdep::len(toBox);
      // visible if any part of the box can be within the forward cone:
      // angle(toBox, fwd) - asin(radius/dist) <= halfAngle, and not fully behind.
      if (dist < 1e-6f) visible = true;
      else {
        const float cosang = along / dist;
        const float ang = std::acos(std::max(-1.0f, std::min(1.0f, cosang)));
        const float slack = (radius < dist) ? std::asin(radius / dist) : 3.14159265f;
        visible = (along + radius > 0.0f) && (ang - slack <= halfAngle);
      }
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
      const Mat4 rel = swInv * tw;
      const SFRotation oriLocal = rotationFromMatrix(rel);
      // Change-gate: emit only when pose actually changes (ENV-04 mirror).
      auto &st = trSensorState_[node];
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

  // ENV-04: last emitted ProximitySensor pose, for change-gating.
  struct ProxState { bool has = false; SFVec3f pos{}; SFRotation ori{}; };

  // ENV-01: last emitted TransformSensor pose, for change-gating.
  struct TransformSensorState { bool has = false; SFVec3f pos{}; SFRotation ori{}; };

  ViewVolume viewVolume_{};
  std::unordered_map<X3DNode *, int> lodLevel_;      // last announced level
  std::unordered_map<X3DNode *, bool> sensorActive_; // last isActive
  std::unordered_map<X3DNode *, ProxState> proxState_;
  std::unordered_map<X3DNode *, TransformSensorState> trSensorState_;
};

} // namespace x3d::runtime
#endif
