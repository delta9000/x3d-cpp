// LocalFogSystem.hpp — §24.4.3 LocalFog extraction: collect every ENABLED
// LocalFog in the scene, resolved to WORLD scale at collection time, each
// carrying the full placement path of its enclosing grouping scope. namespace
// x3d::runtime::extract. Header-only, node-as-truth (field reads go through
// geombounds reflection).
//
// WHY a separate walk (the LightSystem pattern): LocalFog is bound-independent
// — unlike global Fog it is NOT selected by the binding stack. It applies to
// geometry within its PARENT grouping node's scope. This walk re-accumulates
// worldM and the enclosing grouping scope down EACH path (the
// worldOfRec idiom) so a USE'd LocalFog under two Transforms keeps each
// placement distinct.
//
// enabled==false LocalFogs are SKIPPED entirely (like on==false lights): a
// disabled LocalFog leaves the nearest outer enabled LocalFog, or global Fog,
// unchanged within its scope.
//
// SEAM/THREADING: single-threaded; collect() is a pure read over the scene graph.
#ifndef X3D_RUNTIME_EXTRACT_LOCAL_FOG_SYSTEM_HPP
#define X3D_RUNTIME_EXTRACT_LOCAL_FOG_SYSTEM_HPP

#include "FieldRead.hpp"       // geombounds::getField/enumToken/forEachChildNode
#include "GeometryBounds.hpp"
#include "LODSelection.hpp"    // traversedChild (§10.4.3 / §23.4.3)
#include "Mat4.hpp"
#include "RecursionLimits.hpp" // MEM-1: kMaxNestingDepth (walk DoS guard)
#include "RenderItem.hpp"      // LocalFogDesc / FogDesc
#include "TransformSystem.hpp" // isTransform/localMatrix (per-path re-accumulation)
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;

class LocalFogSystem {
public:
  std::vector<LocalFogDesc> collect(const Scene &scene,
                                   const geo::GeoProjection &projection) {
    WalkBudget budget(kMaxGraphWalkVisits);
    return collect(scene, projection, budget, SFVec3f{0, 0, 0});
  }

  // #21: collect against a shared node-visit budget (threaded with the geometry
  // walk by the extractor), so one snapshot ceiling covers both. When requested,
  // dependencies receives every LocalFog (even disabled) and each of its static
  // Transform ancestors, independently of whether its scope emits any geometry.
  // The caller owns/clears this additive set; each USE path contributes frames.
  std::vector<LocalFogDesc> collect(const Scene &scene,
                                    const geo::GeoProjection &projection,
                                    WalkBudget &budget,
                                    const SFVec3f &eyeWorld,
                                    std::unordered_set<const X3DNode *> *dependencies = nullptr) {
    std::vector<LocalFogDesc> out;
    std::vector<const X3DNode *> frames;
    PathKey path;
    for (const auto &root : scene.rootNodes) {
      if (!root) continue;
      // A root LocalFog has no enclosing grouping node => scopeRoot null; a
      // root-level LocalFog applies scene-wide (§24.4.3).
      walk(root.get(), Mat4::identity(), path, /*scopeDepth=*/0, out, budget,
           eyeWorld, projection, frames, dependencies);
    }
    return out;
  }

private:
  static bool isTransform(const X3DNode *n) {
    return x3d::runtime::TransformSystem::isTransform(n);
  }

  static bool isGroupingNode(const X3DNode *n) {
    return n && geombounds::hasField(*n, "children");
  }

  // Mean of the upper-3x3 column norms — matches SceneExtractor::fogWorldScale
  // (uniform scale exact; non-uniform is the documented isotropic approximation).
  static float worldScale(const Mat4 &m) {
    auto norm = [&](int c) {
      const float x = m.m[c * 4 + 0], y = m.m[c * 4 + 1], z = m.m[c * 4 + 2];
      return std::sqrt(x * x + y * y + z * z);
    };
    return (norm(0) + norm(1) + norm(2)) / 3.0f;
  }

  static LocalFogDesc makeFog(const X3DNode &n, const Mat4 &worldM,
                              const PathKey &path, std::size_t scopeDepth) {
    LocalFogDesc f;
    f.color = geombounds::getField<SFColor>(n, "color", SFColor{1.0f, 1.0f, 1.0f});
    const std::string tok = enumToken(n, "fogType", "LINEAR");
    f.fogType = (tok == "EXPONENTIAL") ? FogDesc::Type::Exponential
                                       : FogDesc::Type::Linear;
    f.visibilityRange = geombounds::getField<float>(n, "visibilityRange", 0.0f);
    f.visibilityRange *= worldScale(worldM);
    f.scopePath.assign(path.begin(), path.begin() + scopeDepth);
    f.scopeRoot = f.scopePath.empty() ? nullptr : f.scopePath.back();
    return f;
  }

  void walk(const X3DNode *n, const Mat4 &worldM, PathKey &path, std::size_t scopeDepth,
            std::vector<LocalFogDesc> &out, WalkBudget &budget,
            const SFVec3f &eyeWorld, const geo::GeoProjection &projection,
            std::vector<const X3DNode *> &frames,
            std::unordered_set<const X3DNode *> *dependencies) {
    if (!n) return;
    if (!budget.spend()) return;
    if (path.size() >= kMaxNestingDepth) return;
    for (const X3DNode *ancestor : path)
      if (ancestor == n) return; // reject back-edges, not separate USE placements.
    const bool frame = isTransform(n);
    const Mat4 here = frame
        ? worldM * TransformSystem::localMatrix(n, projection) : worldM;

    if (n->nodeTypeName() == "LocalFog") {
      if (dependencies) {
        dependencies->insert(n);
        dependencies->insert(frames.begin(), frames.end());
      }
      // enabled==false (spec default true): skip entirely, exactly like a light
      // with on==false — an outer LocalFog or global Fog remains in scope.
      if (geombounds::getField<bool>(*n, "enabled", true))
        out.push_back(makeFog(*n, here, path, scopeDepth));
      return; // LocalFog bears no children; nothing below it to scope.
    }

    path.push_back(n);
    if (frame) frames.push_back(n);
    const std::size_t childScopeDepth = isGroupingNode(n) ? path.size() : scopeDepth;
    const std::string typeName = n->nodeTypeName();
    if (typeName == "Switch" || typeName == "LOD") {
      if (auto child = traversedChild(*n, here, eyeWorld))
        walk(child.get(), here, path, childScopeDepth, out, budget, eyeWorld,
             projection, frames, dependencies);
    } else {
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        walk(c.get(), here, path, childScopeDepth, out, budget, eyeWorld,
             projection, frames, dependencies);
      });
    }
    if (frame) frames.pop_back();
    path.pop_back();
  }
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_LOCAL_FOG_SYSTEM_HPP
