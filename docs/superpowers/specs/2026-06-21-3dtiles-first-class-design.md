# 3D Tiles as a First-Class Runtime Surface — Design

**Date:** 2026-06-21
**Status:** Design proposal for ratification.
**Grounding:** `x3d-cpp-gen-architecture` (peer runtime surface, not ext-firewalled); `x3d-cpp-gen-ingestion-roadmap` (scene-source vs single-mesh routing split).

## Goal

Make the OGC 3D Tiles 1.1 spec a **peer runtime surface** of x3d-cpp-gen — native C++ types, a streaming fetcher seam, hierarchical culling fed by the existing per-tick pipeline, and full content-format coverage (b3dm / i3dm / pnts / cmpt / vctr) — so embedders can consume large geospatial tilesets (city-scale, photogrammetry, point-cloud, vector) through the same `X3DExecutionContext` + `SceneExtractor` surface they already use for native X3D scenes.

The defining property: tiles **become X3D nodes** when materialised, so they participate in pick / routes / TouchSensor / Script / cascade / extract uniformly. Selection runs as a registered `System` (peer to `NavigationSystem`), not as a parallel runtime.

## Locked decisions

1. **Peer runtime surface, not ext-firewalled.** 3D Tiles is a peer OGC spec, not a non-spec extension. The runtime gains `runtime/3dtiles/` as a sibling to `runtime/extract/`, with a static `x3d_cpp_tiles` library unconditionally built (no `X3D_CPP_BUILD_*` flag). The ext firewall's purpose is to quarantine non-spec code from the spec-correct core; 3D Tiles is a spec we are embracing as first-class.

2. **`TilesetSystem` is a registered `System` inside `X3DExecutionContext`.** Selection runs as `ctx.tick(now) → TilesetSystem::update(now, ctx)`, reusing the existing dirty-tracking / scene-graph / extract machinery. Not a parallel context with a separate scene graph.

3. **Materialise through a synthetic anchor `Group` + `addChildren` / `removeChildren`.** Tiles become X3D subtrees under one anchor Group owned by the system. The existing `DirtyChildren` propagation + `SceneExtractor::delta()` picks up additions / removals / transforms for free — zero new scene-graph mechanism.

4. **Fetcher seam returns bytes (mirrors `AssetResolver`).** Five content codecs (`runtime/3dtiles/codecs/`) own decoding. Embedders supply HTTP / cache / CDN / signed-URL logic via the fetcher; the codecs don't care.

5. **v1 = explicit tilesets, all five content formats.** Implicit tiling (3D Tiles 4.0 Subtree codec) deferred to v1.1. Full JSONata in Style deferred. Vector glyph rasterisation out of scope (line geometry emitted; embedder renders glyphs).

6. **Batch table → X3D `MetadataSet`; Style → `Appearance` override.** Per-feature attributes are first-class X3D Metadata nodes so embedders can `ROUTE Tile_X_metadata.set_myField_changed TO TouchSensor.isOver`. 3D Tiles Style JSON parses into a `StyleDescriptor` evaluated against the batch table and applied as a post-pass over the materialised `Appearance`.

## Architecture

```
embedder
   │
   ├── x3d::sdk                (existing façade)
   │     X3DExecutionContext ───┐
   │     SceneExtractor        │
   │     AssetResolver         │   shared seam (bytes by URL)
   │                           │
   └── x3d::sdk::tiles   ◀──── NEW
         TilesetContext        │   thin façade
         TilesetFetcher        │   embedder-supplied seam (bytes by URL)
         TilesetSystem ◀───────┘   registered via ctx.addSystem(tilesetSystem)
              │
              ▼  per tick (inside ctx.tick(now))
         selection(refine, geometricError, frustum, viewerRequestVolume)
              │
              ├─ fetcher.fetch(tileset.json) ─► codec → Tileset (in-memory tree)
              └─ fetcher.fetch(content URL)  ─► codec → TileContent
                                                  │
                                                  ▼
                                          Materialiser::into(parentGroup)
                                                  │
                                                  ▼
                                          <TilesetRoot> Group
                                          ├─ Tile A subtree  (Shape + IndexedTriangleSet + MetadataSet)
                                          ├─ Tile B subtree
                                          └─ …
                                                  │
                                                  ▼
                                         (existing) SceneExtractor::delta()
                                                  │
                                                  ▼
                                             RenderItem stream
```

