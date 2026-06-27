// mesh_builder_t4_test.cpp — M2.5 T4 acceptance: MeshBuilder parametric
// tessellation of the analytic primitives Box / Sphere / Cone / Cylinder.
//
// Proofs:
//   1) Box — 6 faces * 2 triangles = 12 triangles (36 corners). Every face
//      normal points OUTWARD (sign(n . centroidDir) > 0 for the +X/-X/... slabs).
//   2) Sphere — default density 16 (rings) x 16 (segments). Triangle count
//      matches a UV-sphere with poles (caps fans + body quads). Every vertex
//      normal is the OUTWARD radial direction (n . (p-center) > 0).
//   3) Cone — 24 radial slices. Side normals point outward+up; bottom-cap
//      normals point straight down. No top cap.
//   4) Cylinder — 24 radial slices, side + top + bottom caps. Side normals
//      radial-outward; top cap +Y; bottom cap -Y. Toggling side/top/bottom
//      drops exactly those triangles.
//   5) Density knobs — MeshBuildOptions.sphereRings/sphereSegments/radialSlices
//      scale the triangle count.
//   6) CCW outward winding — for every emitted triangle the geometric face
//      normal (CCW cross product) agrees in sign with the stored per-corner
//      normal, so the primitives compose under the SAME cull rule as meshes.
#include "MeshBuilder.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

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
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

static float dot(const SFVec3f &a, const SFVec3f &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
static SFVec3f sub(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.x - b.x, a.y - b.y, a.z - b.z};
}
static SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
}

// Every emitted triangle: the geometric (CCW) face normal must agree in sign
// with the average of its three stored per-corner normals (outward winding).
static void assertOutwardWinding(const MeshData &m) {
  CHECK((m.hasNormals));
  CHECK((m.normals.size() == m.positions.size()));
  CHECK((m.indices.size() % 3 == 0));
  for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
    const SFVec3f &a = m.positions[m.indices[t]];
    const SFVec3f &b = m.positions[m.indices[t + 1]];
    const SFVec3f &c = m.positions[m.indices[t + 2]];
    SFVec3f g = cross(sub(b, a), sub(c, a)); // CCW geometric normal
    SFVec3f avg{(m.normals[m.indices[t]].x + m.normals[m.indices[t + 1]].x +
                 m.normals[m.indices[t + 2]].x) /
                    3.0f,
                (m.normals[m.indices[t]].y + m.normals[m.indices[t + 1]].y +
                 m.normals[m.indices[t + 2]].y) /
                    3.0f,
                (m.normals[m.indices[t]].z + m.normals[m.indices[t + 1]].z +
                 m.normals[m.indices[t + 2]].z) /
                    3.0f};
    CHECK((dot(g, avg) > 0.0f)); // CCW winding matches outward normal
  }
}

