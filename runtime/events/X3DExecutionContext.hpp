// X3DExecutionContext.hpp
// Ties together the route graph, the event cascade, the active-node registry,
// and a clock. This is the object a browser drives each frame: register routes
// and behaviors, then call tick(now) to advance time and run the resulting
// event cascade to quiescence.
#ifndef X3D_RUNTIME_EXECUTION_CONTEXT_HPP
#define X3D_RUNTIME_EXECUTION_CONTEXT_HPP

#include "BindingSystem.hpp"
#include "BoundsSystem.hpp"
#include "CycleBreaker.hpp"
#include "DirtyTracker.hpp"
#include "HeadPose.hpp"
#include "KeyState.hpp"
#include "PickSystem.hpp"
#include "PointerState.hpp"
#include "ViewpointOffset.hpp"
#include "TransformSystem.hpp"
#include "X3DActiveNode.hpp"
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"
#include "X3DFieldAddress.hpp"
#include "X3DSystem.hpp"

#include <any>
#include <cstdint> // pickCalls_ diagnostic counter
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

class Scene;
struct BridgeResult;

/**
 * @brief Runtime execution context: routes + cascade + active nodes + clock.
 * @details Owns the routing graph and a single long-lived cascade built over
 *          it. `postEvent` seeds an event; behaviors call it from `update` or
 *          from inputOnly handlers. `tick(now)` advances the clock, lets every
 *          active node emit its time-driven events, then runs the cascade —
 *          events posted while the cascade is draining (e.g. an interpolator's
 *          handler emitting value_changed) are processed in the same frame.
 */
class X3DExecutionContext {
public:
  /** @brief Register a ROUTE from a source field endpoint to a sink endpoint.
   */
  void addRoute(const FieldAddress &from, const FieldAddress &to) {
    graph_.addRoute(from, to);
  }

  /** @brief Remove a ROUTE (dynamic SAI deleteRoute, §4.3.7). No-op if absent.
   */
  void removeRoute(const FieldAddress &from, const FieldAddress &to) {
    graph_.removeRoute(from, to);
  }

  /**
   * @brief Bridge a parsed Scene's DEF-named ROUTEs onto this context.
   * @details Thin wrapper over `buildRoutes(scene, *this)` so a browser can
   *          drive a loaded document in one call. Defined inline in
   *          X3DSceneBridge.hpp (which owns the resolution + validation logic);
   *          include that header to use this method.
   * @return The bridge result (routes added + any rejected with reasons).
   */
  BridgeResult buildFrom(Scene &scene);

  /// Build the M2a scene-graph layer for a parsed Scene: index the Transform
  /// hierarchy and route the cascade's field deliveries into the dirty tracker.
  void buildSceneGraph(Scene &scene) {
    // Sanitize first: sever any containment cycle (a node that is its own
    // ancestor, e.g. from a malformed <X DEF='a' USE='a'/>) so the recursive
    // walkers below (transform/bounds/binding/pick + later extract) traverse a
    // DAG and cannot stack-overflow. No-op on well-formed scenes. See CycleBreaker.
    breakContainmentCycles(scene);
    transforms_.buildIndex(scene);
    bounds_.buildBounds(scene, transforms_);
    bindings_.enrollScene(scene,
        [this](X3DNode *n, const std::string &f, std::any v) { postEvent(n, f, std::move(v)); },
        [this] { return now(); },
        [this](BindTransition k) { lastVpTransition_ = k; });
    bindings_.bindDefaults();
    pick_.build(scene);
    cascade_.setFieldObserver(
        [this](const FieldAddress &a) { classifyDirty(a); });
  }

  /** @brief Remove all registered ROUTEs from the execution context. */
  void clearRoutes() { graph_.clear(); }

