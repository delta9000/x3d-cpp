// X3DSceneBridge.hpp
// Bridges a parsed Scene's DEF-named ROUTEs onto the pointer-based EventGraph
// held by an X3DExecutionContext. A free-function bridge (stateless,
// reflection-driven) mirroring the codec style: it resolves each Route's DEF
// names to FieldAddress endpoints, validates the pairing against the nodes'
// reflected FieldTables, adds the valid edges to the context, and reports the
// rejected ones as diagnostics. Node-agnostic: nothing here knows a concrete
// node type — direction and type checks come entirely from reflection.
#ifndef X3D_RUNTIME_SCENE_BRIDGE_HPP
#define X3D_RUNTIME_SCENE_BRIDGE_HPP

#include "X3DExecutionContext.hpp"
#include "EventUtilitySystem.hpp"
#include "FollowerRegistration.hpp"
#include "InterpolatorRegistration.hpp"
#include "KeyDeviceSensorSystem.hpp"
#include "LoadSensorSystem.hpp"
#include "NavigationSystem.hpp"
#include "PointingSensorSystem.hpp"
#include "TimeSensorSystem.hpp"
#include "ViewDependentSystem.hpp"
#include "ViewpointBindSystem.hpp"

#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"

#include "DynamicField.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DScene.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief One rejected ROUTE plus a human-readable reason.
 * @details `index` is the position of the offending route in `Scene.routes`
 *          so a caller can correlate the diagnostic back to the source.
 */
struct RouteError {
  std::size_t index = 0; // position in Scene.routes
  std::string reason;    // human-readable explanation
};

/**
 * @brief Outcome of bridging a Scene's routes onto an execution context.
 * @details `routesAdded` counts the edges actually registered with the
 *          context. `rejected` lists routes that failed validation (unknown
 *          field / wrong direction / type mismatch) and were NOT added. Routes
 *          with an unresolved endpoint (forward-ref / IMPORTed / unknown DEF)
 *          are skipped silently — they affect neither count, matching the
 *          serialization tolerance for dangling DEF names.
 */
struct BridgeResult {
  std::size_t routesAdded = 0;
  std::vector<RouteError> rejected;

  bool ok() const { return rejected.empty(); }
};

namespace detail {

/// Find a field by its X3D name in a node's EFFECTIVE table (static fields()
/// plus Script author fields from the dynamic-field store, so author-field
/// ROUTE endpoints resolve and wire — S1). Returns a COPY because
/// effectiveFields() builds a temporary table that includes the synthesized
/// author FieldInfos; a pointer into it would dangle. std::nullopt if absent.
inline std::optional<FieldInfo> findField(const X3DNode &node,
                                          const std::string &x3dName) {
  for (FieldInfo &f : effectiveFields(node)) {
    if (f.x3dName == x3dName) {
      return std::move(f);
    }
  }
  return std::nullopt;
}

/// A ROUTE source must be readable-as-event: outputOnly or inputOutput.
inline bool isRoutableSource(AccessType a) {
  return a == AccessType::OutputOnly || a == AccessType::InputOutput;
}

/// A ROUTE sink must be writable-as-event: inputOnly or inputOutput.
inline bool isRoutableSink(AccessType a) {
  return a == AccessType::InputOnly || a == AccessType::InputOutput;
}

} // namespace detail

/**
 * @brief Resolve a Scene's DEF-named ROUTEs to endpoints and populate `ctx`.
 * @details Calls `scene.resolveRoutes()` to refresh each Route's from/to
 *          weak_ptrs from the DEF table, then for every route, in order:
 *
 *          1. Unresolved endpoint (from/to weak_ptr expired) -> skipped
 *             silently; not counted, not rejected.
 *          2. Unknown field on either endpoint -> rejected.
 *          3. Direction: source must be outputOnly/inputOutput, sink must be
 *             inputOnly/inputOutput -> else rejected.
 *          4. Type: from.type must equal to.type (X3D performs no implicit
 *             field-type coercion across a ROUTE) -> else rejected.
 *
 *          Valid routes are added via `ctx.addRoute()` using the resolved raw
 *          node pointers; the context observes the nodes (the Scene owns their
 *          lifetime). Never throws on a bad route — diagnostics are returned.
 */