### Module layout

```
runtime/3dtiles/
  Tileset.hpp                       data model
  TilesetFetcher.hpp                seam
  TilesetSystem.hpp                 registered behavior
  Materialiser.hpp                  TileContent -> X3D subtree
  StyleParser.hpp                   Style JSON -> StyleDescriptor
  codecs/
    TilesetJsonReader.hpp
    B3dmReader.hpp                  (uses EmbeddedGltfReader)
    I3dmReader.hpp                  (uses EmbeddedGltfReader)
    PntsReader.hpp
    CmptReader.hpp
    VctrReader.hpp
    EmbeddedGltfReader.hpp          minimal glTF for b3dm/i3dm
  tests/                            gate tests (no firewall; unconditional)
include/x3d/3dtiles.hpp             re-exports under x3d::sdk::tiles
```

New CMake target `x3d_cpp_tiles` (static lib) is added unconditionally. The `x3d_cpp::sdk` INTERFACE target gains `target_link_libraries(... x3d_cpp::tiles)`.

### Data model

`runtime/3dtiles/Tileset.hpp` carries plain-old data — no scene-graph pointers, no shared ownership. Tree-walking happens on this tree; materialisation adds X3D nodes separately.

```cpp
namespace x3d::runtime::tiles {

struct Tileset {
  std::string version;          // "1.1"
  Asset       asset;            // { version, generator }
  double      geometricError;   // root fallback
  Tile        root;
  JsonValue   properties;
  std::vector<std::string> extensionsUsed;
};

struct Tile {
  BoundingVolume                  boundingVolume;
  std::optional<BoundingVolume>   viewerRequestVolume;
  double                          geometricError = 0.0;
  TileRefine                      refine = TileRefine::Replace;
  std::optional<TileContentRef>   content;       // {url, kind, metadata}
  std::vector<Tile>               children;
  std::optional<Mat4>             transform;     // applied to children
  JsonValue                       metadata;
};

struct BoundingVolume {
  std::variant<Box, Region, Sphere> vol;
  // Box    { double c[3], half[3] }
  // Region { w, s, e, n (rad), minH, maxH (m) }
  // Sphere { double c[3], r }
};

enum class TileRefine { Replace, Add };
enum class TileContentKind { B3dm, I3dm, Pnts, Cmpt, Vctr };

struct TileContent {
  TileContentKind kind;
  // b3dm / i3dm
  Gltf            gltf;
  BatchTable      batchTable;
  std::size_t     featureCount = 0;
  // i3dm
  std::vector<Instance> instances;   // {translation, rotation, scale, batchId}
  // pnts
  std::vector<float>    pntsPositions;
  std::vector<float>    pntsNormals;
  std::vector<uint8_t>  pntsColors;
  std::size_t           pntsCount = 0;
  // cmpt
  std::vector<TileContent> inner;
  // vctr
  std::vector<float> vctrPositions;
  std::vector<std::pair<std::string, std::vector<float>>> vctrAttributes;
};

} // namespace x3d::runtime::tiles
```

### Fetcher seam

`runtime/3dtiles/TilesetFetcher.hpp`:

```cpp
namespace x3d::runtime::tiles {

enum class TilesetFetchStatus { Ready, Pending, Failed };
struct TilesetFetchResult {
  TilesetFetchStatus status;
  std::vector<uint8_t> bytes;
};

using TilesetFetcher = std::function<TilesetFetchResult(
    const std::string &url, TilesetFetchKind kind)>;

// Kind ∈ { TilesetJson, B3dm, I3dm, Pnts, Cmpt, Vctr }
// Pending is permitted (render-time lazy); embedder can cache, retry, prefetch.
TilesetFetcher makeFileTilesetFetcher(std::filesystem::path root);
TilesetFetcher makeNullTilesetFetcher();   // always Failed (default for tests)

} // namespace x3d::runtime::tiles
```

Bytes, not typed content — the codecs own decoding. Embedders supply HTTP/2, signed URLs, CDN, caching; the codecs don't care.

### Codecs

Five pure parsers in `runtime/3dtiles/codecs/`. Each takes `bytes` + `len`, returns a `TileContent`:

| Header | Reads | Decodes |
|---|---|---|
| `TilesetJsonReader.hpp` | `tileset.json` (UTF-8 JSON) | `Tileset` tree (cap depth/children at parser boundary to prevent pathological tilesets) |
| `B3dmReader.hpp` | binary b3dm (header + batch table + glTF) | `TileContent{b3dm, gltf, batchTable, featureCount}` |
| `I3dmReader.hpp` | binary i3dm (header + batch table + feature table + glTF) | `TileContent{i3dm, gltf, batchTable, instances}` |
| `PntsReader.hpp` | binary pnts (header + batch table + per-attribute slabs) | `TileContent{pnts, ...}` |
| `CmptReader.hpp` | binary cmpt (header + inner tiles) | recursive — returns cmpt with inner contents |
| `VctrReader.hpp` | binary vctr (header + batch table + attributes) | `TileContent{vctr, ...}` |

Plus `EmbeddedGltfReader.hpp` — minimal glTF reader sufficient for b3dm/i3dm (POSITION / NORMAL / TEXCOORD_0 / indices / base material). ~800 lines of pure parser. **Distinct from** the broader Spec-3 glTF→`Scene` work in ADR-0017 (which is whole-scene ingestion via InlineResolver); v1 ships only the embedded-glTF reader. ADR-0017's wider work may refactor against this reader when it lands.

### Materialiser

`runtime/3dtiles/Materialiser.hpp` — pure function unit: given a `Tile` + decoded `TileContent`, produces a synthetic X3D subtree and inserts it under a parent group.

```cpp
namespace x3d::runtime::tiles {

std::shared_ptr<X3DNode>
materialise(const Tile &tile, const TileContent &content,
            std::shared_ptr<X3DNode> parent, X3DExecutionContext &ctx);

void dematerialise(const Tile &tile,
                   std::shared_ptr<X3DNode> parent, X3DExecutionContext &ctx);

void applyStyle(const StyleDescriptor &style, const Tile &tile,
                std::shared_ptr<X3DNode> subtree);

} // namespace x3d::runtime::tiles
```

Subtree shape:

- **b3dm** → `<Transform>` (tile.transform) → `<Shape DEF="Tile_<id>">` → `<IndexedTriangleSet>` + `<Appearance>` + `<Material>` + `<MetadataSet DEF="Tile_<id>_metadata">` (batch table)
- **i3dm** → `<Transform>` (parent) → N × `<Transform DEF="Inst_<i>">` → `<Shape>` → `<IndexedTriangleSet>` + appearance
- **pnts** → `<Shape>` → `<PointSet>` + `<Appearance>` + `<Material>` + `<MetadataSet>` (no triangulation; `topology=Points` propagates to mesh builder)
- **cmpt** → recursive materialisation of each inner content sharing the parent
- **vctr** → `<Shape>` → `<IndexedLineSet>` + appearance + metadata (no glyph; embedder renders glyphs)

Each tile subtree's root has a `DEF="Tile_<id>"` so it is addressable via the X3D scene graph.

### Style

`runtime/3dtiles/StyleParser.hpp`:

```cpp
struct StyleDescriptor {
  std::string name;
  std::map<std::string, JsonValue> defines;
  struct Condition { JsonValue expr; };
  std::vector<std::pair<Condition, RGB>>   color;
  std::vector<std::pair<Condition, float>> alpha;
  std::vector<std::pair<Condition, bool>>  show;
};
```

v1 ships a **minimal condition evaluator** covering `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`, `in`, property access, batch-table lookup. Full JSONata deferred.

### System

`runtime/3dtiles/TilesetSystem.hpp` — a `System` subclass:

```cpp
namespace x3d::runtime::tiles {

class TilesetSystem : public System {
public:
  TilesetSystem(TilesetFetcher fetcher);

  void registerTileset(Tileset ts);
  void setStyle(std::optional<StyleDescriptor> style);

  void attach(const X3DNode *node, X3DExecutionContext &ctx) override;
  void update(double now, X3DExecutionContext &ctx) override;
  void detach(X3DExecutionContext &ctx) override;

  std::size_t selectedTileCount() const;
  std::size_t loadedTileCount() const;
  std::size_t pendingFetchCount() const;
  const std::vector<const Tile*> &currentSelection() const;
  const TilesetDiagnostics &diagnostics() const noexcept;

private:
  void selectAndMaterialise(X3DExecutionContext &ctx);

  TilesetFetcher fetcher_;
  std::optional<Tileset> tileset_;
  std::optional<StyleDescriptor> style_;

  std::shared_ptr<X3DNode> anchor_;
  std::unordered_set<TileId> selected_;
  std::unordered_set<TileId> loaded_;
  std::unordered_set<TileId> pending_;
  // ...selection state, fetched-content cache, etc.
};

} // namespace x3d::runtime::tiles
```

