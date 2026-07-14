// mesh_builder_txc1_test.cpp — TXC-1: spec-exact seam placement for the
// analytic primitives Sphere / Cone / Cylinder.
//
// ISO/IEC 19775-1:
//   §13.3.7 Sphere: "the texture covers the entire surface, wrapping
//     counterclockwise from the back of the sphere (i.e., longitudinal arc
//     intersecting the -Z-axis) when viewed from the top of the sphere. The
//     texture has a seam at the back where the X=0 plane intersects the sphere
//     and Z values are negative."
//     => S=0 at the -Z seam, S increases CCW-from-above; T=0 at -Y pole, T=1
//        at +Y pole.
//
//   §13.3.2 Cone: "the texture wraps counterclockwise (from above) starting at
//     the back of the cone. The texture has a vertical seam at the back in the
//     X=0 plane, from the apex (0, height/2, 0) to the point
//     (0, -height/2, -bottomRadius)."
//     "The bottom cap texture appears right side up when the top of the cone is
//     rotated towards the -Z-axis."
//     => Side seam at -Z (X=0, Z<0); S=0 there, increasing CCW; T=0 at base,
//        T=1 at apex. Bottom cap: -Z rim → T=1, +Z rim → T=0.
//
//   §13.3.3 Cylinder: "On the sides, the texture wraps counterclockwise (from
//     above) starting at the back of the cylinder. The texture has a vertical
//     seam at the back, intersecting the X=0 plane."
//     "The top texture appears right side up when the top of the cylinder is
//     tilted toward the +Z-axis, and the bottom texture appears right side up
//     when the top of the cylinder is tilted toward the -Z-axis."
//     => Side seam at -Z; S=0 there, increasing CCW. Top cap: +Z rim → T=1,
//        -Z rim → T=0. Bottom cap: -Z rim → T=1, +Z rim → T=0.
//
//   §13.3.1 Box: (not in scope for seam — verified separately in TC4 and is
//     already spec-exact; no change needed).
//
// Tests fail against the pre-TXC-1 code (seam placed at +Z) and pass after.
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

static constexpr float EPS = 1e-4f;
static const float PI = 3.14159265358979323846f;

static bool feq(float a, float b) { return std::fabs(a - b) < EPS; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm,
                 std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

// Return true iff some corner whose 3-D position has Z < -threshold and
// |X| < threshold carries s == 0.
// (The seam is the -Z meridian in the X=0 plane; any tessellation point on
// that arc should carry s=0.)
static bool seamAtNegZ(const MeshData &m, float threshold = 0.05f) {
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    if (p.z < -threshold && std::fabs(p.x) < threshold)
      if (feq(m.texcoords[i].x, 0.0f)) return true;
  }
  return false;
}

// Return true iff some corner on the +Z side (z > threshold, |x| < threshold)
// carries s == 0. This would indicate the WRONG seam placement.
static bool seamAtPosZ(const MeshData &m, float threshold = 0.05f) {
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    if (p.z > threshold && std::fabs(p.x) < threshold)
      if (feq(m.texcoords[i].x, 0.0f)) return true;
  }
  return false;
}

