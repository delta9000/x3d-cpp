// mesh_builder_t3_test.cpp — M2.5 T3 acceptance: MeshBuilder attribute
// resolution (Normal / Color / ColorRGBA / TextureCoordinate) + flat-normal
// generation + per-vertex/per-face expansion.
//
// Proofs:
//   1) Flat-normal correctness — with NO Normal node the builder fills `normals`
//      (one per corner) with the EXACT geometric normal of each triangle; a
//      triangle in the z=0 plane (CCW) has normal +Z.
//   2) Authored normalIndex honoring (per-vertex) — normalIndex remaps which
//      Normal.vector entry each corner gets, independent of coordIndex.
//   3) colorIndex honoring (per-vertex) — colorIndex selects the Color entry per
//      corner; resolved colors are parallel to positions.
//   4) Per-FACE expansion — colorPerVertex=false makes the whole face take one
//      color (indexed by face / colorIndex[face]); both triangles of a quad-face
//      get the same color.
//   5) ColorRGBA carried vs Color promoted — a Color (MFColor) node yields
//      alpha=1; a ColorRGBA (MFColorRGBA) node carries its authored alpha.
//   6) hasColors / hasNormals flags + texCoord resolution.
#include "GeoFrame.hpp"
#include "MeshBuilder.hpp"

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

