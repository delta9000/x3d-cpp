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

#include "FieldRead.hpp"
#include "X3DExecutionContext.hpp"
#include "EventUtilitySystem.hpp"
#include "FollowerRegistration.hpp"
#include "InterpolatorRegistration.hpp"
#include "KeyDeviceSensorSystem.hpp"
#include "LoadSensorSystem.hpp"
#include "AnchorSystem.hpp"
#include "MediaTimeSystem.hpp"
#include "SoundTimeSystem.hpp"
#include "NavigationSystem.hpp"
#include "PickSensorSystem.hpp"
#include "PointingSensorSystem.hpp"
#include "TimeSensorSystem.hpp"
#include "ViewDependentSystem.hpp"
#include "ViewpointBindSystem.hpp"
#include "../hanim/HAnimMotionSystem.hpp"

#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"

#include "DynamicField.hpp"
#include "InlineRuntimeSystem.hpp"
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
 * @details `index` is the position within the route collection identified by
 *          `scope`, so a caller can correlate it with its source.
 */
struct RouteError {
  enum class Scope { Scene, ProtoBody, Inline };
  std::size_t index = 0;
  std::string reason;    // human-readable explanation
  Scope scope = Scope::Scene;
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
                                          const std::string &x3dName,
                                          const DynamicFieldStore &store) {
  for (FieldInfo &f : effectiveFields(node, store)) {
    if (f.x3dName == x3dName) {
      return std::move(f);
    }
  }
  return std::nullopt;
}

