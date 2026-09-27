// PointingSensorSystem.hpp — the pointing-device sensor driver for the M2.5
// input seam. Resolves the pointer bearing (PointerState) against the scene via
// the context's PickSystem each tick and drives the spec-complete pointing
// sensors: the TouchSensor (isOver / isActive / touchTime / hitPoint_changed /
// hitNormal_changed / hitTexCoord_changed — ISO/IEC 19775-1 §20.4.4) and the
// three drag sensors PlaneSensor / SphereSensor / CylinderSensor (§20.4.2 /
// §20.4.3 / §20.4.1 + the drag-sensor common protocol §20.2.2). Stateful across
// ticks (over-sensor + grab bookkeeping).
//
// The resolution + grab + isOver/isActive machinery (§20.2.1: nearest geometry,
// lowest enabled sensor on the hit path; the grab takes exclusive ownership of
// motion until button-up) is shared by ALL pointing-device sensors. This file
// is the stateful glue: it resolves + grabs, then dispatches per-type behavior
// by `nodeTypeName()`:
//   - TouchSensor  → hit/touch outputs (unchanged from the M2.5 seam).
//   - drag sensors → on activation record the activation hit + sensor frame; on
//     each grabbed motion run the matching PURE drag math (drag/PlaneDrag.hpp,
//     SphereDrag.hpp, CylinderDrag.hpp) and emit trackPoint_changed + the
//     sensor's <value>_changed; on deactivation honor autoOffset.
// The drag math itself lives in the drag/*.hpp headers — this file holds no
// per-motion geometry, only the cross-tick activation state and the dispatch.
//
// Codegen-free: every emit* / accessor used here already exists (generated);
// this System only calls them. namespace x3d::runtime.
#ifndef X3D_RUNTIME_POINTING_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_POINTING_SENSOR_SYSTEM_HPP

#include "FieldRead.hpp"
#include "GeometryBounds.hpp" // geombounds::getField (reflection-generic reads)
#include "Mat4.hpp"
#include "PickSystem.hpp"
#include "Ray.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "drag/CylinderDrag.hpp" // pure CylinderSensor drag math (U1)
#include "drag/PlaneDrag.hpp"    // pure PlaneSensor drag math (U1)
#include "drag/SphereDrag.hpp"   // pure SphereSensor drag math (U1)

#include "x3d/nodes/CylinderSensor.hpp"        // emit* + accessors (generated)
#include "x3d/nodes/PlaneSensor.hpp"           // emit* + accessors (generated)
#include "x3d/nodes/SphereSensor.hpp"          // emit* + accessors (generated)
#include "x3d/nodes/TouchSensor.hpp"           // emit* + nodeTypeName/getEnabled (generated)
#include "x3d/nodes/X3DSensorNode.hpp"         // getEnabled (generic enabled probe)

#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives pointing-device sensors (TouchSensor + drag sensors) from the
 *        input seam.
 * @details Registered as a System on X3DExecutionContext; its `update(now,ctx)`
 *          runs each tick before the cascade drains. Resolution follows the
 *          spec: nearest geometry (§20.2.3, via pickClosest) → lowest enabled
 *          pointing-device sensor on the hit's path (§20.2.1). A grab (active
 *          sensor) takes exclusive ownership of motion until button-up. The
 *          grabbed sensor's type selects the per-motion behavior.
 */
class PointingSensorSystem : public System {
public:
  // Pointing sensors are resolved live from the pick path each tick, so attach()
  // does no per-node enrollment — but it DOES take a one-time inventory: a scene
  // with zero pointing-device sensors can never resolve a pick to one, so the
  // per-motion whole-scene pick (the dominant per-tick cost on large static
  // scenes) is pure waste there and is skipped in update(). The inventory only
  // suppresses the pick when the bridge actually ran the per-node pass; if attach
  // was never called (a test that registers the system directly), inventoried_
  // stays false and the system picks exactly as before — conservative by default.
  void attach(X3DNode *node, X3DExecutionContext & /*ctx*/) override {
    inventoried_ = true;
    if (isPointingSensor(node))
      ++sensorCount_;
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (isPointingSensor(node) && sensorCount_) --sensorCount_;
    erase(over_, node);
    active_.erase(std::remove_if(active_.begin(), active_.end(),
        [node](const ActiveSensor &a) { return a.node == node; }), active_.end());
  }