  /**
   * @brief Register a behavior System (event-driven or time-driven).
   * @details The System is retained; call `attach(node, *this)` on it for each
   *          node it should service (event-driven systems wire their handler
   *          there). Each `tick` calls every registered system's `update`.
   */
  void addSystem(std::shared_ptr<System> system) {
    systems_.push_back(std::move(system));
  }

  /**
   * @brief Deprecated compatibility shim: adapt an ActiveNode to a System.
   * @details Wraps the per-node ActiveNode in a one-node System so existing
   *          callers (and the legacy reference behaviors) keep working while
   * the codebase migrates to `addSystem`. New code should implement System
   *          directly. The wrapped node is updated each tick exactly as before.
   */
  [[deprecated("use addSystem(std::shared_ptr<System>) instead")]]
  void addActiveNode(std::shared_ptr<ActiveNode> node) {
    systems_.push_back(std::make_shared<ActiveNodeAdapter>(std::move(node)));
  }

  /**
   * @brief Register a post-cascade hook run after the per-tick cascade drains.
   * @details ISO 19775-1 §29.2.4 requires a Script's eventsProcessed() to run
   *          AFTER the batch of received events has drained — i.e. after the
   *          cascade, not in the pre-cascade System pass. The ScriptSystem
   *          installs its eventsProcessed phase here; it is invoked each tick
   *          after `cascade_.process()` and may post + drain further events.
   *          Hooks run in registration order. (Generic so the context need not
   *          depend on ScriptSystem's type.)
   */
  void addPostCascadeHook(std::function<void(X3DExecutionContext &)> hook) {
    postCascade_.push_back(std::move(hook));
  }

  /**
   * @brief Convenience: register a ScriptSystem as the FIRST System (so its
   *        prepareEvents/update runs before other sensors and the cascade) and
   *        wire its post-cascade eventsProcessed phase.
   * @details `sys` must expose `update(now, ctx)` (prepareEvents phase) and
   *          `runEventsProcessed(ctx)` (post-cascade phase). Templated so this
   *          header stays free of a ScriptSystem include (avoids the cycle:
   *          ScriptSystem includes this header).
   */
  template <class ScriptSystemT>
  void addScriptSystem(std::shared_ptr<ScriptSystemT> sys) {
    systems_.insert(systems_.begin(), sys);  // FIRST: prepareEvents before routes
    addPostCascadeHook(
        [sys](X3DExecutionContext &ctx) { sys->runEventsProcessed(ctx); });
  }

  /** @brief Seed an event; processed by the next process()/tick() drain. */
  void postEvent(X3DNode *node, const std::string &field, std::any value) {
    cascade_.postEvent(node, field, std::move(value));
  }

  /**
   * @brief Dirty-aware direct field write for use by Systems.
   * @details Writes `value` into `field` on `node` via reflection, then calls
   *          `classifyDirty` so the dirty-tracker is updated exactly as if the
   *          write had been delivered by the event cascade.  Use this instead of
   *          a raw `info.set` call when a System needs to poke a field directly
   *          (not via a posted event) — omitting `classifyDirty` would leave the
   *          dirty state stale, which is a silent footgun (M2C-3).
   *
   *          If `node` is null, the field is not found, or the field has no write
   *          thunk, this is a no-op (mirrors `EventCascade::deliver` behaviour).
   *
   *          Field-scan logic mirrors `EventCascade::deliver`; if you change the
   *          lookup here, update that method too (intentional small duplication —
   *          the cascade fans one value to N sinks via const&, this moves to one).
   */
  void writeField(X3DNode *node, const std::string &field, std::any value) {
    if (!node) return;
    for (const auto &info : node->fields()) {
      if (info.x3dName == field) {
        if (info.set) {
          info.set(*node, std::move(value));
          classifyDirty(FieldAddress{node, field});
        }
        return;
      }
    }
  }

