// mesh_builder_tc4_test.cpp — M2.5 TC4 acceptance: DEFAULT (implicit) texture-
// coordinate generation for the analytic primitives Box / Sphere / Cone /
// Cylinder. These primitives carry NO texCoord field; the per-primitive mapping
// is ALWAYS the spec default, generated during tessellation alongside the
// positions+normals the T4 tessellators already produce (ISO/IEC 19775-1).
//
// Proofs:
//   1) Box — each of the 6 faces' 4 corners cover {(0,0),(1,0),(1,1),(0,1)};
//      texcoords parallel to positions (one per expanded corner).
//   2) Sphere — t monotonic from ~0 (bottom/-Y pole ring) to ~1 (top/+Y pole
//      ring); s wraps 0..1 around longitude. One texcoord per corner.
//   3) Cylinder — side s = angle/(2pi) around, t = 0..1 bottom-to-top; caps
//      mapped radially in [0,1]x[0,1]. One texcoord per corner.
//   4) Cone — side s = angle/(2pi) around, t = 0 at base .. 1 at apex; bottom
//      cap radial in [0,1]x[0,1]. One texcoord per corner.
//   5) Toggling caps off keeps texcoords parallel to positions (no orphans).
#include "MeshBuilder.hpp"

#include "X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// Does any expanded corner carry exactly UV (s,t)?
static bool hasUv(const MeshData &m, float s, float t) {
  for (const auto &uv : m.texcoords)
    if (feq(uv.x, s) && feq(uv.y, t)) return true;
  return false;
}

