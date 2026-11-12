// X3DSceneBridge.hpp
// Bridges a parsed Scene's DEF-named ROUTEs onto the pointer-based EventGraph
// held by an X3DExecutionContext, and wires the standard behavior systems.
// Reflection-driven: direction and type checks come from the nodes' FieldTables.
// The definitions live in X3DSceneBridge.cpp; include a system's own header to
// use the system objects these functions return.
#ifndef X3D_RUNTIME_SCENE_BRIDGE_HPP
#define X3D_RUNTIME_SCENE_BRIDGE_HPP

#include "AnchorSystem.hpp"       // AnchorSystem::AnchorHandler
#include "AssetResolver.hpp"      // extract::AssetResolver, AssetResolverWithBase
#include "InlineResolver.hpp"
#include "ShaderOptions.hpp"      // extract::ShaderOptions
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

class LoadSensorSystem;
class ShaderSystem;
class TextureProjectorSystem;
class GeneratedCubeMapSystem;
class NavigationSystem;

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
 *          field / wrong direction / type mismatch) and were not added. Routes
 *          with an unresolved endpoint (forward-ref / IMPORTed / unknown DEF)
 *          are skipped silently — they affect neither count, matching the
 *          serialization tolerance for dangling DEF names.
 */
struct BridgeResult {
  std::size_t routesAdded = 0;
  std::vector<RouteError> rejected;

  bool ok() const { return rejected.empty(); }
};

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
 *          expanded primary and may fan out through multiple is targets.
 *          Pre-resolved PROTO and Inline routes undergo the same physical
 *          endpoint checks; their diagnostic indices use their own scope.
 *
 *          Valid routes are added via `ctx.addRoute()` using the resolved raw
 *          node pointers; the context observes the nodes (the Scene owns their
 *          lifetime). Never throws on a bad route — diagnostics are returned.
 */
BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx);

namespace detail {

/// Visit every node reachable from `scene` exactly once (DEF/USE-shared nodes
/// are visited once), descending the generic SFNode/MFNode field slots — the
/// Visited-set guards against shared
/// subgraphs and cycles. `f` receives each raw node pointer.
void forEachNode(const Scene &scene, const std::function<void(X3DNode *)> &f);

} // namespace detail

/// Call after buildSceneGraph. Kept here to avoid an include cycle with ViewDependentSystem.
void attachViewDependent(Scene &scene, X3DExecutionContext &ctx);

/// Call after buildSceneGraph; each system filters attachments by node type.
void attachInterpolators(Scene &scene, X3DExecutionContext &ctx);

/// §39: call after buildSceneGraph; each follower system filters attachments by node type.
void attachFollowers(Scene &scene, X3DExecutionContext &ctx);

/// §30: call after buildSceneGraph; each system filters attachments by node type.
void attachEventUtilities(Scene &scene, X3DExecutionContext &ctx);

/// §21: call after buildSceneGraph; keyboard events drive the attached sensors.
void attachKeyDeviceSensors(Scene &scene, X3DExecutionContext &ctx);

/**
 * Call after buildSceneGraph. A null resolver reports Failed; inject a backend
 * for byte loading. Returns the system for policy hooks and retains a scene observer.
 * Local-file resolvers must confine paths; see ADR-0038.
 */
std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene &scene, X3DExecutionContext &ctx,
                  extract::AssetResolver resolver = nullptr,
                  extract::AssetResolverWithBase resolverWithBase = {},
                  std::string baseUrl = {});

/**
 * @brief §31 shader selection events: one ShaderSystem over every Appearance,
 *        shader and part in `scene`, emitting isSelected/isValid and honoring
 *        activate. Pass the same ShaderOptions the extractor uses
 *        (MeshBuildOptions::shaders) so both agree on the selection.
 */
std::shared_ptr<ShaderSystem>
attachShaders(Scene &scene, X3DExecutionContext &ctx,
              extract::ShaderOptions options = {});

/**
 * @brief §42 texture projectors: emit aspectRatio for PixelTexture images.
 */
std::shared_ptr<TextureProjectorSystem>
attachTextureProjectors(Scene &scene, X3DExecutionContext &ctx);

/**
 * @brief §34.4.2 GeneratedCubeMapTexture: reset `update` `NEXT_FRAME_ONLY` to
 *        `NONE` at the start of the frame after the one that rendered it.
 */
std::shared_ptr<GeneratedCubeMapSystem>
attachGeneratedCubeMaps(Scene &scene, X3DExecutionContext &ctx);

/**
 * Attach standard behavior systems after buildSceneGraph and buildFrom (for ROUTEs).
 * TimeSensor precedes interpolators so fraction events propagate in the same tick.
 * Add Script/Physics separately, and attachInteractive for pointer navigation.
 * A null assetResolver uses the IO-free failed-loading stub.
 *
 * One mutable node graph belongs to one live activation. Installed input handlers
 * hold weak activation/system guards; retirement leaves retained nodes inert and
 * preserves replacement callbacks. Custom handlers must use ctx.guardCallback.
 * Retire or destroy only outside runtime calls and callbacks.
 */
void attachStandardRuntime(Scene &scene, X3DExecutionContext &ctx,
                           extract::AssetResolver assetResolver = nullptr,
                           InlineResolver inlineResolver = {},
                           std::string baseUrl = {},
                           extract::ShaderOptions shaderOptions = {},
                           extract::AssetResolverWithBase assetResolverWithBase = {});

/// Install pointing-device sensors without anchors or navigation. Call after
/// buildSceneGraph(scene), before the first tick (and before navigation, if any).
void attachPointingSensors(Scene &scene, X3DExecutionContext &ctx);

/**
 * @brief Interactive consumer wiring (reference browser / CAVE-preview).
 * @details Adds the two pointer-driven systems in arbitration order:
 *          PointingSensorSystem first so a sensor grab claims the pointer before
 *          NavigationSystem reads it the same tick. Returns the NavigationSystem
 *          so the embedder can call setForcedMode (the dev mode-cycle key).
 *          `anchorHandler` receives activated Anchors whose url is not an
 *          in-scene "#Viewpoint" (load a world, open a window, or ignore).
 *          NavigationSystem/PointingSensorSystem both resolve their targets live
 *          from the input seam each tick, so neither needs per-node attach.
 *          Call after `buildSceneGraph(scene)`.
 */
std::shared_ptr<NavigationSystem>
attachInteractive(Scene &scene, X3DExecutionContext &ctx,
                  AnchorSystem::AnchorHandler anchorHandler = {});

/// Enabled pointing sensors tied at the lowest scope of `pick`
/// (PointingSensorSystem::resolve); empty on a miss.
std::vector<X3DNode *> resolvePointingSensors(const PickResult &pick);

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCENE_BRIDGE_HPP
