// PickSystem.hpp — on-demand ray pick over the scene graph. Broad phase by M2b
// world AABBs; narrow phase against geometry in its local frame. Accumulates the
// world transform during the traversal (no per-node world table needed).
// M2D-2: analytic ray-Cone/Cylinder; strip/fan triangle extraction via
// buildLocalMesh (TriangleStripSet/TriangleFanSet/IndexedTriangleStripSet/
// IndexedTriangleFanSet). AABB proxy remains the fallback for unhandled types.
// namespace x3d::runtime.
#ifndef X3D_RUNTIME_PICK_SYSTEM_HPP
#define X3D_RUNTIME_PICK_SYSTEM_HPP

#include "Billboard.hpp"      // billboardLocalMatrix (§23.4.1, M2e)
#include "BoundsSystem.hpp"
#include "RecursionLimits.hpp" // MEM-1: kMaxNestingDepth (walk DoS guard)
#include "GeometryBounds.hpp" // geombounds::getField/getNode/hasField, localGeometryBounds
#include "Intersect.hpp"
#include "Mat4.hpp"
#include "MeshBuilder.hpp" // extract::buildLocalMesh — promoted exact-triangle extraction
#include "Ray.hpp"
#include "RenderItem.hpp"  // extract::PathKey (root→hit node chain)
#include "TransformSystem.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

struct PickResult {
  bool hit = false;
  X3DNode *node = nullptr;
  SFVec3f point{0,0,0};
  float distance = 0.0f;
  // M2.5 (TouchSensor seam): the root→hit-geometry node chain (mirrors how
  // SceneExtractor builds a PathKey), the WORLD-space surface normal at the
  // hit, and the surface texture coordinate at the hit (§13 parameterization
  // for analytic primitives; barycentric-interpolated for mesh geometry).
  extract::PathKey path;
  SFVec3f normal{0,0,0};
  SFVec2f texCoord{0,0};
  // #21: true if the pick walk hit maxVisits and stopped early — the result is a
  // partial best-so-far over a pathologically wide (acyclic "doubling DAG") scene.
  bool budgetExceeded = false;
};

// Narrow-phase result: entry parameter t plus the LOCAL-frame surface normal
// and the surface texture coordinate at the hit. The caller transforms the
// normal to world (inverse-transpose of the node's world matrix); texCoord is a
// surface attribute and is NOT spatially transformed (§5.1 of the M2.5 design).
struct NarrowHit {
  float t = 0.0f;
  SFVec3f normal{0,0,0}; // local frame
  SFVec2f texCoord{0,0};
};

class PickSystem {
public:
  void build(const Scene &scene) {
    roots_.clear();
    for (const auto &r : scene.rootNodes) if (r) roots_.push_back(r.get());
  }

  // M2e: the bound viewer pose threaded in by the caller (X3DExecutionContext::
  // pick) so Billboard nodes orient view-facing during the pick-path world
  // accumulation, matching the extractor's per-path billboard rotation. The
  // default camera is inert for non-billboard scenes; a scene with a Billboard
  // must pass the live viewer pose for the ray to resolve against the rotated
  // geometry.
  PickResult pickClosest(const Ray &worldRay, const BoundsSystem &bounds,
                         const SFVec3f &cameraPos = {0, 0, 0},
                         const SFVec3f &cameraUp = {0, 1, 0},
                         std::size_t maxVisits = kMaxGraphWalkVisits) const {
    PickResult best;
    extract::PathKey path;
    // #21: pickNode tests geometry per path (each placement has its own world
    // frame), so it cannot memoize like worldOf — a node-visit budget bounds an
    // acyclic ("doubling DAG") fan-out and flags the partial result.
    WalkBudget budget(maxVisits);
    for (X3DNode *r : roots_)
      pickNode(r, Mat4::identity(), worldRay, bounds, cameraPos, cameraUp, path, best, budget);
    best.budgetExceeded = budget.tripped;
    return best;
  }

  // Accumulated world transform at `target` (product of ancestor Transform local
  // matrices). Identity if not found. Used for camera pose in viewMatrix.
  Mat4 worldOf(const X3DNode *target) const {
    Mat4 out = Mat4::identity();
    // #21: memoize fully-explored subtrees so a wide (acyclic "doubling") DAG is
    // searched in O(nodes), not O(paths). worldOf returns the FIRST-found
    // placement, so pruning already-explored subtrees never changes the result.
    std::unordered_set<const X3DNode *> visited;
    for (X3DNode *r : roots_)
      if (worldOfRec(r, Mat4::identity(), target, out, visited)) break;
    return out;
  }

private:
  // Delegates to TransformSystem so all transform-bearing types stay in sync.
  // Billboard's view-dependent rotation is applied separately in pickNode (it
  // depends on the live camera pose, not just the static node fields).
  static bool isTransform(const X3DNode *n) { return TransformSystem::isTransform(n); }