inline BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx) {
  BridgeResult result;
  ctx.clearRoutes();          // avoid dangling pointers from previous scenes
  scene.resolveRoutes();

  // (0) Pre-resolved body-internal ROUTEs of expanded PROTO instances: their
  // endpoints are concrete cloned nodes (proto-local DEF scope, never in
  // scene.defs), so register them directly rather than via DEF-name lookup.
  for (const ResolvedProtoRoute &pr : scene.resolvedProtoRoutes) {
    if (!pr.from || !pr.to) {
      continue;
    }
    ctx.addRoute({pr.from.get(), pr.fromField}, {pr.to.get(), pr.toField});
    ++result.routesAdded;
  }

  // (0b) Pre-resolved internal ROUTEs of expanded Inlines: endpoints are
  // concrete nodes in the inlined child's DEF scope (never in scene.defs),
  // so register them directly rather than via DEF-name lookup (mirrors
  // resolvedProtoRoutes above).
  for (const ResolvedProtoRoute &ir : scene.resolvedInlineRoutes) {
    if (!ir.from || !ir.to) {
      continue;
    }
    ctx.addRoute({ir.from.get(), ir.fromField}, {ir.to.get(), ir.toField});
    ++result.routesAdded;
  }

  // Redirect one endpoint of an external route through the PROTO interface map:
  // if (node, field) is an exposed interface event field of an expanded
  // instance, add a route per IS-mapped body target (against the already-known
  // opposite endpoint `other`) and report success so the caller skips the
  // normal unknown-field handling. `asSource` selects which side the redirected
  // endpoint feeds: as the route source (true) or the sink (false).
  //
  // The redirected edge MUST be validated with the same direction + type rules
  // the normal route path enforces (X3DRoute.hpp::validateRoute parity — see
  // AUD-PROTO-EXP / BACKLOG.md "Note" after the CDC rows): an interface field
  // that maps to an inputOnly body target cannot be used as a ROUTE source
  // (the body has no `get` thunk, so the cascade would never deliver); an
  // outputOnly body target cannot be used as a sink; types must match.
  auto redirectEndpoint = [&](X3DNode *node, const std::string &field,
                              bool asSource, const FieldAddress &other,
                              std::size_t routeIndex) -> bool {
    auto nIt = scene.protoRedirects.find(node);
    if (nIt == scene.protoRedirects.end()) {
      return false;
    }
    auto fIt = nIt->second.find(field);
    if (fIt == nIt->second.end()) {
      return false;
    }
    // Look up the OPPOSITE endpoint's FieldInfo so we can validate type
    // parity (X3D performs no implicit field-type coercion across a ROUTE,
    // §4.4.8.2). The opposite endpoint is a regular Scene DEF-resolved node
    // here, not a redirect target, so its field must exist on the node.
    std::optional<FieldInfo> otherInfo =
        detail::findField(*other.node, other.field);
    if (!otherInfo) {
      // The opposite endpoint is unknown — the normal route path would reject
      // this anyway; do the same on the redirect path so diagnostics stay
      // consistent. (Defensive: redirectEndpoint is only called when the
      // opposite endpoint has already been resolved by Scene::resolveRoutes,
      // which leaves dangling DEFs as expired weak_ptrs — callers skip those
      // before getting here.)
      result.rejected.push_back(
          {routeIndex, "redirect: opposite endpoint '" + other.field +
                          "' has no reflected field"});
      return true;
    }
    for (const ProtoRedirect &t : fIt->second) {
      std::optional<FieldInfo> bodyInfo =
          detail::findField(*t.targetNode, t.targetField);
      if (!bodyInfo) {
        result.rejected.push_back(
            {routeIndex, "redirect: body target has no field '" +
                            t.targetField + "'"});
        continue;
      }
      // Direction parity with the normal route path.
      if (asSource && !detail::isRoutableSource(bodyInfo->access)) {
        result.rejected.push_back(
            {routeIndex,
             "redirect: interface field '" + field + "' resolves to body '" +
                 t.targetField + "' which is not routable as an event source "
                 "(must be outputOnly or inputOutput)"});
        continue;
      }
      if (!asSource && !detail::isRoutableSink(bodyInfo->access)) {
        result.rejected.push_back(
            {routeIndex,
             "redirect: interface field '" + field + "' resolves to body '" +
                 t.targetField + "' which is not routable as an event sink "
                 "(must be inputOnly or inputOutput)"});
        continue;
      }
      // Type parity — exact tag match, no implicit coercion.
      if (bodyInfo->type != otherInfo->type) {
        result.rejected.push_back(
            {routeIndex,
             "redirect: type mismatch routing body '" + t.targetField +
                 "' to '" + other.field + "'"});
        continue;
      }
      if (asSource) {
        ctx.addRoute({t.targetNode.get(), t.targetField}, other);
      } else {
        ctx.addRoute(other, {t.targetNode.get(), t.targetField});
      }
      ++result.routesAdded;
    }
    return true;
  };

  for (std::size_t i = 0; i < scene.routes.size(); ++i) {
    const Route &route = scene.routes[i];

    // (1) Unresolved endpoints: skip silently (forward-ref / IMPORT / unknown).
    std::shared_ptr<X3DNode> fromNode = route.from.lock();
    std::shared_ptr<X3DNode> toNode = route.to.lock();
    if (!fromNode || !toNode) {
      continue;
    }

    // (2) Unknown field on either endpoint. An unknown field that names an
    // exposed PROTO interface event field is redirected onto its IS-mapped body
    // endpoint(s) instead of being rejected (the opposite endpoint is taken as
    // the route's other side, resolved below by normal lookup).
    std::optional<FieldInfo> fromField =
        detail::findField(*fromNode, route.fromField);
    if (!fromField) {
      if (redirectEndpoint(fromNode.get(), route.fromField, /*asSource=*/true,
                           {toNode.get(), route.toField}, i)) {
        continue;
      }
      result.rejected.push_back(
          {i, "unknown source field '" + route.fromField + "' on node '" +
                  route.fromNode + "'"});
      continue;
    }
    std::optional<FieldInfo> toField = detail::findField(*toNode, route.toField);
    if (!toField) {
      if (redirectEndpoint(toNode.get(), route.toField, /*asSource=*/false,
                           {fromNode.get(), route.fromField}, i)) {
        continue;
      }
      result.rejected.push_back(
          {i, "unknown sink field '" + route.toField + "' on node '" +
                  route.toNode + "'"});
      continue;
    }

    // (3) Direction.
    if (!detail::isRoutableSource(fromField->access)) {
      result.rejected.push_back(
          {i, "source field '" + route.fromNode + "." + route.fromField +
                  "' is not routable as an event source (must be outputOnly "
                  "or inputOutput)"});
      continue;
    }
    if (!detail::isRoutableSink(toField->access)) {
      result.rejected.push_back(
          {i, "sink field '" + route.toNode + "." + route.toField +
                  "' is not routable as an event sink (must be inputOnly or "
                  "inputOutput)"});
      continue;
    }

    // (4) Type compatibility: exact tag match (no implicit coercion).
    if (fromField->type != toField->type) {
      result.rejected.push_back(
          {i, "type mismatch routing '" + route.fromNode + "." +
                  route.fromField + "' to '" + route.toNode + "." +
                  route.toField + "'"});
      continue;
    }

    // Valid: register the edge. The context only observes the nodes.
    ctx.addRoute({fromNode.get(), route.fromField},
                 {toNode.get(), route.toField});
    ++result.routesAdded;
  }

  return result;
}

