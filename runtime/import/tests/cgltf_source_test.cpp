// cgltf_source_test.cpp — the built-in glTF backend: the committed twobox.glb
// and small .gltf files written by the tests (buffers as data: URIs).
#include "AssetImport.hpp"
#include "gltf/CgltfSource.hpp"

#include "doctest/doctest.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace imp = x3d::runtime::importer;
namespace fs = std::filesystem;

namespace {

std::string base64(const std::vector<std::uint8_t> &in) {
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  for (std::size_t i = 0; i < in.size(); i += 3) {
    const std::uint32_t n = (std::uint32_t(in[i]) << 16) | (i + 1 < in.size() ? std::uint32_t(in[i + 1]) << 8 : 0) |
                            (i + 2 < in.size() ? std::uint32_t(in[i + 2]) : 0);
    out += t[(n >> 18) & 63];
    out += t[(n >> 12) & 63];
    out += i + 1 < in.size() ? t[(n >> 6) & 63] : '=';
    out += i + 2 < in.size() ? t[n & 63] : '=';
  }
  return out;
}

/// Little-endian float32 / uint16 packing into one buffer.
struct Buffer {
  std::vector<std::uint8_t> bytes;
  std::size_t floats(std::initializer_list<float> v) {
    const std::size_t at = bytes.size();
    for (float f : v) {
      std::uint8_t b[4];
      std::memcpy(b, &f, 4);
      bytes.insert(bytes.end(), b, b + 4);
    }
    return at;
  }
  std::size_t shorts(std::initializer_list<std::uint16_t> v) {
    const std::size_t at = bytes.size();
    for (std::uint16_t s : v) {
      bytes.push_back(static_cast<std::uint8_t>(s & 0xff));
      bytes.push_back(static_cast<std::uint8_t>(s >> 8));
    }
    while (bytes.size() % 4) bytes.push_back(0);
    return at;
  }
  std::string uri() const { return "data:application/octet-stream;base64," + base64(bytes); }
};

std::string write(const std::string &name, const std::string &json) {
  const fs::path dir = fs::temp_directory_path() / "x3d_import_cgltf";
  fs::create_directories(dir);
  const fs::path p = dir / name;
  std::ofstream(p) << json;
  return p.string();
}

/// A one-triangle glTF: POSITION (0), TEXCOORD_0 (1), indices (2), plus
/// whatever accessors `extraAccessors` adds (starting at index 3).
std::string triangleGltf(Buffer &b, const std::string &primitiveExtra, const std::string &rest,
                         const std::string &extraAccessors = "", const std::string &extraViews = "") {
  const std::size_t pos = b.floats({0, 0, 0, 1, 0, 0, 0, 1, 0});
  const std::size_t uv = b.floats({0, 0, 1, 0, 0, 1});
  const std::size_t idx = b.shorts({0, 1, 2});
  return std::string("{\"asset\":{\"version\":\"2.0\"},\"buffers\":[{\"byteLength\":") +
         std::to_string(b.bytes.size()) + ",\"uri\":\"" + b.uri() +
         "\"}],\"bufferViews\":[{\"buffer\":0,\"byteOffset\":" + std::to_string(pos) +
         ",\"byteLength\":36},"
         "{\"buffer\":0,\"byteOffset\":" + std::to_string(uv) +
         ",\"byteLength\":24},{\"buffer\":0,\"byteOffset\":" + std::to_string(idx) + ",\"byteLength\":6}" +
         extraViews +
         "],\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\","
         "\"min\":[0,0,0],\"max\":[1,1,0]},{\"bufferView\":1,\"componentType\":5126,\"count\":3,"
         "\"type\":\"VEC2\"},{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}" +
         extraAccessors +
         "],\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"TEXCOORD_0\":1},\"indices\":2" +
         primitiveExtra + "}]}],\"nodes\":[{\"mesh\":0,\"name\":\"tri\"}],\"scenes\":[{\"nodes\":[0]}],\"scene\":0" +
         rest + "}";
}

} // namespace

TEST_CASE("cgltf: twobox.glb carries meshes, material, texture, camera and light") {
  imp::CgltfSource src;
  const imp::ImportScene s = src.load(std::string(X3D_IMPORT_FIXTURES) + "/twobox.glb");
  REQUIRE(s.meshes.size() == 1);
  CHECK(s.meshes[0].indices.size() == 36);
  CHECK(s.meshes[0].positions.size() == 24);
  CHECK(s.meshes[0].normals.size() == 24);
  REQUIRE(s.meshes[0].uvSets.size() == 1);
  CHECK(s.meshes[0].uvSets[0].size() == 24);
  REQUIRE(s.materials.size() == 1);
  REQUIRE(s.materials[0].pbr.has_value());
  CHECK(s.materials[0].pbr->metallic == doctest::Approx(0.0f));
  CHECK(s.materials[0].pbr->roughness == doctest::Approx(0.6f));
  CHECK(s.materials[0].textures.baseColor.has_value());
  CHECK(s.embedded.size() == 1);
  CHECK(s.nodes.size() == 4);
  REQUIRE(s.roots.size() == 1);
  CHECK(s.nodes[static_cast<std::size_t>(s.roots[0])].childIndices.size() == 3);
  REQUIRE(s.cameras.size() == 1);
  CHECK(s.cameras[0].yfov == doctest::Approx(0.7853982f));
  REQUIRE(s.lights.size() == 1);
  CHECK(s.lights[0].kind == imp::ImportLight::Kind::Point);
  CHECK(s.lights[0].range == doctest::Approx(20.0f));
}

