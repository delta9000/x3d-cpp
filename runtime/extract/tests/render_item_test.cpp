// render_item_test.cpp — T0 compile-only descriptor test.
//
// Statically constructs every descriptor in extract/RenderItem.hpp and exercises
// the load-bearing contract bits (PathKey full-vector equality vs bucket-only
// hash; GeomId content identity; MaterialDesc::toRGBA alpha = 1 - transparency).
// This is the golden compile gate for the pure-POD descriptor layer: if the
// header stops being a self-contained leaf (or a field's type drifts) this TU
// fails to compile.
//
// v1-closure additions tested (2026-06-16):
//   * TextureRef::resolvedPixels — defaults Failed; can carry a Ready result.
//   * MeshData::isGlyphMesh — defaults false; set true for Text glyph quads.
#include "RenderItem.hpp"
#include "TextureResolver.hpp"

#include "doctest/doctest.h"
#include <cmath>
#include <type_traits>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-6f; }

TEST_CASE("render_item_test") {
  // Two distinct fake node identities (never dereferenced — pointer identity
  // only). reinterpret_cast of integer literals keeps this a pure compile/POD
  // test with no dependency on the generated node graph.
  const X3DNode *nA = reinterpret_cast<const X3DNode *>(0x1000);
  const X3DNode *nB = reinterpret_cast<const X3DNode *>(0x2000);

  // --- PathKey: full-vector equality, bucket-only hash --------------------
  PathKey p1{nA, nB};
  PathKey p2{nA, nB};
  PathKey p3{nB, nA}; // different order => different placement.
  PathKeyEqual eq;
  PathKeyHash hash;
  CHECK((eq(p1, p2)));     // same chain => equal identity.
  CHECK((!eq(p1, p3)));    // reordered chain => distinct identity.
  (void)hash(p1);         // hash is a usable bucket key.
  CHECK((hash(p1) == hash(p2))); // equal keys must share a bucket.

  // --- RenderItemId handle -------------------------------------------------
  RenderItemId id = 7;
  CHECK((id != kInvalidRenderItemId));
  static_assert(std::is_same_v<RenderItemId, std::uint32_t>);

  // --- GeomId: content identity -------------------------------------------
  GeomId g1{nA, 0};
  GeomId g2{nA, 0};
  GeomId g3{nA, 1}; // bumped contentVersion => different content.
  CHECK((g1 == g2));
  CHECK((g1 != g3));
  GeomIdHash gh;
  CHECK((gh(g1) == gh(g2)));

  // --- MeshData -----------------------------------------------------------
  MeshData mesh;
  mesh.positions = {SFVec3f{0, 0, 0}, SFVec3f{1, 0, 0}, SFVec3f{0, 1, 0}};
  mesh.indices = {0, 1, 2};
  mesh.normals = {SFVec3f{0, 0, 1}};
  mesh.texcoords = {SFVec2f{0, 0}, SFVec2f{1, 0}, SFVec2f{0, 1}};
  mesh.colors = {SFColorRGBA{1, 0, 0, 1}};
  mesh.ccw = true;
  mesh.solid = true;
  mesh.hasNormals = true;
  mesh.hasColors = true;
  CHECK((mesh.indices.size() == 3));

  // T-TEXT: isGlyphMesh defaults false for all non-text geometry.
  CHECK((mesh.isGlyphMesh == false));

  // T-TEXT: a glyph mesh is a plain MeshData with isGlyphMesh=true (routing flag).
  // Each quad: 4 positions (XY plane, Z=0), 4 texcoords (atlas UV), 6 indices.
  MeshData glyphMesh;
  glyphMesh.positions  = {SFVec3f{0,0,0}, SFVec3f{1,0,0}, SFVec3f{1,1,0}, SFVec3f{0,1,0}};
  glyphMesh.texcoords  = {SFVec2f{0,0},   SFVec2f{1,0},   SFVec2f{1,1},   SFVec2f{0,1}};
  glyphMesh.indices    = {0,1,2, 0,2,3};  // two CCW triangles per quad.
  glyphMesh.isGlyphMesh = true;
  CHECK((glyphMesh.isGlyphMesh == true));
  CHECK((glyphMesh.indices.size() == 6)); // 2 tris per glyph quad.

  // --- TextureRef / SamplerParams -----------------------------------------
  SamplerParams sampler{};
  TextureRef tex;
  tex.slot = TextureRef::Slot::BaseColor;
  tex.source = TextureRef::Source::Url;
  tex.url = {"diffuse.png"};
  tex.sampler = sampler;
  tex.inlinePixels = SFImage{};
  tex.channel = 0;
  CHECK((tex.url.size() == 1));

  // T-TEX: resolvedPixels defaults to Failed (= PoC white-fallback contract).
  CHECK((tex.resolvedPixels.failed()));
  CHECK((!tex.resolvedPixels.ready()));
  CHECK((!tex.resolvedPixels.pending()));

  // T-TEX: extractor threads a Ready result onto the field after resolver call.
  TexturePixels px;
  px.width  = 2;
  px.height = 2;
  px.rgba.assign(2 * 2 * 4, 0xFF); // solid white RGBA8
  tex.resolvedPixels = TexturePixelResult::makeReady(px);
  CHECK((tex.resolvedPixels.ready()));
  CHECK((tex.resolvedPixels.pixels->width  == 2));
  CHECK((tex.resolvedPixels.pixels->height == 2));
  CHECK((tex.resolvedPixels.pixels->rgba.size() == 16));

  // T-TEX: Pending state is valid (atlas not yet uploaded; retry next frame).
  TextureRef pendingTex;
  pendingTex.resolvedPixels = TexturePixelResult::makePending();
  CHECK((pendingTex.resolvedPixels.pending()));

  // T-TEX: Source::Inline textures do NOT use the resolver; resolvedPixels stays
  // Failed and the consumer reads inlinePixels instead.
  TextureRef inlineTex;
  inlineTex.source = TextureRef::Source::Inline;
  inlineTex.inlinePixels = SFImage{2, 2, 3, std::vector<unsigned char>(12, 0xFF)};
  CHECK((inlineTex.inlinePixels.width == 2));
  CHECK((inlineTex.resolvedPixels.failed())); // never set for Inline sources.

  // --- MaterialDesc + toRGBA documented alpha = 1 - transparency ----------
  MaterialDesc mat;
  CHECK((mat.model == MaterialModel::Phong));
  CHECK((feq(mat.toRGBA().a, 1.0f))); // default transparency=0 => alpha=1.
  mat.transparency = 0.25f;
  CHECK((feq(mat.toRGBA().a, 0.75f)));
  mat.model = MaterialModel::Physical;
  mat.physical.baseColor = SFColor{0.1f, 0.2f, 0.3f};
  mat.physical.metallic = 0.0f;
  mat.physical.roughness = 0.5f;
  mat.textures.push_back(tex);
  CHECK((mat.textures.size() == 1));

  MaterialDesc unlit;
  unlit.model = MaterialModel::Unlit;
  // unlit bool dropped; model == Unlit is the discriminator.
  unlit.alphaMode = AlphaMode::Blend; // descriptor-only surface compiles.

  // --- LightDesc: authored global carried, not promoted -------------------
  LightDesc dir;
  dir.type = LightDesc::Type::Directional;
  dir.global = false; // DirectionalLight spec default; carried verbatim.
  dir.worldDirection = SFVec3f{0, 0, -1};
  dir.scopeRoot = nA;
  CHECK((dir.global == false));

  LightDesc point;
  point.type = LightDesc::Type::Point;
  point.global = true; // PointLight spec default.
  point.worldLocation = SFVec3f{1, 2, 3};
  CHECK((point.global == true));

  LightDesc spot;
  spot.type = LightDesc::Type::Spot;
  spot.cutOffAngle = 0.5f;
  (void)spot;

  // --- CameraDesc: min-dimension FOV + descriptor-only ortho surface ------
  CameraDesc cam;
  CHECK((feq(cam.fieldOfView, 0.7854f))); // X3D default min-dimension FOV.
  cam.viewMatrix = Mat4::identity();
  cam.cameraChanged = true;
  cam.ortho = true;                       // descriptor-only surface compiles.
  cam.orthoFieldOfView = {-1, -1, 1, 1};

  // --- BackgroundDesc -----------------------------------------------------
  BackgroundDesc bg;
  bg.skyColor = {SFColor{0, 0, 0.2f}, SFColor{0, 0.5f, 1.0f}};
  bg.skyAngle = {1.2f};
  bg.groundColor = {SFColor{0.1f, 0.1f, 0.1f}};
  bg.backgroundChanged = true;
  CHECK((bg.skyColor.size() == 2));

  // --- RenderDelta: the single authoritative change channel ---------------
  RenderDelta delta;
  delta.added = {id};
  delta.removed = {};
  delta.updatedTransform = {id};
  delta.updatedGeometry = {id};
  delta.updatedMaterial = {id};
  delta.cameraChanged = true;
  delta.backgroundChanged = false;
  delta.lightsChanged = true;
  CHECK((delta.added.size() == 1));

  // RenderItem must NOT carry change bits — the design dropped any parallel
  // encoding. (No RenderItem struct exists in this header; change lives only in
  // RenderDelta. This is a contract assertion expressed by construction.)

  return;
}