The selection algorithm is the OGC algorithm from §6 of the 1.1 spec: refine while `screenSpaceError(tile.geometricError, distance, viewport_height) > sse_threshold`; REPLACE swaps the parent's content out, ADD accumulates. Children selection respects `viewerRequestVolume` as a frustum gate.

### Façade

`include/x3d/3dtiles.hpp`:

```cpp
namespace x3d::sdk::tiles {

class TilesetContext {
public:
  explicit TilesetContext(X3DExecutionContext &ctx,
                          TilesetOptions options = {});

  std::size_t loadTileset(const std::string &url,
                          TilesetFetcher fetcher = makeFileTilesetFetcher("."));
  std::size_t loadTilesetText(const std::string &tilesetJson);

  void setStyle(const std::string &styleUrl);
  void setStyleText(const std::string &styleJson);

  void setParent(std::shared_ptr<X3DNode> parent);

  std::size_t tileCount() const;
  std::size_t loadedCount() const;
  const TilesetDiagnostics &diagnostics() const noexcept;
  void setLogFn(TilesetLogFn fn);
};

} // namespace x3d::sdk::tiles
```

The façade owns the anchor `Group` and the `TilesetSystem`; the embedder never names `System` directly.

### Embedder-side options

```cpp
struct TilesetOptions {
  double   sseThresholdPixels      = 16.0;
  std::size_t maxDepth             = 32;
  std::size_t maxChildrenPerNode   = 64;
  std::size_t maxMaterialisedTiles = 4096;   // LRU eviction
  std::size_t maxConcurrentFetches = 32;
  double   fetchTimeoutSeconds     = 30.0;
  bool     useViewerRequestVolume  = true;
  bool     refineReplaceOnly       = false;
};
```

## Data flow

### Setup (one-shot, before any tick)

```
embedder:
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);

  sdk::tiles::TilesetContext tilesets(ctx);
  tilesets.loadTileset("https://cdn/tiles/Scene/tileset.json", myHttpFetcher);
  tilesets.setParent(myRootGroup);   // optional

  while (running) {
    ctx.tick(now);
    auto d = ex.delta();             // existing extract — sees tile changes
    renderer.apply(d);
  }
```

`loadTileset(url, fetcher)` is the only **synchronous** fetch — `tileset.json` must be `Ready` before the loop begins. Failure records a diagnostic, leaves the system in a no-op state, returns 0. After this call, `tilesets.tileCount() == 1` (the root, before selection).

`loadTilesetText(jsonText)` is the alternate path for embedders with pre-loaded bytes — same contract, no fetch.

### Per-tick selection loop

`TilesetSystem::update(now, ctx)` runs after TimeSensor / Interpolator and before Navigation's final-frame pose, so selection sees the **new** camera pose for this tick:

```
update(now, ctx):
  cam    = ctx.boundViewpoint()
  if not cam: return
  eye    = ctx.cameraWorldPosition()
  view   = ctx.viewMatrix()
  fovy   = cam->fieldOfView[1] (or NavigationInfo.avatarSize fallback)
  vpH    = ctx.viewportHeight()           // embedder-threaded; default 1080

  // (1) Deselect: tiles not in new selection are dematerialised.
  newSelection = walkTree(root, eye, view, fovy, vpH)
  for tile in selected_ - newSelection:
    dematerialise(tile, anchor_, ctx)
  selected_ = newSelection

  // (2) Fetch: tiles in newSelection but not loaded_ or pending_.
  for tile in selected_ - (loaded_ | pending_):
    r = fetcher_.fetch(tile.content.url, tile.content.kind)
    if Ready:   cache[tile] = codec.decode(r.bytes); loaded_.insert(tile)
    if Pending: pending_.insert(tile)
    if Failed:  log + skip; don't refine descendants

  // (3) Resolve pending.
  for tile in pending_:
    r = fetcher_.fetch(tile.content.url, tile.content.kind)
    if Ready:   cache[tile] = codec.decode(r.bytes); loaded_.insert(tile); pending_.erase(tile)
    if Failed:  log + pending_.erase(tile); don't refine descendants
    if Pending: still pending

  // (4) Materialise: newly loaded tiles not yet in the scene.
  for tile in loaded_ - materialised_:
    materialise(tile, cache[tile], anchor_, ctx)
    if style_: applyStyle(style_, tile, subtree)
    materialised_.insert(tile)
```

