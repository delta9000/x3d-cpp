// usd_source_test.cpp — the USD backend on committed USDA fixtures and on the
// pinned tinyusdz checkout's USDZ model.
#include "UsdSource.hpp"

#include "AssetImport.hpp"
#include "x3d/sdk.hpp"

#include "doctest/doctest.h"

#include <cmath>
#include <string>

namespace imp = x3d::runtime::importer;
namespace sdk = x3d::sdk;

namespace {
std::string fixture(const std::string &name) { return std::string(X3D_IMPORT_FIXTURES) + "/usd/" + name; }
const imp::ImportNode *named(const imp::ImportScene &s, const std::string &n) {
  for (const auto &node : s.nodes)
    if (node.name == n) return &node;
  return nullptr;
}
} // namespace

TEST_CASE("usd: registered for .usd, .usda, .usdc and .usdz") {
  imp::BackendRegistry r = imp::builtinBackends();
  imp::addUsdBackend(r);
  for (const char *p : {"a.usd", "a.usda", "a.USDC", "a.usdz"}) {
    REQUIRE(r.select(p) != nullptr);
    CHECK(r.select(p)->name == "usd");
  }
}

TEST_CASE("usd: composed references and sublayers resolve geometry") {
  const imp::ImportScene s = imp::UsdSource{}.load(fixture("composed/root.usda"));
  REQUIRE(s.meshes.size() == 2);
  for (const auto &m : s.meshes) {
    CHECK(m.positions.size() == 3);
    CHECK(m.indices.size() == 3);
  }
}

TEST_CASE("usd: UsdPreviewSurface values X3D cannot hold are kept for materialGlsl") {
  const imp::ImportScene s = imp::UsdSource{}.load(fixture("material_params.usda"));
  REQUIRE(!s.materials.empty());
  const imp::ImportMaterial &m = s.materials[0];
  CHECK(m.useSpecularWorkflow);
  CHECK(m.ior == doctest::Approx(1.4f).epsilon(1e-3));
  CHECK(m.clearcoat == doctest::Approx(0.7f).epsilon(1e-3));
  CHECK(m.clearcoatRoughness == doctest::Approx(0.2f).epsilon(1e-3));
  CHECK(m.opacity == doctest::Approx(0.5f).epsilon(1e-3));
  CHECK(m.specular.x == doctest::Approx(0.9f).epsilon(1e-3));
  CHECK(m.alpha == imp::AlphaMode::Mask);
  CHECK(m.alphaCutoff == doctest::Approx(0.3f).epsilon(1e-3));
}

