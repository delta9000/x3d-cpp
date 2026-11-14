// GltfWriter: the GLB container, instancing, the material mapping and the
// conversions glTF needs (winding, texture-coordinate origin).
#include "x3d/sdk.hpp"

#include "doctest/doctest.h"
#include <algorithm>
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

Glb exportScene(const std::string &body, const sdk::GltfWriteOptions &options = {}) {
  const std::string text =
      "<X3D profile='Full' version='4.0'><Scene>" + body + "</Scene></X3D>";
  sdk::X3DDocument doc = sdk::parseDocument(text, sdk::Encoding::XML);
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  sdk::SceneExtractor ex(ctx, doc.scene);
  const auto frame = ex.fullSnapshot();
  Glb out;
  const std::string glb = sdk::writeGlb(ex, frame.added, options, &out.report);

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
  CHECK(g.report.textures == 2);
  CHECK(g.report.droppedTextures == 0);
  CHECK(g.report.embeddedImages == 1); // the PixelTexture
  CHECK(has(g.json, "\"images\":[{\"uri\":\"wood.PNG\"},{\"bufferView\":"));
  CHECK(has(g.json, "\"mimeType\":\"image/png\""));
  CHECK(has(g.json, "\"samplers\":[{\"wrapS\":33071,\"wrapT\":10497}"));
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

TEST_CASE("glb: a PixelTexture is embedded as a PNG, top row first") {
  // 1x2 RGB: X3D lists the bottom row first, so red is the bottom pixel.
  const Glb g = exportScene(
      "<Shape><Appearance><Material/>"
      "<PixelTexture image='1 2 3 0xFF0000 0x0000FF'/></Appearance><Box/></Shape>");
  REQUIRE(g.report.embeddedImages == 1);
  const std::size_t at = g.bin.find(std::string("\x89PNG\r\n\x1a\n", 8));
  REQUIRE(at != std::string::npos);
  CHECK(at % 4 == 0);
  // IHDR: width 1, height 2, depth 8, colour type 2 (RGB).
  const unsigned char *ihdr =
      reinterpret_cast<const unsigned char *>(g.bin.data()) + at + 16;
  CHECK(ihdr[3] == 1);
  CHECK(ihdr[7] == 2);
  CHECK(ihdr[8] == 8);
  CHECK(ihdr[9] == 2);
  // IDAT payload: zlib header (2), stored-block header (5), then two rows of
  // filter byte + RGB. The first row is the image's top: blue.
  const std::size_t idat = g.bin.find("IDAT", at);
  REQUIRE(idat != std::string::npos);
  const unsigned char *rows =
      reinterpret_cast<const unsigned char *>(g.bin.data()) + idat + 4 + 7;
  CHECK(rows[0] == 0);
  CHECK(rows[1] == 0x00);
  CHECK(rows[3] == 0xFF); // blue on top
  CHECK(rows[5] == 0xFF); // red below
  CHECK(rows[7] == 0x00);
  // One image shared by a second shape using an equal PixelTexture.
  const Glb two = exportScene(
      "<Shape><Appearance><Material/><PixelTexture image='1 1 1 255'/>"
      "</Appearance><Box/></Shape>"
      "<Shape><Appearance><Material/><PixelTexture image='1 1 1 255'/>"
      "</Appearance><Sphere/></Shape>");
  CHECK(two.report.embeddedImages == 1);
}

TEST_CASE("glb: lights become KHR_lights_punctual nodes aimed in world space") {
  const Glb g = exportScene(
      "<DirectionalLight direction='1 0 0' color='1 0.5 0' intensity='0.5' global='true'/>"
      "<Transform translation='0 3 0'>"
      "  <PointLight location='1 0 0' radius='7'/>"
      "  <SpotLight direction='0 -1 0' beamWidth='2' cutOffAngle='0.5'/>"
      "</Transform><Shape><Box/></Shape>");
  CHECK(g.report.lights == 3);
  CHECK(g.report.nodes == 1); // items only
  CHECK(has(g.json, "\"extensionsUsed\":[\"KHR_materials_unlit\",\"KHR_lights_punctual\"]"));
  CHECK(has(g.json, "{\"type\":\"directional\",\"color\":[1,0.5,0],\"intensity\":0.5}"));
  CHECK(has(g.json, "{\"type\":\"point\",\"color\":[1,1,1],\"intensity\":1,\"range\":7}"));
  // beamWidth > cutOffAngle: inner is pulled just inside outer.
  CHECK(has(g.json, "\"outerConeAngle\":0.5}"));
  CHECK(has(g.json, "\"innerConeAngle\":0.49999"));
  // -Z turned to +X is a quarter turn about -Y: (0, -sin45, 0, cos45).
  CHECK(has(g.json, "\"rotation\":[0,-0.707106781,0,0.707106781]"));
  CHECK(has(g.json, "\"translation\":[1,3,0]"));
  // -Z turned to -Y is a quarter turn about -X.
  CHECK(has(g.json, "\"translation\":[0,3,0],\"rotation\":[-0.707106781,0,0,0.707106781]"));
}

TEST_CASE("glb: viewpoints become cameras, the bound one first") {
  const Glb g = exportScene(
      "<Transform translation='0 0 5'>"
      "  <Viewpoint DEF='Side' description='side view' position='1 2 3' "
      "    fieldOfView='0.5' nearDistance='0.25' farDistance='80'/></Transform>"
      "<OrthoViewpoint DEF='Top' fieldOfView='-2 -1 2 1' position='0 9 0'/>"
      "<GeoViewpoint/><Shape><Box/></Shape>");
  CHECK(g.report.cameras == 2);
  CHECK(g.report.skippedViewpoints == 1);
  CHECK(has(g.json,
            "\"cameras\":[{\"type\":\"perspective\",\"perspective\":"
            "{\"yfov\":0.5,\"znear\":0.25,\"zfar\":80},\"name\":\"side view\"}"));
  CHECK(has(g.json, "{\"type\":\"orthographic\",\"orthographic\":"
                    "{\"xmag\":2,\"ymag\":1,\"znear\":0,\"zfar\":10000},"
                    "\"name\":\"Top\"}"));
  CHECK(has(g.json, "{\"camera\":0,\"name\":\"side view\","
                    "\"matrix\":[1,0,0,0,0,1,0,0,0,0,1,0,1,2,8,1]}"));
  CHECK(has(g.json, "0,9,0,1]}"));
}

TEST_CASE("glb: a scene with no viewpoint or light writes neither section") {
  const Glb g = exportScene("<Shape><Box/></Shape>");
  CHECK(g.report.cameras == 0);
  CHECK(g.report.lights == 0);
  CHECK(!has(g.json, "\"cameras\""));
  CHECK(!has(g.json, "KHR_lights_punctual"));
}

// An unlit X3D surface takes its colour from the emissive colour and texture:
// an Appearance with a texture and no Material (§12.2.5), or
// UnlitMaterial.emissiveTexture. KHR_materials_unlit reads the base colour.
TEST_CASE("glb: textures on unlit surfaces are the base colour texture") {
  const Glb g = exportScene(
      "<Shape><Appearance><ImageTexture url='\"photo.jpg\"'/></Appearance>"
      "  <Box/></Shape>"
      "<Shape><Appearance><PixelTexture image='1 1 3 0x00FF00'/></Appearance>"
      "  <Box/></Shape>"
      "<Shape><Appearance><UnlitMaterial emissiveColor='1 0 0'>"
      "  <ImageTexture containerField='emissiveTexture' url='\"glow.png\"'/>"
      "  <ImageTexture containerField='normalTexture' url='\"n.png\"'/>"
      "</UnlitMaterial></Appearance><Box/></Shape>");
  CHECK(g.report.textures == 3);
  CHECK(g.report.droppedTextures == 0);
  CHECK(g.report.unmappedTextureSlots == 1); // the unlit shape's normal map
  CHECK(g.report.embeddedImages == 1);
  CHECK(g.report.externalImages == 2);
  CHECK(count(g.json, "\"baseColorTexture\"") == 3);
  CHECK(count(g.json, "KHR_materials_unlit\":{}") == 3);
  CHECK(!has(g.json, "\"emissiveTexture\""));
  CHECK(!has(g.json, "\"normalTexture\""));
  CHECK(has(g.json, "\"baseColorFactor\":[1,0,0,1]"));
  CHECK(has(g.json, "{\"uri\":\"photo.jpg\"}"));
  CHECK(has(g.json, "{\"uri\":\"glow.png\"}"));
}

TEST_CASE("glb: out-of-range material values are clamped to valid glTF") {
  const Glb g = exportScene(
      "<Shape><Appearance><Material diffuseColor='2 -1 0.5' transparency='-3'/>"
      "</Appearance><Box/></Shape>"
      "<Shape><Appearance><PhysicalMaterial metallic='4' roughness='-2'/>"
      "</Appearance><Box/></Shape>");
  CHECK(has(g.json, "\"baseColorFactor\":[1,0,0.5,1]"));
  CHECK(has(g.json, "\"metallicFactor\":1,\"roughnessFactor\":0"));
}

TEST_CASE("glb: readImage embeds PNG and JPEG files and falls back to the url") {
  sdk::GltfWriteOptions options;
  std::vector<std::string> asked;
  options.readImage = [&](const std::string &url, const std::string &) {
    asked.push_back(url);
    if (url == "a.png") return std::string("\x89PNG\r\n\x1a\n-png-bytes-", 19);
    if (url == "b.jpg") return std::string("\xFF\xD8\xFF-jpeg-bytes-");
    if (url == "liar.png") return std::string("GIF89a not what it says");
    return std::string();
  };
  const std::string shape = "<Shape><Appearance><Material/><ImageTexture url='\"";
  const Glb g = exportScene(shape + "a.png\"'/></Appearance><Box/></Shape>" +
                            shape + "b.jpg\"'/></Appearance><Sphere/></Shape>" +
                            shape + "a.png\"'/></Appearance><Cone/></Shape>" +
                            shape + "liar.png\"'/></Appearance><Cylinder/></Shape>" +
                            shape + "gone.png\"'/></Appearance><Box/></Shape>",
                            options);
  CHECK(g.report.embeddedImages == 2); // a.png once, b.jpg
  CHECK(g.report.externalImages == 2); // liar.png, gone.png
  CHECK(count(g.json, "\"mimeType\":\"image/png\"") == 1);
  CHECK(count(g.json, "\"mimeType\":\"image/jpeg\"") == 1);
  CHECK(has(g.json, "{\"uri\":\"liar.png\"}"));
  CHECK(has(g.json, "{\"uri\":\"gone.png\"}"));
  CHECK(has(g.bin, "-png-bytes-"));
  CHECK(has(g.bin, "-jpeg-bytes-"));
  CHECK(std::count(asked.begin(), asked.end(), "a.png") == 1); // read once
}
