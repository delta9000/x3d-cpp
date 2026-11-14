// X3DSceneBridge.cpp — ROUTE bridging and the standard system wiring declared in
// X3DSceneBridge.hpp. Compiled once into x3d_cpp_runtime so that including the
// bridge does not pull every system header into the consumer's TU.
#include "X3DSceneBridge.hpp"

#include "AnchorSystem.hpp"
#include "DynamicField.hpp"
#include "EventUtilitySystem.hpp"
#include "FieldRead.hpp"
#include "FollowerRegistration.hpp"
#include "GeneratedCubeMapSystem.hpp"
#include "InlineRuntimeSystem.hpp"
#include "InterpolatorRegistration.hpp"
#include "KeyDeviceSensorSystem.hpp"
#include "LoadSensorSystem.hpp"
#include "MediaTimeSystem.hpp"
#include "NavigationSystem.hpp"
#include "PickSensorSystem.hpp"
#include "PointingSensorSystem.hpp"
#include "ShaderSystem.hpp"
#include "SoundTimeSystem.hpp"
#include "TextureProjectorSystem.hpp"
#include "TimeSensorSystem.hpp"
#include "ViewDependentSystem.hpp"
#include "ViewpointBindSystem.hpp"
#include "../hanim/HAnimMotionSystem.hpp"
#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"

#include <optional>
#include <unordered_set>

namespace x3d::runtime {

namespace detail {

std::optional<FieldInfo> findField(const X3DNode &node,
                                          const std::string &x3dName,
                                          const DynamicFieldStore &store) {
  for (FieldInfo &f : effectiveFields(node, store)) {
    if (f.x3dName == x3dName) {
      return std::move(f);
    }
  }
  return std::nullopt;
}

std::optional<FieldInfo> findEndpoint(const X3DNode &node,
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

bool isRoutableSource(AccessType a) {
  return a == AccessType::OutputOnly || a == AccessType::InputOutput;
}

bool isRoutableSink(AccessType a) {
  return a == AccessType::InputOnly || a == AccessType::InputOutput;
}

} // namespace detail

BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx) {
  ctx.bindSceneAuthorFields(scene);
  BridgeResult result;
  ctx.clearRoutes();
  ctx.markSceneTopologyChanged(); // Includes replacement of private PROTO ROUTEs.
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
      // No is target: the interface field is still a real endpoint on the
      // instance (independent storage registered at expansion), so resolve it
      // physically — an inputOutput field echoes a set value as its _changed
      // event through the ordinary route graph..
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

void forEachNode(const Scene &scene, const std::function<void(X3DNode *)> &f) {
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

void attachViewDependent(Scene &scene, X3DExecutionContext &ctx) {
  auto vds = std::make_shared<ViewDependentSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { vds->attach(n, ctx); });
  ctx.addSystem(vds);
}

void attachInterpolators(Scene &scene, X3DExecutionContext &ctx) {
  for (auto &sys : makeInterpolatorSystems()) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}

void attachFollowers(Scene &scene, X3DExecutionContext &ctx) {
  for (auto &sys : makeFollowerSystems()) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}

void attachEventUtilities(Scene &scene, X3DExecutionContext &ctx) {
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

void attachKeyDeviceSensors(Scene &scene, X3DExecutionContext &ctx) {
  auto sys = std::make_shared<KeyDeviceSensorSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
}

std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene &scene, X3DExecutionContext &ctx,
                  extract::AssetResolver resolver,
                  extract::AssetResolverWithBase resolverWithBase,
                  std::string baseUrl) {
  auto sys = std::make_shared<LoadSensorSystem>(
      std::move(resolver), std::move(resolverWithBase), std::move(baseUrl));
  sys->setScene(&scene);
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}

std::shared_ptr<ShaderSystem>
attachShaders(Scene &scene, X3DExecutionContext &ctx,
              extract::ShaderOptions options) {
  auto sys = std::make_shared<ShaderSystem>(std::move(options));
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}

std::shared_ptr<TextureProjectorSystem>
attachTextureProjectors(Scene &scene, X3DExecutionContext &ctx) {
  auto sys = std::make_shared<TextureProjectorSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}

std::shared_ptr<GeneratedCubeMapSystem>
attachGeneratedCubeMaps(Scene &scene, X3DExecutionContext &ctx) {
  auto sys = std::make_shared<GeneratedCubeMapSystem>();
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}

void attachStandardRuntime(Scene &scene, X3DExecutionContext &ctx,
                           extract::AssetResolver assetResolver,
                           InlineResolver inlineResolver,
                           std::string baseUrl,
                           extract::ShaderOptions shaderOptions,
                           extract::AssetResolverWithBase assetResolverWithBase) {
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
  attachLoadSensors(scene, ctx, std::move(assetResolver),
                    std::move(assetResolverWithBase), baseUrl); // §9 LoadSensor
  attachShaders(scene, ctx, std::move(shaderOptions)); // §31 isSelected/isValid
  attachGeneratedCubeMaps(scene, ctx); // §34.4.2 `NEXT_FRAME_ONLY` reset
  attachTextureProjectors(scene, ctx); // §42 aspectRatio
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

void attachPointingSensors(Scene &scene, X3DExecutionContext &ctx) {
  auto pss = std::make_shared<PointingSensorSystem>();
  // One-time inventory pass: lets the system skip the per-tick whole-scene pick
  // when the scene holds no pointing-device sensors (the common static-exhibit
  // case). Sensors still resolve live from the pick path; this only counts them.
  detail::forEachNode(scene, [&](X3DNode *n) { pss->attach(n, ctx); });
  ctx.addSystem(pss);
}

std::shared_ptr<NavigationSystem>
attachInteractive(Scene &scene, X3DExecutionContext &ctx,
                  AnchorSystem::AnchorHandler anchorHandler) {
  attachPointingSensors(scene, ctx); // claims pointer first
  auto anchors = std::make_shared<AnchorSystem>(); // §9.4.1 Anchor activation
  anchors->setAnchorHandler(std::move(anchorHandler)); // non-"#Name" urls
  detail::forEachNode(scene, [&](X3DNode *n) { anchors->attach(n, ctx); });
  ctx.addSystem(anchors); // after the sensors, before navigation
  auto nav = std::make_shared<NavigationSystem>();
  ctx.addSystem(nav);                                      // reads pointer after
  return nav;
}

BridgeResult X3DExecutionContext::buildFrom(Scene &scene) {
  bindSceneAuthorFields(scene);
  normalizeRuntimeUnits(scene);
  return buildRoutes(scene, *this);
}

std::vector<X3DNode *> resolvePointingSensors(const PickResult &pick) {
  return PointingSensorSystem::resolve(pick);
}

} // namespace x3d::runtime