// ---- 1. Sphere: seam at -Z, S=0; T=0 at -Y pole, T=1 at +Y pole ----------
static void testSphereSeam() {
  auto g = createX3DNode("Sphere");
  setF(g, "radius", std::any(SFFloat(1.0f)));
  MeshData m = buildLocalMesh(g.get());
  CHECK((!m.texcoords.empty()));
  CHECK((m.texcoords.size() == m.positions.size()));

  // Seam is at -Z: there must be a vertex with z<0, x~0 that carries s=0.
  CHECK((seamAtNegZ(m)));
  // The +Z meridian must NOT carry s=0 (that would be the old wrong seam).
  CHECK((!seamAtPosZ(m)));

  // T=0 at the -Y pole (bottom), T=1 at +Y pole (top).
  float minT = 1e9f, maxT = -1e9f;
  SFVec3f atMinT{}, atMaxT{};
  for (std::size_t i = 0; i < m.texcoords.size(); ++i) {
    const float t = m.texcoords[i].y;
    if (t < minT) { minT = t; atMinT = m.positions[i]; }
    if (t > maxT) { maxT = t; atMaxT = m.positions[i]; }
  }
  CHECK((feq(minT, 0.0f)));
  CHECK((feq(maxT, 1.0f)));
  CHECK((atMinT.y < 0.0f)); // minimum-T corner is near the -Y (south) pole.
  CHECK((atMaxT.y > 0.0f)); // maximum-T corner is near the +Y (north) pole.

  // S wraps 0..1 counterclockwise from above: from -Z, moving toward -X first.
  // At the -Z seam (s=0), a step toward -X should give a small positive s.
  // Check monotonicity: for two vertices at the same latitude, the one further
  // CCW from -Z should have a larger (or equal) s.
  // (We just verify s spans [0,1] and the direction is CCW: for any two corners
  // on the same ring, the one with more negative x and roughly matching z should
  // have higher s than the seam side.)
  float sMin = 1e9f, sMax = -1e9f;
  for (const auto &uv : m.texcoords) {
    sMin = std::min(sMin, uv.x);
    sMax = std::max(sMax, uv.x);
  }
  CHECK((feq(sMin, 0.0f)));
  CHECK((feq(sMax, 1.0f)));

  // Verify CCW direction: from -Z seam (s=0), moving to -X (theta ~= pi/2
  // CCW from above) should give s ~= 0.25. Find any vertex with x ~ -0.7,
  // z ~ 0, radius ~1 and assert s is in (0.1, 0.4).
  bool sawCCW = false;
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    const float r = std::sqrt(p.x * p.x + p.z * p.z);
    if (r < 0.5f) continue;   // skip near poles
    if (p.x < -0.3f && std::fabs(p.z) < 0.5f) {
      // CCW from -Z goes toward -X; s should be around 0.25.
      if (m.texcoords[i].x > 0.1f && m.texcoords[i].x < 0.4f) {
        sawCCW = true;
        break;
      }
    }
  }
  CHECK((sawCCW));
}

// ---- 2. Cone: seam at -Z; bottom cap -Z → T=1, +Z → T=0 ------------------
static void testConeSeam() {
  auto g = createX3DNode("Cone");
  setF(g, "bottomRadius", std::any(SFFloat(1.0f)));
  setF(g, "height", std::any(SFFloat(2.0f)));
  MeshData m = buildLocalMesh(g.get());
  CHECK((!m.texcoords.empty()));
  CHECK((m.texcoords.size() == m.positions.size()));

  // Side seam at -Z (X=0, Z<0 rim point), s=0 there.
  CHECK((seamAtNegZ(m, 0.1f)));
  CHECK((!seamAtPosZ(m, 0.1f)));

  // Side: T=0 at base (y=-1), T=1 at apex (y=+1).
  bool sawApex = false;
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    if (feq(p.y, 1.0f)) { // apex (height/2 = 1)
      CHECK((feq(m.texcoords[i].y, 1.0f)));
      sawApex = true;
    }
  }
  CHECK((sawApex));

  // Bottom cap: cap centre → (0.5, 0.5).
  // At -Z rim of the cap: z ~ -1, x ~ 0, y = -1 → T should be ~1.0.
  // At +Z rim of the cap: z ~ +1, x ~ 0, y = -1 → T should be ~0.0.
  bool sawCapNegZRim = false, sawCapPosZRim = false;
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    if (!feq(p.y, -1.0f)) continue; // not on the bottom cap plane
    if (m.normals[i].y > -0.5f) continue; // cap corners only (downward normal),
                                          // not the side base rim (slanted normal)
    const float r = std::sqrt(p.x * p.x + p.z * p.z);
    if (r < 0.05f) continue; // skip center
    // -Z rim point on cap: x~0, z~-1.
    if (std::fabs(p.x) < 0.1f && p.z < -0.8f) {
      // spec: right-side-up when top rotated toward -Z => -Z rim → T=1.
      CHECK((m.texcoords[i].y > 0.8f));
      sawCapNegZRim = true;
    }
    // +Z rim point on cap: x~0, z~+1.
    if (std::fabs(p.x) < 0.1f && p.z > 0.8f) {
      // +Z rim → T=0.
      CHECK((m.texcoords[i].y < 0.2f));
      sawCapPosZRim = true;
    }
  }
  CHECK((sawCapNegZRim));
  CHECK((sawCapPosZRim));
}