  /**
   * @brief Advance to time `now`: update active nodes, then drain the cascade.
   * @details RTC-6 — ISO 19775-1 §4.4.8.3 step 4: steps 2–3 (evaluate
   *          systems/sensors → drain the routed cascade) repeat until a pass
   *          produces no new events, BEFORE post-processing. This lets a
   *          sensor→route→sensor chain resolve within one tick even when the
   *          downstream sensor is ordered earlier in `systems_` (it would
   *          otherwise read the stale value and lag a frame). The whole loop is
   *          ONE timestamp: the per-field cap (RTC-5) persists across the passes
   *          (each drain continues the timestamp via `process(false)`), so a
   *          field re-emitted every pass is delivered once and the loop is
   *          guaranteed to terminate (productions are monotone and bounded by
   *          the finite set of fields).
   */
  void tick(double now) {
    // Guard against re-entrant tick() (e.g. a System calling tick() from
    // update()).  A recursive tick would clobber the outer tick's timestamp
    // state (now_, dirty_, produced_/fired_ guards) and consume pending
    // events belonging to the outer tick.  Silent no-op is the safest
    // recovery — callers that need nested evaluation should use process().
    if (ticking_) return;
    struct Guard {
      bool &ref;
      Guard(bool &r) : ref(r) { ref = true; }
      ~Guard() { ref = false; }
    } guard{ticking_};

    now_ = now;
    pointerConsumedBySensor_ = false;     // reset per-tick arbitration flag
    dirty_.clear();                       // drop last tick's changed-set
    cascade_.beginTimestamp();            // open the tick's single timestamp
    // §4.4.8.3 step 4: re-evaluate systems + drain until quiescence.
    do {
      for (const auto &s : systems_) {
        s->update(now, *this);
      }
    } while (cascade_.process(/*freshTimestamp=*/false) != 0);  // observer fills dirty_
    // §29.2.4: Script eventsProcessed() runs AFTER the batch drains. Each hook
    // may post + drain further events (it calls process() internally).
    for (const auto &hook : postCascade_) {
      hook(*this);
    }
    transforms_.propagate(dirty_);        // dirtied locals -> world transforms
    bounds_.propagate(dirty_, transforms_); // dirtied geometry -> bounds
  }

  /** @brief Drain any pending events without advancing the clock. */
  void process() { cascade_.process(); }

  double now() const { return now_; }
  const EventGraph &graph() const { return graph_; }

  /// Pull surface (read after tick): the per-node dirty set for this tick.
  const DirtyTracker &dirtyTracker() const { return dirty_; }
  /// Mark a grouping node's ACTIVE-CHILD selection as changed so the next
  /// delta() re-walks its subtree. View-dependent selection — an LOD level
  /// computed from the camera each tick — is not a settable field, so it never
  /// flows through the cascade's field observer (classifyDirty). Without this,
  /// only full-snapshot consumers see the swap; incremental delta() consumers
  /// (e.g. the OpenGL PoC) stay on the stale level. The settable-field analogue
  /// is Switch.whichChoice -> DirtyChildren in classifyDirty (SW-DELTA-1).
  void markActiveChildChanged(X3DNode *n) {
    if (n) dirty_.markDirty(n, DirtyChildren | DirtyBounds);
  }
  /// Pull surface: world transform of a Transform node (identity if unknown).
  Mat4 worldTransform(const X3DNode *n) const {
    return transforms_.worldTransform(n);
  }
  /// Pull surface: world transform of ANY node — Transform nodes get their own
  /// world; non-Transform nodes inherit their nearest ancestor Transform's
  /// world. Identity if unindexed or no Transform ancestor. Required when the
  /// target is reachable via both a non-Transform reference (e.g. targetObject,
  /// use field) AND a Transform ancestor (e.g. children); the scene-graph walk
  /// from roots can find the wrong path, so we walk UP via the parent index.
  Mat4 worldTransformAny(const X3DNode *n) const {
    return transforms_.worldTransformAny(n);
  }
  /// Pull surface: local-frame AABB of a node (empty if unknown).
  Aabb localBounds(const X3DNode *n) const { return bounds_.localBounds(n); }
  /// Pull surface: world-space AABB (= local bounds x composed ancestor
  /// transforms). Uses TransformSystem::worldTransformAny (walk UP via parent
  /// index) rather than PickSystem::worldOf (walk DOWN from roots) so a
  /// non-Transform target reachable via a non-Transform reference (e.g.
  /// TransformSensor.targetObject) still resolves through its Transform
  /// ancestor — the latter would pick whichever root-path finds it first.
  /// O(1) per call; fine for LOOKAT/sensors.
  Aabb worldBounds(const X3DNode *n) const {
    return bounds_.localBounds(n).transformed(transforms_.worldTransformAny(n));
  }