Walk-tree SSE computation:

```
sse(tile, eye, fovy, vpH) =
    (tile.geometricError / dist(eye, tile.boundingVolume.center)) * (vpH / (2 * tan(fovy/2)))
```

A tile refines while `sse > sseThreshold` (default 16 pixels; embedder-overridable). REPLACE-mode refinement drops the parent subtree before materialising children; ADD-mode keeps both. `viewerRequestVolume` is a frustum gate.

### Dirty propagation (existing infrastructure, free)

`addChildren` / `removeChildren` on the anchor `Group` flips `DirtyChildren` on it. The existing chain does the rest:

```
TilesetSystem.addChildren(anchor_, tileSubtree)
  → DirtyTracker flags anchor_ (DirtyChildren)
  → TransformSystem re-walks subtree; flags new transforms DirtyWorldTransform
  → BoundsSystem re-walks; flags new bounds Dirty
  → SceneExtractor::delta() reads changedNodes()
    → added: tile RenderItems (new geometry paths)
    → removed: tile RenderItems (deselected)
    → updatedTransform: tiles whose ancestor moved
  → RenderDelta emitted to renderer
```

No new dirty-tracking code, no new scene-graph mechanism. Tiles are X3D nodes; the existing infrastructure treats them exactly like authoring-placed nodes.

### Threading

Single-threaded producer+consumer (matching the existing invariant). All selection state, fetcher calls, and materialisation run on the tick thread. An embedder wanting concurrent network IO supplies a fetcher that internally uses a thread pool and synchronously joins on the bytes; the system sees a sync API. Matches `AssetResolver`'s contract.

## Error handling

Fail-soft, surface-loud: bad input never crashes the tick loop; diagnostics surface to the embedder; the affected tile is skipped; selection continues.

### Diagnostic surface

```cpp
struct TilesetDiagnostics {
  std::size_t tilesetFetchFailures    = 0;
  std::size_t tilesetParseFailures    = 0;
  std::size_t contentFetchFailures    = 0;
  std::size_t contentFetchPending     = 0;
  std::size_t codecFailures           = 0;
  std::size_t embeddedGltfFailures    = 0;
  std::size_t styleParseFailures      = 0;
  std::size_t styleConditionErrors    = 0;
  std::size_t droppedTiles            = 0;
  std::size_t materialisedTileCount   = 0;
  std::string lastMessage;
};

using TilesetLogFn = std::function<void(const std::string &category,
                                        const std::string &message)>;
```

### Failure matrix

| Failure | Behaviour | Surfaced as |
|---|---|---|
| `tileset.json` fetch Failed at `loadTileset` | No `TilesetSystem` registered; `tileCount() == 0`; main loop runs without tiles | `tilesetFetchFailures++` |
| `tileset.json` malformed | Same | `tilesetParseFailures++` |
| `tileset.json` valid JSON but invalid per spec (missing root, recursive cycle, depth > cap) | Same | `tilesetParseFailures++` |
| Tile content fetch Failed | Tile skipped; descendants not refined; dematerialises on next deselection | `contentFetchFailures++` |
| Tile content fetch Pending | Not materialised this tick; retried next tick; selection still considers it visible | `contentFetchPending++` |
| Content codec failure | Tile skipped; descendants not refined | `codecFailures++` |
| Embedded glTF malformed | Same | `embeddedGltfFailures++` |
| Batch table wrong types | Log; default-initialise missing/typed values | logged; tile materialises with defaults |
| Style JSON malformed | Log; no Style applied | `styleParseFailures++` |
| Style condition references undefined batch key | Condition evaluates false; default appearance | `styleConditionErrors++` |
| Tile with no content (intermediate refinement) | Not materialised; selection still descends | not an error |
| Tile with no children and no content | Empty subtree; not materialised | logged once |
| Materialised tile count > cap | Cap enforced; lowest-LOD / furthest tiles dropped first | `droppedTiles++` |
| Anchor Group detached between ticks | System no-ops; logs once | `lastMessage` set |
| Embedder-supplied fetcher throws | **Not caught.** Embedder responsible for its own exception safety (matches every other embedder seam). | rethrown |