// ---- 3. Cylinder: side seam at -Z; top +Z→T=1; bottom -Z→T=1 -------------
static void testCylinderSeam() {
  auto g = createX3DNode("Cylinder");
  setF(g, "radius", std::any(SFFloat(1.0f)));
  setF(g, "height", std::any(SFFloat(2.0f)));
  MeshData m = buildLocalMesh(g.get());
  CHECK((!m.texcoords.empty()));
  CHECK((m.texcoords.size() == m.positions.size()));

  // Side seam at -Z (X=0, Z<0), s=0.
  CHECK((seamAtNegZ(m, 0.1f)));
  CHECK((!seamAtPosZ(m, 0.1f)));

  // Top cap (+Y face): +Z rim → T=1, -Z rim → T=0.
  // Bottom cap (-Y face): -Z rim → T=1, +Z rim → T=0.
  bool sawTopPosZ  = false, sawTopNegZ  = false;
  bool sawBotNegZ  = false, sawBotPosZ  = false;
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    const float r = std::sqrt(p.x * p.x + p.z * p.z);
    if (r < 0.05f) continue; // cap centre
    // Cap corners carry the axial cap normal (+Y top / -Y bottom); the side rim
    // corners at the same y carry a radial normal (n.y~0) and are excluded.
    const bool isTop = feq(p.y, 1.0f) && m.normals[i].y > 0.5f;
    const bool isBot = feq(p.y, -1.0f) && m.normals[i].y < -0.5f;
    if (!isTop && !isBot) continue;

    // -Z rim: x~0, z~-1.
    if (std::fabs(p.x) < 0.1f && p.z < -0.8f) {
      if (isTop) { // top cap, -Z rim → T=0.
        CHECK((m.texcoords[i].y < 0.2f));
        sawTopNegZ = true;
      }
      if (isBot) { // bottom cap, -Z rim → T=1.
        CHECK((m.texcoords[i].y > 0.8f));
        sawBotNegZ = true;
      }
    }
    // +Z rim: x~0, z~+1.
    if (std::fabs(p.x) < 0.1f && p.z > 0.8f) {
      if (isTop) { // top cap, +Z rim → T=1.
        CHECK((m.texcoords[i].y > 0.8f));
        sawTopPosZ = true;
      }
      if (isBot) { // bottom cap, +Z rim → T=0.
        CHECK((m.texcoords[i].y < 0.2f));
        sawBotPosZ = true;
      }
    }
  }
  CHECK((sawTopPosZ));
  CHECK((sawTopNegZ));
  CHECK((sawBotNegZ));
  CHECK((sawBotPosZ));

  // Side: T=0 at bottom rim (y=-1), T=1 at top rim (y=+1).
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    const SFVec3f &p = m.positions[i];
    // Lateral surface corners live at radius = 1 from Y axis (r ~ 1), not on
    // the cap planes (caps have r at any value but the seam only applies when
    // processing the side part). Check side corners by radius ~1 and y != 0
    // (either +1 or -1).
    const float r = std::sqrt(p.x * p.x + p.z * p.z);
    if (!feq(r, 1.0f)) continue;          // not on the lateral surface
    if (std::fabs(m.normals[i].y) > 0.5f) continue; // exclude cap rim corners
    if (feq(p.y, -1.0f)) CHECK((feq(m.texcoords[i].y, 0.0f)));
    if (feq(p.y,  1.0f)) CHECK((feq(m.texcoords[i].y, 1.0f)));
  }
}

TEST_CASE("mesh_builder_txc1_test") {
  testSphereSeam();
  testConeSeam();
  testCylinderSeam();
  return;
}