/// Resolve a ROUTE endpoint name, accepting the §4.4.2.2 aliases of an
/// inputOutput field `zzz`: `set_zzz` on the sink side and `zzz_changed` on
/// the source side. An exact field name always wins. The returned FieldInfo
/// carries the canonical name the route must be registered under.
inline std::optional<FieldInfo> findEndpoint(const X3DNode &node,
                                             const std::string &x3dName,
                                             bool asSource,
                                             const DynamicFieldStore &store) {
  if (auto exact = findField(node, x3dName, store)) return exact;
  std::string base;
  if (!asSource && x3dName.rfind("set_", 0) == 0)
    base = x3dName.substr(4);
  else if (asSource && x3dName.size() > 8 &&
           x3dName.compare(x3dName.size() - 8, 8, "_changed") == 0)
    base = x3dName.substr(0, x3dName.size() - 8);
  if (base.empty()) return std::nullopt;
  auto field = findField(node, base, store);
  if (field && field->access == AccessType::InputOutput) return field;
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
 *          Declared PROTO interface fields take precedence over fields on an
 *          expanded primary and may fan out through multiple IS targets.
 *          Pre-resolved PROTO and Inline routes undergo the same physical
 *          endpoint checks; their diagnostic indices use their own scope.
 *
 *          Valid routes are added via `ctx.addRoute()` using the resolved raw
 *          node pointers; the context observes the nodes (the Scene owns their
 *          lifetime). Never throws on a bad route — diagnostics are returned.
 */
inline BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx) {
  ctx.bindSceneAuthorFields(scene);
  BridgeResult result;
  ctx.clearRoutes();
  scene.resolveRoutes();
  using Scope = RouteError::Scope;
  auto reject = [&](std::size_t index, Scope scope, std::string reason) {
    result.rejected.push_back({index, std::move(reason), scope});
  };

  // These endpoints are already physical nodes. Never reinterpret them through
  // the PROTO redirect map: nested interfaces can share a primary pointer.
  auto addPhysical = [&](const std::shared_ptr<X3DNode> &from,
                         const std::string &fromName,
                         const std::shared_ptr<X3DNode> &to,
                         const std::string &toName, std::size_t index,
                         Scope scope,
                         std::optional<X3DFieldType> nominalSource,
                         std::optional<X3DFieldType> nominalSink) {
    if (!from || !to) return;
    auto source = detail::findEndpoint(*from, fromName, true, ctx.authorFields());
    auto sink = detail::findEndpoint(*to, toName, false, ctx.authorFields());
    if (!source) {
      reject(index, scope, "unknown source field '" + fromName + "'");
    } else if (!sink) {
      reject(index, scope, "unknown sink field '" + toName + "'");
    } else if (!detail::isRoutableSource(source->access)) {
      reject(index, scope, "source field '" + fromName +
                               "' is not routable as an event source "
                               "(must be outputOnly or inputOutput)");
    } else if (!detail::isRoutableSink(sink->access)) {
      reject(index, scope, "sink field '" + toName +
                               "' is not routable as an event sink "
                               "(must be inputOnly or inputOutput)");
    } else if ((nominalSource && source->type != *nominalSource) ||
               (nominalSink && sink->type != *nominalSink)) {
      reject(index, scope, "interface type mismatch with physical ROUTE target");
    } else if (source->type != sink->type) {
      reject(index, scope, "type mismatch routing '" + fromName +
                               "' to '" + toName + "'");
    } else {
      ctx.addRoute({from.get(), source->x3dName},
                   {to.get(), sink->x3dName});
      ++result.routesAdded;
    }
  };

  for (std::size_t i = 0; i < scene.resolvedProtoRoutes.size(); ++i) {
    const auto &r = scene.resolvedProtoRoutes[i];
    addPhysical(r.from, r.fromField, r.to, r.toField, i, Scope::ProtoBody,
                std::nullopt, std::nullopt);
  }
  for (std::size_t i = 0; i < scene.resolvedInlineRoutes.size(); ++i) {
    const auto &r = scene.resolvedInlineRoutes[i];
    addPhysical(r.from, r.fromField, r.to, r.toField, i, Scope::Inline,
                std::nullopt, std::nullopt);
  }

  struct Endpoint {
    X3DFieldType type;
    AccessType access;
    std::vector<ProtoRedirect> targets;
  };
  auto resolveSceneEndpoint = [&](const std::shared_ptr<X3DNode> &node,
                                  const std::string &field,
                                  const std::string &def, bool asSource,
                                  std::size_t index)
      -> std::optional<Endpoint> {
    const std::string side = asSource ? "source" : "sink";
    auto source = scene.expandedSources.find(node.get());
    if (source != scene.expandedSources.end() &&
        (source->second.declaration || source->second.externDeclaration)) {
      // Every PROTO instance inherits metadata even though authors cannot
      // redeclare it. Its current storage is the expanded primary node.
      if (field == "metadata" || (!asSource && field == "set_metadata") ||
          (asSource && field == "metadata_changed")) {
        auto metadata = detail::findEndpoint(*node, field, asSource, ctx.authorFields());
        if (metadata && metadata->x3dName == "metadata")
          return Endpoint{metadata->type, metadata->access,
                          {{node, metadata->x3dName}}};
      }
      const auto &instance = source->second;
      const auto &fields = instance.externDeclaration
                               ? instance.externDeclaration->interface
                               : instance.declaration->interface;
      const ProtoField *interface =
          findProtoRouteField(fields, field, asSource);
      if (!interface) {
        reject(index, Scope::Scene, "unknown " + side + " interface field '" +
                                        field + "' on node '" + def + "'");
        return std::nullopt;
      }
      auto nIt = scene.protoRedirects.find(node.get());
      if (nIt != scene.protoRedirects.end() &&
          nIt->second.contains(interface->name) &&
          !nIt->second.at(interface->name).empty())
        return Endpoint{interface->type, interface->access,
                        nIt->second.at(interface->name)};
      // No IS target: the interface field is still a real endpoint on the
      // instance (independent storage registered at expansion), so resolve it
      // physically — an inputOutput field echoes a set value as its _changed
      // event through the ordinary route graph. PROTO-INTERFACE-STATE.
      if (auto own = detail::findEndpoint(*node, field, asSource, ctx.authorFields()))
        return Endpoint{interface->type, own->access, {{node, own->x3dName}}};
      reject(index, Scope::Scene, "interface field '" + def + "." + field +
                                      "' has no IS route target");
      return std::nullopt;
    }
    auto reflected = detail::findEndpoint(*node, field, asSource, ctx.authorFields());
    if (!reflected) {
      reject(index, Scope::Scene, "unknown " + side + " field '" + field +
                                      "' on node '" + def + "'");
      return std::nullopt;
    }
    return Endpoint{reflected->type, reflected->access,
                    {{node, reflected->x3dName}}};
  };

  for (std::size_t i = 0; i < scene.routes.size(); ++i) {
    const Route &route = scene.routes[i];
    auto from = route.from.lock();
    auto to = route.to.lock();
    if (!from || !to) continue;
    auto source = resolveSceneEndpoint(from, route.fromField, route.fromNode,
                                       true, i);
    if (!source) continue;
    auto sink = resolveSceneEndpoint(to, route.toField, route.toNode, false, i);
    if (!sink) continue;
    if (!detail::isRoutableSource(source->access)) {
      reject(i, Scope::Scene, "source field '" + route.fromNode + "." +
                                  route.fromField +
                                  "' is not routable as an event source "
                                  "(must be outputOnly or inputOutput)");
      continue;
    }
    if (!detail::isRoutableSink(sink->access)) {
      reject(i, Scope::Scene, "sink field '" + route.toNode + "." +
                                  route.toField +
                                  "' is not routable as an event sink "
                                  "(must be inputOnly or inputOutput)");
      continue;
    }
    if (source->type != sink->type) {
      reject(i, Scope::Scene, "type mismatch routing '" + route.fromNode +
                                  "." + route.fromField + "' to '" +
                                  route.toNode + "." + route.toField + "'");
      continue;
    }
    for (const auto &physicalSource : source->targets)
      for (const auto &physicalSink : sink->targets)
        addPhysical(physicalSource.targetNode, physicalSource.targetField,
                    physicalSink.targetNode, physicalSink.targetField, i,
                    Scope::Scene, source->type, sink->type);
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
    forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      rec(c.get());
    });
  };
  for (const auto &r : scene.rootNodes) rec(r.get());
  for (const auto &p : scene.protoPeerNodes) rec(p.get());
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
 *          oracle; a null resolver installs the IO-free null stub (Failed) — the
 *          SDK ships no concrete backend, so an app injects one (the CLI wires
 *          io::file::makeFileResolver for SEC-3-confined local files). Returns
 *          the system so a headed embedder can set a ChildLoadPolicy / hooks
 *          (mirrors `attachInteractive` returning the NavigationSystem). The
 *          system's `setScene` is wired here so it can read
 *          `Scene::expandedInlines` for parse-time pre-seeding.
 */
