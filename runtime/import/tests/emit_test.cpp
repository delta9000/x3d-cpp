// emit_test.cpp — emitDocument() on hand-built scenes: the node mapping,
// naming, materials, cameras, lights, skins and animation tracks.
#include "AssetImport.hpp"

#include "x3d/sdk.hpp"

#include "doctest/doctest.h"

#include <cmath>
#include <string>

namespace imp = x3d::runtime::importer;
namespace sdk = x3d::sdk;

namespace {

bool has(const std::string &s, const std::string &needle) { return s.find(needle) != std::string::npos; }
std::size_t count(const std::string &s, const std::string &needle) {
  std::size_t n = 0;
  for (auto at = s.find(needle); at != std::string::npos; at = s.find(needle, at + needle.size())) ++n;
  return n;
}

imp::ImportMesh triangle(int material) {
  imp::ImportMesh m;
  m.name = "tri";
  m.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  m.indices = {0, 1, 2};
  m.materialIndex = material;
  return m;
}

/// One root node holding mesh 0.
imp::ImportScene oneMesh(imp::ImportMesh mesh, imp::ImportMaterial mat) {
  imp::ImportScene s;
  s.meshes.push_back(std::move(mesh));
  s.materials.push_back(std::move(mat));
  imp::ImportNode n;
  n.name = "root";
  n.meshIndices = {0};
  s.nodes.push_back(n);
  s.roots = {0};
  return s;
}

std::string xmlOf(const imp::ImportScene &s, imp::EmitReport *rep = nullptr, imp::EmitOptions o = {}) {
  return sdk::XmlWriter{}.writeDocument(imp::emitDocument(s, o, rep));
}

} // namespace

TEST_CASE("emit: hierarchy, unique DEF names and shared meshes") {
  imp::ImportScene s;
  s.meshes.push_back(triangle(-1));
  for (int i = 0; i < 3; ++i) {
    imp::ImportNode n;
    n.name = i == 0 ? "my node" : "child";
    if (i > 0) n.meshIndices = {0};
    s.nodes.push_back(n);
  }
  s.nodes[0].childIndices = {1, 2};
  s.nodes[0].localTransform.m[12] = 5; // translation x
  s.roots = {0};
  imp::EmitReport rep;
  const std::string xml = xmlOf(s, &rep);
  CHECK(rep.transforms == 3);
  CHECK(rep.shapes == 1); // one Shape, USEd by the second placement
  CHECK(has(xml, "DEF=\"my_node\""));
  CHECK(has(xml, "translation=\"5 0 0\""));
  CHECK(has(xml, "DEF=\"child\""));
  CHECK(has(xml, "DEF=\"child_2\""));
  CHECK(count(xml, "<IndexedTriangleSet") == 1);
  CHECK(has(xml, "<Shape USE="));
}

TEST_CASE("emit: material models and alpha modes") {
  imp::ImportMaterial pbr;
  pbr.name = "pbr";
  pbr.pbr = imp::PbrParams{{0.5f, 0.25f, 1.0f, 0.5f}, 0.0f, 0.3f};
  pbr.alpha = imp::AlphaMode::Blend;
  pbr.doubleSided = true;
  std::string xml = xmlOf(oneMesh(triangle(0), pbr));
  CHECK(has(xml, "<PhysicalMaterial"));
  CHECK(has(xml, "baseColor=\"0.5 0.25 1\""));
  CHECK(has(xml, "transparency=\"0.5\""));
  CHECK(has(xml, "alphaMode=\"BLEND\""));
  CHECK(has(xml, "solid=\"false\""));

  imp::ImportMaterial mask;
  mask.alpha = imp::AlphaMode::Mask;
  mask.alphaCutoff = 0.25f;
  xml = xmlOf(oneMesh(triangle(0), mask));
  CHECK(has(xml, "<Material"));
  CHECK(has(xml, "alphaMode=\"MASK\""));
  CHECK(has(xml, "alphaCutoff=\"0.25\""));

  imp::ImportMaterial unlit;
  unlit.unlit = true;
  unlit.diffuse = {0, 1, 0};
  xml = xmlOf(oneMesh(triangle(0), unlit));
  CHECK(has(xml, "<UnlitMaterial"));
  CHECK(has(xml, "emissiveColor=\"0 1 0\""));
}

TEST_CASE("emit: lines and points are unlit IndexedLineSet and PointSet") {
  imp::ImportMesh lines = triangle(0);
  lines.topology = imp::ImportMesh::Topology::Lines;
  lines.indices = {0, 1, 1, 2};
  imp::ImportMaterial m;
  m.diffuse = {1, 0, 0};
  std::string xml = xmlOf(oneMesh(lines, m));
  CHECK(has(xml, "<IndexedLineSet coordIndex=\"0 1 -1 1 2 -1\""));
  CHECK(has(xml, "<UnlitMaterial"));
  imp::ImportMesh points = triangle(0);
  points.topology = imp::ImportMesh::Topology::Points;
  xml = xmlOf(oneMesh(points, m));
  CHECK(has(xml, "<PointSet"));
}

