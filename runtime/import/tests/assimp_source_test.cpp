// assimp_source_test.cpp — the Assimp backend: an OBJ fixture, and glTF
// written by the SDK's exporter read back through Assimp and compared frame
// by frame against the source scene (RoundTripOracle.hpp).
#include "AssimpSource.hpp"
#include "gltf/CgltfSource.hpp"

#include "RoundTripOracle.hpp"

using namespace roundtrip;

namespace {
imp::BackendRegistry withAssimp() {
  imp::BackendRegistry r = imp::builtinBackends();
  imp::addAssimpBackend(r);
  return r;
}
} // namespace

TEST_CASE("assimp: registered for its formats, behind cgltf for glTF") {
  const imp::BackendRegistry r = withAssimp();
  REQUIRE(r.select("m.obj") != nullptr);
  CHECK(r.select("m.obj")->name == "assimp");
  CHECK(r.select("m.FBX")->name == "assimp");
  CHECK(r.select("m.dae")->name == "assimp");
  CHECK(r.select("m.glb")->name == "gltf");
  CHECK(r.byName("assimp") != nullptr);
}

TEST_CASE("assimp: an OBJ with its MTL material") {
  const imp::ImportScene s = imp::AssimpSource{}.load(std::string(X3D_IMPORT_FIXTURES) + "/tri.obj");
  REQUIRE(s.meshes.size() == 1);
  CHECK(s.meshes[0].positions.size() == 3);
  CHECK(s.meshes[0].indices.size() == 3);
  CHECK(s.meshes[0].normals.size() == 3);
  REQUIRE(s.meshes[0].materialIndex >= 0);
  const auto &m = s.materials[static_cast<std::size_t>(s.meshes[0].materialIndex)];
  CHECK(m.name == "tri_mat");
  CHECK(m.diffuse.x == doctest::Approx(0.8f));
  CHECK(m.diffuse.y == doctest::Approx(0.2f));
}

TEST_CASE("assimp: glb with an embedded texture, camera and light") {
  const imp::ImportScene s = imp::AssimpSource{}.load(std::string(X3D_IMPORT_FIXTURES) + "/twobox.glb");
  REQUIRE(s.meshes.size() >= 1);
  CHECK(s.embedded.size() == 1);
  REQUIRE(s.cameras.size() == 1);
  CHECK(s.cameras[0].yfov == doctest::Approx(0.7853982f).epsilon(1e-3));
  REQUIRE(s.lights.size() == 1);
  CHECK(s.lights[0].kind == imp::ImportLight::Kind::Point);
  int camNode = -1;
  for (std::size_t i = 0; i < s.nodes.size(); ++i)
    if (s.nodes[i].camera == 0) camNode = static_cast<int>(i);
  CHECK(camNode >= 0);
}

TEST_CASE("assimp: agrees with the built-in glTF backend on twobox.glb") {
  // The ImportSource seam is generic if two independent parsers agree on the
  // invariants of one input. Vertex order, mesh splitting and material naming
  // differ between them, so only counts and authored PBR values are compared.
  const std::string path = std::string(X3D_IMPORT_FIXTURES) + "/twobox.glb";
  const imp::ImportScene a = imp::CgltfSource{}.load(path);
  const imp::ImportScene b = imp::AssimpSource{}.load(path);
  const auto tris = [](const imp::ImportScene &s) {
    std::size_t t = 0;
    for (const auto &m : s.meshes) t += m.indices.size() / 3;
    return t;
  };
  CHECK(tris(a) == tris(b));
  CHECK(a.cameras.size() == b.cameras.size());
  CHECK(a.lights.size() == b.lights.size());
  CHECK(a.embedded.size() == b.embedded.size());
  // Assimp's glTF importer appends an unused default material, so only the
  // authored one (index 0 in both) is compared.
  REQUIRE(!a.materials.empty());
  REQUIRE(!b.materials.empty());
  REQUIRE(a.materials[0].pbr.has_value());
  REQUIRE(b.materials[0].pbr.has_value());
  CHECK(a.materials[0].pbr->metallic == doctest::Approx(b.materials[0].pbr->metallic).epsilon(1e-4));
  CHECK(a.materials[0].pbr->roughness == doctest::Approx(b.materials[0].pbr->roughness).epsilon(1e-4));
  CHECK(a.materials[0].pbr->baseColor.x == doctest::Approx(b.materials[0].pbr->baseColor.x).epsilon(1e-4));
}

TEST_CASE("assimp: round trip of static geometry and a TRS animation") {
  RoundTrip rt = roundTrip(
      "<Transform DEF='T' translation='1 0 0'>"
      "  <Shape><Appearance><PhysicalMaterial baseColor='0.2 0.4 0.6'/></Appearance><Box/></Shape>"
      "</Transform>"
      "<Shape><Appearance><PhysicalMaterial baseColor='1 0 0'/></Appearance><Sphere radius='0.5'/></Shape>"
      "<TimeSensor DEF='Clock' cycleInterval='2' loop='true'/>"
      "<PositionInterpolator DEF='P' key='0 0.5 1' keyValue='0 0 0  3 1 0  0 0 0'/>"
      "<OrientationInterpolator DEF='R' key='0 1' keyValue='0 1 0 0  0 1 0 3'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='P' toField='set_fraction'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='R' toField='set_fraction'/>"
      "<ROUTE fromNode='P' fromField='value_changed' toNode='T' toField='translation'/>"
      "<ROUTE fromNode='R' fromField='value_changed' toNode='T' toField='rotation'/>",
      "assimp_trs", withAssimp(), "assimp");
  CHECK(rt.imported.report.backend == "assimp");
  compareOver(rt, {0.0, 0.35, 0.9, 1.6});
}

TEST_CASE("assimp: round trip of a skinned humanoid") {
  RoundTrip rt = roundTrip(
      "<HAnimHumanoid DEF='Hum' name='Hum' version='2.0'>"
      "  <HAnimJoint DEF='hip' name='hip' containerField='skeleton'"
      "      skinCoordIndex='0 1 2 3' skinCoordWeight='1 1 0.5 0.5'>"
      "    <HAnimJoint DEF='knee' name='knee' center='0 1 0' skinCoordIndex='2 3 4 5'"
      "        skinCoordWeight='0.5 0.5 1 1'/>"
      "  </HAnimJoint>"
      "  <HAnimJoint USE='hip' containerField='joints'/>"
      "  <HAnimJoint USE='knee' containerField='joints'/>"
      "  <Coordinate DEF='SkinC' containerField='skinCoord'"
      "      point='0 0 0 0.2 0 0 0 1 0 0.2 1 0 0 2 0 0.2 2 0'/>"
      "  <Shape containerField='skin'><Appearance><PhysicalMaterial baseColor='0.5 0.5 1'/></Appearance>"
      "    <IndexedFaceSet coordIndex='0 1 3 2 -1 2 3 5 4 -1'>"
      "      <Coordinate USE='SkinC'/></IndexedFaceSet></Shape>"
      "</HAnimHumanoid>"
      "<TimeSensor DEF='Clock' cycleInterval='2' loop='true'/>"
      "<OrientationInterpolator DEF='Bend' key='0 1' keyValue='0 0 1 0  0 0 1 1.2'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Bend' toField='set_fraction'/>"
      "<ROUTE fromNode='Bend' fromField='value_changed' toNode='knee' toField='rotation'/>",
      "assimp_skin", withAssimp(), "assimp");
  CHECK(rt.imported.report.emit.humanoids == 1);
  compareOver(rt, {0.0, 0.5, 1.3});
}