inline std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene &scene, X3DExecutionContext &ctx,
                  extract::AssetResolver resolver = nullptr) {
  auto sys = std::make_shared<LoadSensorSystem>(std::move(resolver));
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
 *        LoadSensor resolves through (null → the IO-free null stub; an app
 *        injects a concrete backend, e.g. the CLI's confined local-file
 *        resolver).
 * @details Call after `buildSceneGraph(scene)` (and `buildFrom(scene)` for
 *          ROUTEs). TimeSensor is attached first so its `fraction_changed` is
 *          available to interpolators within the same tick's cascade drain.
 *          Embedders add Script/Physics separately, and interactive consumers
 *          add `attachInteractive` on top.
 *          One node graph belongs to one live activation: do not attach the
 *          same mutable nodes to two live contexts. All installed input handlers
 *          have weak activation/system guards; context retirement makes retained
 *          nodes inert and never clears a replacement activation's callbacks.
 *          Custom systems must use ctx.guardCallback(*this, handler) for the same
 *          guarantee. Retire/destroy only outside runtime calls and callbacks.
 *          The CLI's attachFullRuntime composes the same set by hand (Script/Physics on
 *          top); converging it onto this helper is a deferred dedup follow-up.
 */
inline void attachStandardRuntime(Scene &scene, X3DExecutionContext &ctx,
                                  extract::AssetResolver assetResolver = nullptr,
                                  InlineResolver inlineResolver = {},
                                  std::string baseUrl = {}) {
  auto tss = std::make_shared<TimeSensorSystem>();        // §8 Time — the clock
  detail::forEachNode(scene, [&](X3DNode *n) { tss->attach(n, ctx); });
  ctx.addSystem(tss);
  auto media = std::make_shared<MediaTimeSystem>(); // §8.2.4 AudioClip/MovieTexture timing
  detail::forEachNode(scene, [&](X3DNode *n) { media->attach(n, ctx); });
  ctx.addSystem(media);
  auto soundTime = std::make_shared<SoundTimeSystem>(); // §16 source/processor timing
  detail::forEachNode(scene, [&](X3DNode *n) { soundTime->attach(n, ctx); });
  ctx.addSystem(soundTime);
  auto motions = std::make_shared<hanim::HAnimMotionSystem>(); // §26 H-Anim motion
  detail::forEachNode(scene, [&](X3DNode *n) { motions->attach(n, ctx); });
  ctx.addSystem(motions);
  attachInterpolators(scene, ctx);    // §19 keyframe animation
  attachFollowers(scene, ctx);        // §39 damper/chaser smoothing
  attachEventUtilities(scene, ctx);   // §30 trigger/sequencer/filter logic
  attachViewDependent(scene, ctx);    // §22/§23 LOD/Billboard/Proximity/Visibility
  attachKeyDeviceSensors(scene, ctx); // §21 KeySensor/StringSensor
  auto picks = std::make_shared<PickSensorSystem>(); // §38 pick sensors
  detail::forEachNode(scene, [&](X3DNode *n) { picks->attach(n, ctx); });
  ctx.addSystem(picks);
  attachLoadSensors(scene, ctx, std::move(assetResolver)); // §9 LoadSensor
  if (inlineResolver) {
    auto inlines = std::make_shared<InlineRuntimeSystem>(
        scene, std::move(inlineResolver), std::move(baseUrl));
    detail::forEachNode(scene, [&](X3DNode *n) { inlines->attach(n, ctx); });
    for (const auto &[_, original] : scene.expandedInlines)
      inlines->attach(original.get(), ctx);
    ctx.addSystem(inlines);
  }
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
  auto anchors = std::make_shared<AnchorSystem>(); // §9.4.1 Anchor activation
  detail::forEachNode(scene, [&](X3DNode *n) { anchors->attach(n, ctx); });
  ctx.addSystem(anchors); // after the sensors, before navigation
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
  bindSceneAuthorFields(scene);
  normalizeRuntimeUnits(scene);
  return buildRoutes(scene, *this);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCENE_BRIDGE_HPP
