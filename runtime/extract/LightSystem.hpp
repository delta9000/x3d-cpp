// LightSystem.hpp — M2.5 extraction (Layer A6): collect every active light in the
// scene, resolved to WORLD space at collection time. namespace
// x3d::runtime::extract. Header-only, golden-untouched, node-as-truth (no member
// state on generated nodes; field reads go through geombounds reflection).
//
// WHY world-at-collection (the M2C-1 sidestep): a light's direction/location are
// authored in its LOCAL frame. Like PickSystem::worldOfRec / SceneExtractor::walk,
// this walk RE-ACCUMULATES worldM down EACH path via TransformSystem::localMatrix.
// It NEVER reads ctx.worldTransform()/TransformSystem.world_ — that table is
// first-path-only, so a USE'd light under two Transforms would collapse to one
// placement. Re-accumulating keeps each placement's world frame distinct.
//
// FIDELITY (the spec-correctness pillar this task defends):
//   * The authored `global` flag is CARRIED, NOT promoted. Verified spec
//     defaults: DirectionalLight global=false, PointLight/SpotLight global=true.
//     A consumer must NOT silently promote a global=false light to scene-wide;
//     LightSystem reads the authored value and hands it through untouched.
//   * scopeRoot is the enclosing grouping node the light was collected under, so
//     a non-global light can be scoped to that subtree by a consumer.
//   * worldDirection/worldLocation are world-frame; direction is renormalized
//     after the (possibly non-uniformly scaled) transform.
//   * on==false lights are skipped entirely.
//
// SEAM/THREADING: single-threaded; collect() is a pure read over the scene graph.
#ifndef X3D_RUNTIME_EXTRACT_LIGHT_SYSTEM_HPP
#define X3D_RUNTIME_EXTRACT_LIGHT_SYSTEM_HPP

#include "FieldRead.hpp"
#include "GeometryBounds.hpp"  // geombounds::getField/getNode/hasField
#include "Mat4.hpp"            // transformDirection/transformPoint
#include "RecursionLimits.hpp" // MEM-1: kMaxNestingDepth (walk DoS guard)
#include "RenderItem.hpp"      // LightDesc
#include "TransformSystem.hpp" // localMatrix (static; per-path re-accumulation)
#include "LODSelection.hpp" // traversedChild (§10.4.3 / §23.4.3)
#include "MaterialSystem.hpp" // matsys::refOf (projector textures)
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;

class LightSystem {
public:
  // One full walk; returns every active (on==true) light, world-resolved. The
  // self-budgeted overload caps the fan-out with a fresh default budget.
  std::vector<LightDesc> collect(const Scene &scene,
                                const geo::GeoProjection &projection) {
    WalkBudget budget(kMaxGraphWalkVisits);
    return collect(scene, projection, budget, SFVec3f{0, 0, 0});
  }

  std::vector<LightDesc> collect(const Scene &scene,
                                const geo::GeoProjection &projection,
                                const SFVec3f &eyeWorld) {
    WalkBudget budget(kMaxGraphWalkVisits);
    return collect(scene, projection, budget, eyeWorld);
  }

  // #21: collect against a shared node-visit budget — the extractor threads the
  // SAME budget through light collection and its geometry walk, so one snapshot
  // ceiling covers both. `budget.tripped` reports an early stop.
  std::vector<LightDesc> collect(const Scene &scene,
                                const geo::GeoProjection &projection,
                                WalkBudget &budget) {
    return collect(scene, projection, budget, SFVec3f{0, 0, 0});
  }

  std::vector<LightDesc> collect(const Scene &scene,
                                const geo::GeoProjection &projection,
                                WalkBudget &budget, const SFVec3f &eyeWorld) {
    std::vector<LightDesc> out;
    auto leaf = [&](const X3DNode *n, const Mat4 &worldM, const X3DNode *scopeRoot) {
      bool isLight = false;
      LightDesc::Type type = lightType(n->nodeTypeName(), isLight);
      if (!isLight) return false;
      // on==true gate (spec default true) — skip disabled lights entirely.
      if (geombounds::getField<bool>(*n, "on", true))
        out.push_back(makeLight(*n, type, worldM, scopeRoot));
      return true; // a light bears no children; nothing below it to scope.
    };
    for (const auto &root : scene.rootNodes) {
      if (!root) continue;
      // A root light has no enclosing grouping node => scopeRoot null.
      walk(root.get(), Mat4::identity(), /*scopeRoot=*/nullptr, leaf, budget,
           eyeWorld, projection);
    }
    return out;
  }

