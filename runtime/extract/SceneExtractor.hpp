// SceneExtractor.hpp — M2.5 extraction (Layer A5), FULL form (task T7). namespace
// x3d::runtime::extract. Header-only, golden-untouched, node-as-truth (all caches
// are side tables keyed by const X3DNode* / dense ids — ZERO node state).
//
// T7 widens the T7a vertical-slice spine (which already gets an unlit triangle on
// screen) into the full extraction surface, all behind an already-rendering
// pipeline:
//
//   (a) VISIBILITY-aware DFS. The traversal special-cases BY nodeTypeName BEFORE
//       the generic child loop — it does NOT reuse PickSystem's blind forEachChild
//       for visibility-bearing nodes:
//         * Switch: read whichChoice (SFInt32, default -1). <0 or out-of-range =>
//           draw NOTHING; otherwise recurse ONLY children[whichChoice]. A blind
//           child loop would (wrongly) draw the first child at -1 and every child
//           otherwise — the exact bug this special-case closes.
//         * LOD: static PoC selects children[0] (highest detail), DOCUMENTED.
//           Range selection needs an eye position not threaded into this DFS, so
//           the level-0 pick is deliberate and stable; LOD draws exactly one
//           child, never all levels.
//         * Group/Transform/Anchor/Billboard/Collision/StaticGroup and the rest:
//           pass-through grouping nodes via the generic children/SFNode loop.
//           Billboard is treated as identity orientation (documented wrong once
//           the camera moves — view-facing rotation is a documented limitation).
//       worldM is mutated by any transform-bearing node (Transform,
//       HAnimHumanoid, HAnimJoint, CADPart — see TransformSystem::isTransform).
//       Billboard is view-dependent (deferred to M2c/M2d).
//
//   (b) Real geometry + material + lights. MeshBuilder (T2/T3/T4 — ALL types now,
//       not just the three triangle sets) produces the local mesh; MaterialSystem
//       (T5) resolves the Shape's Appearance to a real MaterialDesc (replacing the
//       T7a Unlit-white stub); LightSystem (T6) world-resolves the scene's lights.
//
//   (c) THREE reverse indices + an interior-node entry-matrix cache, built during
//       the DFS, so the T8 incremental delta() can resolve a changed node -> its
//       affected RenderItemIds in O(1) and resume a DirtyChildren subtree re-walk
//       from a cached entry matrix in O(subtree):
//         * transformDeps  : every Transform ANCESTOR on an item's path -> ids.
//         * geomDeps       : the geometry node -> ids.
//         * materialDeps   : every appearance-subtree node (appearance, material,
//                            each texture / textureProperties reachable from the
//                            Shape) -> ids.
//         * entryMatrix_   : each interior path-prefix node -> its accumulated
//                            entry worldM (the matrix in effect ABOVE that node).
//
//   (d) camera()/lights()/background() read-outs. camera() surfaces an
//       OrthoViewpoint with ortho=true (its MFFloat fieldOfView l/b/r/t carried
//       through, PoC-out-of-scope but contract-stable).
//
// IDENTITY remains per-PATH; GEOMETRY/MATERIAL are content/node-keyed and
// legitimately shared across placements (USE). The walk NEVER reads
// ctx.worldTransform()/TransformSystem.world_ for an item matrix (the M2C-1
// first-path-only table); it re-accumulates worldM fresh down each path.
//
// SEAM/THREADING: single-threaded producer+consumer; the mutable interning caches
// are not safe to share across threads (stated as a seam invariant).
#ifndef X3D_RUNTIME_EXTRACT_SCENE_EXTRACTOR_HPP
#define X3D_RUNTIME_EXTRACT_SCENE_EXTRACTOR_HPP

