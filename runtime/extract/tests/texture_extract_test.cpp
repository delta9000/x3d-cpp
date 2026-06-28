// texture_extract_test.cpp — T-TEX v1 end-to-end texture extraction.
//
// Covers the new TextureExtract.hpp seam that closes textures end-to-end:
//   1. AUTHORED TextureCoordinate is extracted onto MeshData.texcoords
//      (texCoordIndex / coordIndex fallback per §13.3.6) — confirmed via the
//      existing MeshBuilder path (authored coords WIN over default §13 coords).
//      MultiTextureCoordinate and TextureCoordinate3D/4D are normalized onto the
//      current MeshData.texcoords (s,t) seam by taking the first usable channel
//      and first two coordinates.
//   2. TextureTransform (center/rotation/scale/translation, §18.4.10) is baked
//      into the mesh texcoords by applyTextureTransformToMesh().
//   3. textureTransformParamsOf(appearance) reads the authored TextureTransform
//      node from Appearance.textureTransform.
//   4. ExtendedSampler / TextureProperties boundary+filter modes are surfaced on
//      TextureRef::extSampler (§18.4.9), and repeatS/T derive a BoundaryMode when
//      no TextureProperties is present (§18.2.3).
//   5. TextureCoordinateGenerator on a geometry's texCoord slot surfaces a
//      TexCoordGenDesc descriptor (mode + parameter, §18.4.8).
//   6. resolveTextureRefs() threads a consumer TextureResolver onto each
//      TextureRef::resolvedPixels (Url goes through the resolver; Inline does NOT).
#include "MaterialSystem.hpp"
#include "MeshBuilder.hpp"
#include "TextureExtract.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static constexpr float EPS = 1e-4f;
static bool feq(float a, float b) { return std::fabs(a - b) < EPS; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// Set an SFEnum field by its X3D token string (via the reflection setEnumString).
static void setEnum(const std::shared_ptr<X3DNode> &n, const char *nm,
                    const std::string &token) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.setEnumString) { f.setEnumString(*n, token); return; }
}