namespace detail {

/// Visit every node reachable from `scene` exactly once (DEF/USE-shared nodes
/// are visited once), descending the generic SFNode/MFNode field slots — the
/// same blind descent PickSystem uses. Visited-set guards against shared
/// subgraphs and cycles. `f` receives each raw node pointer.
template <class F> inline void forEachNode(const Scene &scene, F &&f) {
  std::unordered_set<const X3DNode *> seen;
  std::function<void(X3DNode *)> rec = [&](X3DNode *n) {
    if (!n || !seen.insert(n).second) return;
    f(n);
    for (const auto &fi : n->fields()) {
      if (!fi.get) continue;
      if (fi.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(fi.get(*n));
        if (c) rec(c.get());
      } else if (fi.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi.get(*n)))
          if (c) rec(c.get());
      }
    }
  };
  for (const auto &r : scene.rootNodes) rec(r.get());
}

} // namespace detail

/**
 * @brief Production wiring (M2e): create + register one ViewDependentSystem and
 *        attach it to every LOD / ProximitySensor / VisibilitySensor in `scene`.
 * @details Lives here (not in X3DExecutionContext.hpp) because that header
 *          cannot include ViewDependentSystem.hpp (include cycle). Call after
 *          `buildSceneGraph(scene)`; the system then runs each `tick`. The
 *          attach filter is owned by ViewDependentSystem::attach — this just
 *          offers every node to it.
 */