static std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}
static void attachCoord(const std::shared_ptr<X3DNode> &g, std::vector<SFVec3f> pts) {
  setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(std::move(pts)))));
}
static void attachNormal(const std::shared_ptr<X3DNode> &g, std::vector<SFVec3f> v) {
  auto n = createX3DNode("Normal");
  setF(n, "vector", std::any(std::move(v)));
  setF(g, "normal", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static void attachColor(const std::shared_ptr<X3DNode> &g, std::vector<SFColor> c) {
  auto n = createX3DNode("Color");
  setF(n, "color", std::any(std::move(c)));
  setF(g, "color", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static void attachColorRGBA(const std::shared_ptr<X3DNode> &g, std::vector<SFColorRGBA> c) {
  auto n = createX3DNode("ColorRGBA");
  setF(n, "color", std::any(std::move(c)));
  setF(g, "color", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static void attachTexCoord(const std::shared_ptr<X3DNode> &g, std::vector<SFVec2f> p) {
  auto n = createX3DNode("TextureCoordinate");
  setF(n, "point", std::any(std::move(p)));
  setF(g, "texCoord", std::any(std::static_pointer_cast<X3DNode>(n)));
}

static bool vecEq(const SFVec3f &a, const SFVec3f &b) {
  return feq(a.x, b.x) && feq(a.y, b.y) && feq(a.z, b.z);
}
static bool rgbaEq(const SFColorRGBA &a, float r, float g, float b, float al) {
  return feq(a.r, r) && feq(a.g, g) && feq(a.b, b) && feq(a.a, al);
}

TEST_CASE("mesh_builder_t3_test") {
  const SFVec3f P0{0, 0, 0}, P1{1, 0, 0}, P2{1, 1, 0}, P3{0, 1, 0};

  // ---- 1. Flat-normal correctness: no Normal node -> +Z geometric normal -----
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2}); // CCW in z=0 plane.
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.indices.size() == 3));
    CHECK((m.hasNormals && !m.hasColors));
    CHECK((m.normals.size() == 3)); // one per corner.
    for (const auto &n : m.normals) CHECK((vecEq(n, SFVec3f{0, 0, 1})));
  }

  // ---- 1b. Flat normal of a clockwise triangle points -Z ---------------------
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P2, P1}); // reversed winding.
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    for (const auto &n : m.normals) CHECK((vecEq(n, SFVec3f{0, 0, -1})));
  }

  // ---- 2. normalIndex honoring (per-vertex) ----------------------------------
  // 3 distinct normals; normalIndex remaps which Normal.vector entry each corner
  // receives, independently of coordIndex. Uses IndexedFaceSet because ONLY the
  // *Indexed*FaceSet family carries the separate normalIndex/colorIndex/
  // texCoordIndex fields (the IndexedTriangle*/Quad* family share the single
  // `index` for coords AND attributes — proven in test 2b/3b below).
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1}));
    const SFVec3f NA{1, 0, 0}, NB{0, 1, 0}, NC{0, 0, 1};
    attachNormal(g, {NA, NB, NC});
    setF(g, "normalPerVertex", std::any(true));
    setF(g, "normalIndex", std::any(std::vector<int>{2, 0, 1, -1})); // corner0->NC.
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.normals.size() == 3));
    CHECK((vecEq(m.normals[0], NC)));
    CHECK((vecEq(m.normals[1], NA)));
    CHECK((vecEq(m.normals[2], NB)));
  }

  // ---- 2b. IndexedTriangleSet: no separate normalIndex -> `index` indexes -----
  // the Normal array per corner (the value from `index` doubles as the normal
  // index). The triangle's coords are P0,P1,P2; normals NA,NB,NC pick by coord.
  {
    auto g = createX3DNode("IndexedTriangleSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "index", std::any(std::vector<int>{2, 0, 1})); // coord order remapped.
    const SFVec3f NA{1, 0, 0}, NB{0, 1, 0}, NC{0, 0, 1};
    attachNormal(g, {NA, NB, NC});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.normals.size() == 3));
    CHECK((vecEq(m.normals[0], NC))); // coord 2 -> NC
    CHECK((vecEq(m.normals[1], NA))); // coord 0 -> NA
    CHECK((vecEq(m.normals[2], NB))); // coord 1 -> NB
  }

  // ---- 3. colorIndex honoring (per-vertex, IndexedFaceSet) -------------------
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1}));
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}, SFColor{0, 0, 1}});
    setF(g, "colorPerVertex", std::any(true));
    setF(g, "colorIndex", std::any(std::vector<int>{1, 2, 0, -1}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.hasColors));
    CHECK((m.colors.size() == 3));
    CHECK((rgbaEq(m.colors[0], 0, 1, 0, 1)));
    CHECK((rgbaEq(m.colors[1], 0, 0, 1, 1)));
    CHECK((rgbaEq(m.colors[2], 1, 0, 0, 1)));
  }

  // ---- 3b. colorIndex empty -> coordIndex doubles as the color index ---------
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "coordIndex", std::any(std::vector<int>{2, 0, 1, -1})); // remapped.
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}, SFColor{0, 0, 1}});
    // no colorIndex -> color[coordIndex] : corner0 uses coord 2 -> blue, etc.
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((rgbaEq(m.colors[0], 0, 0, 1, 1))); // coord 2
    CHECK((rgbaEq(m.colors[1], 1, 0, 0, 1))); // coord 0
    CHECK((rgbaEq(m.colors[2], 0, 1, 0, 1))); // coord 1
  }

  // ---- 4. Per-FACE expansion: colorPerVertex=false ---------------------------
  // An IndexedFaceSet quad-face -> 2 triangles (6 corners); with
  // colorPerVertex=false the WHOLE face takes a single color (face 0). Both
  // triangles' six corners must carry that one color.
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
    attachColor(g, {SFColor{0.2f, 0.4f, 0.6f}, SFColor{1, 1, 1}});
    setF(g, "colorPerVertex", std::any(false)); // per-FACE.
    // no colorIndex -> face 0 uses color[0].
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.colors.size() == 6)); // quad fan -> 2 tris -> 6 corners.
    for (const auto &c : m.colors) CHECK((rgbaEq(c, 0.2f, 0.4f, 0.6f, 1)));
  }

  // ---- 4b. Per-FACE with colorIndex selecting a face color -------------------
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2, P3});
    // two triangular faces.
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1, 0, 2, 3, -1}));
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}, SFColor{0, 0, 1}});
    setF(g, "colorPerVertex", std::any(false));
    setF(g, "colorIndex", std::any(std::vector<int>{2, 0})); // face0->blue, face1->red
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.colors.size() == 6)); // 2 faces, 1 tri each -> 6 corners.
    // face 0 (corners 0..2) -> blue
    CHECK((rgbaEq(m.colors[0], 0, 0, 1, 1) && rgbaEq(m.colors[2], 0, 0, 1, 1)));
    // face 1 (corners 3..5) -> red
    CHECK((rgbaEq(m.colors[3], 1, 0, 0, 1) && rgbaEq(m.colors[5], 1, 0, 0, 1)));
  }

  // ---- 5. ColorRGBA carried vs Color promoted --------------------------------
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2});
    attachColorRGBA(g, {SFColorRGBA{1, 0, 0, 0.25f}, SFColorRGBA{0, 1, 0, 0.5f},
                        SFColorRGBA{0, 0, 1, 0.75f}});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.hasColors));
    CHECK((rgbaEq(m.colors[0], 1, 0, 0, 0.25f))); // authored alpha PRESERVED.
    CHECK((rgbaEq(m.colors[1], 0, 1, 0, 0.5f)));
    CHECK((rgbaEq(m.colors[2], 0, 0, 1, 0.75f)));
  }
  {
    // Same geometry, Color (no alpha) -> alpha promoted to 1.
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2});
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}, SFColor{0, 0, 1}});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((rgbaEq(m.colors[0], 1, 0, 0, 1)));
    CHECK((rgbaEq(m.colors[1], 0, 1, 0, 1)));
    CHECK((rgbaEq(m.colors[2], 0, 0, 1, 1)));
  }

  // ---- 6. TextureCoordinate resolution + authored Normal honored -------------
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2});
    attachTexCoord(g, {SFVec2f{0, 0}, SFVec2f{1, 0}, SFVec2f{1, 1}});
    // authored normals all +Y (NOT the geometric +Z) so we prove they're honored.
    attachNormal(g, {SFVec3f{0, 1, 0}, SFVec3f{0, 1, 0}, SFVec3f{0, 1, 0}});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == 3));
    CHECK((feq(m.texcoords[0].x, 0) && feq(m.texcoords[0].y, 0)));
    CHECK((feq(m.texcoords[2].x, 1) && feq(m.texcoords[2].y, 1)));
    // authored normal +Y honored, not flat +Z.
    for (const auto &n : m.normals) CHECK((vecEq(n, SFVec3f{0, 1, 0})));
  }

  // ---- 7. ccw / solid carried from reflection --------------------------------
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "ccw", std::any(false));
    setF(g, "solid", std::any(false));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.ccw == false && m.solid == false));
  }
  {
    auto g = createX3DNode("TriangleSet"); // defaults true.
    attachCoord(g, {P0, P1, P2});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.ccw == true && m.solid == true));
  }

  return;
}