  // §42 texture projectors (TextureProjector, TextureProjectorParallel): the
  // same walk, world resolution and scoping as lights; on==false skipped. The
  // texture ref is extracted but not resolved (SceneExtractor::projectors()).
  std::vector<ProjectorDesc> collectProjectors(const Scene &scene,
                                               const geo::GeoProjection &projection,
                                               WalkBudget &budget,
                                               const SFVec3f &eyeWorld) {
    std::vector<ProjectorDesc> out;
    auto leaf = [&](const X3DNode *n, const Mat4 &worldM, const X3DNode *scopeRoot) {
      const std::string t = n->nodeTypeName();
      if (t != "TextureProjector" && t != "TextureProjectorParallel") return false;
      if (geombounds::getField<bool>(*n, "on", true))
        out.push_back(makeProjector(*n, t == "TextureProjectorParallel", worldM,
                                    scopeRoot));
      return true;
    };
    for (const auto &root : scene.rootNodes)
      if (root)
        walk(root.get(), Mat4::identity(), nullptr, leaf, budget, eyeWorld, projection);
    return out;
  }

private:
  // Delegates to TransformSystem so all transform-bearing types stay in sync.
  // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
  static bool isTransform(const X3DNode *n) {
    return x3d::runtime::TransformSystem::isTransform(n);
  }

  // A grouping node is any node carrying a `children` MFNode slot (Group,
  // Transform, Switch, Anchor, ...). The enclosing one becomes a child light's
  // scopeRoot — the subtree a non-global light is scoped to.
  static bool isGroupingNode(const X3DNode *n) {
    return n && geombounds::hasField(*n, "children");
  }

  static LightDesc::Type lightType(const std::string &t, bool &isLight) {
    isLight = true;
    if (t == "DirectionalLight") return LightDesc::Type::Directional;
    if (t == "PointLight") return LightDesc::Type::Point;
    if (t == "SpotLight") return LightDesc::Type::Spot;
    isLight = false;
    return LightDesc::Type::Directional;
  }