## v1 deferrals (explicit, not gaps)

- **Implicit tiling (3D Tiles 4.0)** — Subtree binary codec deferred to v1.1. Codec folder reserves `runtime/3dtiles/codecs/SubtreeReader.hpp` as a placeholder. `Tileset.extensionsUsed` is parsed and surfaced; an `"3DTILES_implicit_tiling"` extension triggers a diagnostic pointing at v1.1.
- **Full JSONata in Style** — minimal evaluator covers `==` / `!=` / `<` / `<=` / `>` / `>=` / `&&` / `||` / `!` / `in` / property access / batch-table lookup. Full JSONata deferred.
- **Vector glyph rendering (vctr)** — codec extracts geometry + attributes; only line geometry is emitted (`IndexedLineSet`). Embedder renders glyphs/icons by inspecting `vctrAttributes`. Full vector styling deferred.
- **3D Tiles Stream (WebSocket extension)** — not supported; HTTPS / static tile content only.
- **Subdivision mesh content format** — deferred (rarely used; large surface).

## Testing

Tests live in `runtime/3dtiles/tests/` and register in `CMakeLists.txt` unconditionally. The naming convention mirrors existing subsystems.

### Codec unit tests (one per format)

| ctest target | Coverage |
|---|---|
| `x3d_tiles_tileset_json_reader` | Valid JSON; missing root; recursive cycle; depth > cap; `extensionsUsed`; `geometricError` defaults; `transform` matrix |
| `x3d_tiles_b3dm_reader` | Header magic; batch table decode (JSON + binary); feature count; embedded glTF magic |
| `x3d_tiles_i3dm_reader` | Header / batch table / feature table (per-instance TRS + batchId); glTF glb vs gltf paths |
| `x3d_tiles_pnts_reader` | Per-attribute slabs (POSITION/NORMAL/COLOR); batch table; `pointsLength` consistency |
| `x3d_tiles_cmpt_reader` | Recursive inner tiles; mixed inner kinds |
| `x3d_tiles_vctr_reader` | Header; attribute decode (strings/numerics); batch table |
| `x3d_tiles_embedded_gltf_reader` | glb binary chunk; minimal JSON chunk (POSITION/NORMAL/TEXCOORD_0/indices/MATERIAL); buffer + bufferView + accessor; malformed/missing required accessor |

### Fetcher tests

| ctest target | Coverage |
|---|---|
| `x3d_tiles_fetcher_file` | `makeFileTilesetFetcher(tempDir)` resolves relative paths; missing → Failed; gzip content |
| `x3d_tiles_fetcher_null` | Always Failed; no materialisation; `contentFetchFailures++` |
| `x3d_tiles_fetcher_pending` | Pending once → Ready; materialises on second tick |
| `x3d_tiles_fetcher_caching` | Call count; re-selected tile fetched exactly once |

### Materialiser tests

| ctest target | Coverage |
|---|---|
| `x3d_tiles_materialise_b3dm` | Subtree shape (Transform → Shape → IndexedTriangleSet + Appearance + Material + MetadataSet); DEF naming |
| `x3d_tiles_materialise_i3dm` | N instance transforms; TRS + batchId; per-instance MetadataSet |
| `x3d_tiles_materialise_pnts` | PointSet (not IndexedTriangleSet); `topology=Points` |
| `x3d_tiles_materialise_cmpt` | Recursive; one subtree per inner content; shared parent |
| `x3d_tiles_materialise_vctr` | IndexedLineSet; attributes surfaced as metadata; no glyph |
| `x3d_tiles_dematerialise` | After materialise, dematerialise removes subtree; SceneExtractor::delta() emits `removed` |
| `x3d_tiles_apply_style` | Style with `color: {condition: ...}` applied; Material.diffuseColor mutated |

### TilesetSystem tests