TEST_CASE("cgltf: texture coordinates flip to X3D's bottom-left origin") {
  Buffer b;
  const imp::ImportScene s = imp::CgltfSource{}.load(write("flip.gltf", triangleGltf(b, "", "")));
  REQUIRE(s.meshes.size() == 1);
  const auto &uv = s.meshes[0].uvSets.at(0);
  CHECK(uv[0].y == doctest::Approx(1.0f));
  CHECK(uv[2].y == doctest::Approx(0.0f));
  CHECK(uv[1].x == doctest::Approx(1.0f));
}

TEST_CASE("cgltf: an unsupported required extension is refused") {
  Buffer b;
  const std::string path =
      write("required.gltf", triangleGltf(b, "", ",\"extensionsUsed\":[\"KHR_draco_mesh_compression\"],"
                                                 "\"extensionsRequired\":[\"KHR_draco_mesh_compression\"]"));
  CHECK_THROWS_WITH_AS(imp::CgltfSource{}.load(path), doctest::Contains("KHR_draco_mesh_compression"),
                       std::runtime_error);
}

TEST_CASE("cgltf: KHR_texture_transform is baked into a texture coordinate set") {
  Buffer b;
  const std::string json = triangleGltf(
      b, ",\"material\":0",
      ",\"extensionsUsed\":[\"KHR_texture_transform\"],"
      "\"images\":[{\"uri\":\"tex.png\"}],\"textures\":[{\"source\":0}],"
      "\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0,"
      "\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.5,0],\"scale\":[2,2]}}}}}]");
  const imp::ImportScene s = imp::CgltfSource{}.load(write("transform.gltf", json));
  REQUIRE(s.materials.size() == 1);
  REQUIRE(s.materials[0].textures.baseColor);
  const int set = s.materials[0].textures.baseColor->uvSet;
  CHECK(set == 1);
  REQUIRE(s.meshes[0].uvSets.size() == 2);
  // glTF (u, v) = (1, 0) -> (2*1 + 0.5, 2*0) = (2.5, 0) -> X3D t = 1 - 0 = 1.
  CHECK(s.meshes[0].uvSets[1][1].x == doctest::Approx(2.5f));
  CHECK(s.meshes[0].uvSets[1][1].y == doctest::Approx(1.0f));
  CHECK(*s.materials[0].textures.baseColor->externalPath == "tex.png");
}

TEST_CASE("cgltf: KHR_animation_pointer channels target material properties") {
  Buffer b;
  const std::size_t t = b.floats({0, 1});
  const std::size_t c = b.floats({1, 0, 0, 1, 0, 0, 1, 1});
  const std::string json = triangleGltf(
      b, ",\"material\":0",
      ",\"extensionsUsed\":[\"KHR_animation_pointer\"],\"materials\":[{\"name\":\"m\"}],"
      "\"animations\":[{\"name\":\"fade\",\"samplers\":[{\"input\":3,\"output\":4}],"
      "\"channels\":[{\"sampler\":0,\"target\":{\"path\":\"pointer\",\"extensions\":"
      "{\"KHR_animation_pointer\":{\"pointer\":\"/materials/0/pbrMetallicRoughness/baseColorFactor\"}}}}]}]",
      ",{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\",\"min\":[0],\"max\":[1]},"
      "{\"bufferView\":4,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"}",
      ",{\"buffer\":0,\"byteOffset\":" + std::to_string(t) + ",\"byteLength\":8},{\"buffer\":0,\"byteOffset\":" +
          std::to_string(c) + ",\"byteLength\":32}");
  const imp::ImportScene s = imp::CgltfSource{}.load(write("pointer.gltf", json));
  REQUIRE(s.animations.size() == 1);
  REQUIRE(s.animations[0].channels.size() == 1);
  const auto &ch = s.animations[0].channels[0];
  CHECK(ch.path == imp::AnimationChannel::Path::MaterialBaseColor);
  CHECK(ch.target == 0);
  CHECK(ch.times.size() == 2);
  CHECK(ch.values.size() == 8);
}