TEST_CASE("emit: cameras keep the smaller-side field of view") {
  imp::ImportScene s;
  imp::ImportCamera portrait;
  portrait.name = "tall";
  portrait.yfov = 1.0f;
  portrait.aspectRatio = 0.5f;
  imp::ImportCamera ortho;
  ortho.kind = imp::ImportCamera::Kind::Orthographic;
  ortho.xmag = 2;
  ortho.ymag = 1;
  s.cameras = {portrait, ortho};
  imp::ImportNode n;
  n.camera = 0;
  s.nodes.push_back(n);
  s.roots = {0};
  imp::EmitReport rep;
  const auto doc = imp::emitDocument(s, {}, &rep);
  const std::string xml = sdk::XmlWriter{}.writeDocument(doc);
  CHECK(rep.viewpoints == 2);
  CHECK(has(xml, "description=\"tall\""));
  const float fov = 2.0f * std::atan(std::tan(0.5f) * 0.5f);
  CHECK(has(xml, "fieldOfView=\"" + std::to_string(fov).substr(0, 5)));
  CHECK(has(xml, "<OrthoViewpoint"));
  CHECK(has(xml, "fieldOfView=\"-2 -1 2 1\""));
}

TEST_CASE("emit: light values X3D cannot hold are clamped and reported") {
  imp::ImportScene s;
  imp::ImportLight spot;
  spot.kind = imp::ImportLight::Kind::Spot;
  spot.intensity = 5;
  spot.range = 0;
  spot.innerConeAngle = 0.2f;
  spot.outerConeAngle = 0.4f;
  s.lights = {spot};
  imp::ImportNode n;
  n.light = 0;
  s.nodes.push_back(n);
  s.roots = {0};
  imp::EmitReport rep;
  const std::string xml = xmlOf(s, &rep);
  CHECK(has(xml, "<SpotLight"));
  // global TRUE and intensity 1 are the SpotLight defaults, so the writer
  // leaves them out.
  CHECK(!has(xml, "global=\"false\""));
  CHECK(!has(xml, "intensity="));
  CHECK(has(xml, "beamWidth=\"0.2\""));
  CHECK(has(xml, "cutOffAngle=\"0.4\""));
  CHECK(rep.warnings.size() == 1);
}

TEST_CASE("emit: STEP keys jump, CUBICSPLINE is sampled, autoplay picks the clock") {
  imp::ImportScene s = oneMesh(triangle(0), imp::ImportMaterial{});
  s.nodes[0].trs = imp::Trs{};
  imp::AnimationChannel step;
  step.path = imp::AnimationChannel::Path::Translation;
  step.interpolation = imp::AnimationChannel::Interpolation::Step;
  step.target = 0;
  step.times = {0, 1, 2};
  step.values = {0, 0, 0, 1, 0, 0, 2, 0, 0};
  imp::AnimationChannel cubic;
  cubic.path = imp::AnimationChannel::Path::Scale;
  cubic.interpolation = imp::AnimationChannel::Interpolation::CubicSpline;
  cubic.target = 0;
  cubic.times = {0, 2};
  cubic.values = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0};
  s.animations.push_back({"walk", {step}});
  s.animations.push_back({"grow", {cubic}});
  imp::EmitReport rep;
  imp::EmitOptions o;
  o.samplesPerInterval = 4;
  const std::string xml = xmlOf(s, &rep, o);
  CHECK(rep.timeSensors == 2);
  CHECK(rep.interpolators == 2);
  CHECK(rep.routes == 4);
  // STEP: each key is held to the next one, then jumps.
  CHECK(has(xml, "key=\"0 0.5 0.5 1 1\""));
  CHECK(has(xml, "keyValue=\"0 0 0 0 0 0 1 0 0 1 0 0 2 0 0\""));
  // CUBICSPLINE: 4 samples per interval.
  CHECK(has(xml, "key=\"0 0.25 0.5 0.75 1\""));
  CHECK(has(xml, "cycleInterval=\"2\""));
  CHECK(count(xml, "enabled=\"false\"") == 1);
  CHECK(has(xml, "description=\"walk\""));
}

TEST_CASE("emit: a skin becomes an HAnimHumanoid with the bind pose") {
  imp::ImportScene s;
  imp::ImportMesh m = triangle(-1);
  m.influences = {{{0, 1.0f}}, {{0, 0.5f}, {1, 0.5f}}, {{1, 1.0f}}};
  s.meshes.push_back(m);
  imp::ImportNode hip, knee, body;
  hip.name = "hip";
  knee.name = "knee";
  knee.localTransform.m[13] = 1; // y = 1
  hip.childIndices = {1};
  body.name = "body";
  body.meshIndices = {0};
  body.skin = 0;
  s.nodes = {hip, knee, body};
  s.roots = {0, 2};
  imp::ImportSkin skin;
  skin.joints = {0, 1};
  imp::Mat4 kneeIbm;
  kneeIbm.m[13] = -1;
  skin.inverseBind = {imp::Mat4{}, kneeIbm};
  s.skins.push_back(skin);
  imp::EmitReport rep;
  const auto doc = imp::emitDocument(s, {}, &rep);
  const std::string xml = sdk::XmlWriter{}.writeDocument(doc);
  CHECK(rep.humanoids == 1);
  CHECK(rep.joints == 2);
  CHECK(has(xml, "<HAnimHumanoid"));
  CHECK(has(xml, "skinCoordIndex=\"0 1\""));
  CHECK(has(xml, "skinCoordWeight=\"1 0.5\""));
  CHECK(has(xml, "jointBindingPositions=\"0 0 0 0 1 0\""));
  // The document runs: the humanoid builds a skin binding.
  auto session = sdk::RuntimeSession::create(sdk::parseDocument(xml, sdk::Encoding::XML));
  const auto snap = session->fullSnapshot();
  REQUIRE(snap.added.size() == 1);
  CHECK(session->extractor().item(snap.added[0]).skin.has_value());
}