TEST_CASE("mesh_builder_tc4_test") {
  // ---- 1. Box: every face's 4 corners cover {(0,0),(1,0),(1,1),(0,1)} -------
  {
    auto g = createX3DNode("Box");
    setF(g, "size", std::any(SFVec3f{2, 2, 2}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((!m.texcoords.empty()));
    CHECK((m.texcoords.size() == m.positions.size())); // one per corner.
    // The four canonical corners must ALL be present (each face maps the unit
    // square; 12 triangles still only use the 4 corner UVs).
    CHECK((hasUv(m, 0.0f, 0.0f)));
    CHECK((hasUv(m, 1.0f, 0.0f)));
    CHECK((hasUv(m, 1.0f, 1.0f)));
    CHECK((hasUv(m, 0.0f, 1.0f)));
    // EVERY box UV is a corner of the unit square (only 0/1 components).
    for (const auto &uv : m.texcoords) {
      CHECK((feq(uv.x, 0.0f) || feq(uv.x, 1.0f)));
      CHECK((feq(uv.y, 0.0f) || feq(uv.y, 1.0f)));
    }
    // Each of the 6 faces (2 tris = 6 corners) must, across its corners, cover
    // all four canonical UVs. Faces are emitted contiguously (12 positions each
    // -> 6 corners per face after the 0..N-1 expansion: 6 faces * 6 corners).
    for (int f = 0; f < 6; ++f) {
      bool c00 = false, c10 = false, c11 = false, c01 = false;
      for (int k = 0; k < 6; ++k) {
        const auto &uv = m.texcoords[f * 6 + k];
        if (feq(uv.x, 0) && feq(uv.y, 0)) c00 = true;
        if (feq(uv.x, 1) && feq(uv.y, 0)) c10 = true;
        if (feq(uv.x, 1) && feq(uv.y, 1)) c11 = true;
        if (feq(uv.x, 0) && feq(uv.y, 1)) c01 = true;
      }
      CHECK((c00 && c10 && c11 && c01));
    }
  }

  // ---- 2. Sphere: t monotonic bottom(~0)..top(~1); s wraps 0..1 ------------
  {
    auto g = createX3DNode("Sphere");
    setF(g, "radius", std::any(SFFloat(1.0f)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((!m.texcoords.empty()));
    CHECK((m.texcoords.size() == m.positions.size()));
    // t correlates with height: the -Y pole (bottom) has t~0, +Y pole t~1.
    // Find min/max t and assert the corresponding positions are bottom/top.
    float minT = 1e9f, maxT = -1e9f;
    SFVec3f atMin{}, atMax{};
    for (std::size_t i = 0; i < m.texcoords.size(); ++i) {
      const float t = m.texcoords[i].y;
      if (t < minT) { minT = t; atMin = m.positions[i]; }
      if (t > maxT) { maxT = t; atMax = m.positions[i]; }
    }
    CHECK((feq(minT, 0.0f)));
    CHECK((feq(maxT, 1.0f)));
    CHECK((atMin.y < 0.0f));  // smallest-t vertex is near the -Y (bottom) pole.
    CHECK((atMax.y > 0.0f));  // largest-t vertex is near the +Y (top) pole.
    // t is the spec LATITUDE mapping (linear in the polar angle, NOT in height),
    // so it is MONOTONIC with y: a higher vertex never has a smaller t. Check
    // every pair of corners that differ noticeably in height.
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      for (std::size_t k = 0; k < m.positions.size(); ++k)
        if (m.positions[i].y > m.positions[k].y + 1e-3f)
          CHECK((m.texcoords[i].y >= m.texcoords[k].y - 1e-4f));
    // s ranges across [0,1]: the seam at theta=0 gives s=0, the wrap gives s=1.
    float minS = 1e9f, maxS = -1e9f;
    for (const auto &uv : m.texcoords) {
      minS = std::min(minS, uv.x);
      maxS = std::max(maxS, uv.x);
    }
    CHECK((feq(minS, 0.0f)));
    CHECK((feq(maxS, 1.0f)));
  }

  // ---- 3. Cylinder: side s=angle/2pi, t=0..1; caps radial in [0,1]^2 -------
  {
    auto g = createX3DNode("Cylinder");
    setF(g, "radius", std::any(SFFloat(1.0f)));
    setF(g, "height", std::any(SFFloat(2.0f)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((!m.texcoords.empty()));
    CHECK((m.texcoords.size() == m.positions.size()));
    // All UVs in [0,1].
    for (const auto &uv : m.texcoords) {
      CHECK((uv.x >= -1e-4f && uv.x <= 1.0f + 1e-4f));
      CHECK((uv.y >= -1e-4f && uv.y <= 1.0f + 1e-4f));
    }
    // Cap centers map to the radial center (0.5,0.5).
    CHECK((hasUv(m, 0.5f, 0.5f)));
  }
  // Cylinder side-only (caps off): every bottom-rim corner (y ~ -1) has t~0 and
  // every top-rim corner (y ~ +1) has t~1; s = j/slices wraps 0..1.
  {
    auto g = createX3DNode("Cylinder");
    setF(g, "radius", std::any(SFFloat(1.0f)));
    setF(g, "height", std::any(SFFloat(2.0f)));
    setF(g, "top", std::any(SFBool(false)));
    setF(g, "bottom", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    bool sawBottom = false, sawTop = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      const SFVec3f &p = m.positions[i];
      if (feq(p.y, -1.0f)) { CHECK((feq(m.texcoords[i].y, 0.0f))); sawBottom = true; }
      if (feq(p.y, 1.0f))  { CHECK((feq(m.texcoords[i].y, 1.0f))); sawTop = true; }
    }
    CHECK((sawBottom && sawTop));
    CHECK((hasUv(m, 0.0f, 0.0f))); // s wraps 0..1
    CHECK((hasUv(m, 1.0f, 1.0f)));
  }

  // ---- 4. Cone: side t=0 base..1 apex; bottom cap radial -------------------
  {
    auto g = createX3DNode("Cone");
    setF(g, "bottomRadius", std::any(SFFloat(1.0f)));
    setF(g, "height", std::any(SFFloat(2.0f)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((!m.texcoords.empty()));
    CHECK((m.texcoords.size() == m.positions.size()));
    for (const auto &uv : m.texcoords) {
      CHECK((uv.x >= -1e-4f && uv.x <= 1.0f + 1e-4f));
      CHECK((uv.y >= -1e-4f && uv.y <= 1.0f + 1e-4f));
    }
    // Apex is at +Y (y=+1 for height 2). Apex corners have t~1; base rim t~0.
    bool sawApex = false, sawBase = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      const SFVec3f &p = m.positions[i];
      if (feq(p.y, 1.0f)) { CHECK((feq(m.texcoords[i].y, 1.0f))); sawApex = true; }
      // base rim on the lateral surface: y=-1, radius ~ 1.
      const float r = std::sqrt(p.x * p.x + p.z * p.z);
      if (feq(p.y, -1.0f) && feq(r, 1.0f)) sawBase = true;
    }
    CHECK((sawApex && sawBase));
    // Bottom cap center maps to (0.5,0.5).
    CHECK((hasUv(m, 0.5f, 0.5f)));
  }

  // ---- 5. Caps off: texcoords stay parallel to positions -------------------
  {
    auto g = createX3DNode("Cylinder");
    setF(g, "top", std::any(SFBool(false)));
    setF(g, "bottom", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.texcoords.size() == m.positions.size()));
  }
  {
    auto g = createX3DNode("Cone");
    setF(g, "side", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.texcoords.size() == m.positions.size()));
  }

  return;
}