| ctest target | Coverage |
|---|---|
| `x3d_tiles_select_simple` | Synthetic 2-level tree, fixed camera; root + correct children at threshold=16, threshold=64 |
| `x3d_tiles_select_replace_vs_add` | REPLACE: parent dematerialises when children materialise; ADD: both present |
| `x3d_tiles_select_viewer_request_volume` | Eye outside child VRV → child not refined even when SSE > threshold |
| `x3d_tiles_select_depth_cap` | Depth > maxDepth → stop refining; log |
| `x3d_tiles_pending_lifecycle` | Fetcher returns Pending then Ready; selection visible from first tick, materialisation on Ready tick |
| `x3d_tiles_failure_skips_descendants` | Content fetch Failed; descendants not refined; `contentFetchFailures++` |
| `x3d_tiles_materialised_cap` | Working set > cap; LRU eviction; `droppedTiles++` |
| `x3d_tiles_anchor_detached` | Embedder removes anchor; subsequent ticks no-op; diagnostic set |
| `x3d_tiles_diagnostics_counters` | Counters increment across failure scenarios |

### End-to-end integration

| ctest target | Coverage |
|---|---|
| `x3d_tiles_e2e_load_extract` | Synthetic 5-tile b3dm-only tileset in temp dir; tick 3 frames at varying eye positions; assert SceneExtractor::delta() matches expected added/removed sets per frame |
| `x3d_tiles_e2e_with_style` | Same + Style JSON; Material.diffuseColor reflects Style conditions |
| `x3d_tiles_e2e_with_batch_routes` | b3dm with batch table; ROUTE Tile_X_metadata.set_myField_changed TO TouchSensor.isOver wires correctly (proves "tiles are X3D") |

### SDK façade gate

| ctest target | Coverage |
|---|---|
| `x3d_tiles_sdk_facade` | Compile-and-run surface lock: include only `x3d/3dtiles.hpp`; drive `TilesetContext::loadTilesetText` → register → tick → diagnostics → query |

### Conformance

`tests/conformance/3dtiles/` ingests OGC's reference fixtures (subset) and asserts zero parse failures. Run via `mise run conformance`.

### Performance regression

`x3d_tiles_select_bench` — 10 000-tile synthetic tree; selection cost should be O(selected) per tick, not O(tree). Registered separately, not part of `ctest --preset dev`.

### Coverage

`x3d_tiles_coverage` — walks every public API in `x3d::sdk::tiles::*`; each exercised by at least one other test (compile-and-link surface check).

## Wiki + docs deliverables

1. New subsystem page `docs/wiki/subsystems/3dtiles.md` (purpose, key files, interfaces, tests). Cross-linked from `architecture.md` (runtime surface diagram gains one new block) and `sdk-facade.md` (`x3d::sdk` namespace listing gains `x3d::sdk::tiles`).
2. New ADR `docs/wiki/decisions/0021-3dtiles-first-class.md` documenting the framing (peer runtime surface, NOT ext-firewalled), seam shape (`TilesetFetcher` returning bytes), materialisation strategy (anchor Group + addChildren/removeChildren), and v1 scope deferrals (implicit tiling, full JSONata, vector glyphs).
3. Two entries in `docs/superpowers/BACKLOG.md` under a new "3D Tiles follow-ups" section:
   - **`3DTILES-1`** — Implicit tiling (3D Tiles 4.0): Subtree binary codec + `3DTILES_implicit_tiling` extension handling. **SCHEDULED → v1.1**, gated on corpus need. Track against the first consumer that supplies a Subtree-format tileset.
   - **`3DTILES-2`** — Full JSONata evaluator for Style conditions. **OPEN**. Minimal evaluator covers equality / numeric compare / logical ops / property access / batch-table lookup.

## What ships NOT in v1

- Implicit tiling (4.0)
- Full JSONata
- Vector glyph rasterisation
- 3D Tiles Stream (WebSocket)
- Subdivision mesh content format
- The broader Spec-3 glTF→`Scene` InlineResolver work from ADR-0017 (separate milestone)

## Out-of-scope decisions deferred to v1.1

- Whether to ship a **shared content cache** at the SDK level (vs. embedder-supplied via the fetcher). Current design keeps caching entirely on the embedder side.
- Whether to expose **per-tile query** (given a `PathKey` or `TileId`, return the `Tile` and decoded `TileContent`). Useful for embedder inspection; deferred.
- Whether to support **mutually-exclusive group tiles** (3D Tiles 1.1 §6.5). Uncommon; deferred unless a corpus need surfaces.