  static SFVec3f normalize(const SFVec3f &v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len <= 1e-12f) return v; // degenerate; hand back as-is.
    return SFVec3f{v.x / len, v.y / len, v.z / len};
  }

  // `leaf(node, world, scopeRoot)` collects a light-like node and returns true
  // to stop the descent there.
  template <typename Leaf>
  void walk(const X3DNode *n, const Mat4 &worldM, const X3DNode *scopeRoot,
            Leaf &leaf, WalkBudget &budget,
            const SFVec3f &eyeWorld,
            const geo::GeoProjection &projection,
            std::size_t depth = 0) {
    if (!n) return;
    // #21: bound total node-visits so a wide acyclic ("doubling DAG") light
    // fan-out collects a bounded number of LightDescs (the shared budget makes
    // the extractor report the trip via budgetExceeded()).
    if (!budget.spend()) return;
    // MEM-1: a hard depth cap keeps light collection from stack-overflowing on a
    // USE-cyclic / pathologically deep graph (this walk runs before the
    // extractor's own walk in fullSnapshot, so it must be self-safe too).
    if (depth >= kMaxNestingDepth) return;
    Mat4 here = isTransform(n)
        ? worldM * TransformSystem::localMatrix(n, projection) : worldM;

    if (leaf(n, here, scopeRoot)) return;

    // Descend. The scopeRoot handed to children is THIS node when it is a
    // grouping node, else the inherited one (a non-grouping passthrough keeps
    // the enclosing group as the scope anchor).
    const X3DNode *childScope = isGroupingNode(n) ? n : scopeRoot;
    const std::string typeName = n->nodeTypeName();
    if (typeName == "Switch" || typeName == "LOD") {
      if (auto child = traversedChild(*n, here, eyeWorld))
        walk(child.get(), here, childScope, leaf, budget, eyeWorld, projection,
             depth + 1);
      return;
    }
    forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      walk(c.get(), here, childScope, leaf, budget, eyeWorld, projection, depth + 1);
    });
  }

  // Read every light field reflection-generic by spec name; world-resolve the
  // direction/location through the accumulated matrix. Unset fields fall back to
  // their X3D spec defaults (which also match the LightDesc member defaults).
  static LightDesc makeLight(const X3DNode &n, LightDesc::Type type,
                             const Mat4 &worldM, const X3DNode *scopeRoot) {
    LightDesc L;
    L.type = type;

    // Shared across all three: color/intensity/ambientIntensity.
    L.color = geombounds::getField<SFColor>(n, "color", SFColor{1, 1, 1});
    L.intensity = geombounds::getField<float>(n, "intensity", 1.0f);
    L.ambientIntensity =
        geombounds::getField<float>(n, "ambientIntensity", 0.0f);

    // X3D 4.0 §17.3.1 shadow controls (X3DLightNode, all three types).
    L.shadows = geombounds::getField<bool>(n, "shadows", false);
    L.shadowIntensity =
        geombounds::getField<float>(n, "shadowIntensity", 1.0f);

    // CARRIED authored global — verified defaults differ by type, so the
    // fallback matches the spec for THIS node type (never blanket-promoted).
    bool defaultGlobal = (type != LightDesc::Type::Directional);
    L.global = geombounds::getField<bool>(n, "global", defaultGlobal);
    L.scopeRoot = scopeRoot;

    // Directional/Spot carry a direction; world-resolve (w=0) + renormalize.
    if (type == LightDesc::Type::Directional || type == LightDesc::Type::Spot) {
      SFVec3f dir = geombounds::getField<SFVec3f>(n, "direction", SFVec3f{0, 0, -1});
      L.worldDirection = normalize(worldM.transformDirection(dir));
    }
    // Point/Spot carry a location/attenuation/radius; world-resolve location.
    if (type == LightDesc::Type::Point || type == LightDesc::Type::Spot) {
      SFVec3f loc = geombounds::getField<SFVec3f>(n, "location", SFVec3f{0, 0, 0});
      L.worldLocation = worldM.transformPoint(loc);
      L.attenuation =
          geombounds::getField<SFVec3f>(n, "attenuation", SFVec3f{1, 0, 0});
      const float radius = geombounds::getField<float>(n, "radius", 100.0f);
      // §17.4.2–3: ancestor scale affects a positional light's radius.
      const SFVec3f sx = worldM.transformDirection({1, 0, 0});
      const SFVec3f sy = worldM.transformDirection({0, 1, 0});
      const SFVec3f sz = worldM.transformDirection({0, 0, 1});
      const auto length = [](const SFVec3f &v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
      };
      L.radius = radius * std::max({length(sx), length(sy), length(sz)});
    }
    // Spot-only beam parameters.
    if (type == LightDesc::Type::Spot) {
      L.beamWidth = geombounds::getField<float>(n, "beamWidth", 1.5708f);
      L.cutOffAngle = geombounds::getField<float>(n, "cutOffAngle", 0.7854f);
    }
    return L;
  }

  static SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
  }
  static float length(const SFVec3f &v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

  // §42.4: the projector frame is location/direction/upVector in the node's
  // local frame (ADR-0061). upVector (perspective only; parallel has none)
  // falls back to +Y, then +Z, when it is parallel to the direction, as the
  // defaults (direction 0 0 1, upVector 0 0 1) are. Distances and the parallel
  // extents scale with the local frame along their own axes.
  static ProjectorDesc makeProjector(const X3DNode &n, bool parallel,
                                     const Mat4 &worldM, const X3DNode *scopeRoot) {
    ProjectorDesc P;
    P.type = parallel ? ProjectorDesc::Type::Parallel : ProjectorDesc::Type::Perspective;
    P.color = geombounds::getField<SFColor>(n, "color", SFColor{1, 1, 1});
    P.intensity = geombounds::getField<float>(n, "intensity", 1.0f);
    P.ambientIntensity = geombounds::getField<float>(n, "ambientIntensity", 0.0f);
    P.shadows = geombounds::getField<bool>(n, "shadows", false);
    P.shadowIntensity = geombounds::getField<float>(n, "shadowIntensity", 1.0f);
    P.global = geombounds::getField<bool>(n, "global", true);
    P.scopeRoot = scopeRoot;

    SFVec3f dir = normalize(geombounds::getField<SFVec3f>(n, "direction", SFVec3f{0, 0, 1}));
    if (length(dir) <= 1e-6f) dir = {0, 0, 1};
    SFVec3f up = parallel ? SFVec3f{0, 1, 0}
                          : geombounds::getField<SFVec3f>(n, "upVector", SFVec3f{0, 0, 1});
    for (const SFVec3f &candidate : {up, SFVec3f{0, 1, 0}, SFVec3f{0, 0, 1}}) {
      up = candidate;
      if (length(cross(dir, up)) > 1e-4f * std::max(length(up), 1e-6f)) break;
    }
    const SFVec3f right = normalize(cross(dir, up));
    const SFVec3f trueUp = cross(right, dir);

    const SFVec3f loc = geombounds::getField<SFVec3f>(n, "location", SFVec3f{0, 0, 0});
    P.worldLocation = worldM.transformPoint(loc);
    const SFVec3f wd = worldM.transformDirection(dir);
    const SFVec3f wu = worldM.transformDirection(trueUp);
    const SFVec3f wr = worldM.transformDirection(right);
    const float depthScale = length(wd), upScale = length(wu), rightScale = length(wr);
    P.worldDirection = normalize(wd);
    const SFVec3f r = normalize(cross(P.worldDirection, wu));
    P.worldUp = cross(r, P.worldDirection);

    const float nearD = geombounds::getField<float>(n, "nearDistance", -1.0f);
    const float farD = geombounds::getField<float>(n, "farDistance", -1.0f);
    P.nearDistance = nearD > 0.0f ? nearD * depthScale : -1.0f;
    P.farDistance = farD > 0.0f ? farD * depthScale : -1.0f;

    // view: rows right, up, -direction; translation moves the location to 0.
    const SFVec3f &f = P.worldDirection, &u = P.worldUp, &o = P.worldLocation;
    Mat4 v = Mat4::identity();
    v.m[0] = r.x; v.m[4] = r.y; v.m[8] = r.z;
    v.m[1] = u.x; v.m[5] = u.y; v.m[9] = u.z;
    v.m[2] = -f.x; v.m[6] = -f.y; v.m[10] = -f.z;
    v.m[12] = -(r.x * o.x + r.y * o.y + r.z * o.z);
    v.m[13] = -(u.x * o.x + u.y * o.y + u.z * o.z);
    v.m[14] = f.x * o.x + f.y * o.y + f.z * o.z;
    P.view = v;

    P.texture = matsys::refOf(geombounds::getNode(n, "texture"), TextureRef::Slot::BaseColor);
    if (P.texture.source == TextureRef::Source::Inline &&
        P.texture.inlinePixels.width > 0 && P.texture.inlinePixels.height > 0)
      P.aspectRatio = static_cast<float>(P.texture.inlinePixels.width) /
                      static_cast<float>(P.texture.inlinePixels.height);

    if (parallel) {
      // fieldOfView (minX, minY, maxX, maxY) in the projector's local frame.
      const SFVec4f fov = geombounds::getField<SFVec4f>(n, "fieldOfView", SFVec4f{-1, -1, 1, 1});
      P.parallelFieldOfView = {fov.x * rightScale, fov.y * upScale, fov.z * rightScale,
                               fov.w * upScale};
    } else {
      P.fieldOfView = geombounds::getField<float>(n, "fieldOfView", 0.7854f);
    }
    P.updateProjection();
    return P;
  }
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_LIGHT_SYSTEM_HPP