  void update(double now, X3DExecutionContext &ctx) override {
    const PointerState &ps = ctx.pointerState();

    // 0. A fully-inventoried scene with no pointing-device sensors can never
    //    resolve a pick to a sensor — skip the whole-scene pick entirely (the
    //    §20 resolution below is a no-op there, but the pick is not).
    if (inventoried_ && sensorCount_ == 0)
      return;

    // 1. No events when the pointer state is unchanged since the last tick
    //    (§20.4.4 — over/hit events are generated only on pointer motion, not
    //    when geometry animates under a still pointer).
    // §20.4.2: enabled=FALSE deactivates an active sensor even without motion.
    const bool hadGrab = !active_.empty();
    for (auto it = active_.begin(); it != active_.end();) {
      auto *sensor = dynamic_cast<x3d::nodes::X3DSensorNode *>(it->node);
      if (sensor && !sensor->getEnabled()) {
        emitActive(ctx, it->node, false);
        if (contains(over_, it->node)) {
          setOver(ctx, it->node, false);
          erase(over_, it->node);
        }
        it = active_.erase(it);
      } else {
        ++it;
      }
    }
    if (hadGrab && active_.empty() && ps.buttonDown) {
      lastRevision_ = ps.revision;
      buttonWasDown_ = true;
      return;
    }
    if (ps.revision == lastRevision_) {
      if (!active_.empty()) ctx.setPointerConsumedBySensor(true);
      return;
    }
    lastRevision_ = ps.revision;

    // 2. GRABBED: an active sensor owns all motion until release. No other
    //    pointing-device sensor receives events during the grab (§20.2.1).
    if (!active_.empty()) {
      // Re-pick to know whether the pointer is still over the grabbed sensor's
      // geometry (touchTime + isOver depend on it). present==false ⇒ not over.
      PickResult pick;
      std::vector<X3DNode *> resolved;
      if (ps.present) {
        pick = ctx.pick(ps.ray);
        if (pick.hit) resolved = resolve(pick);
      }
      // Per-motion behavior is type-specific. TouchSensor emits hit outputs
      // only while over; drag sensors track the bearing regardless of whether
      // it is still over the sensor geometry (the grab follows the pointer onto
      // the virtual geometry — §20.2.2).
      for (auto &a : active_) {
        const bool stillOver = contains(resolved, a.node);
        if (isTouchSensor(a.node)) {
          if (stillOver) emitHit(ctx, asTouch(a.node), pick);
        } else {
          emitDragMotion(ctx, a, ps.ray);
        }
        if (stillOver != contains(over_, a.node)) {
          setOver(ctx, a.node, stillOver);
          if (stillOver) over_.push_back(a.node);
          else erase(over_, a.node);
        }
        if (!ps.buttonDown) {
          emitActive(ctx, a.node, false);
          if (isTouchSensor(a.node)) {
            if (stillOver) emitTouch(ctx, asTouch(a.node), now);
          } else {
            emitDragDeactivate(ctx, a);
          }
        }
      }
      if (!ps.buttonDown) active_.clear();
      // Keep the button-edge detector current even while grabbed, so that after
      // this grab releases (button-up) a NEW button-down press on the following
      // tick is seen as an edge and can re-activate the sensor (§20.4.4: after
      // deactivation the sensor may be re-activated by a new press). Without
      // this, buttonWasDown_ stays stale-TRUE across the whole grab and the next
      // press is missed.
      buttonWasDown_ = ps.buttonDown;
      // Arbitration (§20.2.1): while this system holds a grab, the pointer is
      // exclusively ours — tell NavigationSystem (which runs after us) to skip
      // pointer-drag this tick. Reset each tick by X3DExecutionContext::tick.
      if (!active_.empty()) ctx.setPointerConsumedBySensor(true);
      return;
    }

    // 3. NOT grabbed: resolve the lowest enabled sensor under the pointer.
    std::vector<X3DNode *> resolved;
    PickResult pick;
    if (ps.present) {
      pick = ctx.pick(ps.ray);
      if (pick.hit)
        resolved = resolve(pick);
    }

    // §20.2.1: every tied lowest sensor receives the same hover and press.
    for (X3DNode *s : over_)
      if (!contains(resolved, s)) setOver(ctx, s, false);
    for (X3DNode *s : resolved)
      if (!contains(over_, s)) setOver(ctx, s, true);
    over_ = resolved;

    // d. Hit outputs (only while over a resolved TouchSensor). Drag sensors do
    //    not emit any tracking output until activated (§20.2.2).
    for (X3DNode *s : resolved)
      if (isTouchSensor(s)) emitHit(ctx, asTouch(s), pick);

    // e. Activation: button-down edge while over ⇒ grab begins (§20.2.1).
    if (!resolved.empty() && ps.buttonDown && !buttonWasDown_) {
      for (X3DNode *s : resolved) {
        emitActive(ctx, s, true);
        active_.push_back(ActiveSensor{s});
        if (!isTouchSensor(s)) beginDrag(ctx, active_.back(), pick, ps.ray);
      }
    }
    buttonWasDown_ = ps.buttonDown;
    // Arbitration (§20.2.1): if activation just began this tick, claim the
    // pointer so NavigationSystem yields it (covers the activation-tick case;
    // the already-grabbed case is handled in the GRABBED branch above).
    if (!active_.empty()) ctx.setPointerConsumedBySensor(true);
  }

private:
  struct ActiveSensor {
    X3DNode *node;
    Mat4 dragFrame = Mat4::identity();
    SFVec3f dragP0Local{0, 0, 0};
    SFVec3f dragBearingDirLocal{0, 0, -1};
    SFVec3f lastTranslation{0, 0, 0};
    SFRotation lastRotation{0, 1, 0, 0};
    float lastAngle = 0.0f;
  };