  // Mesh narrow-phase against triangle-bearing types handled by
  // extract::buildLocalMesh. Includes the original three (IndexedFaceSet /
  // IndexedTriangleSet / TriangleSet) plus the four strip/fan variants added in
  // M2D-2 (TriangleStripSet / TriangleFanSet / IndexedTriangleStripSet /
  // IndexedTriangleFanSet).
  static bool isMeshType(const std::string &t) {
    return t == "IndexedFaceSet" || t == "IndexedTriangleSet" || t == "TriangleSet"
        || t == "TriangleStripSet" || t == "TriangleFanSet"
        || t == "IndexedTriangleStripSet" || t == "IndexedTriangleFanSet";
  }

  // §13 texture-coordinate generation for the analytic primitives, evaluated at
  // the local hit point P. Verified against ISO/IEC 19775-1 §13 (M2.5 design).
  static constexpr float kPi = 3.14159265358979323846f;

  // Longitude → s for the body-of-revolution surfaces (Sphere/Cone/Cylinder
  // sides). Seam at X=0, Z<0 (the back) → s=0/1; front (+Z) → s=0.5; +X → 0.75;
  // -X → 0.25 (the canonical VRML/X3D convention annotated in §13).
  static float lonToS(float x, float z) { return std::atan2(x, z) / (2.0f * kPi) + 0.5f; }

  // Sphere (§13.3.7): seam at X=0, Z<0; wraps CCW viewed from +Y.
  static SFVec2f sphereTexCoord(const SFVec3f &p, float r) {
    const float s = lonToS(p.x, p.z);
    const float yc = (r > 0.0f) ? std::clamp(p.y / r, -1.0f, 1.0f) : 0.0f;
    const float t = std::asin(yc) / kPi + 0.5f;
    return SFVec2f{s, t};
  }

  // Box (§13.3.1): six faces, each a full [0,1]² unit square. Face is the
  // dominant axis of P normalized by half-extents.
  static SFVec2f boxTexCoord(const SFVec3f &p, const SFVec3f &half, SFVec3f &n) {
    const float hx = half.x > 0 ? half.x : 1.0f;
    const float hy = half.y > 0 ? half.y : 1.0f;
    const float hz = half.z > 0 ? half.z : 1.0f;
    const float ax = std::fabs(p.x / hx), ay = std::fabs(p.y / hy), az = std::fabs(p.z / hz);
    if (az >= ax && az >= ay) { // FRONT/BACK (±Z)
      if (p.z >= 0) { n = {0,0,1};  return SFVec2f{( p.x/hx + 1)/2, (p.y/hy + 1)/2}; }
      else          { n = {0,0,-1}; return SFVec2f{(-p.x/hx + 1)/2, (p.y/hy + 1)/2}; }
    } else if (ax >= ay) { // RIGHT/LEFT (±X)
      if (p.x >= 0) { n = {1,0,0};  return SFVec2f{(-p.z/hz + 1)/2, (p.y/hy + 1)/2}; }
      else          { n = {-1,0,0}; return SFVec2f{( p.z/hz + 1)/2, (p.y/hy + 1)/2}; }
    } else { // TOP/BOTTOM (±Y)
      if (p.y >= 0) { n = {0,1,0};  return SFVec2f{( p.x/hx + 1)/2, (-p.z/hz + 1)/2}; }
      else          { n = {0,-1,0}; return SFVec2f{( p.x/hx + 1)/2, ( p.z/hz + 1)/2}; }
    }
  }

