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

#include "GeometryBounds.hpp"  // geombounds::getField/getNode/hasField
#include "Mat4.hpp"            // transformDirection/transformPoint
#include "RecursionLimits.hpp" // MEM-1: kMaxNestingDepth (walk DoS guard)
#include "RenderItem.hpp"      // LightDesc
#include "TransformSystem.hpp" // localMatrix (static; per-path re-accumulation)
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

class LightSystem {
public:
  // One full walk; returns every active (on==true) light, world-resolved. The
  // self-budgeted overload caps the fan-out with a fresh default budget.
  std::vector<LightDesc> collect(const Scene &scene) {
    WalkBudget budget(kMaxGraphWalkVisits);
    return collect(scene, budget);
  }

  // #21: collect against a shared node-visit budget — the extractor threads the
  // SAME budget through light collection and its geometry walk, so one snapshot
  // ceiling covers both. `budget.tripped` reports an early stop.
  std::vector<LightDesc> collect(const Scene &scene, WalkBudget &budget) {
    std::vector<LightDesc> out;
    for (const auto &root : scene.rootNodes) {
      if (!root) continue;
      // A root light has no enclosing grouping node => scopeRoot null.
      walk(root.get(), Mat4::identity(), /*scopeRoot=*/nullptr, out, budget);
    }
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

  void walk(const X3DNode *n, const Mat4 &worldM, const X3DNode *scopeRoot,
            std::vector<LightDesc> &out, WalkBudget &budget,
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
    Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;

    bool isLight = false;
    LightDesc::Type type = lightType(n->nodeTypeName(), isLight);
    if (isLight) {
      // on==true gate (spec default true) — skip disabled lights entirely.
      if (geombounds::getField<bool>(*n, "on", true))
        out.push_back(makeLight(*n, type, here, scopeRoot));
      // A light bears no children; nothing below it to scope.
      return;
    }

    // Descend. The scopeRoot handed to children is THIS node when it is a
    // grouping node, else the inherited one (a non-grouping passthrough keeps
    // the enclosing group as the scope anchor).
    const X3DNode *childScope = isGroupingNode(n) ? n : scopeRoot;
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) walk(c.get(), here, childScope, out, budget, depth + 1);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get(), here, childScope, out, budget, depth + 1);
      }
    }
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
      L.radius = geombounds::getField<float>(n, "radius", 100.0f);
    }
    // Spot-only beam parameters.
    if (type == LightDesc::Type::Spot) {
      L.beamWidth = geombounds::getField<float>(n, "beamWidth", 1.5708f);
      L.cutOffAngle = geombounds::getField<float>(n, "cutOffAngle", 0.7854f);
    }
    return L;
  }
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_LIGHT_SYSTEM_HPP