inline void attachViewDependent(Scene &scene, X3DExecutionContext &ctx) {
  auto vds = std::make_shared<ViewDependentSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { vds->attach(n, ctx); });
  ctx.addSystem(vds);
}

/**
 * @brief Production wiring (PIV-1): register every interpolator System and
 *        attach it to every interpolator node in `scene`.
 * @details The PIV-1 production caller `registerInterpolatorSystems` lacked: it
 *          add-registers the systems but never attaches them to nodes, so
 *          interpolators stayed inert (set_fraction fired the no-op default).
 *          This mirrors `attachViewDependent` — call after `buildSceneGraph` so
 *          interpolators animate out-of-box. Each system's `attach` filters by
 *          node type, so offering every node to every system is safe.
 */
inline void attachInterpolators(Scene &scene, X3DExecutionContext &ctx) {
  for (auto &sys : makeInterpolatorSystems()) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}

/**
 * @brief Production wiring (§39 Followers): register all 14 follower systems
 *        (7 types × Damper + Chaser) and attach each to every matching node
 *        in `scene`.
 * @details Call after `buildSceneGraph` alongside `attachInterpolators` so
 *          follower nodes react to ROUTEd set_destination / set_value events
 *          out-of-box. Each system's `attach` filters by node type, so
 *          offering every node to every system is safe.
 */
inline void attachFollowers(Scene &scene, X3DExecutionContext &ctx) {
  for (auto &sys : makeFollowerSystems()) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}

/**
 * @brief Production wiring (§30 Event Utilities): register + attach the trigger,
 *        sequencer, and filter Systems to every matching node in `scene`.
 * @details Mirrors `attachInterpolators` — call after `buildSceneGraph` so the
 *          previously-inert Event Utilities nodes (BooleanTrigger/IntegerTrigger/
 *          TimeTrigger, Boolean/IntegerSequencer, BooleanFilter/BooleanToggle)
 *          react to ROUTEd events. Each system's `attach` filters by node type.
 */
inline void attachEventUtilities(Scene &scene, X3DExecutionContext &ctx) {
  std::vector<std::shared_ptr<System>> systems = {
      std::make_shared<BooleanTriggerSystem>(),
      std::make_shared<IntegerTriggerSystem>(),
      std::make_shared<TimeTriggerSystem>(),
      std::make_shared<BooleanFilterSystem>(),
      std::make_shared<BooleanToggleSystem>(),
      std::make_shared<SequencerSystem<x3d::nodes::BooleanSequencer, SFBool>>(),
      std::make_shared<SequencerSystem<x3d::nodes::IntegerSequencer, SFInt32>>(),
  };
  for (auto &sys : systems) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}

/**
 * @brief Production wiring (§21 Key Device Sensors): register + attach the
 *        KeyDeviceSensorSystem to every KeySensor / StringSensor in `scene`.
 * @details Call after `buildSceneGraph` so the previously-inert key sensors emit
 *          on the consumer's keyboard input (fed via ctx.pushKeyCharacter /
 *          pushActionKey / pushModifierKey / pushStringTerminator / pushStringDeletion).
 *          One time-driven System services all key sensors; it drains the
 *          KeyState::events queue each tick.
 */
inline void attachKeyDeviceSensors(Scene &scene, X3DExecutionContext &ctx) {
  auto sys = std::make_shared<KeyDeviceSensorSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
}

/**
 * @brief Production wiring (§9 Networking): register + attach a LoadSensorSystem
 *        to every LoadSensor in `scene`, observing each sensor's watched
 *        X3DUrlObject children through the AssetResolver seam.
 * @details Call after `buildSceneGraph`. `resolver` is the embedder's byte
 *          oracle; a null resolver installs the SEC-3-confined local-file
 *          default rooted at `baseUrl`. Returns the system so a headed embedder
 *          can set a ChildLoadPolicy / hooks (mirrors `attachInteractive`
 *          returning the NavigationSystem). The system's `setScene` is wired here
 *          so it can read `Scene::expandedInlines` for parse-time pre-seeding.
 */