// --- 1. AUTHORED TextureCoordinate wins over default §13 coords -------------
static void testAuthoredTexCoord() {
  // A single triangle TriangleSet with an authored TextureCoordinate. The
  // authored point[] must surface verbatim (positional order for non-indexed).
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{
                           {0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tc = createX3DNode("TextureCoordinate");
  setF(tc, "point", std::any(std::vector<SFVec2f>{
                        {0.1f, 0.2f}, {0.7f, 0.3f}, {0.4f, 0.9f}}));
  auto g = createX3DNode("TriangleSet");
  setF(g, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(g, "texCoord", std::any(std::shared_ptr<X3DNode>(tc)));

  MeshData m = buildLocalMesh(g.get());
  CHECK((m.positions.size() == 3));
  CHECK((m.texcoords.size() == 3));
  CHECK((feq(m.texcoords[0].x, 0.1f) && feq(m.texcoords[0].y, 0.2f)));
  CHECK((feq(m.texcoords[1].x, 0.7f) && feq(m.texcoords[1].y, 0.3f)));
  CHECK((feq(m.texcoords[2].x, 0.4f) && feq(m.texcoords[2].y, 0.9f)));
}

static void testMultiTextureCoordinateFirstUsableChannel() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{
                           {0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto empty = createX3DNode("TextureCoordinate");
  auto tc = createX3DNode("TextureCoordinate");
  setF(tc, "point", std::any(std::vector<SFVec2f>{
                        {0.2f, 0.1f}, {0.8f, 0.1f}, {0.3f, 0.7f}}));
  auto mtc = createX3DNode("MultiTextureCoordinate");
  setF(mtc, "texCoord", std::any(MFNode{empty, tc}));
  auto g = createX3DNode("TriangleSet");
  setF(g, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(g, "texCoord", std::any(std::shared_ptr<X3DNode>(mtc)));

  MeshData m = buildLocalMesh(g.get());
  CHECK((m.texcoords.size() == 3));
  CHECK((m.texcoordSets.size() == 2));
  CHECK((feq(m.texcoords[0].x, 0.2f) && feq(m.texcoords[0].y, 0.1f)));
  CHECK((feq(m.texcoords[1].x, 0.8f) && feq(m.texcoords[1].y, 0.1f)));
  CHECK((feq(m.texcoords[2].x, 0.3f) && feq(m.texcoords[2].y, 0.7f)));
  CHECK((m.texcoordSets[0].size() == 3));
  CHECK((feq(m.texcoordSets[0][0].x, 0.0f) &&
         feq(m.texcoordSets[0][0].y, 0.0f)));
  CHECK((m.texcoordSets[1].size() == 3));
  CHECK((feq(m.texcoordSets[1][2].x, 0.3f) &&
         feq(m.texcoordSets[1][2].y, 0.7f)));
}

static void testTextureCoordinate3DAnd4DProjectToST() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{
                           {0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tc3 = createX3DNode("TextureCoordinate3D");
  setF(tc3, "point", std::any(std::vector<SFVec3f>{
                         {0.1f, 0.2f, 0.3f},
                         {0.4f, 0.5f, 0.6f},
                         {0.7f, 0.8f, 0.9f}}));
  auto g3 = createX3DNode("TriangleSet");
  setF(g3, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(g3, "texCoord", std::any(std::shared_ptr<X3DNode>(tc3)));

  MeshData m3 = buildLocalMesh(g3.get());
  CHECK((m3.texcoords.size() == 3));
  CHECK((feq(m3.texcoords[0].x, 0.1f) && feq(m3.texcoords[0].y, 0.2f)));
  CHECK((feq(m3.texcoords[1].x, 0.4f) && feq(m3.texcoords[1].y, 0.5f)));
  CHECK((feq(m3.texcoords[2].x, 0.7f) && feq(m3.texcoords[2].y, 0.8f)));

  auto tc4 = createX3DNode("TextureCoordinate4D");
  setF(tc4, "point", std::any(std::vector<SFVec4f>{
                         {0.15f, 0.25f, 0.35f, 1.0f},
                         {0.45f, 0.55f, 0.65f, 1.0f},
                         {0.75f, 0.85f, 0.95f, 1.0f}}));
  auto g4 = createX3DNode("TriangleSet");
  setF(g4, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(g4, "texCoord", std::any(std::shared_ptr<X3DNode>(tc4)));

  MeshData m4 = buildLocalMesh(g4.get());
  CHECK((m4.texcoords.size() == 3));
  CHECK((feq(m4.texcoords[0].x, 0.15f) && feq(m4.texcoords[0].y, 0.25f)));
  CHECK((feq(m4.texcoords[1].x, 0.45f) && feq(m4.texcoords[1].y, 0.55f)));
  CHECK((feq(m4.texcoords[2].x, 0.75f) && feq(m4.texcoords[2].y, 0.85f)));
}

// --- 2. applyTextureTransformToMesh bakes the transform into texcoords ------
static void testApplyTransformToMesh() {
  MeshData m;
  m.texcoords = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};

  // Pure translation (0.5, 0.25): each (s,t) shifts by (+0.5,+0.25).
  TextureTransform2DParams p;
  p.translationS = 0.5f;
  p.translationT = 0.25f;
  applyTextureTransformToMesh(m, p);
  CHECK((feq(m.texcoords[0].x, 0.5f) && feq(m.texcoords[0].y, 0.25f)));
  CHECK((feq(m.texcoords[1].x, 1.5f) && feq(m.texcoords[1].y, 0.25f)));
  CHECK((feq(m.texcoords[2].x, 0.5f) && feq(m.texcoords[2].y, 1.25f)));

  // Identity transform must be a no-op (cheap early-exit; bytes unchanged).
  MeshData m2;
  m2.texcoords = {{0.3f, 0.7f}};
  applyTextureTransformToMesh(m2, TextureTransform2DParams{});
  CHECK((feq(m2.texcoords[0].x, 0.3f) && feq(m2.texcoords[0].y, 0.7f)));

  // Scale (2,2) about default center 0: doubles each coordinate.
  MeshData m3;
  m3.texcoords = {{0.25f, 0.5f}};
  TextureTransform2DParams sp;
  sp.scaleS = 2.0f;
  sp.scaleT = 2.0f;
  applyTextureTransformToMesh(m3, sp);
  CHECK((feq(m3.texcoords[0].x, 0.5f) && feq(m3.texcoords[0].y, 1.0f)));
}

// --- 3. textureTransformParamsOf reads Appearance.textureTransform ---------
static void testTextureTransformParamsOf() {
  // No appearance / no textureTransform => identity.
  CHECK((isIdentityTextureTransform(textureTransformParamsOf(nullptr))));
  {
    auto app = createX3DNode("Appearance");
    CHECK((isIdentityTextureTransform(textureTransformParamsOf(app.get()))));
  }
  // Authored TextureTransform => fields surfaced.
  {
    auto tt = createX3DNode("TextureTransform");
    setF(tt, "center", std::any(SFVec2f{0.5f, 0.5f}));
    setF(tt, "rotation", std::any(1.0f));
    setF(tt, "scale", std::any(SFVec2f{2.0f, 3.0f}));
    setF(tt, "translation", std::any(SFVec2f{0.1f, 0.2f}));
    auto app = createX3DNode("Appearance");
    setF(app, "textureTransform", std::any(std::shared_ptr<X3DNode>(tt)));

    TextureTransform2DParams p = textureTransformParamsOf(app.get());
    CHECK((!isIdentityTextureTransform(p)));
    CHECK((feq(p.centerS, 0.5f) && feq(p.centerT, 0.5f)));
    CHECK((feq(p.rotation, 1.0f)));
    CHECK((feq(p.scaleS, 2.0f) && feq(p.scaleT, 3.0f)));
    CHECK((feq(p.translationS, 0.1f) && feq(p.translationT, 0.2f)));
  }
  // MultiTextureTransform => first usable 2D child surfaced on the current
  // single-texture transform seam.
  {
    auto tt = createX3DNode("TextureTransform");
    setF(tt, "translation", std::any(SFVec2f{0.25f, 0.5f}));
    auto tt2 = createX3DNode("TextureTransform");
    setF(tt2, "scale", std::any(SFVec2f{2.0f, 4.0f}));
    auto mtt = createX3DNode("MultiTextureTransform");
    setF(mtt, "textureTransform", std::any(MFNode{tt, tt2}));
    auto app = createX3DNode("Appearance");
    setF(app, "textureTransform", std::any(std::shared_ptr<X3DNode>(mtt)));

    TextureTransform2DParams p = textureTransformParamsOf(app.get());
    CHECK((feq(p.translationS, 0.25f) && feq(p.translationT, 0.5f)));
    auto params = textureTransformParamsListOf(app.get());
    CHECK((params.size() == 2));
    CHECK((feq(params[0].translationS, 0.25f) &&
           feq(params[0].translationT, 0.5f)));
    CHECK((feq(params[1].scaleS, 2.0f) && feq(params[1].scaleT, 4.0f)));

    MeshData mesh;
    mesh.texcoordSets = {{{0.0f, 0.0f}}, {{0.5f, 0.25f}}};
    mesh.texcoords = mesh.texcoordSets[0];
    applyTextureTransformsToMesh(mesh, params);
    CHECK((feq(mesh.texcoordSets[0][0].x, 0.25f) &&
           feq(mesh.texcoordSets[0][0].y, 0.5f)));
    CHECK((feq(mesh.texcoordSets[1][0].x, 1.0f) &&
           feq(mesh.texcoordSets[1][0].y, 1.0f)));
    CHECK((feq(mesh.texcoords[0].x, 0.25f) &&
           feq(mesh.texcoords[0].y, 0.5f)));
  }
  // TextureTransformMatrix3D => 4x4 matrix projects the transformed homogeneous
  // (s,t,0,1) coordinate back to the current 2D mesh texcoord seam.
  {
    SFMatrix4f matrix{};
    for (int i = 0; i < 4; ++i) matrix.matrix[i][i] = 1.0f;
    matrix.matrix[0][0] = 2.0f;
    matrix.matrix[1][1] = 3.0f;
    matrix.matrix[0][3] = 0.25f;
    matrix.matrix[1][3] = 0.5f;
    auto tm = createX3DNode("TextureTransformMatrix3D");
    setF(tm, "matrix", std::any(matrix));
    auto app = createX3DNode("Appearance");
    setF(app, "textureTransform", std::any(std::shared_ptr<X3DNode>(tm)));

    TextureTransform2DParams p = textureTransformParamsOf(app.get());
    MeshData mesh;
    mesh.texcoords = {{0.5f, 0.25f}};
    applyTextureTransformToMesh(mesh, p);
    CHECK((feq(mesh.texcoords[0].x, 1.25f)));
    CHECK((feq(mesh.texcoords[0].y, 1.25f)));
  }
}

// --- 4. ExtendedSampler / TextureProperties boundary+filter modes ----------
static void testExtendedSampler() {
  // No TextureProperties: repeatS/T derive BoundaryMode (Repeat / ClampToEdge).
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "repeatS", std::any(true));
    setF(tex, "repeatT", std::any(false));
    ExtendedSamplerParams s = extendedSamplerOf(tex);
    CHECK((s.repeatS == true && s.repeatT == false));
    CHECK((s.boundaryModeS == BoundaryMode::Repeat));
    CHECK((s.boundaryModeT == BoundaryMode::ClampToEdge));
  }
  // With TextureProperties: boundary/filter modes govern (override repeat bools).
  {
    auto tp = createX3DNode("TextureProperties");
    setEnum(tp, "boundaryModeS", "CLAMP_TO_EDGE");
    setEnum(tp, "boundaryModeT", "MIRRORED_REPEAT");
    setEnum(tp, "magnificationFilter", "NEAREST_PIXEL");
    setEnum(tp, "minificationFilter", "AVG_PIXEL_AVG_MIPMAP");
    setF(tp, "generateMipMaps", std::any(true));
    setF(tp, "anisotropicDegree", std::any(4.0f));
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "textureProperties", std::any(std::shared_ptr<X3DNode>(tp)));

    ExtendedSamplerParams s = extendedSamplerOf(tex);
    CHECK((s.boundaryModeS == BoundaryMode::ClampToEdge));
    CHECK((s.boundaryModeT == BoundaryMode::MirroredRepeat));
    CHECK((s.magnificationFilter == MagFilter::NearestPixel));
    CHECK((s.minificationFilter == MinFilter::AvgPixelAvgMipmap));
    CHECK((s.generateMipmaps == true));
    CHECK((feq(s.anisotropicDegree, 4.0f)));
  }
  // 3D textures carry repeatR; without TextureProperties it derives boundaryModeR (T3D-2).
  {
    auto tex = createX3DNode("ComposedTexture3D");
    setF(tex, "repeatS", std::any(true));
    setF(tex, "repeatR", std::any(false));
    ExtendedSamplerParams s = extendedSamplerOf(tex);
    CHECK((s.repeatR == false));
    CHECK((s.boundaryModeR == BoundaryMode::ClampToEdge));
  }
  // With TextureProperties, boundaryModeR governs the R axis (T3D-2).
  {
    auto tp = createX3DNode("TextureProperties");
    setEnum(tp, "boundaryModeR", "MIRRORED_REPEAT");
    auto tex = createX3DNode("ImageTexture3D");
    setF(tex, "textureProperties", std::any(std::shared_ptr<X3DNode>(tp)));
    ExtendedSamplerParams s = extendedSamplerOf(tex);
    CHECK((s.boundaryModeR == BoundaryMode::MirroredRepeat));
  }
}

// --- 5. TextureCoordinateGenerator surfaces a TexCoordGenDesc --------------
static void testTexCoordGen() {
  // Geometry whose texCoord is a TextureCoordinateGenerator (no point array).
  auto gen = createX3DNode("TextureCoordinateGenerator");
  setEnum(gen, "mode", "CAMERASPACEREFLECTIONVECTOR");
  setF(gen, "parameter", std::any(std::vector<float>{1.5f}));
  auto g = createX3DNode("IndexedFaceSet");
  setF(g, "texCoord", std::any(std::shared_ptr<X3DNode>(gen)));

  bool has = false;
  TexCoordGenDesc d = texCoordGenOf(g.get(), &has);
  CHECK((has));
  CHECK((d.mode == TexCoordGenMode::CameraSpaceReflectionVector));
  CHECK((d.parameter.size() == 1 && feq(d.parameter[0], 1.5f)));

  // A plain TextureCoordinate (or none) => no generator.
  auto g2 = createX3DNode("IndexedFaceSet");
  bool has2 = true;
  texCoordGenOf(g2.get(), &has2);
  CHECK((!has2));
}

// --- 6. resolveTextureRefs threads a resolver onto resolvedPixels ----------
static void testResolverRoundTrip() {
  // A stub resolver that returns a 1x1 magenta pixel for any URL.
  TextureResolver resolver = [](const std::string &url) -> TexturePixelResult {
    if (url.empty()) return TexturePixelResult::makeFailed();
    TexturePixels px;
    px.width = 1;
    px.height = 1;
    px.rgba = {255, 0, 255, 255};
    return TexturePixelResult::makeReady(std::move(px));
  };

  std::vector<TextureRef> refs;
  {
    TextureRef url;
    url.source = TextureRef::Source::Url;
    url.url = {"tex.png"};
    refs.push_back(url);
  }
  {
    TextureRef inl;
    inl.source = TextureRef::Source::Inline; // must NOT go through the resolver.
    refs.push_back(inl);
  }
  {
    TextureRef empty;
    empty.source = TextureRef::Source::Url; // empty url list => Failed.
    refs.push_back(empty);
  }

  resolveTextureRefs(refs, resolver);

  // Url with a real URL => Ready with the magenta pixel.
  CHECK((refs[0].resolvedPixels.ready()));
  CHECK((refs[0].resolvedPixels.pixels.width == 1));
  CHECK((refs[0].resolvedPixels.pixels.rgba.size() == 4));
  CHECK((refs[0].resolvedPixels.pixels.rgba[0] == 255 &&
         refs[0].resolvedPixels.pixels.rgba[1] == 0));

  // Inline => resolver NEVER called; stays Failed (consumer reads inlinePixels).
  CHECK((refs[1].resolvedPixels.failed()));

  // Url with empty url list => Failed.
  CHECK((refs[2].resolvedPixels.failed()));

  // The null stub resolver fails everything (default white-fallback PoC path).
  std::vector<TextureRef> refs2 = {refs[0]};
  refs2[0].resolvedPixels = TexturePixelResult::makeFailed();
  resolveTextureRefs(refs2, makeNullTextureResolver());
  CHECK((refs2[0].resolvedPixels.failed()));
}

TEST_CASE("texture_extract_test") {
  testAuthoredTexCoord();
  testMultiTextureCoordinateFirstUsableChannel();
  testTextureCoordinate3DAnd4DProjectToST();
  testApplyTransformToMesh();
  testTextureTransformParamsOf();
  testExtendedSampler();
  testTexCoordGen();
  testResolverRoundTrip();
  return;
}