TEST_CASE("usd: stage frame, camera, lights, xform animation, subsets, displayColor") {
  const imp::ImportScene s = imp::UsdSource{}.load(fixture("stage.usda"));
  // Z-up centimetres: one root that rotates -90 degrees about X and scales 0.01.
  REQUIRE(s.roots.size() == 1);
  const imp::ImportNode &root = s.nodes[static_cast<std::size_t>(s.roots[0])];
  CHECK(root.name == "stage");
  CHECK(root.localTransform.m[0] == doctest::Approx(0.01f));
  CHECK(root.localTransform.m[6] == doctest::Approx(-0.01f));
  CHECK(root.localTransform.m[9] == doctest::Approx(0.01f));

  REQUIRE(s.cameras.size() == 1);
  CHECK(s.cameras[0].yfov == doctest::Approx(2 * std::atan(12.0f / 50.0f)).epsilon(1e-4));
  CHECK(s.cameras[0].aspectRatio == doctest::Approx(1.5f));
  CHECK(s.cameras[0].znear == doctest::Approx(0.5f));
  CHECK(s.cameras[0].zfar == doctest::Approx(500.0f));
  REQUIRE(named(s, "Cam"));
  CHECK(named(s, "Cam")->camera == 0);

  REQUIRE(s.lights.size() == 2);
  const imp::ImportLight *sun = nullptr, *lamp = nullptr;
  for (const auto &l : s.lights) (l.name == "Sun" ? sun : lamp) = &l;
  REQUIRE(sun);
  REQUIRE(lamp);
  CHECK(sun->kind == imp::ImportLight::Kind::Directional);
  CHECK(sun->intensity == doctest::Approx(0.75f)); // 0.375 * 2^1
  CHECK(sun->color.y == doctest::Approx(0.5f));
  CHECK(lamp->kind == imp::ImportLight::Kind::Spot);
  CHECK(lamp->outerConeAngle == doctest::Approx(30.0f * 3.14159265f / 180.0f).epsilon(1e-4));
  CHECK(lamp->innerConeAngle == doctest::Approx(15.0f * 3.14159265f / 180.0f).epsilon(1e-4));

  REQUIRE(s.animations.size() == 1);
  const auto &ch = s.animations[0].channels;
  REQUIRE(ch.size() == 3);
  CHECK(ch[0].path == imp::AnimationChannel::Path::Translation);
  REQUIRE(ch[0].times.size() == 3);
  CHECK(ch[0].times[1] == doctest::Approx(1.0f)); // timecode 24 at 24 per second
  CHECK(ch[0].values[3] == doctest::Approx(100.0f));

  // The quad splits into its two materialBind subsets.
  const imp::ImportNode *quad = named(s, "Quad");
  REQUIRE(quad);
  REQUIRE(quad->meshIndices.size() == 2);
  for (int mi : quad->meshIndices) CHECK(s.meshes[static_cast<std::size_t>(mi)].indices.size() == 3);
  const auto &m0 = s.materials[static_cast<std::size_t>(s.meshes[static_cast<std::size_t>(quad->meshIndices[0])].materialIndex)];
  const auto &m1 = s.materials[static_cast<std::size_t>(s.meshes[static_cast<std::size_t>(quad->meshIndices[1])].materialIndex)];
  CHECK(m0.name != m1.name);

  // An unbound mesh gets its displayColor; doubleSided reaches the material.
  const imp::ImportNode *plain = named(s, "Plain");
  REQUIRE(plain);
  REQUIRE(plain->meshIndices.size() == 1);
  const auto &pm = s.materials[static_cast<std::size_t>(s.meshes[static_cast<std::size_t>(plain->meshIndices[0])].materialIndex)];
  CHECK(pm.diffuse.y == doctest::Approx(0.9f));
  CHECK(pm.doubleSided);
}

TEST_CASE("usd: a skeleton, its SkelAnimation and a blend shape") {
  const imp::ImportScene s = imp::UsdSource{}.load(fixture("skel.usda"));
  REQUIRE(s.skins.size() == 1);
  CHECK(s.skins[0].joints.size() == 2);
  const imp::ImportNode *leg = named(s, "Leg");
  REQUIRE(leg);
  CHECK(leg->skin == 0);
  REQUIRE(leg->meshIndices.size() == 1);
  const imp::ImportMesh &m = s.meshes[static_cast<std::size_t>(leg->meshIndices[0])];
  CHECK(m.influences.size() == m.positions.size());
  REQUIRE(m.morphTargets.size() == 1);
  REQUIRE(s.animations.size() == 1);
  bool rotation = false, weights = false;
  for (const auto &c : s.animations[0].channels) {
    rotation = rotation || c.path == imp::AnimationChannel::Path::Rotation;
    weights = weights || c.path == imp::AnimationChannel::Path::Weights;
  }
  CHECK(rotation);
  CHECK(weights);

  // The emitted document runs and skins the mesh.
  imp::ImportOptions o;
  o.textures = false;
  const imp::ImportResult r = imp::importScene(s, "", o);
  CHECK(r.report.emit.humanoids == 1);
  const std::string xml = sdk::XmlWriter{}.writeDocument(r.document);
  auto session = sdk::RuntimeSession::create(sdk::parseDocument(xml, sdk::Encoding::XML));
  CHECK(session->routes().rejected.empty());
}

TEST_CASE("usd: a USDZ archive keeps its texture's original bytes") {
  const imp::ImportScene s = imp::UsdSource{}.load(std::string(X3D_TINYUSDZ_MODELS) + "/texture-cat-plane.usdz");
  REQUIRE(s.embedded.size() >= 1);
  const imp::EmbeddedTexture &e = s.embedded[0];
  CHECK(e.hintExt == "jpg");
  REQUIRE(e.bytes.size() >= 3);
  CHECK(e.bytes[0] == 0xFF);
  CHECK(e.bytes[1] == 0xD8);
  CHECK(e.width == 0);
}
