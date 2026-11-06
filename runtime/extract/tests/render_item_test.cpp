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

  // PathKey: full-vector equality, bucket-only hash
  PathKey p1{nA, nB};
  PathKey p2{nA, nB};
  PathKey p3{nB, nA}; // different order => different placement.
  PathKeyEqual eq;
  PathKeyHash hash;
  CHECK((eq(p1, p2)));     // same chain => equal identity.
  CHECK((!eq(p1, p3)));    // reordered chain => distinct identity.
  (void)hash(p1);         // hash is a usable bucket key.
  CHECK((hash(p1) == hash(p2))); // equal keys must share a bucket.

  // RenderItemId handle
  static_assert(std::is_same_v<RenderItemId, std::uint32_t>);

  // GeomId: content identity
  GeomId g1{nA, 0};
  GeomId g2{nA, 0};
  GeomId g3{nA, 1}; // bumped contentVersion => different content.
  CHECK((g1 == g2));
  CHECK((g1 != g3));
  GeomIdHash gh;
  CHECK((gh(g1) == gh(g2)));

  // MeshData
  MeshData mesh;
  // isGlyphMesh defaults false for all non-text geometry.
  CHECK((mesh.isGlyphMesh == false));

  TextureRef tex;

  // resolvedPixels defaults to Failed (= PoC white-fallback contract).
  CHECK((tex.resolvedPixels.failed()));
  CHECK((!tex.resolvedPixels.ready()));
  CHECK((!tex.resolvedPixels.pending()));

  // extractor threads a Ready result onto the field after resolver call.
  TexturePixels px;
  px.width  = 2;
  px.height = 2;
  px.rgba.assign(2 * 2 * 4, 0xFF); // solid white RGBA8
  tex.resolvedPixels = TexturePixelResult::makeReady(px);
  CHECK((tex.resolvedPixels.ready()));
  CHECK((tex.resolvedPixels.pixels->width  == 2));
  CHECK((tex.resolvedPixels.pixels->height == 2));
  CHECK((tex.resolvedPixels.pixels->rgba.size() == 16));

  // Pending state is valid (atlas not yet uploaded; retry next frame).
  TextureRef pendingTex;
  pendingTex.resolvedPixels = TexturePixelResult::makePending();
  CHECK((pendingTex.resolvedPixels.pending()));

  // MaterialDesc + toRGBA documented alpha = 1 - transparency
  MaterialDesc mat;
  CHECK((mat.model == MaterialModel::Phong));
  CHECK((feq(mat.toRGBA().a, 1.0f))); // default transparency=0 => alpha=1.
  mat.transparency = 0.25f;
  CHECK((feq(mat.toRGBA().a, 0.75f)));

  // CameraDesc: default min-dimension field of view
  CameraDesc cam;
  CHECK((feq(cam.fieldOfView, 0.7854f))); // X3D default min-dimension FOV.
  return;
}