  /// Pull surface: the currently bound node per bindable category (null if none).
  X3DNode *boundBindable(const std::string &category) const { return bindings_.bound(category); }
  X3DNode *boundViewpoint() const { return bindings_.bound("Viewpoint"); }
  // BIND-06: a deleted bound node behaves as set_bind FALSE (pop its stack).
  void removeBoundNode(X3DNode *node) { bindings_.removeNode(node); }
  X3DNode *boundNavigationInfo() const { return bindings_.bound("NavigationInfo"); }
  X3DNode *boundBackground() const { return bindings_.bound("Background"); }
  X3DNode *boundFog() const { return bindings_.bound("Fog"); }

  // BIND-09: side-channel — kind of the latest viewpoint bind transition.
  BindTransition lastViewpointBindTransition() const { return lastVpTransition_; }
  void setLastViewpointBindTransition(BindTransition k) { lastVpTransition_ = k; }

  // ------------------------------------------------------------------
  // Consumer → runtime pointer input (M2.5 input seam, spec §4.1/§6).
  // Call these between ticks to feed the current pointer state; each
  // bumps PointerState::revision so PointingSensorSystem can skip ticks
  // where nothing changed (§20.4.4 — isOver events only on pointer motion).
  // The consumer is responsible for un-projecting the screen position to a
  // world-space ray; no projection/near/far parameters ever cross this seam.
  // ------------------------------------------------------------------

  /// Set the current world-space bearing (ray origin + direction).
  void setPointer(const Ray &worldRay) { pointer_.setRay(worldRay); }

  /// Set the primary-button pressed state.
  void setPointerButton(bool down) { pointer_.setButtonDown(down); }

  /// Set whether the pointer is present (inside the view / active input region).
  void setPointerPresent(bool present) { pointer_.setPresent(present); }

  /// Set the normalized cursor position (x rightward, y upward, ~[0,1] across
  /// the view). This is the camera-independent signal NavigationSystem uses for
  /// EXAMINE/FLY drag — feed raw cursor pixels divided by the framebuffer size.
  void setPointerScreen(float x, float y) { pointer_.setScreen(x, y); }

  /// Read-only access to the current pointer snapshot (for PointingSensorSystem).
  const PointerState &pointerState() const { return pointer_; }

  // ------------------------------------------------------------------
  // Consumer → runtime keyboard input (M2D PDS-4, unit U3).
  // Call setKey between ticks to feed key press/release events; each call
  // bumps KeyState::revision so NavigationSystem can skip ticks where no
  // key activity occurred (mirrors the PointerState revision pattern).
  // ------------------------------------------------------------------

  /// Record a key press (down=true) or release (down=false).
  void setKey(int code, bool down) { keys_.setKey(code, down); }