#include "Aabb.hpp"
#include "x3d/nodes/Billboard.hpp"           // billboardLocalMatrix (§23.4.1, M2e) + viewdep
#include "GeometryBounds.hpp"      // geombounds::getField/getNode/hasField
#include "LightSystem.hpp"         // extract::LightSystem (T6)
#include "Mat4.hpp"
#include "MaterialSystem.hpp"      // extract::materialOf (T5)
#include "MeshBuilder.hpp"         // extract::buildLocalMesh (all types, T2/T3/T4)
#include "PackedMesh.hpp"          // PackedMesh (Phase 1 binary geometry)
#include "RecursionLimits.hpp"     // MEM-1: kMaxNestingDepth (walk DoS guard)
#include "RenderItem.hpp"          // RenderItem descriptors + RenderDelta
#include "TextExtract.hpp"         // buildTextMesh + outputOnly metrics
#include "TextureExtract.hpp"      // T-TEX: TextureTransform bake + resolver + sampler enrich
#include "TextureResolver.hpp"     // TextureResolver seam (embedder-supplied decode)
#include "TransformSystem.hpp"     // localMatrix (static; per-path re-accumulation)
#include "ViewDependentSystem.hpp" // lodSelectLevel (§23.4.3, M2e)
#include "X3DExecutionContext.hpp" // camera/background/light pull surfaces
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::runtime::extract {

using namespace x3d::core;

// ---------------------------------------------------------------------------
// RenderItem — the per-PATH stored record. IDENTITY is the PathKey; GEOMETRY and
// MATERIAL are content/node-keyed and legitimately shared across placements.
// (Kept here, not in RenderItem.hpp, because RenderItem.hpp is a pure-POD leaf
// that must NOT pull in node/transform headers; this is the engine-side record.)
// ---------------------------------------------------------------------------
struct RenderItem {
  PathKey path;                       // full root..leaf node chain (identity).
  Mat4 worldTransform = Mat4::identity(); // per-PATH world matrix (re-accumulated).
  GeomId geometry;                    // content identity of the mesh.
  MaterialDesc material;              // real MaterialSystem result (T5).
  std::optional<ShaderProgramDesc> shaderProgram;  // absent = fixed-function path (Phase 3)
  // LOCAL-frame triangles, SHARED across every placement of one GeomId
  // (ADR-0045). Never null — a Packed item points at emptyMeshRef() — so
  // `item.mesh->positions` needs no null check.
  MeshRef mesh = emptyMeshRef();

  // Phase 1 binary geometry union. For AoS items (all pre-Phase-1 paths),
  // geometry_ext.kind == AoS and geometry_ext.aos is empty (mesh above is the
  // authoritative AoS data). For Packed items (emitted via emitPacked()),
  // geometry_ext.kind == Packed and geometry_ext.packed carries the binary slab.
  Geometry geometry_ext;

  // M25-5: indices into the SceneExtractor::lights_ vector for lights that
  // illuminate this placement. Global lights (LightDesc::global==true) appear
  // for EVERY item; scoped lights (global==false) appear only when their
  // scopeRoot is an ancestor-or-equal of this item's PathKey.
  std::vector<std::size_t> lights;

  // M2e (§23.4.4): set when this item's origin is beyond the effective far
  // distance (Viewpoint.farDistance, else NavigationInfo.visibilityLimit). A
  // far-cull HINT — the item is still emitted; a consumer may skip drawing it.
  bool beyondVisibilityLimit = false;

  // X3DShapeNode.castShadow (X3D default TRUE): whether this shape occludes
  // light for shadow generation. The renderer's shadow-visibility query only
  // considers shapes with castShadow == true (§17; technique is consumer-defined
  // — see ADR-0028). Carried, not interpreted, by the SDK.
  bool castShadow = true;
};

class SceneExtractor {
public:
  // Holds the ctx (camera/dirty pull surface) and the scene (root traversal).
  // X3DExecutionContext intentionally does NOT expose its roots, so the scene is
  // passed alongside it — the PoC main owns both. Both must outlive the extractor.
  // The embedder may supply MeshBuildOptions ONCE at construction (B5): the geo
  // projection seam and tessellation density. It is held by value (copyable) and
  // FORWARDED to every buildLocalMesh call (full walk + delta re-extract), so the
  // SDK never calls geodesy itself — it invokes the embedder std::function. With
  // the default (no geoProjection wired) GeoElevationGrid renders via flat-
  // fallback. Defaulted so existing callers are source-compatible.
  // The embedder may ALSO supply a TextureResolver (T-TEX): the SDK never decodes
  // image bytes — it threads the embedder's decoded pixels onto each emitted
  // TextureRef.resolvedPixels (Source::Url only; Inline/Movie are skipped). The
  // default is makeNullTextureResolver() (always Failed => the PoC white-fallback),
  // so existing callers are source-compatible.
  SceneExtractor(const X3DExecutionContext &ctx, const Scene &scene,
                 MeshBuildOptions meshOptions = {},
                 TextureResolver textureResolver = makeNullTextureResolver())
      : ctx_(ctx), scene_(scene), meshOptions_(std::move(meshOptions)),
        textureResolver_(std::move(textureResolver)) {}

  // fullSnapshot — one full walk; (re)builds the PathIndex + per-item records +
  // the three reverse indices + the entry-matrix cache and returns EVERY item in
  // `added` (so frame 0 and frame N share one upload path).
  RenderDelta fullSnapshot() {
    items_.clear();
    index_.clear();
    transformDeps_.clear();
    geomDeps_.clear();
    materialDeps_.clear();
    geomNodeOf_.clear();
    entryMatrix_.clear();
    skippedGeometry_.clear(); // B2: recount unsupported drops for this full walk.
    // ADR-0045: a full walk is authoritative — it must re-read current field
    // state, so no mesh survives from a previous snapshot. Within THIS walk the
    // caches then collapse N placements onto one build/one allocation.
    rawMeshCache_.clear();
    bakedMeshCache_.clear();
    textureMemo_.clear();

    // #21: one node-visit budget shared across this snapshot's light collection
    // and geometry walk, bounding an acyclic "doubling DAG" fan-out.
    walkBudget_ = WalkBudget(meshOptions_.maxWalkVisits);

    // M25-5: collect all active lights once per snapshot. emit() uses this to
    // tag each RenderItem with the lights whose scope covers its PathKey.
    LightSystem ls;
    lights_ = ls.collect(scene_, walkBudget_);

    RenderDelta delta;
    for (const auto &root : scene_.rootNodes) {
      if (!root) continue;
      PathKey path;
      walk(root.get(), Mat4::identity(), path, delta);
    }
    delta.cameraChanged = true;
    delta.backgroundChanged = true;
    delta.lightsChanged = true;
    snapped_ = true;
    lastDeltaGen_ = ctx_.tickGeneration(); // seed the one-delta-per-tick guard.
    return delta;
  }

  // ---------------------------------------------------------------------------
  // delta — INCREMENTAL change channel. Reads ctx.dirtyTracker() ONCE for the
  // current tick and partitions the changed nodes into RenderDelta buckets,
  // NEVER re-walking the whole scene for a transform/geometry/material-only tick.
  //
  // ONE-DELTA-PER-TICK CONTRACT, made TOTAL (no misuse is undefined):
  //
  // tick() clears the dirty set at the start of each advance, so that set is the
  // delta of exactly ONE tick. Two situations used to be assert()ed out, which
  // was wrong twice over: asserts compile out under NDEBUG (this repo's `ci`
  // preset is RelWithDebInfo), so a release consumer got silently different
  // behaviour from the debug one it was tested against; and the guard keyed on
  // ctx_.now(), which made a paused / fixed-timestep / deterministic-replay
  // consumer — one that legitimately ticks twice at the same clock value —
  // trip an assert for doing nothing wrong. Both now have defined answers:
  //
  //   * delta() with no prior fullSnapshot()  =>  returns fullSnapshot().
  //     There is no baseline to diff against, and a full snapshot IS the correct
  //     baseline: it reports every item in `added`, which frame 0 and frame N
  //     already share one upload path for. The consumer needs no special case.
  //
  //   * delta() twice with no intervening tick()  =>  returns an EMPTY delta.
  //     Not an error — an honest answer. Nothing CAN have changed: only tick()
  //     advances state, and the previous delta() already reported this tick's
  //     changes. (Previously this asserted, and under NDEBUG silently returned a
  //     bogus re-diff.)
  //
  // The guard keys on ctx_.tickGeneration() — a monotonic count of advances that
  // cannot repeat — NOT on ctx_.now(), which an embedder may legitimately hold
  // still or replay.
  //
  // Per changed node n (dispatch is mutually exclusive — a Transform is only ever
  // in transformDeps_, a geometry node only in geomDeps_, an appearance-subtree
  // node only in materialDeps_):
  //   * DirtyLocalTransform|DirtyWorldTransform & n in transformDeps_  =>
  //       RE-ACCUMULATE each dependent item's worldTransform along its STORED
  //       PathKey (O(path) per item) — NEVER ctx.worldTransform() (first-path).
  //   * DirtyField & n in geomDeps_  =>  bump GeomId.contentVersion + re-extract
  //       the LOCAL mesh => updatedGeometry. Gated on the DirtyField bit, NOT the
  //       bare DirtyBounds bit (ancestor-scale animation sets DirtyBounds on the
  //       Transform/ancestors via BoundsSystem.recomputeUp, never DirtyField on
  //       the geometry leaf — so a parent scale does NOT pollute updatedGeometry).
  //   * DirtyField & n in materialDeps_  =>  re-read the MaterialDesc from the
  //       owning Shape's Appearance => updatedMaterial.
  //   * DirtyChildren & n is a grouping node  =>  resume the subtree walk from
  //       n's cached entry matrix (O(subtree)) => added/removed.
  // ---------------------------------------------------------------------------
  RenderDelta delta() {
    // No baseline yet — a full snapshot IS the baseline (see contract above).
    if (!snapped_) return fullSnapshot();

    const std::uint64_t gen = ctx_.tickGeneration();
    if (gen == lastDeltaGen_) return {}; // no advance since the last delta().
    lastDeltaGen_ = gen;

    // #21: fresh node-visit budget for this tick's incremental re-walks (a dirty
    // subtree can also be a wide acyclic fan-out).
    walkBudget_ = WalkBudget(meshOptions_.maxWalkVisits);

    RenderDelta delta;
    const DirtyTracker &dirty = ctx_.dirtyTracker();
    std::unordered_set<RenderItemId> transformSeen, geomSeen, materialSeen;
    // Shared across every reaccumulateWorld() in this delta() so a transform that
    // sits on the path of many dirty items is recomposed once, not once per item.
    LocalXfCache localXf;

    for (const X3DNode *n : dirty.changedNodes()) {
      const unsigned f = dirty.flags(n);

      // --- transform: re-accumulate worldM along each dependent's PathKey -----
      if (f & (DirtyLocalTransform | DirtyWorldTransform)) {
        for (RenderItemId id : depsOf(transformDeps_, n)) {
          // Dedup the re-accumulation itself: propagate() flags every node in a
          // dirtied subtree, so the same item is reached via several dirty
          // ancestors — but its full-path world matrix only needs computing once.
          if (transformSeen.insert(id).second) {
            reaccumulateWorld(id, localXf);
            delta.updatedTransform.push_back(id);
          }
        }
      }

      // --- geometry CONTENT change (DirtyField, NOT bare DirtyBounds) ----------
      if (f & DirtyField) {
        const auto &gids = depsOf(geomDeps_, n);
        if (!gids.empty()) {
          // Re-extract the GEOMETRY node's local mesh once (n may be a content
          // child like Coordinate — geomNodeOf_ maps it back to the geometry node
          // whose mesh to rebuild); bump contentVersion so the consumer's
          // GeomId-keyed GPU cache orphans the old buffer.
          auto git = geomNodeOf_.find(n);
          const X3DNode *geomNode = git == geomNodeOf_.end() ? n : git->second;
          // The content changed, so every cached mesh derived from this geometry
          // (the raw build AND every TextureTransform-baked variant of it) is
          // stale. Evict first, then rebuild ONCE through the same cache — so the
          // N dependent placements re-share a single new allocation instead of
          // taking N full copies of it (ADR-0045).
          evictMeshCache(geomNode);
          MeshRef mesh = cachedRawMesh(geomNode, nullptr);
          for (RenderItemId id : gids) {
            RenderItem &rec = items_[id];
            rec.geometry.contentVersion++;
            // Re-bake per dependent: two placements of this geometry may sit
            // under different TextureTransforms. bakedMesh() is cache-backed, so
            // placements that agree still share one allocation.
            if (!mesh->indices.empty())
              rec.mesh = bakedMesh(geomNode, mesh, ttParamsOfItem(id));
            if (geomSeen.insert(id).second) delta.updatedGeometry.push_back(id);
          }
        }
      }

      // --- appearance-subtree change => re-read MaterialDesc ------------------
      if (f & DirtyField) {
        for (RenderItemId id : depsOf(materialDeps_, n)) {
          refreshMaterial(id);
          if (materialSeen.insert(id).second) delta.updatedMaterial.push_back(id);
        }
      }

      // --- DirtyChildren on a grouping node => subtree re-walk ----------------
      if (f & DirtyChildren) rewalkSubtree(n, delta);
    }

    // camera/lights/background are recomputed full each tick by the consumer; the
    // change bits are surfaced unconditionally for a caching consumer (the PoC
    // recomputes them every frame regardless).
    delta.cameraChanged = true;
    delta.backgroundChanged = true;
    delta.lightsChanged = true;
    return delta;
  }

  // Dense-id accessors (the consumer keys arrays on RenderItemId).
  const RenderItem &item(RenderItemId id) const { return items_.at(id); }
  std::size_t itemCount() const { return items_.size(); }

  // --- reverse-index read-outs (the T8 delta() resolution surface) ----------
  // Each returns the RenderItemIds affected by a change on the given node; an
  // unknown node yields a stable empty vector (no allocation churn for misses).
  const std::vector<RenderItemId> &transformDepsOf(const X3DNode *n) const {
    return depsOf(transformDeps_, n);
  }
  const std::vector<RenderItemId> &geomDepsOf(const X3DNode *n) const {
    return depsOf(geomDeps_, n);
  }
  const std::vector<RenderItemId> &materialDepsOf(const X3DNode *n) const {
    return depsOf(materialDeps_, n);
  }

  // entryMatrixOf — the accumulated worldM IN EFFECT ABOVE an interior path-node
  // (keyed by that node). Used by a DirtyChildren subtree re-walk to resume from
  // the correct entry matrix in O(subtree) without reading TransformSystem.world_.
  // Returns identity for an unknown node.
  Mat4 entryMatrixOf(const X3DNode *n) const {
    auto it = entryMatrix_.find(n);
    return it == entryMatrix_.end() ? Mat4::identity() : it->second;
  }

  // M25-5: lightsOf — indices into snapshotLights() (NOT lights()) for the lights
  // that illuminate this placement. Global lights appear for EVERY item; scoped
  // (global=false) lights appear only when their scopeRoot is an ancestor-or-equal
  // of the item's path. Returns a stable ref valid until the next fullSnapshot().
  const std::vector<std::size_t> &lightsOf(RenderItemId id) const {
    return items_.at(id).lights;
  }

  // M25-5: snapshotLights — the exact light list the lightsOf() indices reference,
  // captured once by the last fullSnapshot(). Use THIS (not lights()) to resolve a
  // RenderItem's light indices to LightDescs: the indices are positions in this
  // vector, so the two must come from the same walk. NOTE: delta() does NOT refresh
  // this snapshot, so items emitted by an incremental rewalk reflect the light
  // topology as of the last fullSnapshot() (re-snapshot if lightsChanged matters).
  const std::vector<LightDesc> &snapshotLights() const { return lights_; }

  // camera — the bound Viewpoint resolved for the frame. viewMatrix from the ctx
  // (first-path-resolved, documented). fieldOfView read reflection-generic; an
  // OrthoViewpoint is surfaced ortho=true with its MFFloat {l,b,r,t} fieldOfView.
  CameraDesc camera() const {
    CameraDesc cam;
    cam.viewMatrix = ctx_.viewMatrix();
    if (const X3DNode *vp = ctx_.boundViewpoint()) {
      if (vp->nodeTypeName() == "OrthoViewpoint") {
        cam.ortho = true;
        // OrthoViewpoint.fieldOfView is an MFFloat {l,b,r,t}; surfaced verbatim
        // (projection-param shape differs; PoC-out-of-scope but contract-stable).
        cam.orthoFieldOfView =
            geombounds::getField<std::vector<float>>(*vp, "fieldOfView", {});
      } else {
        // Perspective viewpoints carry an SFFloat min-dimension fieldOfView.
        cam.fieldOfView =
            geombounds::getField<float>(*vp, "fieldOfView", 0.7854f);
      }
    }
    cam.cameraChanged = true;
    return cam;
  }

  // lights — world-resolved active lights (recomputed full each call for the PoC;
  // few, cheap). Delegates to LightSystem's per-path accumulation (the worldOfRec
  // idiom), so a USE'd light under two Transforms is not first-path-collapsed.
  // For per-RenderItem light indices use snapshotLights() (the cached snapshot the
  // lightsOf() indices are positions in); this fresh collect is for standalone use.
  std::vector<LightDesc> lights() const {
    LightSystem ls;
    return ls.collect(scene_);
  }

  // background — the bound Background's sky/ground gradient, read reflection-
  // generic. backgroundChanged is surfaced for a caching consumer.
  BackgroundDesc background() const {
    BackgroundDesc bg;
    if (const X3DNode *b = ctx_.boundBackground()) {
      bg.skyColor = geombounds::getField<std::vector<SFColor>>(*b, "skyColor", {});
      bg.skyAngle = geombounds::getField<std::vector<float>>(*b, "skyAngle", {});
      bg.groundColor =
          geombounds::getField<std::vector<SFColor>>(*b, "groundColor", {});
      bg.groundAngle =
          geombounds::getField<std::vector<float>>(*b, "groundAngle", {});
    }
    bg.backgroundChanged = true;
    return bg;
  }

  // sceneWorldBounds — union over every emitted item of (its LOCAL mesh AABB
  // transformed by that item's per-path world matrix). Per-path-correct: it does
  // NOT use ctx.worldBounds(root), which is first-path and undercounts a second
  // USE placement.
  Aabb sceneWorldBounds() const {
    Aabb out; // empty == union identity.
    for (const RenderItem &it : items_) {
      Aabb local;
      for (const SFVec3f &p : it.mesh->positions) local.expand(p);
      if (!local.empty) out.unionWith(local.transformed(it.worldTransform));
    }
    return out;
  }

  // skippedGeometryCounts — B2 unsupported-geometry coverage signal. Pull accessor
  // returning nodeTypeName -> count of Shapes whose geometry node was an
  // UNRECOGNIZED type (a geometry-bearing node MeshBuilder cannot tessellate, so it
  // was dropped from the render set). A recognized-but-legitimately-empty geometry
  // (IFS empty coordIndex, whichChoice=-1, a degenerate ElevationGrid) is NOT
  // counted — only genuine coverage gaps appear here. The map accumulates over the
  // walks since the last fullSnapshot() (which clears it); this lets a consumer
  // MEASURE coverage as B3/B5 land new geometry arms. (The per-frame
  // RenderDelta.unsupportedGeometry push channel is DEFERRED — named trigger
  // "B2->per-frame push" at the drop site in walk().)
  const std::map<std::string, int> &skippedGeometryCounts() const {
    return skippedGeometry_;
  }

  // #21: true if the last fullSnapshot()/delta() walk (geometry OR light
  // collection) hit maxWalkVisits and stopped early — i.e. the returned
  // RenderDelta is a PARTIAL view of a pathologically wide (acyclic "doubling
  // DAG") scene. A graceful, non-throwing signal; reset at the start of each walk.
  bool budgetExceeded() const { return walkBudget_.tripped; }

private:
  using DepMap = std::unordered_map<const X3DNode *, std::vector<RenderItemId>>;

  // Delegates to TransformSystem so all transform-bearing types stay in sync.
  // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
  static bool isTransform(const X3DNode *n) {
    return TransformSystem::isTransform(n);
  }

  static const std::vector<RenderItemId> &depsOf(const DepMap &m,
                                                 const X3DNode *n) {
    static const std::vector<RenderItemId> kEmpty;
    auto it = m.find(n);
    return it == m.end() ? kEmpty : it->second;
  }

  // RE-ACCUMULATE an item's world matrix along its STORED PathKey by re-reading
  // each Transform ancestor's CURRENT local matrix (O(path)). This is the
  // incremental, per-path-correct staleness fix: it NEVER reads
  // ctx.worldTransform()/TransformSystem.world_ (the M2C-1 first-path table), so
  // a USE-shared placement under two Transforms updates each placement correctly.
  // Per-node memo for one delta(): isTransform() allocates a type-name string and
  // localMatrix() does five reflective field scans + a quaternion compose, so a
  // dirty ancestor shared by every dependent item would otherwise be recomposed
  // O(items * depth) times. Field values are stable between the end of tick()'s
  // cascade and the next tick(), so caching per delta() is safe; localMatrix is a
  // function of the node's OWN fields (path-independent), so the cache stays
  // correct under DEF/USE instancing — only the accumulated product differs by
  // path, and that product is still formed fresh per PathKey below.
  struct LocalXf {
    bool isXform;
    Mat4 local;
  };
  using LocalXfCache = std::unordered_map<const X3DNode *, LocalXf>;

  const LocalXf &localXfOf(const X3DNode *n, LocalXfCache &cache) {
    auto it = cache.find(n);
    if (it != cache.end()) return it->second;
    LocalXf v;
    v.isXform = isTransform(n);
    v.local = v.isXform ? TransformSystem::localMatrix(n) : Mat4::identity();
    return cache.emplace(n, v).first->second;
  }

  void reaccumulateWorld(RenderItemId id, LocalXfCache &cache) {
    RenderItem &rec = items_[id];
    Mat4 w = Mat4::identity();
    // The PathKey is root..leaf; the leaf Shape itself is not a Transform, but a
    // defensive isTransform() guard keeps this identical to the walk() idiom.
    for (const X3DNode *n : rec.path) {
      const LocalXf &x = localXfOf(n, cache);
      if (x.isXform) w = w * x.local;
    }
    rec.worldTransform = w;
  }

  // Re-read the MaterialDesc for an item from its Shape's Appearance. The Shape
  // is the LAST node on the stored PathKey (emission is anchored on the Shape).
  void refreshMaterial(RenderItemId id) {
    RenderItem &rec = items_[id];
    if (rec.path.empty()) return;
    const X3DNode *shape = rec.path.back();
    auto appearance = geombounds::getNode(*shape, "appearance");
    rec.material = materialOf(appearance ? appearance.get() : nullptr);
    // T-TEX: re-enrich + re-resolve the textures of the refreshed material so an
    // appearance-subtree change (a new ImageTexture url, a TextureProperties edit)
    // re-runs the resolver and re-derives the §18.4.8/9 descriptor surface. The
    // TextureTransform bake lives on the MESH texcoords (updatedGeometry), not the
    // material — a textureTransform-only change is a documented v1 narrowing.
    enrichTextureRefs(rec.material.textures, /*texNodes=*/{}, rec.geometry.node);
    // Memoized (ADR-0045): one Appearance is routinely shared by many placements,
    // so an appearance change calls refreshMaterial() once PER DEPENDENT — without
    // the memo that is N decodes of the same URL. A CHANGED url is a memo miss and
    // still re-resolves, which is what this path exists to do. (Same-url content
    // changing underneath us does not dirty the node, so it would not re-resolve
    // here either way — the memo costs no freshness that was on offer.)
    resolveTextureRefs(rec.material.textures, textureResolver_, &textureMemo_);
  }

  // Resume the subtree walk under a grouping node G whose children changed,
  // starting from G's CACHED entry matrix (the world frame ABOVE G) — O(subtree),
  // never reading TransformSystem.world_. Emits new placements into delta.added
  // (emit() interns new PathKeys) and computes delta.removed as the items that
  // previously passed through G but were NOT re-emitted this walk.
  void rewalkSubtree(const X3DNode *g, RenderDelta &delta) {
    // Pre-image: every existing item whose stored path contains G.
    std::unordered_set<RenderItemId> before;
    for (RenderItemId id = 0; id < items_.size(); ++id)
      for (const X3DNode *n : items_[id].path)
        if (n == g) { before.insert(id); break; }

    // Reconstruct the PATH PREFIX down to (and including) G so re-emitted items
    // keep their full root..leaf identity. Take it from any pre-image item's
    // stored path (all share the prefix up to G); fall back to {G} if none.
    PathKey prefix;
    if (!before.empty()) {
      const PathKey &p = items_[*before.begin()].path;
      for (const X3DNode *n : p) {
        prefix.push_back(n);
        if (n == g) break;
      }
    } else {
      prefix.push_back(g);
    }

    const Mat4 entry = entryMatrixOf(g); // world frame ABOVE G (cached at walk).

    // Re-walk G's children (G contributes its own local matrix if it's a
    // Transform), collecting the ids emitted this pass. `emit` is delta-aware:
    // existing PathKeys reuse their id, new ones append to delta.added.
    RenderDelta sub; // capture this subtree's emitted ids in isolation.
    // Pop G off the prefix so walk() re-pushes it (keeping path identity exact).
    prefix.pop_back();
    walk(g, entry, prefix, sub);

    // walk()/emit() reports EVERY re-emitted placement in sub.added — including
    // pre-existing ones (an existing PathKey reuses its dense id). So the true
    // delta.added is only the ids that were NOT in the pre-image; delta.removed is
    // the pre-image ids not re-emitted this pass.
    std::unordered_set<RenderItemId> live(sub.added.begin(), sub.added.end());
    for (RenderItemId id : sub.added)
      if (!before.count(id)) delta.added.push_back(id);
    for (RenderItemId id : before)
      if (!live.count(id)) delta.removed.push_back(id);
  }

  // ---------------------------------------------------------------------------
  // VISIBILITY-aware recursion. worldM is the matrix accumulated ABOVE `n`;
  // `here` folds in n's own local matrix when n is a Transform. The entry matrix
  // (= worldM, the matrix in effect above n) is cached per interior path-node so
  // a later DirtyChildren subtree re-walk can resume correctly.
  // ---------------------------------------------------------------------------
  void walk(const X3DNode *n, const Mat4 &worldM, PathKey &path,
            RenderDelta &delta) {
    if (!n) return;
    // #21: bound total node-visits so an acyclic fan-out ("doubling DAG") that
    // escapes the MEM-1 cycle/depth guards cannot emit per-path without limit.
    // On exhaustion spend() latches; the partial result is surfaced by
    // budgetExceeded().
    if (!walkBudget_.spend()) return;
    // MEM-1: bound the walk so an unsanitized graph (USE-cyclic or pathologically
    // deep, the runtime twin of SEC-1) cannot stack-overflow the extractor. The
    // path is the live root->n ancestor chain, so a hard depth cap plus a
    // path-membership test before descending breaks back-edges. (The normal
    // pipeline severs cycles in buildSceneGraph; this keeps walk() self-safe when
    // an embedder drives it directly.)
    if (path.size() >= kMaxNestingDepth) return;
    for (const X3DNode *ancestor : path)
      if (ancestor == n) return; // n is its own ancestor: containment cycle.
    // X3D 4.0 visibility: a node with visible=FALSE (and its subtree) is not displayed.
    if (geombounds::hasField(*n, "visible") &&
        !geombounds::getField<bool>(*n, "visible", true))
      return;
    Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
    if (n->nodeTypeName() == "Billboard") {
      const SFVec3f axis = geombounds::getField<SFVec3f>(*n, "axisOfRotation", {0, 1, 0});
      here = worldM * billboardLocalMatrix(worldM, ctx_.cameraWorldPosition(),
                                           ctx_.cameraWorldUp(), axis);
    }
    path.push_back(n);

    // Cache the entry matrix (the world frame ABOVE this node) for O(subtree)
    // incremental re-walks. Keyed by node; the last-writer wins for a USE'd
    // interior node (a documented narrowing — incremental re-walk of a USE'd
    // grouping node resolves to its most-recently-walked placement; per-path
    // entry-matrix keying is the BACKLOG follow-up if a USE'd group ever animates
    // its children topology under two frames).
    entryMatrix_[n] = worldM;

    // RenderItem emission is anchored strictly on hasField("geometry") at a Shape
    // — exactly PickSystem::pickNode, not on DFS leafness.
    if (geombounds::hasField(*n, "geometry")) {
      if (auto geom = geombounds::getNode(*n, "geometry")) {
        bool recognized = false;
        // ADR-0045: build-once per DISTINCT geometry node, not per placement.
        MeshRef mesh = cachedRawMesh(geom.get(), &recognized);
        // T-TEXT: a Text node also EMITS its outputOnly fields (textBounds/
        // lineBounds/origin). buildLocalMesh produces the glyph geometry only;
        // here, owning the non-const node, we recompute the layout once and set
        // the node outputs so a Script/ROUTE consumer can read them. (Recompute
        // is cheap — pure layout — and keeps buildLocalMesh side-effect-free.)
        if (geom->nodeTypeName() == "Text") {
          TextLayoutResult layout;
          (void)buildTextMesh(*geom, meshOptions_.fontMetrics, &layout);
          extract::setTextOutputs(*geom, layout);
        }
        // Any geometry MeshBuilder cannot tessellate yields an empty mesh and is
        // skipped (no proxy path at this seam) — UNLESS the embedder has wired
        // an externalGeometryResolver, in which case we offer the unrecognized
        // node to it and emit via emitPacked() if it returns a non-empty PackedMesh.
        if (!mesh->indices.empty()) {
          emit(*n, path, here, geom.get(), std::move(mesh), delta);
        } else if (!recognized) {
          if (meshOptions_.externalGeometryResolver) {
            // Phase 1 seam: offer unrecognized geometry to the embedder resolver.
            // An empty PackedMesh means Pending (the resolver is not ready yet);
            // a non-empty PackedMesh means Ready and we emit it. The Pending path
            // is silent (not counted as a skipped-geometry gap) — it will retry
            // on the next fullSnapshot() or delta() that revisits this node.
            PackedMesh packed =
                meshOptions_.externalGeometryResolver(geom.get(), AssetResolver{});
            if (!packed.empty()) {
              emitPacked(*n, path, here, geom.get(), std::move(packed), delta);
            }
            // else: Pending — silently skip this tick.
          } else {
            // B2 observability: an UNRECOGNIZED geometry type (unknown nodeTypeName
            // carrying a geometry field — e.g. Extrusion, still unsupported until B3)
            // is a coverage gap, counted here by type. A recognized-but-empty mesh
            // (IFS empty coordIndex, whichChoice=-1 reached no Shape, a degenerate
            // ElevationGrid) is NOT counted — that is legitimate emptiness, not a gap.
            ++skippedGeometry_[geom->nodeTypeName()];
            // DEFERRED (named trigger: B2->per-frame push) — the per-frame
            // RenderDelta.unsupportedGeometry push channel. This pull accessor
            // (skippedGeometryCounts()) is the cumulative side; the delta() push is
            // wired when a consumer needs per-tick unsupported-geometry events.
          }
        }
      }
    }

    // VISIBILITY special-cases BY nodeTypeName, BEFORE the generic child loop.
    const std::string t = n->nodeTypeName();
    if (t == "Switch") {
      // whichChoice (default -1): <0 or out-of-range => draw nothing; else recurse
      // ONLY the selected child. NEVER the blind child loop (would draw all/first).
      const int which = geombounds::getField<int>(*n, "whichChoice", -1);
      const auto kids = childrenOf(*n);
      if (which >= 0 && which < static_cast<int>(kids.size()) && kids[which])
        walk(kids[which].get(), here, path, delta);
      path.pop_back();
      return;
    }
    if (t == "LOD") {
      const auto kids = childrenOf(*n);
      if (!kids.empty()) {
        const SFVec3f center = geombounds::getField<SFVec3f>(*n, "center", {0, 0, 0});
        // §23.4.3: distance measured in the LOD's LOCAL frame (here includes scale).
        const SFVec3f eyeLocal = here.inverse().transformPoint(ctx_.cameraWorldPosition());
        const float d = viewdep::len(viewdep::sub(eyeLocal, center));
        int lvl = lodSelectLevel(*n, d);
        if (lvl >= static_cast<int>(kids.size())) lvl = static_cast<int>(kids.size()) - 1;
        if (kids[lvl]) walk(kids[lvl].get(), here, path, delta);
      }
      path.pop_back();
      return;
    }

    // Generic pass-through grouping recursion (Group/Transform/Anchor/Billboard/
    // Collision/StaticGroup/...): every SFNode + MFNode field slot. Gated to node
    // slots only (never metadata scalars); inputOnly slots have no getter.
    for (const auto &f : n->fields()) {
      if (!f.get) continue; // inputOnly node fields (addChildren/...) have no getter
      // COL-2 §23.4.2: Collision.proxy is collision-only geometry — never rendered.
      if (t == "Collision" && f.x3dName == std::string("proxy")) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (!c) continue;
        // CAD-1 §32.4.2: CADFace.shape accepts only Shape|LOD|Transform; a
        // non-conforming node in that slot is not part of the visual scene.
        if (t == "CADFace" && f.x3dName == std::string("shape")) {
          const std::string ct = c->nodeTypeName();
          if (ct != "Shape" && ct != "LOD" && ct != "Transform") continue;
        }
        walk(c.get(), here, path, delta);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get(), here, path, delta);
      }
    }
    path.pop_back();
  }

  // The `children` MFNode slot of a grouping node (empty if absent).
  static std::vector<std::shared_ptr<X3DNode>> childrenOf(const X3DNode &n) {
    for (const auto &f : n.fields())
      if (f.x3dName == std::string("children") && f.get &&
          f.type == X3DFieldType::MFNode)
        return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(n));
    return {};
  }

  // Intern this PATH into a dense RenderItemId; store the per-path record and
  // populate the three reverse indices for it.
  void emit(const X3DNode &shape, const PathKey &path, const Mat4 &worldM,
            const X3DNode *geom, MeshRef mesh, RenderDelta &delta) {
    // Resolve the real material from the Shape's Appearance (T5). A Shape with no
    // appearance => Unlit white (MaterialSystem's always-draws debug fallback).
    auto appearance = geombounds::getNode(shape, "appearance");
    MaterialDesc material = materialOf(appearance ? appearance.get() : nullptr);
    // X3DShapeNode.castShadow (default TRUE) — carried for the shadow seam.
    const bool castShadow = geombounds::getField<bool>(shape, "castShadow", true);

    // T-TEX (v1-closure): textures end-to-end. (1) BAKE the authored
    // TextureTransform (§18.4.10) into the mesh's texcoords — authored or default,
    // both are in UV space. (2) Attach the geometry-borne TextureCoordinateGenerator
    // descriptor (§18.4.8) to each material texture ref. (3) Thread the embedder's
    // TextureResolver onto each Url ref's resolvedPixels (Inline/Movie skipped).
    // (1) is per-APPEARANCE, not per-placement: the bake is a pure function of
    // (geometry content, TextureTransform params), so it is cached on that pair
    // and shared by every placement that agrees on both (ADR-0045). One geometry
    // USE'd under two DIFFERENT TextureTransforms legitimately yields two meshes.
    mesh = bakedMesh(geom, std::move(mesh),
                     textureTransformParamsListOf(appearance ? appearance.get()
                                                             : nullptr));
    enrichTextureRefs(material.textures, /*texNodes=*/{}, geom);
    // ADR-0045: decode-once per URL. Without the memo the embedder's decoder was
    // invoked once per PLACEMENT (200 calls + 200 MiB retained for one 512x512
    // texture USE'd 200 times).
    resolveTextureRefs(material.textures, textureResolver_, &textureMemo_);

    // §23.4.4 effective far = Viewpoint.farDistance>0 ? : NavigationInfo.visibilityLimit>0 ? : inf.
    float far = 0.0f; // 0 => infinite
    if (const X3DNode *vp = ctx_.boundViewpoint()) {
      float fd = geombounds::getField<float>(*vp, "farDistance", -1.0f);
      if (fd > 0.0f) far = fd;
    }
    if (far == 0.0f) {
      if (const X3DNode *ni = ctx_.boundNavigationInfo())
        far = geombounds::getField<float>(*ni, "visibilityLimit", 0.0f);
    }
    bool beyond = false;
    if (far > 0.0f) {
      const SFVec3f origin = worldM.transformPoint(SFVec3f{0, 0, 0});
      const SFVec3f eye = ctx_.cameraWorldPosition();
      const float d = viewdep::len(viewdep::sub(origin, eye));
      beyond = d > far; // conservative: item origin beyond far
    }

    auto it = index_.find(path);
    RenderItemId id;
    if (it == index_.end()) {
      id = static_cast<RenderItemId>(items_.size());
      index_.emplace(path, id);
      RenderItem rec;
      rec.path = path;
      rec.worldTransform = worldM;
      rec.geometry = GeomId{geom, /*contentVersion=*/0};
      rec.material = std::move(material);
      rec.mesh = std::move(mesh);
      rec.beyondVisibilityLimit = beyond;
      rec.castShadow = castShadow;
      items_.push_back(std::move(rec));
    } else {
      // Stable interning across full snapshots: reuse the dense id, refresh the
      // per-path world matrix + content (a rebuild reflects current field state).
      id = it->second;
      RenderItem &rec = items_[id];
      rec.worldTransform = worldM;
      rec.geometry = GeomId{geom, /*contentVersion=*/0};
      rec.material = std::move(material);
      rec.mesh = std::move(mesh);
      rec.beyondVisibilityLimit = beyond;
      rec.castShadow = castShadow;
    }

    // M25-5: tag this item with every light whose scope covers its PathKey.
    // Global lights always apply; scoped lights apply when their scopeRoot is
    // an ancestor (appears anywhere) on the item's path.
    tagLights(items_[id], path);

    buildReverseIndices(id, path, geom, appearance.get());
    delta.added.push_back(id);
  }

  // Phase 1: emit a binary-geometry RenderItem (Packed path).
  // Called from walk() when the externalGeometryResolver returns a non-empty PackedMesh.
  // Mirrors emit() but sets geometry_ext.kind = Packed (and leaves mesh empty so
  // sceneWorldBounds(), which reads mesh.positions, naturally returns an empty Aabb
  // for packed items — the packed mesh carries its own bounds field).
  void emitPacked(const X3DNode& shape, const PathKey& path, const Mat4& worldM,
                  const X3DNode* geom, PackedMesh packed, RenderDelta& delta) {
    auto appearance = geombounds::getNode(shape, "appearance");
    MaterialDesc material = materialOf(appearance ? appearance.get() : nullptr);
    const bool castShadow = geombounds::getField<bool>(shape, "castShadow", true);
    enrichTextureRefs(material.textures, /*texNodes=*/{}, geom);
    // ADR-0045: decode-once per URL. Without the memo the embedder's decoder was
    // invoked once per PLACEMENT (200 calls + 200 MiB retained for one 512x512
    // texture USE'd 200 times).
    resolveTextureRefs(material.textures, textureResolver_, &textureMemo_);

    auto it = index_.find(path);
    RenderItemId id;
    if (it == index_.end()) {
      id = static_cast<RenderItemId>(items_.size());
      index_.emplace(path, id);
      RenderItem rec;
      rec.path = path;
      rec.worldTransform = worldM;
      rec.geometry = GeomId{geom, /*contentVersion=*/0};
      rec.material = std::move(material);
      rec.mesh = {};  // AoS mesh empty for packed items.
      rec.geometry_ext.kind = Geometry::Kind::Packed;
      rec.geometry_ext.packed = std::move(packed);
      rec.beyondVisibilityLimit = false;
      rec.castShadow = castShadow;
      items_.push_back(std::move(rec));
    } else {
      id = it->second;
      RenderItem& rec = items_[id];
      rec.worldTransform = worldM;
      rec.geometry = GeomId{geom, /*contentVersion=*/0};
      rec.material = std::move(material);
      rec.geometry_ext.kind = Geometry::Kind::Packed;
      rec.geometry_ext.packed = std::move(packed);
      rec.castShadow = castShadow;
    }
    tagLights(items_[id], path);
    buildReverseIndices(id, path, geom, appearance.get());
    delta.added.push_back(id);
  }

  // M25-5: compute the light index list for one RenderItem. Called by emit()
  // after the RenderItem record is in items_[id]. lights_ must be populated
  // before emit() is first called (fullSnapshot() does this).
  void tagLights(RenderItem &rec, const PathKey &path) {
    rec.lights.clear();
    for (std::size_t i = 0; i < lights_.size(); ++i) {
      const LightDesc &L = lights_[i];
      if (L.global) {
        // Global light: illuminates every placement.
        rec.lights.push_back(i);
      } else {
        // Scoped light: illuminates geometry whose path includes the scopeRoot.
        // L.scopeRoot is the enclosing grouping node; the item is in-scope iff
        // that node appears on its PathKey (i.e. the shape is a sibling or
        // descendant of the light inside that grouping node).
        if (L.scopeRoot) {
          for (const X3DNode *n : path)
            if (n == L.scopeRoot) { rec.lights.push_back(i); break; }
        }
        // scopeRoot == nullptr means the light had no enclosing grouping node
        // (it is a root-level node). By X3D spec a root-level non-global light
        // illuminates the whole scene, so treat it as scene-wide.
        else {
          rec.lights.push_back(i);
        }
      }
    }
  }

  // Populate transformDeps/geomDeps/materialDeps for one emitted RenderItem.
  void buildReverseIndices(RenderItemId id, const PathKey &path,
                           const X3DNode *geom, const X3DNode *appearance) {
    // transformDeps: every Transform ANCESTOR on this item's path. A change on any
    // of them re-accumulates this item's worldTransform.
    for (const X3DNode *anc : path)
      if (isTransform(anc)) appendDep(transformDeps_, anc, id);

    // geomDeps: the geometry node itself AND its direct content child-nodes
    // (Coordinate/Normal/Color/TextureCoordinate/...). classifyDirty marks a
    // content change on the node that OWNS the field — e.g. Coordinate.point lands
    // DirtyField on the Coordinate, NOT the parent IndexedFaceSet — so without the
    // child registration a `point` animation would never reach this RenderItem.
    // Every registered content node also maps back to its geometry node so delta()
    // re-extracts the geometry's mesh (not the child's).
    if (geom) {
      appendDep(geomDeps_, geom, id);
      geomNodeOf_[geom] = geom;
      for (const auto &f : geom->fields()) {
        if (!f.get) continue;
        if (f.type == X3DFieldType::SFNode) {
          auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*geom));
          if (c) { appendDep(geomDeps_, c.get(), id); geomNodeOf_[c.get()] = geom; }
        } else if (f.type == X3DFieldType::MFNode) {
          for (const auto &c :
               std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*geom)))
            if (c) { appendDep(geomDeps_, c.get(), id); geomNodeOf_[c.get()] = geom; }
        }
      }
    }

    // materialDeps: every appearance-subtree node reachable from the Shape
    // (appearance, material, textures, textureProperties). classifyDirty marks a
    // Material/Appearance field change ONLY on the owning node — which sits BELOW
    // the Shape and is neither a path ancestor nor the renderable leaf — so the
    // material channel is dead without this subtree-keyed index.
    if (appearance) {
      std::unordered_set<const X3DNode *> seen;
      collectMaterialSubtree(appearance, id, seen);
    }
  }

  static void appendDep(DepMap &m, const X3DNode *n, RenderItemId id) {
    auto &v = m[n];
    // Avoid duplicate ids for the SAME node (e.g. a Transform appearing twice on
    // a path is impossible, but a defensive de-dup keeps deltas clean).
    if (v.empty() || v.back() != id) v.push_back(id);
  }

  // Recurse the appearance subtree via SFNode/MFNode slots, recording each node
  // -> id in materialDeps_. Bounded by `seen` against a USE cycle.
  void collectMaterialSubtree(const X3DNode *n, RenderItemId id,
                              std::unordered_set<const X3DNode *> &seen) {
    if (!n || !seen.insert(n).second) return;
    appendDep(materialDeps_, n, id);
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) collectMaterialSubtree(c.get(), id, seen);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) collectMaterialSubtree(c.get(), id, seen);
      }
    }
  }

  // -------------------------------------------------------------------------
  // ADR-0045: content-keyed mesh cache — the SDK side of upload-once/instance-N.
  //
  // GeomId already told the CONSUMER which placements share content; the walk
  // ignored its own key and re-tessellated per placement, retaining N copies.
  // These two caches make the identity structural: one build and one allocation
  // per DISTINCT (geometry content [, TextureTransform params]).
  //
  // LIFETIME: both are cleared by fullSnapshot() — a fresh full walk must re-read
  // current field state, which is what makes a rebuild authoritative. Within one
  // snapshot, and across the delta()s that follow it, an entry is only evicted
  // when that geometry's content actually changes (evictMeshCache).
  // -------------------------------------------------------------------------
  struct RawMeshEntry {
    MeshRef mesh = emptyMeshRef();
    bool recognized = false; // B2: memoized alongside, same cost to recompute.
  };
  // Ready-only texture resolves, keyed by URL. Pending/Failed are deliberately
  // NOT memoized so a not-yet-decoded texture keeps retrying (see
  // resolveTextureRefs). Cleared by fullSnapshot() alongside the mesh caches.
  std::unordered_map<std::string, TexturePixelResult> textureMemo_;

  // Raw (pre-TextureTransform) build, keyed by geometry node.
  std::unordered_map<const X3DNode *, RawMeshEntry> rawMeshCache_;
  // TextureTransform-baked variants, keyed by (geometry node, params bytes). Only
  // populated when a TextureTransform is actually authored — the common
  // untransformed case shares the raw entry directly and allocates nothing here.
  std::map<std::pair<const X3DNode *, std::string>, MeshRef> bakedMeshCache_;

  // Serialize a params list into a cache key. Field-by-field (NOT a memcpy of the
  // struct) — TextureTransform2DParams has padding after `hasMatrix`, and hashing
  // indeterminate padding bytes would make cache hits nondeterministic.
  static std::string ttParamsKey(const std::vector<TextureTransform2DParams> &ps) {
    std::string k;
    k.reserve(ps.size() * (7 * sizeof(float) + 1));
    auto put = [&k](float v) {
      char b[sizeof(float)];
      std::memcpy(b, &v, sizeof(float));
      k.append(b, sizeof(float));
    };
    for (const auto &p : ps) {
      for (float v : {p.centerS, p.centerT, p.rotation, p.scaleS, p.scaleT,
                      p.translationS, p.translationT})
        put(v);
      k.push_back(p.hasMatrix ? '\1' : '\0');
      if (p.hasMatrix)
        for (float v : p.matrix) put(v);
    }
    return k;
  }

  // The one place buildLocalMesh() is called. `recognized` may be null.
  MeshRef cachedRawMesh(const X3DNode *geom, bool *recognized) {
    auto it = rawMeshCache_.find(geom);
    if (it == rawMeshCache_.end()) {
      bool rec = false;
      MeshData built = buildLocalMesh(geom, meshOptions_, &rec);
      it = rawMeshCache_
               .emplace(geom,
                        RawMeshEntry{std::make_shared<const MeshData>(
                                         std::move(built)),
                                     rec})
               .first;
    }
    if (recognized) *recognized = it->second.recognized;
    return it->second.mesh;
  }

  // Bake `params` into `raw`, sharing the result across every caller that agrees
  // on (geom, params). An empty params list means no TextureTransform is authored
  // and applyTextureTransformsToMesh() is a documented no-op — return the raw
  // mesh untouched so the common case costs nothing.
  MeshRef bakedMesh(const X3DNode *geom, MeshRef raw,
                    const std::vector<TextureTransform2DParams> &params) {
    if (params.empty()) return raw;
    auto key = std::make_pair(geom, ttParamsKey(params));
    auto it = bakedMeshCache_.find(key);
    if (it == bakedMeshCache_.end()) {
      MeshData baked = *raw; // the one deliberate copy: one per distinct bake.
      applyTextureTransformsToMesh(baked, params);
      it = bakedMeshCache_
               .emplace(std::move(key),
                        std::make_shared<const MeshData>(std::move(baked)))
               .first;
    }
    return it->second;
  }

  // Drop every cached mesh derived from `geom` (the raw build + all its baked
  // variants) so the next lookup rebuilds from current field state.
  void evictMeshCache(const X3DNode *geom) {
    rawMeshCache_.erase(geom);
    // Baked keys are (geom, paramsBytes); the map is ordered by that pair, so all
    // of one geometry's variants form a contiguous range starting at (geom, "").
    auto lo = bakedMeshCache_.lower_bound({geom, std::string{}});
    auto hi = lo;
    while (hi != bakedMeshCache_.end() && hi->first.first == geom) ++hi;
    bakedMeshCache_.erase(lo, hi);
  }

  // The TextureTransform params governing an already-emitted item, read back from
  // the Shape at the leaf of its stored path (delta()'s geometry-rebuild arm has
  // the item, not the appearance).
  std::vector<TextureTransform2DParams> ttParamsOfItem(RenderItemId id) const {
    const RenderItem &rec = items_[id];
    if (rec.path.empty() || !rec.path.back()) return {};
    auto appearance = geombounds::getNode(*rec.path.back(), "appearance");
    return textureTransformParamsListOf(appearance ? appearance.get() : nullptr);
  }

  const X3DExecutionContext &ctx_;
  const Scene &scene_;

  // Embedder-supplied build options (tessellation density + B5 geo-projection
  // seam), set once at construction and forwarded to every buildLocalMesh call.
  MeshBuildOptions meshOptions_;

  // Embedder-supplied texture decode seam (T-TEX), set once at construction.
  // Default = makeNullTextureResolver() (always Failed => PoC white-fallback).
  TextureResolver textureResolver_;

  std::vector<RenderItem> items_; // dense, indexed by RenderItemId.
  std::unordered_map<PathKey, RenderItemId, PathKeyHash, PathKeyEqual> index_;

  // M25-5: world-resolved active lights for the current snapshot. Populated
  // once per fullSnapshot() (before the DFS walk) so emit() can tag each item.
  std::vector<LightDesc> lights_;

  // The three reverse indices + the interior-node entry-matrix cache (T8 inputs).
  DepMap transformDeps_;
  DepMap geomDeps_;
  DepMap materialDeps_;
  // A content child-node (Coordinate/Normal/...) -> the geometry node whose mesh
  // delta() must re-extract when that child's content field changes.
  std::unordered_map<const X3DNode *, const X3DNode *> geomNodeOf_;
  std::unordered_map<const X3DNode *, Mat4> entryMatrix_;

  // B2 unsupported-geometry signal: nodeTypeName -> count of dropped Shapes whose
  // geometry was an UNRECOGNIZED type. Ordered map (std::map) so skippedGeometry
  // Counts() iterates deterministically (a coverage report is reproducible).
  std::map<std::string, int> skippedGeometry_;

  // #21: per-walk node-visit budget (set from meshOptions_.maxWalkVisits at the
  // start of fullSnapshot()/delta()). Latches `tripped` when exhausted; surfaced
  // by budgetExceeded().
  WalkBudget walkBudget_;

  // One-delta-per-tick contract state. snapped_ says a baseline exists (an
  // un-snapshotted delta() promotes itself to fullSnapshot()); lastDeltaGen_ is
  // the tick generation the last full/delta read consumed, so a second delta()
  // with no intervening advance returns empty instead of re-diffing a spent set.
  // Generation, not clock: ctx_.now() may legitimately repeat (paused /
  // fixed-timestep / replay), tickGeneration() cannot.
  bool snapped_ = false;
  std::uint64_t lastDeltaGen_ = 0;
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_SCENE_EXTRACTOR_HPP