inline std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene &scene, X3DExecutionContext &ctx,
                  extract::AssetResolver resolver = nullptr,
                  std::string baseUrl = "") {
  auto sys = std::make_shared<LoadSensorSystem>(std::move(resolver),
                                                std::move(baseUrl));
  sys->setScene(&scene);
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}

/**
 * @brief Production wiring: attach the full STANDARD behavior runtime — every
 *        system an X3D browser runs, MINUS the embedder-plugged seams (Script
 *        needs a JS backend; Physics needs an engine). After this call an
 *        authored `TimeSensor → Interpolator → Transform` chain animates out of
 *        the box, view-dependent nodes track the camera, key sensors fire,
 *        LoadSensors report their watched children's load state, and the
 *        viewpoint bind stack is live. `assetResolver` is the byte oracle
 *        LoadSensor resolves through (null → the SEC-3-confined local-file
 *        default rooted at `baseUrl`).
 * @details Call after `buildSceneGraph(scene)` (and `buildFrom(scene)` for
 *          ROUTEs). TimeSensor is attached first so its `fraction_changed` is
 *          available to interpolators within the same tick's cascade drain.
 *          Embedders add Script/Physics separately, and interactive consumers
 *          add `attachInteractive` on top. The CLI's `attachFullRuntime`
 *          currently composes the same system set by hand (Script/Physics on
 *          top); converging it onto this helper is a deferred dedup follow-up.
 */
inline void attachStandardRuntime(Scene &scene, X3DExecutionContext &ctx,
                                  extract::AssetResolver assetResolver = nullptr,
                                  std::string baseUrl = "") {
  auto tss = std::make_shared<TimeSensorSystem>();        // §8 Time — the clock
  detail::forEachNode(scene, [&](X3DNode *n) { tss->attach(n, ctx); });
  ctx.addSystem(tss);
  attachInterpolators(scene, ctx);    // §19 keyframe animation
  attachFollowers(scene, ctx);        // §39 damper/chaser smoothing
  attachEventUtilities(scene, ctx);   // §30 trigger/sequencer/filter logic
  attachViewDependent(scene, ctx);    // §22/§23 LOD/Billboard/Proximity/Visibility
  attachKeyDeviceSensors(scene, ctx); // §21 KeySensor/StringSensor
  attachLoadSensors(scene, ctx, std::move(assetResolver), // §9 LoadSensor
                    std::move(baseUrl));
  attachViewpointBind(ctx);           // §23.3.1 post-cascade viewpoint bind hook
}

/**
 * @brief Interactive consumer wiring (reference browser / CAVE-preview).
 * @details Adds the two pointer-driven systems in arbitration order:
 *          PointingSensorSystem FIRST so a sensor grab claims the pointer before
 *          NavigationSystem reads it the same tick. Returns the NavigationSystem
 *          so the embedder can call setForcedMode (the dev mode-cycle key).
 *          NavigationSystem/PointingSensorSystem both resolve their targets live
 *          from the input seam each tick, so neither needs per-node attach.
 *          Call after `buildSceneGraph(scene)`.
 */
inline std::shared_ptr<NavigationSystem>
attachInteractive(Scene &scene, X3DExecutionContext &ctx) {
  auto pss = std::make_shared<PointingSensorSystem>();
  // One-time inventory pass: lets the system skip the per-tick whole-scene pick
  // when the scene holds no pointing-device sensors (the common static-exhibit
  // case). Sensors still resolve live from the pick path; this only counts them.
  detail::forEachNode(scene, [&](X3DNode *n) { pss->attach(n, ctx); });
  ctx.addSystem(pss); // claims pointer first
  auto nav = std::make_shared<NavigationSystem>();
  ctx.addSystem(nav);                                      // reads pointer after
  return nav;
}

/**
 * @brief Convenience: bridge `scene` onto this context (see buildRoutes).
 * @details Declared on X3DExecutionContext (forward-declared Scene/BridgeResult)
 *          and defined here, where both types are complete.
 */
inline BridgeResult X3DExecutionContext::buildFrom(Scene &scene) {
  return buildRoutes(scene, *this);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCENE_BRIDGE_HPP