  // §21 Key Device Sensor input: discrete keystroke events for KeySensor /
  // StringSensor (richer than the nav `held` set — see KeyState::KeyEvent).
  /// Push a character key press/release (the produced UTF-8 character).
  void pushKeyCharacter(const std::string &c, bool down) { keys_.pushCharacter(c, down); }
  /// Push an action key press/release (§21 Table 21.2 value).
  void pushActionKey(int code, bool down) { keys_.pushActionKey(code, down); }
  /// Push a modifier key press/release (1=shift, 2=control, 3=alt).
  void pushModifierKey(int which, bool down) { keys_.pushModifierKey(which, down); }
  /// Push the OS string-terminate key (StringSensor finalText).
  void pushStringTerminator() { keys_.pushTerminator(); }
  /// Push the OS delete-preceding-character key (StringSensor deletion).
  void pushStringDeletion() { keys_.pushDeletion(); }
  /// Drop consumed key events (KeyDeviceSensorSystem calls this each tick).
  void clearKeyEvents() { keys_.clearEvents(); }

  /// Read-only access to the current keyboard snapshot (for NavigationSystem).
  const KeyState &keyState() const { return keys_; }

  // Nav-vs-sensor arbitration (§20.2.1): true while a pointing-device sensor
  // grab owns the pointer this tick. Reset at tick start; PointingSensorSystem
  // sets it; NavigationSystem honors it (skips pointer-drag). See ADR / the
  // reference-consumer interaction spec.
  bool pointerConsumedBySensor() const { return pointerConsumedBySensor_; }
  void setPointerConsumedBySensor(bool v) { pointerConsumedBySensor_ = v; }

  /// Accumulated world transform at `node` (product of ancestor Transform
  /// local matrices). Identity if not found. A pointing-device sensor has no
  /// transform of its own, so this is its parent-group frame — the coordinate
  /// system its hitPoint_changed/hitNormal_changed outputs are expressed in
  /// (§20.4.4, M2.5 design §5.1). Delegates to PickSystem::worldOf.
  Mat4 worldOf(const X3DNode *node) const { return pick_.worldOf(node); }

  /// Closest pick along a world-space ray (PickResult.hit == false on a miss).
  /// Threads the live viewer pose so geometry under a Billboard is picked in the
  /// view-rotated frame (matches the extractor's per-path billboard rotation).
  // Diagnostic: number of whole-scene picks performed via pick() process-wide.
  // Pointing-sensor resolution picks the scene on every pointer motion, so a
  // scene with no pointing-device sensors should never incur one; tests snapshot
  // this around tick() to assert PointingSensorSystem skips the pick entirely.
  static inline std::uint64_t pickCalls_ = 0;
  static std::uint64_t pickCallCount() { return pickCalls_; }

  PickResult pick(const Ray &worldRay) const {
    ++pickCalls_;
    return pick_.pickClosest(worldRay, bounds_, cameraWorldPosition(),
                             cameraWorldUp());
  }

  /// World->camera (view) matrix from the bound Viewpoint (identity if none).
  // CONF-VIEWNAV: the EFFECTIVE view = worldOf · authoredPose · navOffset · headPose.
  // Authored position/orientation stay pristine (§23.2.3); navigation accumulates
  // the per-viewpoint offset and the consumer supplies the head pose.
  Mat4 viewMatrix() const {
    X3DNode *vp = boundViewpoint();
    if (!vp) return Mat4::identity();
    SFVec3f pos = geombounds::getVec3fLenient(*vp, "position", {0,0,0});
    SFRotation ori = geombounds::getField<SFRotation>(*vp, "orientation", {0,0,1,0});
    Mat4 cam = pick_.worldOf(vp) * Mat4::translation(pos) * Mat4::rotation(ori) *
               viewpointOffset(vp).local *
               (Mat4::translation(head_.position) * Mat4::rotation(head_.orientation));
    return cam.inverse();
  }

  // Per-viewpoint user offset (process-local; §23.3.1 relative viewing transform).
  const ViewpointOffset &viewpointOffset(X3DNode *vp) const {
    static const ViewpointOffset kIdentity{};
    auto it = offsets_.find(vp);
    return it == offsets_.end() ? kIdentity : it->second;
  }
  void setViewpointOffset(X3DNode *vp, const ViewpointOffset &off) { offsets_[vp] = off; }