  static NarrowHit narrowPhase(const X3DNode *geom, const Ray &local, bool &gotHit) {
    gotHit = false;
    NarrowHit hit;
    const std::string t = geom->nodeTypeName();
    if (t == "Sphere") {
      const float r = geombounds::getField<float>(*geom, "radius", 1.0f);
      if (auto h = raySphere(local, r)) {
        hit.t = *h;
        const SFVec3f p = local.pointAt(*h);
        // outward normal = position / radius (sphere centered at origin).
        const float inv = r > 0 ? 1.0f / r : 0.0f;
        hit.normal = {p.x*inv, p.y*inv, p.z*inv};
        hit.texCoord = sphereTexCoord(p, r);
        gotHit = true;
      }
      return hit;
    }
    // M2D-2: analytic Cone — axis=Y, apex at (0,+height/2,0), base radius.
    if (t == "Cone") {
      const float r = geombounds::getField<float>(*geom, "bottomRadius", 1.0f);
      const float h = geombounds::getField<float>(*geom, "height", 2.0f);
      const bool side   = geombounds::getField<bool>(*geom, "side",   true);
      const bool bottom = geombounds::getField<bool>(*geom, "bottom", true);
      if (auto hh = rayCone(local, r, h, side, bottom)) {
        hit.t = *hh;
        const SFVec3f p = local.pointAt(*hh);
        const float halfH = h * 0.5f;
        if (bottom && std::fabs(p.y - (-halfH)) < 1e-4f && (p.x*p.x + p.z*p.z) <= r*r + 1e-4f) {
          // Bottom cap (§13.3.2): right-side-up toward -Z; +x→+s, +z→+t.
          hit.normal = {0,-1,0};
          hit.texCoord = SFVec2f{p.x/(2*r) + 0.5f, p.z/(2*r) + 0.5f};
        } else {
          // Lateral surface (§13.3.2).
          // Local normal of a Y-axis cone: radial xz component, plus +Y from slope.
          const float rad = std::sqrt(p.x*p.x + p.z*p.z);
          const float k = (h > 0) ? r / h : 0.0f; // slope
          SFVec3f nrm{0, 1, 0};
          if (rad > 1e-6f) { nrm = {p.x/rad, k, p.z/rad}; }
          const float nl = std::sqrt(nrm.x*nrm.x + nrm.y*nrm.y + nrm.z*nrm.z);
          if (nl > 1e-12f) nrm = {nrm.x/nl, nrm.y/nl, nrm.z/nl};
          hit.normal = nrm;
          const float s = lonToS(p.x, p.z);
          const float tc = (h > 0) ? (halfH - p.y) / h : 0.0f;
          hit.texCoord = SFVec2f{s, tc};
        }
        gotHit = true;
      }
      return hit;
    }
    // M2D-2: analytic Cylinder — axis=Y, caps at ±height/2.
    if (t == "Cylinder") {
      const float r = geombounds::getField<float>(*geom, "radius", 1.0f);
      const float h = geombounds::getField<float>(*geom, "height", 2.0f);
      const bool side   = geombounds::getField<bool>(*geom, "side",   true);
      const bool top    = geombounds::getField<bool>(*geom, "top",    true);
      const bool bottom = geombounds::getField<bool>(*geom, "bottom", true);
      if (auto hh = rayCylinder(local, r, h, side, top, bottom)) {
        hit.t = *hh;
        const SFVec3f p = local.pointAt(*hh);
        const float halfH = h * 0.5f;
        if (std::fabs(p.y - halfH) < 1e-4f && (p.x*p.x + p.z*p.z) <= r*r + 1e-4f) {
          // Top cap (§13.3.3): right-side-up toward +Z; +x→+s, -z→+t.
          hit.normal = {0,1,0};
          hit.texCoord = SFVec2f{p.x/(2*r) + 0.5f, -p.z/(2*r) + 0.5f};
        } else if (std::fabs(p.y - (-halfH)) < 1e-4f && (p.x*p.x + p.z*p.z) <= r*r + 1e-4f) {
          // Bottom cap (§13.3.3): right-side-up toward -Z; +x→+s, +z→+t.
          hit.normal = {0,-1,0};
          hit.texCoord = SFVec2f{p.x/(2*r) + 0.5f, p.z/(2*r) + 0.5f};
        } else {
          // Lateral surface (§13.3.3): radial normal; seam at X=0,Z<0.
          const float rad = std::sqrt(p.x*p.x + p.z*p.z);
          SFVec3f nrm = (rad > 1e-6f) ? SFVec3f{p.x/rad, 0, p.z/rad} : SFVec3f{1,0,0};
          hit.normal = nrm;
          const float s = lonToS(p.x, p.z);
          const float tc = (h > 0) ? (p.y + halfH) / h : 0.0f;
          hit.texCoord = SFVec2f{s, tc};
        }
        gotHit = true;
      }
      return hit;
    }
    if (isMeshType(t)) {
      extract::MeshData mesh = extract::buildLocalMesh(geom);
      // Behavior-identical to the promoted inline extraction: when the type
      // yields triangles, pick against them; when it yields NONE (e.g. missing
      // coord), fall through to the rayAabb proxy exactly as before.
      if (!mesh.indices.empty()) {
        bool found = false;
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
          const std::uint32_t ia = mesh.indices[i], ib = mesh.indices[i+1], ic = mesh.indices[i+2];
          const auto &a = mesh.positions[ia];
          const auto &b = mesh.positions[ib];
          const auto &c = mesh.positions[ic];
          float u, v;
          auto h = rayTriangleBary(local, a, b, c, u, v);
          if (h && (!found || *h < hit.t)) {
            found = true;
            hit.t = *h;
            const float w = 1.0f - u - v; // weight of a
            // Geometric face normal (CCW): (b-a) x (c-a), normalized.
            const SFVec3f e1{b.x-a.x, b.y-a.y, b.z-a.z};
            const SFVec3f e2{c.x-a.x, c.y-a.y, c.z-a.z};
            SFVec3f nrm{e1.y*e2.z - e1.z*e2.y, e1.z*e2.x - e1.x*e2.z, e1.x*e2.y - e1.y*e2.x};
            const float nl = std::sqrt(nrm.x*nrm.x + nrm.y*nrm.y + nrm.z*nrm.z);
            if (nl > 1e-12f) nrm = {nrm.x/nl, nrm.y/nl, nrm.z/nl};
            hit.normal = nrm;
            // Barycentric-interpolate vertex texcoords; (0,0) if none authored.
            if (mesh.texcoords.size() > ia && mesh.texcoords.size() > ib && mesh.texcoords.size() > ic) {
              const auto &ta = mesh.texcoords[ia];
              const auto &tb = mesh.texcoords[ib];
              const auto &tc = mesh.texcoords[ic];
              hit.texCoord = SFVec2f{w*ta.x + u*tb.x + v*tc.x, w*ta.y + u*tb.y + v*tc.y};
            } else {
              hit.texCoord = SFVec2f{0,0};
            }
          }
        }
        gotHit = found;
        return hit;
      }
    }
    // Long-tail / AABB-proxy fallback (exact for Box). Best-effort normal from
    // the AABB face the local hit lands on; texcoord (0,0). For Box this yields
    // the spec §13.3.1 per-face parameterization (size = full extents).
    if (auto h = rayAabb(local, localGeometryBounds(geom))) {
      hit.t = *h;
      const SFVec3f p = local.pointAt(*h);
      if (t == "Box") {
        const SFVec3f sz = geombounds::getField<SFVec3f>(*geom, "size", {2,2,2});
        SFVec3f half{sz.x*0.5f, sz.y*0.5f, sz.z*0.5f};
        SFVec3f n{0,0,0};
        hit.texCoord = boxTexCoord(p, half, n);
        hit.normal = n;
      } else {
        // Generic AABB proxy: pick the dominant face from the local hit point.
        const Aabb bb = localGeometryBounds(geom);
        const SFVec3f c{(bb.min.x+bb.max.x)*0.5f, (bb.min.y+bb.max.y)*0.5f, (bb.min.z+bb.max.z)*0.5f};
        const SFVec3f e{std::max(1e-6f,(bb.max.x-bb.min.x)*0.5f),
                        std::max(1e-6f,(bb.max.y-bb.min.y)*0.5f),
                        std::max(1e-6f,(bb.max.z-bb.min.z)*0.5f)};
        const float rx = (p.x-c.x)/e.x, ry = (p.y-c.y)/e.y, rz = (p.z-c.z)/e.z;
        const float ax = std::fabs(rx), ay = std::fabs(ry), az = std::fabs(rz);
        if (az >= ax && az >= ay)      hit.normal = {0,0, rz >= 0 ? 1.0f : -1.0f};
        else if (ax >= ay)             hit.normal = {rx >= 0 ? 1.0f : -1.0f, 0, 0};
        else                           hit.normal = {0, ry >= 0 ? 1.0f : -1.0f, 0};
        hit.texCoord = SFVec2f{0,0};
      }
      gotHit = true;
    }
    return hit;
  }

  void pickNode(const X3DNode *n, const Mat4 &worldM, const Ray &worldRay,
                const BoundsSystem &bounds, const SFVec3f &cameraPos,
                const SFVec3f &cameraUp, extract::PathKey &path,
                PickResult &best, WalkBudget &budget) const {
    // #21: bound total node-visits so an acyclic ("doubling DAG") fan-out cannot
    // test geometry per-path without limit (the partial best-so-far is flagged
    // via PickResult.budgetExceeded).
    if (!budget.spend()) return;
    // MEM-1: bound the pick walk so an unsanitized graph (USE-cyclic or
    // pathologically deep) cannot stack-overflow. `path` is the live root→n
    // ancestor chain; a depth cap plus a path-membership test before descending
    // breaks back-edges (the runtime twin of SEC-1).
    if (path.size() >= kMaxNestingDepth) return;
    for (const X3DNode *ancestor : path)
      if (ancestor == n) return; // n is its own ancestor: containment cycle.
    path.push_back(n); // accumulate the root→this-node chain (PathKey).
    Mat4 childM = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
    // M2e: Billboard contributes a view-dependent local rotation, exactly as the
    // extractor's per-path walk (SceneExtractor::walk). Apply it here in the
    // narrow-phase walk so geometry under a Billboard is picked in the rotated
    // frame, not the un-rotated parent frame.
    if (n->nodeTypeName() == "Billboard") {
      const SFVec3f axis = geombounds::getField<SFVec3f>(*n, "axisOfRotation", {0, 1, 0});
      childM = worldM * billboardLocalMatrix(worldM, cameraPos, cameraUp, axis);
    }
    if (geombounds::hasField(*n, "geometry")) {
      if (auto geom = geombounds::getNode(*n, "geometry")) {
        Aabb wb = bounds.localBounds(n).transformed(worldM);
        if (rayAabb(worldRay, wb)) {
          Mat4 inv = worldM.inverse();
          Ray local{inv.transformPoint(worldRay.origin), inv.transformDirection(worldRay.direction)};
          bool gotHit = false;
          NarrowHit h = narrowPhase(geom.get(), local, gotHit);
          if (gotHit) {
            SFVec3f wp = worldM.transformPoint(local.pointAt(h.t));
            float dx = wp.x-worldRay.origin.x, dy = wp.y-worldRay.origin.y, dz = wp.z-worldRay.origin.z;
            float d = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (!best.hit || d < best.distance) {
              best.hit = true; best.node = const_cast<X3DNode*>(n); best.point = wp; best.distance = d;
              best.path = path; // root→this geometry-bearing node chain.
              // World-space normal = normalize(inverse-transpose(worldM) · n).
              best.normal = normalize(invTranspose(worldM).transformDirection(h.normal));
              best.texCoord = h.texCoord; // surface attribute — not transformed.
            }
          }
        }
      }
    }
    forEachChild(n, [&](const X3DNode *c) {
      pickNode(c, childM, worldRay, bounds, cameraPos, cameraUp, path, best, budget);
    });
    path.pop_back();
  }

  // Inverse-transpose of the upper-left 3x3 (carried in a Mat4) for transforming
  // surface normals to world space. Translation is irrelevant for directions.
  static Mat4 invTranspose(const Mat4 &m) {
    Mat4 inv = m.inverse();
    Mat4 r = Mat4::identity();
    // transpose of inv's upper-left 3x3 (column-major: m[c*4+row]).
    for (int c = 0; c < 3; ++c)
      for (int row = 0; row < 3; ++row)
        r.m[c*4+row] = inv.m[row*4+c];
    return r;
  }

  static SFVec3f normalize(const SFVec3f &v) {
    float l = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return l > 1e-12f ? SFVec3f{v.x/l, v.y/l, v.z/l} : v;
  }

  // Pure Transform accumulation: the product of ancestor Transform local
  // matrices to `target`. Deliberately does NOT fold in Billboard view-facing
  // rotation — its sole caller (X3DExecutionContext::viewMatrix) uses this to
  // compute the bound Viewpoint's world frame, and injecting view-dependent
  // billboard orientation here would feed camera pose back into the very
  // computation that defines it.
  bool worldOfRec(const X3DNode *n, const Mat4 &worldM, const X3DNode *target,
                  Mat4 &out, std::unordered_set<const X3DNode *> &visited,
                  std::size_t depth = 0) const {
    // MEM-1: a hard depth cap stops a USE-cyclic / pathologically deep graph from
    // overrunning the stack when `target` is absent (full traversal). worldOf()
    // returns identity on a miss, so bailing here is the existing not-found path.
    if (depth >= kMaxNestingDepth) return false;
    // #21: skip a subtree already explored without finding `target`. A node is
    // only re-reached after its first exploration fully returned (DFS), so if it
    // returned false then, `target` is provably not under it. The first path that
    // reaches `target` is unaffected, so worldOf's first-found result is preserved.
    if (!visited.insert(n).second) return false;
    Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
    if (n == target) { out = here; return true; }
    bool found = false;
    forEachChild(n, [&](const X3DNode *c) { if (!found && worldOfRec(c, here, target, out, visited, depth + 1)) found = true; });
    return found;
  }

  template <class F>
  static void forEachChild(const X3DNode *n, F &&f) {
    for (const auto &fi : n->fields()) {
      if (!fi.get) continue;
      if (fi.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(fi.get(*n));
        if (c) f(c.get());
      } else if (fi.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi.get(*n)))
          if (c) f(c.get());
      }
    }
  }

  std::vector<X3DNode *> roots_;
};

} // namespace x3d::runtime
#endif
