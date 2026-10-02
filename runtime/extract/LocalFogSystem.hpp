// LocalFogSystem.hpp — §24.4.3 LocalFog extraction: collect every ENABLED
// LocalFog in the scene, resolved to WORLD scale at collection time, each
// carrying the enclosing grouping node it is scoped to. namespace
// x3d::runtime::extract. Header-only, node-as-truth (field reads go through
// geombounds reflection).
//
// WHY a separate walk (the LightSystem pattern): LocalFog is bound-independent
// — unlike global Fog it is NOT selected by the binding stack. It applies to
// geometry within its PARENT grouping node's scope, exactly like a non-global
// light's scopeRoot. This walk re-accumulates worldM down EACH path (the
// worldOfRec idiom) so a USE'd LocalFog under two Transforms keeps each
// placement distinct.
//
// enabled==false LocalFogs are SKIPPED entirely (like on==false lights): a
// disabled LocalFog does not override global Fog, so global Fog applies
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
  std::vector<LocalFogDesc> collect(const Scene &scene) {
    WalkBudget budget(kMaxGraphWalkVisits);
    return collect(scene, budget, SFVec3f{0, 0, 0});
  }

  // #21: collect against a shared node-visit budget (threaded with the geometry
  // walk by the extractor), so one snapshot ceiling covers both. When requested,
  // dependencies receives every LocalFog (even disabled) and each of its static
  // Transform ancestors, independently of whether its scope emits any geometry.
  // The caller owns/clears this additive set; each USE path contributes frames.
  std::vector<LocalFogDesc> collect(const Scene &scene, WalkBudget &budget,
                                    const SFVec3f &eyeWorld,
                                    std::unordered_set<const X3DNode *> *dependencies = nullptr) {
    std::vector<LocalFogDesc> out;
    std::vector<const X3DNode *> frames;
    for (const auto &root : scene.rootNodes) {
      if (!root) continue;
      // A root LocalFog has no enclosing grouping node => scopeRoot null; a
      // root-level LocalFog applies scene-wide (§24.4.3).
      walk(root.get(), Mat4::identity(), /*scopeRoot=*/nullptr, out, budget,
           eyeWorld, frames, dependencies);
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
                              const X3DNode *scopeRoot) {
    LocalFogDesc f;
    f.color = geombounds::getField<SFColor>(n, "color", SFColor{1.0f, 1.0f, 1.0f});
    const std::string tok = enumToken(n, "fogType", "LINEAR");
    f.fogType = (tok == "EXPONENTIAL") ? FogDesc::Type::Exponential
                                       : FogDesc::Type::Linear;
    f.visibilityRange = geombounds::getField<float>(n, "visibilityRange", 0.0f);
    f.visibilityRange *= worldScale(worldM);
    f.scopeRoot = scopeRoot;
    return f;
  }

  void walk(const X3DNode *n, const Mat4 &worldM, const X3DNode *scopeRoot,
            std::vector<LocalFogDesc> &out, WalkBudget &budget,
            const SFVec3f &eyeWorld, std::vector<const X3DNode *> &frames,
            std::unordered_set<const X3DNode *> *dependencies,
            std::size_t depth = 0) {
    if (!n) return;
    if (!budget.spend()) return;
    if (depth >= kMaxNestingDepth) return;
    const bool frame = isTransform(n);
    const Mat4 here = frame ? worldM * TransformSystem::localMatrix(n) : worldM;

    if (n->nodeTypeName() == "LocalFog") {
      if (dependencies) {
        dependencies->insert(n);
        dependencies->insert(frames.begin(), frames.end());
      }
      // enabled==false (spec default true): skip entirely, exactly like a light
      // with on==false — global Fog applies unchanged within this scope.
      if (geombounds::getField<bool>(*n, "enabled", true))
        out.push_back(makeFog(*n, here, scopeRoot));
      return; // LocalFog bears no children; nothing below it to scope.
    }

    if (frame) frames.push_back(n);
    const X3DNode *childScope = isGroupingNode(n) ? n : scopeRoot;
    const std::string typeName = n->nodeTypeName();
    if (typeName == "Switch" || typeName == "LOD") {
      if (auto child = traversedChild(*n, here, eyeWorld))
        walk(child.get(), here, childScope, out, budget, eyeWorld,
             frames, dependencies, depth + 1);
    } else {
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        walk(c.get(), here, childScope, out, budget, eyeWorld,
             frames, dependencies, depth + 1);
      });
    }
    if (frame) frames.pop_back();
  }
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_LOCAL_FOG_SYSTEM_HPP