  // CAVE head-tracking seam (process-local; composed after the navigation offset).
  void setHeadPose(const SFVec3f &pos, const SFRotation &ori) { head_.set(pos, ori); }
  const HeadPose &headPose() const { return head_; }

  /// Viewer world-space position = camera-to-world translation (M2e).
  SFVec3f cameraWorldPosition() const {
    return viewMatrix().inverse().transformPoint(SFVec3f{0, 0, 0});
  }
  /// Viewer world-space up vector (+Y of the camera frame) (M2e).
  SFVec3f cameraWorldUp() const {
    return viewMatrix().inverse().transformDirection(SFVec3f{0, 1, 0});
  }

private:
  /// Map a delivered field to dirty flags on its node.
  void classifyDirty(const FieldAddress &a) {
    if (!a.node) return;
    static const char *kTRS[] = {"translation", "rotation", "scale", "center",
                                 "scaleOrientation"};
    static const char *kChildren[] = {"children", "addChildren", "removeChildren"};
    unsigned flags = DirtyField;
    // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
    bool isTransformNode = TransformSystem::isTransform(a.node);
    for (const char *f : kChildren)
      if (a.field == f) flags = DirtyChildren;
    // Switch.whichChoice selects the active child: changing it is an active-CHILD
    // change for extraction, so it must trigger a subtree re-walk (DirtyChildren)
    // — not a plain DirtyField, which delta() ignores for a grouping node (it is
    // in neither geomDeps_ nor materialDeps_). Without this, incremental delta()
    // consumers (e.g. the OpenGL PoC) never see the swap; only full-snapshot
    // consumers (cpuraster) do. The extractor already reads whichChoice on a full
    // walk, so this only fixes the INCREMENTAL channel.
    if (a.field == "whichChoice") flags = DirtyChildren;
    if (isTransformNode)
      for (const char *f : kTRS)
        if (a.field == f) flags = DirtyLocalTransform;
    static const char *kBounds[] = {"size", "radius", "height", "bottomRadius",
        "point", "coord", "controlPoint", "crossSection", "spine", "scale",
        "geometry", "bboxSize", "string", "maxExtent"};
    for (const char *f : kBounds)
      if (a.field == f) { flags |= DirtyBounds; break; }
    if (flags & DirtyChildren) flags |= DirtyBounds;
    dirty_.markDirty(a.node, flags);
  }

  /**
   * @brief One-node System wrapping a legacy ActiveNode (for `addActiveNode`).
   */
  class ActiveNodeAdapter : public System {
  public:
    explicit ActiveNodeAdapter(std::shared_ptr<ActiveNode> node)
        : node_(std::move(node)) {}
    void attach(X3DNode *, X3DExecutionContext &) override {}
    void update(double now, X3DExecutionContext &ctx) override {
      if (node_) {
        node_->update(now, ctx);
      }
    }

  private:
    std::shared_ptr<ActiveNode> node_;
  };

  EventGraph graph_;
  EventCascade cascade_{graph_};
  std::vector<std::shared_ptr<System>> systems_;
  std::vector<std::function<void(X3DExecutionContext &)>> postCascade_;
  DirtyTracker dirty_;
  TransformSystem transforms_;
  BoundsSystem bounds_;
  BindingSystem bindings_;
  PickSystem pick_;
  PointerState pointer_;
  KeyState keys_;
  HeadPose head_;                                          // CONF-VIEWNAV head seam
  std::unordered_map<X3DNode *, ViewpointOffset> offsets_; // per-viewpoint user offset
  BindTransition lastVpTransition_ = BindTransition::None; // BIND-09
  double now_ = 0.0;
  bool ticking_ = false; // reentrancy guard for tick()
  bool pointerConsumedBySensor_ = false; // per-tick nav/sensor arbitration flag
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_EXECUTION_CONTEXT_HPP