TEST_CASE("mesh_builder_t4_test") {
  const SFVec3f center{0, 0, 0};

  // ---- 1. Box: 12 triangles, all normals outward ------------------------
  {
    auto g = createX3DNode("Box");
    setF(g, "size", std::any(SFVec3f{2, 2, 2}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 36)); // 6 faces * 2 tris * 3
    CHECK((m.positions.size() == 36));
    CHECK((m.hasNormals));
    // Outward: for a box centered at origin each corner normal points away
    // from center, i.e. n . p > 0.
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      CHECK((dot(m.normals[i], sub(m.positions[i], center)) > 0.0f));
    assertOutwardWinding(m);
  }

  // ---- 2. Sphere: default 16x16 UV sphere, radial outward normals -------
  {
    auto g = createX3DNode("Sphere");
    setF(g, "radius", std::any(SFFloat(1.0f)));
    MeshData m = buildLocalMesh(g.get());
    // UV sphere with R rings, S segments: top+bottom cap rows are triangles,
    // the (R-2) middle rows are quads (2 tris). Total tris = S*2*(R-1).
    // R=16, S=16 -> 16*2*15 = 480 triangles.
    CHECK((m.indices.size() == 480 * 3));
    CHECK((m.hasNormals));
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      // radial outward: normal parallel to position vector, same direction.
      CHECK((dot(m.normals[i], sub(m.positions[i], center)) > 0.0f));
      // on the unit sphere |p| ~ radius
      float len = std::sqrt(dot(m.positions[i], m.positions[i]));
      CHECK((feq(len, 1.0f)));
    }
    assertOutwardWinding(m);
  }

  // ---- 3. Cone: 24 radial slices, side + bottom cap ---------------------
  {
    auto g = createX3DNode("Cone");
    setF(g, "bottomRadius", std::any(SFFloat(1.0f)));
    setF(g, "height", std::any(SFFloat(2.0f)));
    MeshData m = buildLocalMesh(g.get());
    // side: 24 tris ; bottom cap fan: 24 tris -> 48 tris.
    CHECK((m.indices.size() == 48 * 3));
    CHECK((m.hasNormals));
    // Bottom cap normals point straight down (-Y); apex/side normals have +X/+Z
    // radial component. Just assert at least one of each and outward winding.
    bool sawDown = false, sawRadial = false;
    for (std::size_t i = 0; i < m.normals.size(); ++i) {
      if (feq(m.normals[i].y, -1.0f) && feq(m.normals[i].x, 0.0f) &&
          feq(m.normals[i].z, 0.0f))
        sawDown = true;
      if (m.normals[i].x * m.normals[i].x + m.normals[i].z * m.normals[i].z >
          0.01f)
        sawRadial = true;
    }
    CHECK((sawDown && sawRadial));
    assertOutwardWinding(m);
  }

  // ---- 3b. Cone with side off -> only bottom cap ------------------------
  {
    auto g = createX3DNode("Cone");
    setF(g, "side", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 24 * 3)); // bottom cap only
  }
  // ---- 3c. Cone with bottom off -> only side ----------------------------
  {
    auto g = createX3DNode("Cone");
    setF(g, "bottom", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 24 * 3)); // side only
  }

  // ---- 4. Cylinder: 24 slices, side + top + bottom ----------------------
  {
    auto g = createX3DNode("Cylinder");
    setF(g, "radius", std::any(SFFloat(1.0f)));
    setF(g, "height", std::any(SFFloat(2.0f)));
    MeshData m = buildLocalMesh(g.get());
    // side: 24*2 quads tris = 48 ; top fan 24 ; bottom fan 24 -> 96 tris.
    CHECK((m.indices.size() == 96 * 3));
    CHECK((m.hasNormals));
    bool sawUp = false, sawDown = false, sawRadial = false;
    for (std::size_t i = 0; i < m.normals.size(); ++i) {
      if (feq(m.normals[i].y, 1.0f)) sawUp = true;
      if (feq(m.normals[i].y, -1.0f)) sawDown = true;
      if (m.normals[i].x * m.normals[i].x + m.normals[i].z * m.normals[i].z >
          0.01f)
        sawRadial = true;
    }
    CHECK((sawUp && sawDown && sawRadial));
    assertOutwardWinding(m);
  }
  // ---- 4b. Cylinder caps toggled off -> side only -----------------------
  {
    auto g = createX3DNode("Cylinder");
    setF(g, "top", std::any(SFBool(false)));
    setF(g, "bottom", std::any(SFBool(false)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 48 * 3)); // side quads only
  }

  // ---- 5. Density knobs scale the triangle counts -----------------------
  {
    auto g = createX3DNode("Sphere");
    MeshBuildOptions opt;
    opt.sphereRings = 8;
    opt.sphereSegments = 8;
    MeshData m = buildLocalMesh(g.get(), opt);
    // S*2*(R-1) = 8*2*7 = 112 triangles.
    CHECK((m.indices.size() == 112 * 3));
  }
  {
    auto g = createX3DNode("Cylinder");
    MeshBuildOptions opt;
    opt.radialSlices = 8;
    MeshData m = buildLocalMesh(g.get(), opt);
    // side 8*2 + top 8 + bottom 8 = 32 tris.
    CHECK((m.indices.size() == 32 * 3));
  }

  return;
}
