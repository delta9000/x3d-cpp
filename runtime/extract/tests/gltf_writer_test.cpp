// GltfWriter: the GLB container, instancing, the material mapping and the
// conversions glTF needs (winding, texture-coordinate origin).
#include "x3d/sdk.hpp"

#include "doctest/doctest.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace sdk = x3d::sdk;

namespace {

struct Glb {
  std::string json;
  std::string bin;
  sdk::GltfWriteReport report;
};

std::uint32_t u32(const std::string &s, std::size_t at) {
  std::uint32_t v = 0;
  std::memcpy(&v, s.data() + at, 4);
  return v;
}

Glb exportScene(const std::string &body) {
  const std::string text =
      "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
  sdk::X3DDocument doc = sdk::parseDocument(text, sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  sdk::SceneExtractor ex(ctx, doc.scene);
  const auto frame = ex.fullSnapshot();
  Glb out;
  const std::string glb = sdk::writeGlb(ex, frame.added, {}, &out.report);

  REQUIRE(glb.size() >= 20);
  CHECK(glb.compare(0, 4, "glTF") == 0);
  CHECK(u32(glb, 4) == 2);
  CHECK(u32(glb, 8) == glb.size());
  CHECK(glb.size() % 4 == 0);
  const std::uint32_t jsonLen = u32(glb, 12);
  CHECK(glb.compare(16, 4, "JSON") == 0);
  CHECK(jsonLen % 4 == 0);
  out.json = glb.substr(20, jsonLen);
  if (glb.size() > 20 + jsonLen) {
    const std::uint32_t binLen = u32(glb, 20 + jsonLen);
    CHECK(glb.compare(24 + jsonLen, 4, std::string("BIN\0", 4)) == 0);
    CHECK(binLen % 4 == 0);
    out.bin = glb.substr(28 + jsonLen, binLen);
    CHECK(28 + jsonLen + binLen == glb.size());
  }
  return out;
}

bool has(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

std::size_t count(const std::string &text, const std::string &needle) {
  std::size_t n = 0;
  for (std::size_t at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
    ++n;
  return n;
}

} // namespace

TEST_CASE("glb: an empty scene is a valid, bufferless asset") {
  const Glb g = exportScene("");
  CHECK(has(g.json, "\"asset\":{\"version\":\"2.0\""));
  CHECK(has(g.json, "\"scenes\":[{}]"));
  CHECK(!has(g.json, "\"buffers\""));
  CHECK(g.bin.empty());
  CHECK(g.report.nodes == 0);
}

TEST_CASE("glb: DEF/USE placements share one mesh and keep their transforms") {
  const Glb g = exportScene(
      "<Transform DEF='Left' translation='-2 0 0'>"
      "  <Shape DEF='S'><Appearance><Material diffuseColor='1 0 0'/></Appearance>"
      "    <Box/></Shape></Transform>"
      "<Transform DEF='Right' translation='2 0 0'><Shape USE='S'/></Transform>");
  CHECK(g.report.nodes == 2);
  CHECK(g.report.meshes == 1);
  CHECK(g.report.materials == 1);
  CHECK(g.report.triangles == 24); // 12 per box, two placements
  CHECK(count(g.json, "\"mesh\":0") == 2);
  // Column-major matrix: translation in elements 12..14.
  CHECK(has(g.json, "0,0,1,0,-2,0,0,1]"));
  CHECK(has(g.json, "0,0,1,0,2,0,0,1]"));
  CHECK(has(g.json, "\"POSITION\":0"));
  CHECK(has(g.json, "\"NORMAL\":"));
  CHECK(has(g.json, "\"min\":[-1,-1,-1],\"max\":[1,1,1]"));
  CHECK(has(g.json, "\"baseColorFactor\":[1,0,0,1]"));
  CHECK(has(g.json, "\"metallicFactor\":0"));
  CHECK(has(g.json, "\"mode\":4"));
  CHECK(has(g.json, "\"byteLength\":" + std::to_string(g.bin.size()) + "}]"));
}

TEST_CASE("glb: material models map to PBR, unlit and alpha") {
  const Glb g = exportScene(
      "<Shape><Appearance><PhysicalMaterial baseColor='0.2 0.4 0.6' "
      "  metallic='0.25' roughness='0.5' emissiveColor='0 0 1'/></Appearance>"
      "  <Sphere/></Shape>"
      "<Shape><Appearance><Material transparency='0.5'/></Appearance>"
      "  <Sphere/></Shape>"
      "<Shape><Appearance><UnlitMaterial emissiveColor='0 1 0'/></Appearance>"
      "  <Sphere/></Shape>");
  CHECK(g.report.materials == 3);
  CHECK(has(g.json, "\"baseColorFactor\":[0.200000003,0.400000006,0.600000024,1]"));
  CHECK(has(g.json, "\"metallicFactor\":0.25,\"roughnessFactor\":0.5"));
  CHECK(has(g.json, "\"emissiveFactor\":[0,0,1]"));
  CHECK(has(g.json, "0.5],\"metallicFactor\":0"));
  CHECK(has(g.json, "\"alphaMode\":\"BLEND\""));
  CHECK(has(g.json, "\"baseColorFactor\":[0,1,0,1]"));
  CHECK(has(g.json, "\"extensions\":{\"KHR_materials_unlit\":{}}"));
  CHECK(has(g.json, "\"extensionsUsed\":[\"KHR_materials_unlit\"]"));
}

TEST_CASE("glb: clockwise faces are turned round and solid=FALSE is two-sided") {
  const std::string tri = [](const char *ccw) {
    return std::string("<Shape><IndexedFaceSet solid='false' ccw='") + ccw +
           "' coordIndex='0 1 2 -1'><Coordinate point='0 0 0 1 0 0 0 1 0'/>"
           "</IndexedFaceSet></Shape>";
  }("false");
  const Glb g = exportScene(tri);
  CHECK(has(g.json, "\"doubleSided\":true"));
  REQUIRE(g.report.triangles == 1);
  // The index view is the last one written; three uint32 at the buffer's end.
  REQUIRE(g.bin.size() >= 12);
  const std::size_t at = g.bin.size() - 12;
  const std::uint32_t a = u32(g.bin, at), b = u32(g.bin, at + 4),
                      c = u32(g.bin, at + 8);
  // Whatever order the builder emitted, the last two were swapped: the
  // triangle the writer stores has the opposite orientation.
  auto pos = [&](std::uint32_t i, int k) {
    float v;
    std::memcpy(&v, g.bin.data() + i * 12 + static_cast<std::size_t>(k) * 4, 4);
    return v;
  };
  const float ux = pos(b, 0) - pos(a, 0), uy = pos(b, 1) - pos(a, 1);
  const float vx = pos(c, 0) - pos(a, 0), vy = pos(c, 1) - pos(a, 1);
  CHECK(ux * vy - uy * vx < 0); // authored CCW in XY, declared clockwise
}

TEST_CASE("glb: url textures are referenced and t is flipped") {
  const Glb g = exportScene(
      "<Shape><Appearance><Material/>"
      "  <ImageTexture url='\"wood.tga\" \"wood.PNG\"' repeatS='false'/>"
      "</Appearance>"
      "<IndexedFaceSet coordIndex='0 1 2 -1' texCoordIndex='0 1 2 -1'>"
      "  <Coordinate point='0 0 0 1 0 0 0 1 0'/>"
      "  <TextureCoordinate point='0 0 1 0 0 1'/>"
      "</IndexedFaceSet></Shape>"
      "<Shape><Appearance><Material/><PixelTexture image='1 1 1 255'/>"
      "</Appearance><Box/></Shape>");
  CHECK(g.report.textures == 1);
  CHECK(g.report.droppedTextures == 1); // the PixelTexture
  CHECK(has(g.json, "\"images\":[{\"uri\":\"wood.PNG\"}]"));
  CHECK(has(g.json, "\"samplers\":[{\"wrapS\":33071,\"wrapT\":10497}]"));
  CHECK(has(g.json, "\"baseColorTexture\":{\"index\":0}"));
  CHECK(has(g.json, "\"TEXCOORD_0\":"));
  // The first mesh's views are position, normal, texcoord: 3 vertices each.
  // t = 0,0,1 authored becomes 1,1,0.
  const std::size_t uv = 3 * 12 * 2;
  auto f = [&](std::size_t i) {
    float v;
    std::memcpy(&v, g.bin.data() + uv + i * 4, 4);
    return v;
  };
  float tsum = f(1) + f(3) + f(5);
  CHECK(tsum == doctest::Approx(2.0f));
}

TEST_CASE("glb: lines are written as unlit LINES with vertex colours") {
  const Glb g = exportScene(
      "<Shape><IndexedLineSet coordIndex='0 1 2 -1' colorPerVertex='true'>"
      "  <Coordinate point='0 0 0 1 0 0 1 1 0'/>"
      "  <Color color='1 0 0 0 1 0 0 0 1'/>"
      "</IndexedLineSet></Shape>");
  CHECK(g.report.nodes == 1);
  CHECK(g.report.triangles == 0);
  CHECK(has(g.json, "\"mode\":1"));
  CHECK(has(g.json, "\"COLOR_0\":"));
  CHECK(has(g.json, "\"baseColorFactor\":[1,1,1,1]"));
  CHECK(has(g.json, "KHR_materials_unlit"));
}

// RigidBody.geometry is an MFNode; the walk used to read it as a shape's SFNode
// geometry, which asserted in debug builds (`x3d extract` on any physics scene).
TEST_CASE("glb: a RigidBody's collidable list is not mistaken for shape geometry") {
  const Glb g = exportScene(
      "<RigidBodyCollection><RigidBody>"
      "  <CollidableShape DEF='C' containerField='geometry'>"
      "    <Shape containerField='shape'><Box/></Shape></CollidableShape>"
      "</RigidBody></RigidBodyCollection>"
      "<CollidableShape USE='C'/>");
  CHECK(g.report.nodes >= 1);
  CHECK(has(g.json, "\"mode\":4"));
}