  static bool contains(const std::vector<X3DNode *> &nodes, X3DNode *node) {
    return std::find(nodes.begin(), nodes.end(), node) != nodes.end();
  }
  static void erase(std::vector<X3DNode *> &nodes, X3DNode *node) {
    nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());
  }

  // ---- sensor type predicates ----------------------------------------------

  static bool isTouchSensor(const X3DNode *n) {
    return n && n->nodeTypeName() == "TouchSensor";
  }
  static bool isDragSensor(const X3DNode *n) {
    if (!n)
      return false;
    const std::string &t = n->nodeTypeName();
    return t == "PlaneSensor" || t == "SphereSensor" || t == "CylinderSensor";
  }
  static bool isPointingSensor(const X3DNode *n) {
    return isTouchSensor(n) || isDragSensor(n);
  }

  static x3d::nodes::TouchSensor *asTouch(X3DNode *n) {
    return dynamic_cast<x3d::nodes::TouchSensor *>(n);
  }

  // ---- sensor resolution ----------------------------------------------------

  // Is `n` an enabled pointing-device sensor (TouchSensor or a drag sensor)? A
  // disabled sensor tracks nothing; enabling mid-drag must not activate it
  // (§20.2.1) — that latter rule is enforced structurally by the grab branch
  // (it never re-resolves enablement). enabled is read generically through the
  // X3DSensorNode base (shared by every pointing sensor).
  static X3DNode *asEnabledPointingSensor(const X3DNode *n) {
    if (!isPointingSensor(n))
      return nullptr;
    auto *sensor =
        dynamic_cast<x3d::nodes::X3DSensorNode *>(const_cast<X3DNode *>(n));
    if (!sensor || !sensor->getEnabled())
      return nullptr;
    return const_cast<X3DNode *>(n);
  }

  // Lowest enabled pointing-device sensor in the hit's hierarchy (§20.2.1). The
  // pick path is root→hit-geometry; a sensor watches geometry that is a
  // descendant of the sensor's PARENT GROUP and is typically a sibling of that
  // geometry. So we walk the path from the hit geometry UPWARD and, at each
  // grouping ancestor, inspect its children for an enabled sensor sibling; the
  // deepest such ancestor's sensor is the lowest one and wins.
  static std::vector<X3DNode *> resolve(const PickResult &pick) {
    const extract::PathKey &path = pick.path;
    // path.back() is the hit geometry-bearing node; its parent is path[n-2].
    for (std::size_t i = path.size(); i-- > 1;) {
      const X3DNode *parent = path[i - 1];
      auto siblings = enabledSensorChildrenOf(parent);
      if (!siblings.empty()) return siblings;
    }
    return {};
  }

  static std::vector<X3DNode *> enabledSensorChildrenOf(const X3DNode *group) {
    std::vector<X3DNode *> found;
    if (!group) return found;
    forEachChildNode(*group, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      if (X3DNode *s = asEnabledPointingSensor(c.get()); s && !contains(found, s))
        found.push_back(s);
    });
    return found;
  }

  // ---- TouchSensor output emission (unchanged from the M2.5 seam) -----------
  // Each emit goes through the cascade so any ROUTEs fan out at `now`. We post
  // the event (postEvent) so author ROUTEs from the outputOnly field fire; the
  // node's own emit* thunk is applied by the cascade during the drain.

  static void setOver(X3DExecutionContext &ctx, X3DNode *s, bool v) {
    ctx.postEvent(s, "isOver", std::any(SFBool{v}));
  }
  static void emitActive(X3DExecutionContext &ctx, X3DNode *s, bool v) {
    ctx.postEvent(s, "isActive", std::any(SFBool{v}));
  }
  static void emitTouch(X3DExecutionContext &ctx, x3d::nodes::TouchSensor *ts, double now) {
    ctx.postEvent(ts, "touchTime", std::any(SFTime{now}));
  }

  // hitPoint/hitNormal in the sensor's coordinate frame; hitTexCoord raw
  // (surface attribute, NOT spatially transformed) — §5.1 of the M2.5 design.
  static void emitHit(X3DExecutionContext &ctx, x3d::nodes::TouchSensor *ts,
                      const PickResult &pick) {
    const Mat4 M = ctx.worldOf(ts);
    const Mat4 inv = M.inverse();
    const SFVec3f localPoint = inv.transformPoint(pick.point);
    // Normals transform by the inverse-transpose of the POINT transform. The
    // world→sensor-local point transform is inv(M); its inverse-transpose is
    // transpose(inv(inv(M))) = transpose(M). (PickSystem does the local→world
    // direction by transpose(inv(M)); this is the mirror for world→local.)
    const SFVec3f localNormal =
        normalize(upperTranspose(M).transformDirection(pick.normal));
    ctx.postEvent(ts, "hitPoint_changed", std::any(SFVec3f{localPoint}));
    ctx.postEvent(ts, "hitNormal_changed", std::any(SFVec3f{localNormal}));
    ctx.postEvent(ts, "hitTexCoord_changed", std::any(SFVec2f{pick.texCoord}));
  }

  // ---- drag-sensor glue (§20.2.2 + §20.4.x) ---------------------------------
  // beginDrag freezes the activation state; emitDragMotion runs the per-type
  // pure drag math (drag/*.hpp) each grabbed motion; emitDragDeactivate applies
  // autoOffset. The actual geometry lives in the U1 headers — this is glue only.

  // Build the sensor frame M_sensor and freeze the activation hit (sensor-local)
  // + the activation bearing direction (sensor-local, for CylinderSensor's
  // once-only disk/cylinder mode decision). M_sensor = worldOf(sensor) ·
  // R(axisRotation); the sensor's own world matrix is its parent group's world
  // matrix (a pointing sensor carries no transform of its own).
  void beginDrag(X3DExecutionContext &ctx, ActiveSensor &a, const PickResult &pick,
                 const Ray &activationRay) {
    X3DNode *s = a.node;
    const Mat4 M_world = ctx.worldOf(s);
    Mat4 M_sensor = M_world;
    if (auto *p = dynamic_cast<x3d::nodes::PlaneSensor *>(s))
      M_sensor = M_world * Mat4::rotation(p->getAxisRotation());
    else if (auto *c = dynamic_cast<x3d::nodes::CylinderSensor *>(s))
      M_sensor = M_world * Mat4::rotation(c->getAxisRotation());
    // SphereSensor has no axisRotation: frame is the parent world matrix.

    const Mat4 inv = M_sensor.inverse();
    a.dragFrame = M_sensor;
    a.dragP0Local = inv.transformPoint(pick.point);
    a.dragBearingDirLocal = inv.transformDirection(activationRay.direction);
    // Initialize the held last-value to the sensor's current offset so that a
    // deactivation with no intervening motion (autoOffset) is a no-op.
    if (auto *p = dynamic_cast<x3d::nodes::PlaneSensor *>(s))
      a.lastTranslation = p->getOffset();
    else if (auto *sp = dynamic_cast<x3d::nodes::SphereSensor *>(s))
      a.lastRotation = sp->getOffset();
    else if (auto *c = dynamic_cast<x3d::nodes::CylinderSensor *>(s))
      a.lastAngle = c->getOffset();
  }

  // One grabbed pointer motion: dispatch to the matching pure drag math and
  // emit trackPoint_changed + the sensor's <value>_changed. On an invalid
  // result (bearing parallel/missed the virtual geometry) we hold the last
  // valid value (spec-allowed, §20.4.x) — the drag math already returns the
  // held value, so we still emit it for a continuous output stream.
  void emitDragMotion(X3DExecutionContext &ctx, ActiveSensor &a, const Ray &ray) {
    X3DNode *s = a.node;
    if (auto *p = dynamic_cast<x3d::nodes::PlaneSensor *>(s)) {
      PlaneDragResult res =
          planeDrag(a.dragFrame, a.dragP0Local, ray, p->getOffset(),
                    p->getMinPosition(), p->getMaxPosition());
      ctx.postEvent(s, "trackPoint_changed", std::any(SFVec3f{res.trackPoint}));
      ctx.postEvent(s, "translation_changed",
                    std::any(SFVec3f{res.translation}));
      a.lastTranslation = res.translation;
    } else if (auto *sp = dynamic_cast<x3d::nodes::SphereSensor *>(s)) {
      SphereDragResult res =
          sphereDrag(a.dragFrame, a.dragP0Local, ray, sp->getOffset());
      ctx.postEvent(s, "trackPoint_changed", std::any(SFVec3f{res.trackPoint}));
      ctx.postEvent(s, "rotation_changed", std::any(SFRotation{res.rotation}));
      a.lastRotation = res.rotation;
    } else if (auto *c = dynamic_cast<x3d::nodes::CylinderSensor *>(s)) {
      CylinderDragResult res =
          cylinderDrag(a.dragFrame, a.dragP0Local, a.dragBearingDirLocal, ray,
                       c->getDiskAngle(), c->getOffset(), c->getMinAngle(),
                       c->getMaxAngle());
      ctx.postEvent(s, "trackPoint_changed", std::any(SFVec3f{res.trackPoint}));
      ctx.postEvent(s, "rotation_changed", std::any(SFRotation{res.rotation}));
      a.lastAngle = res.rotation.angle;
    }
  }

  // Deactivation: if autoOffset TRUE, offset ← last <value>_changed and emit
  // offset_changed (posting to the inputOutput `offset` field updates the
  // stored value and fans out the implicit offset_changed ROUTEs). §20.2.2.
  void emitDragDeactivate(X3DExecutionContext &ctx, const ActiveSensor &a) {
    X3DNode *s = a.node;
    if (auto *p = dynamic_cast<x3d::nodes::PlaneSensor *>(s)) {
      if (p->getAutoOffset())
        ctx.postEvent(s, "offset", std::any(SFVec3f{a.lastTranslation}));
    } else if (auto *sp = dynamic_cast<x3d::nodes::SphereSensor *>(s)) {
      if (sp->getAutoOffset())
        ctx.postEvent(s, "offset", std::any(SFRotation{a.lastRotation}));
    } else if (auto *c = dynamic_cast<x3d::nodes::CylinderSensor *>(s)) {
      if (c->getAutoOffset())
        ctx.postEvent(s, "offset", std::any(SFFloat{a.lastAngle}));
    }
  }

  // Transpose of the upper-left 3x3 of m (used to carry a WORLD surface normal
  // into the sensor-local frame; see emitHit for the derivation). Column-major:
  // m.m[c*4+row]; the transpose swaps c and row.
  static Mat4 upperTranspose(const Mat4 &m) {
    Mat4 r = Mat4::identity();
    for (int c = 0; c < 3; ++c)
      for (int row = 0; row < 3; ++row)
        r.m[c * 4 + row] = m.m[row * 4 + c];
    return r;
  }

  static SFVec3f normalize(const SFVec3f &v) {
    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return l > 1e-12f ? SFVec3f{v.x / l, v.y / l, v.z / l} : v;
  }

  // ---- cross-tick state -----------------------------------------------------
  unsigned long lastRevision_ = static_cast<unsigned long>(-1); // force 1st run
  std::vector<X3DNode *> over_;         // tied sensors currently isOver==TRUE
  std::vector<ActiveSensor> active_;    // independently grabbed tied sensors
  bool buttonWasDown_ = false;          // for button-DOWN edge detection
  bool inventoried_ = false;            // did attach() scan the scene?
  int sensorCount_ = 0;                 // pointing-device sensors found by attach()

};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_POINTING_SENSOR_SYSTEM_HPP