TEST_CASE("custom vertex attributes follow expanded coordinate vertices") {
  auto makeAttributes = [](const std::shared_ptr<X3DNode> &g) {
    auto f = createX3DNode("FloatVertexAttribute");
    setF(f, "name", std::any(std::string("weights")));
    setF(f, "numComponents", std::any(SFInt32{3}));
    setF(f, "value", std::any(std::vector<float>{1,2,3, 4,5,6, 7,8,9, 10,11,12}));
    auto m = createX3DNode("Matrix3VertexAttribute");
    setF(m, "name", std::any(std::string("basis")));
    MFMatrix3f matrices(4);
    for (int v = 0; v < 4; ++v)
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) matrices[v].matrix[r][c] = float(v * 9 + r * 3 + c);
    setF(m, "value", std::any(std::move(matrices)));
    setF(g, "attrib", std::any(MFNode{f, m}));
  };
  for (const char *type : {"IndexedFaceSet", "TriangleSet"}) {
    auto g = createX3DNode(type);
    attachCoord(g, {{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
    if (std::string(type) == "IndexedFaceSet") setF(g, "coordIndex", std::any(std::vector<int>{2,0,3,1,-1}));
    makeAttributes(g);
    const MeshData mesh = buildLocalMesh(g.get(), geo::builtinProjection());
    REQUIRE(mesh.vertexAttributes.size() == 2);
    CHECK(mesh.vertexAttributes[0].name == "weights");
    CHECK(mesh.vertexAttributes[0].components == 3);
    CHECK(mesh.vertexAttributes[1].name == "basis");
    CHECK(mesh.vertexAttributes[1].components == 9);
    REQUIRE(mesh.vertexAttributes[0].values.size() == mesh.positions.size() * 3);
    REQUIRE(mesh.vertexAttributes[1].values.size() == mesh.positions.size() * 9);
    for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
      const int source = mesh.positions[i].y == 0.0f ? int(mesh.positions[i].x)
                                                     : 3 - int(mesh.positions[i].x);
      for (int c = 0; c < 3; ++c) CHECK(mesh.vertexAttributes[0].values[i * 3 + c] == float(source * 3 + c + 1));
      for (int c = 0; c < 9; ++c) CHECK(mesh.vertexAttributes[1].values[i * 9 + c] == float(source * 9 + c));
    }
  }
}
