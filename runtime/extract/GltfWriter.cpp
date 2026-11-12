// GltfWriter.cpp — see GltfWriter.hpp for what is and is not exported.
#include "GltfWriter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <unordered_map>

namespace x3d::runtime::extract {
namespace {

// glTF constants.
constexpr int kFloat = 5126, kUint32 = 5125;
constexpr int kArrayBuffer = 34962, kElementArrayBuffer = 34963;
constexpr int kRepeat = 10497, kClampToEdge = 33071;

std::string num(double v) {
  if (!std::isfinite(v)) v = 0.0;
  char buf[40];
  std::snprintf(buf, sizeof buf, "%.9g", v);
  return buf;
}

std::string quote(const std::string &s) {
  std::string out = "\"";
  for (unsigned char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x", c);
          out += buf;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out + "\"";
}

std::string join(const std::vector<std::string> &parts) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i) out += ",";
    out += parts[i];
  }
  return out;
}

std::string array(std::initializer_list<double> values) {
  std::vector<std::string> parts;
  for (double v : values) parts.push_back(num(v));
  return "[" + join(parts) + "]";
}

// The first url glTF core can reference: PNG or JPEG by extension.
std::string imageUrl(const MFString &urls) {
  for (const std::string &u : urls) {
    std::string path = u.substr(0, u.find_first_of("?#"));
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    for (const char *ext : {".png", ".jpg", ".jpeg"}) {
      const std::size_t n = std::strlen(ext);
      if (path.size() > n && path.compare(path.size() - n, n, ext) == 0)
        return u;
    }
  }
  return {};
}

struct MeshAccessors {
  int position = -1, normal = -1, color = -1, indices = -1;
  std::vector<int> texcoords;
  std::size_t triangles = 0;
};

class Writer {
public:
  Writer(const GltfWriteOptions &options, GltfWriteReport &report)
      : options_(options), report_(report) {}

  void add(const RenderItem &item) {
    if (item.geometry_ext.kind == Geometry::Kind::Packed) {
      ++report_.skippedPacked;
      return;
    }
    const MeshData &mesh = *item.mesh;
    if (mesh.positions.empty() || mesh.indices.empty()) {
      ++report_.skippedEmpty;
      return;
    }
    if (item.skin) ++report_.staticSkins;
    if (item.shaderProgram) ++report_.droppedShaders;

    const MeshAccessors &acc = accessorsFor(item.mesh);
    const int material = materialFor(item.material, mesh, acc);
    const auto key = std::make_pair(item.mesh.get(), material);
    auto found = meshIndex_.find(key);
    if (found == meshIndex_.end()) {
      std::vector<std::string> attrs{"\"POSITION\":" +
                                     std::to_string(acc.position)};
      if (acc.normal >= 0)
        attrs.push_back("\"NORMAL\":" + std::to_string(acc.normal));
      for (std::size_t i = 0; i < acc.texcoords.size(); ++i)
        attrs.push_back("\"TEXCOORD_" + std::to_string(i) +
                        "\":" + std::to_string(acc.texcoords[i]));
      if (acc.color >= 0)
        attrs.push_back("\"COLOR_0\":" + std::to_string(acc.color));
      const int mode = mesh.topology == Topology::Triangles ? 4
                       : mesh.topology == Topology::Lines ? 1 : 0;
      meshes_.push_back("{\"primitives\":[{\"attributes\":{" + join(attrs) +
                        "},\"indices\":" + std::to_string(acc.indices) +
                        ",\"material\":" + std::to_string(material) +
                        ",\"mode\":" + std::to_string(mode) + "}]}");
      found = meshIndex_.emplace(key, static_cast<int>(meshes_.size()) - 1)
                  .first;
    }

    std::string node = "{\"mesh\":" + std::to_string(found->second);
    std::string name;
    for (const X3DNode *n : item.path)
      if (n && !n->getDEF().empty()) name = n->getDEF();
    if (!name.empty()) node += ",\"name\":" + quote(name);
    const auto &m = item.worldTransform.m;
    if (item.worldTransform.m != Mat4::identity().m) {
      std::vector<std::string> cells;
      for (float v : m) cells.push_back(num(v));
      node += ",\"matrix\":[" + join(cells) + "]";
    }
    nodes_.push_back(node + "}");
    report_.triangles += acc.triangles;
  }

  std::string finish() {
    report_.nodes = nodes_.size();
    report_.meshes = meshes_.size();
    report_.materials = materials_.size();
    report_.textures = textures_.size();

    std::vector<std::string> top;
    top.push_back("\"asset\":{\"version\":\"2.0\",\"generator\":" +
                  quote(options_.generator) + "}");
    if (usesUnlit_)
      top.push_back("\"extensionsUsed\":[\"KHR_materials_unlit\"]");
    std::vector<std::string> roots;
    for (std::size_t i = 0; i < nodes_.size(); ++i)
      roots.push_back(std::to_string(i));
    top.push_back("\"scene\":0");
    top.push_back(roots.empty() ? "\"scenes\":[{}]"
                                : "\"scenes\":[{\"nodes\":[" + join(roots) +
                                      "]}]");
    auto section = [&](const char *name, const std::vector<std::string> &v) {
      if (!v.empty())
        top.push_back(std::string("\"") + name + "\":[" + join(v) + "]");
    };
    section("nodes", nodes_);
    section("meshes", meshes_);
    section("materials", materials_);
    section("textures", textures_);
    section("images", images_);
    section("samplers", samplers_);
    section("accessors", accessors_);
    section("bufferViews", views_);
    if (!bin_.empty())
      top.push_back("\"buffers\":[{\"byteLength\":" +
                    std::to_string(bin_.size()) + "}]");
    std::string json = "{" + join(top) + "}";
    while (json.size() % 4) json += ' ';

    std::string out;
    auto u32 = [&](std::size_t v) {
      const std::uint32_t x = static_cast<std::uint32_t>(v);
      const unsigned char b[4] = {
          static_cast<unsigned char>(x), static_cast<unsigned char>(x >> 8),
          static_cast<unsigned char>(x >> 16),
          static_cast<unsigned char>(x >> 24)};
      out.append(reinterpret_cast<const char *>(b), 4);
    };
    const std::size_t total =
        12 + 8 + json.size() + (bin_.empty() ? 0 : 8 + bin_.size());
    out.reserve(total);
    u32(0x46546C67); // "glTF"
    u32(2);
    u32(total);
    u32(json.size());
    u32(0x4E4F534A); // "JSON"
    out += json;
    if (!bin_.empty()) {
      u32(bin_.size());
      u32(0x004E4942); // "BIN\0"
      out += bin_;
    }
    return out;
  }

private:
  // Append little-endian floats / uint32s as a 4-byte-aligned buffer view.
  int view(const void *data, std::size_t bytes, int target) {
    while (bin_.size() % 4) bin_ += '\0';
    const std::size_t offset = bin_.size();
    bin_.append(static_cast<const char *>(data), bytes);
    while (bin_.size() % 4) bin_ += '\0';
    views_.push_back("{\"buffer\":0,\"byteOffset\":" + std::to_string(offset) +
                     ",\"byteLength\":" + std::to_string(bytes) +
                     ",\"target\":" + std::to_string(target) + "}");
    return static_cast<int>(views_.size()) - 1;
  }

  int floatAccessor(const std::vector<float> &data, int components,
                    const char *type, bool bounds) {
    const int v = view(data.data(), data.size() * sizeof(float), kArrayBuffer);
    const std::size_t count = data.size() / static_cast<std::size_t>(components);
    std::string a = "{\"bufferView\":" + std::to_string(v) +
                    ",\"componentType\":" + std::to_string(kFloat) +
                    ",\"count\":" + std::to_string(count) + ",\"type\":\"" +
                    type + "\"";
    if (bounds && count) {
      std::vector<std::string> lo, hi;
      for (int c = 0; c < components; ++c) {
        float mn = data[static_cast<std::size_t>(c)], mx = mn;
        for (std::size_t i = 0; i < count; ++i) {
          const float x = data[i * static_cast<std::size_t>(components) +
                               static_cast<std::size_t>(c)];
          mn = std::min(mn, x);
          mx = std::max(mx, x);
        }
        lo.push_back(num(mn));
        hi.push_back(num(mx));
      }
      a += ",\"min\":[" + join(lo) + "],\"max\":[" + join(hi) + "]";
    }
    accessors_.push_back(a + "}");
    return static_cast<int>(accessors_.size()) - 1;
  }

  const MeshAccessors &accessorsFor(const MeshRef &ref) {
    auto found = meshAccessors_.find(ref.get());
    if (found != meshAccessors_.end()) return found->second;
    const MeshData &mesh = *ref;
    const std::size_t n = mesh.positions.size();
    MeshAccessors acc;

    std::vector<float> flat;
    flat.reserve(n * 3);
    for (const SFVec3f &p : mesh.positions) {
      // Accessor values must be finite.
      flat.push_back(std::isfinite(p.x) ? p.x : 0.0f);
      flat.push_back(std::isfinite(p.y) ? p.y : 0.0f);
      flat.push_back(std::isfinite(p.z) ? p.z : 0.0f);
    }
    acc.position = floatAccessor(flat, 3, "VEC3", true);

    if (mesh.hasNormals && mesh.normals.size() == n) {
      flat.clear();
      for (const SFVec3f &v : mesh.normals) {
        // glTF normals must be unit length.
        const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len > 1e-20f && std::isfinite(len)) {
          flat.push_back(v.x / len);
          flat.push_back(v.y / len);
          flat.push_back(v.z / len);
        } else {
          flat.insert(flat.end(), {0.0f, 0.0f, 1.0f});
        }
      }
      acc.normal = floatAccessor(flat, 3, "VEC3", false);
    }

    auto texcoords = [&](const std::vector<SFVec2f> &uv) {
      flat.clear();
      for (const SFVec2f &t : uv) {
        flat.push_back(std::isfinite(t.x) ? t.x : 0.0f);
        // X3D puts t = 0 at the bottom of the image; glTF at the top.
        flat.push_back(std::isfinite(t.y) ? 1.0f - t.y : 0.0f);
      }
      acc.texcoords.push_back(floatAccessor(flat, 2, "VEC2", false));
    };
    if (mesh.texcoords.size() == n) {
      texcoords(mesh.texcoords);
      // Further UV sets; set 0 is `texcoords`. Stop at the first set that
      // does not match, since TEXCOORD_n must be contiguous.
      for (std::size_t s = 1; s < mesh.texcoordSets.size(); ++s) {
        if (mesh.texcoordSets[s].size() != n) break;
        texcoords(mesh.texcoordSets[s]);
      }
    }

    if (mesh.hasColors && mesh.colors.size() == n) {
      flat.clear();
      for (const SFColorRGBA &c : mesh.colors)
        flat.insert(flat.end(), {c.r, c.g, c.b, c.a});
      acc.color = floatAccessor(flat, 4, "VEC4", false);
    }

    // Indices: drop any primitive that references a missing vertex, and turn
    // clockwise triangles round (glTF front faces are always CCW).
    const std::size_t per = mesh.topology == Topology::Triangles ? 3
                            : mesh.topology == Topology::Lines ? 2 : 1;
    std::vector<std::uint32_t> idx;
    idx.reserve(mesh.indices.size());
    for (std::size_t i = 0; i + per <= mesh.indices.size(); i += per) {
      bool ok = true;
      for (std::size_t k = 0; k < per; ++k)
        ok = ok && mesh.indices[i + k] < n;
      if (!ok) continue;
      for (std::size_t k = 0; k < per; ++k) idx.push_back(mesh.indices[i + k]);
      if (per == 3 && !mesh.ccw)
        std::swap(idx[idx.size() - 1], idx[idx.size() - 2]);
    }
    const int v = view(idx.data(), idx.size() * sizeof(std::uint32_t),
                       kElementArrayBuffer);
    accessors_.push_back("{\"bufferView\":" + std::to_string(v) +
                         ",\"componentType\":" + std::to_string(kUint32) +
                         ",\"count\":" + std::to_string(idx.size()) +
                         ",\"type\":\"SCALAR\"}");
    acc.indices = static_cast<int>(accessors_.size()) - 1;
    acc.triangles = per == 3 ? idx.size() / 3 : 0;
    return meshAccessors_.emplace(ref.get(), std::move(acc)).first->second;
  }

  // A textureInfo object for `ref`, or "" (counted) when it cannot be written.
  std::string textureInfo(const TextureRef &ref, const MeshAccessors &acc,
                          const std::string &extra = {}) {
    const std::string url =
        ref.source == TextureRef::Source::Url ? imageUrl(ref.url) : "";
    if (url.empty() || ref.hasTexCoordGen || acc.texcoords.empty()) {
      ++report_.droppedTextures;
      return {};
    }
    auto intern = [](std::vector<std::string> &list, const std::string &v) {
      auto it = std::find(list.begin(), list.end(), v);
      if (it == list.end()) it = list.insert(list.end(), v);
      return static_cast<int>(it - list.begin());
    };
    const int image = intern(images_, "{\"uri\":" + quote(url) + "}");
    const int sampler = intern(
        samplers_,
        "{\"wrapS\":" + std::to_string(ref.repeatS ? kRepeat : kClampToEdge) +
            ",\"wrapT\":" + std::to_string(ref.repeatT ? kRepeat : kClampToEdge) +
            "}");
    const int texture =
        intern(textures_, "{\"sampler\":" + std::to_string(sampler) +
                              ",\"source\":" + std::to_string(image) + "}");
    std::string info = "{\"index\":" + std::to_string(texture);
    if (ref.channel > 0 &&
        static_cast<std::size_t>(ref.channel) < acc.texcoords.size())
      info += ",\"texCoord\":" + std::to_string(ref.channel);
    return info + extra + "}";
  }

  int materialFor(const MaterialDesc &m, const MeshData &mesh,
                  const MeshAccessors &acc) {
    using Slot = TextureRef::Slot;
    if (m.backMaterial) ++report_.droppedBackMaterials;
    // §11.2.2.5: lines and points without normals are unlit and take the
    // emissive colour.
    const bool unlit =
        m.model == MaterialModel::Unlit ||
        (mesh.topology != Topology::Triangles && acc.normal < 0);
    SFColorRGBA base = unlit ? m.unlitGeometryRGBA() : m.toRGBA();
    // A per-vertex Color replaces the material colour (COLOR_0 multiplies
    // baseColorFactor, so the factor's RGB goes to white).
    if (acc.color >= 0) base.r = base.g = base.b = 1.0f;
    double metallic = 0.0, roughness = 1.0;
    if (m.model == MaterialModel::Physical) {
      metallic = m.physical.metallic;
      roughness = m.physical.roughness;
    } else if (m.model == MaterialModel::Phong) {
      // Blinn-Phong exponent (shininess * 128) to perceptual roughness.
      const double exponent = std::max(0.0, double(m.phong.shininess)) * 128.0;
      roughness = std::pow(2.0 / (exponent + 2.0), 0.25);
    }

    std::vector<std::string> pbr, fields;
    pbr.push_back("\"baseColorFactor\":" +
                  array({base.r, base.g, base.b, base.a}));
    pbr.push_back("\"metallicFactor\":" + num(metallic));
    pbr.push_back("\"roughnessFactor\":" + num(roughness));
    auto slot = [&](std::vector<std::string> &into, const char *key,
                    const TextureRef &ref, const std::string &extra = {}) {
      const std::string info = textureInfo(ref, acc, extra);
      if (!info.empty())
        into.push_back(std::string("\"") + key + "\":" + info);
    };
    bool emissiveTexture = false;
    for (const TextureRef &ref : m.textures) {
      const bool phong = m.model == MaterialModel::Phong;
      if (ref.slot == Slot::BaseColor || (phong && ref.slot == Slot::Diffuse)) {
        slot(pbr, "baseColorTexture", ref);
      } else if (unlit) {
        ++report_.droppedTextures;
      } else if (ref.slot == Slot::Normal) {
        slot(fields, "normalTexture", ref, ",\"scale\":" + num(m.normalScale));
      } else if (ref.slot == Slot::Emissive) {
        const std::size_t before = fields.size();
        slot(fields, "emissiveTexture", ref);
        emissiveTexture = fields.size() > before;
      } else if (ref.slot == Slot::Occlusion) {
        const double strength = phong ? m.phong.occlusionStrength
                                      : m.physical.occlusionStrength;
        slot(fields, "occlusionTexture", ref,
             ",\"strength\":" + num(std::clamp(strength, 0.0, 1.0)));
      } else if (ref.slot == Slot::MetallicRoughness && !phong) {
        slot(pbr, "metallicRoughnessTexture", ref);
      } else {
        ++report_.droppedTextures; // specular, shininess, ambient
      }
    }
    fields.insert(fields.begin(),
                  "\"pbrMetallicRoughness\":{" + join(pbr) + "}");
    if (!unlit) {
      // glTF multiplies the emissive texture by emissiveFactor, whose default
      // is black; an X3D emissiveTexture with a black emissiveColor still
      // shows, so lift the factor to white in that case.
      SFColor e = m.emissive;
      if (emissiveTexture && e.r == 0 && e.g == 0 && e.b == 0) e = {1, 1, 1};
      if (e.r != 0 || e.g != 0 || e.b != 0)
        fields.push_back("\"emissiveFactor\":" +
                         array({std::clamp(e.r, 0.0f, 1.0f),
                                std::clamp(e.g, 0.0f, 1.0f),
                                std::clamp(e.b, 0.0f, 1.0f)}));
    }
    if (m.alphaMode == AlphaMode::Mask) {
      fields.push_back("\"alphaMode\":\"MASK\"");
      fields.push_back("\"alphaCutoff\":" + num(m.alphaCutoff));
    } else if (m.alphaMode == AlphaMode::Blend || base.a < 1.0f) {
      fields.push_back("\"alphaMode\":\"BLEND\"");
    }
    if (m.doubleSided || !mesh.solid)
      fields.push_back("\"doubleSided\":true");
    if (unlit) {
      fields.push_back("\"extensions\":{\"KHR_materials_unlit\":{}}");
      usesUnlit_ = true;
    }
    const std::string json = "{" + join(fields) + "}";
    auto it = materialIndex_.find(json);
    if (it == materialIndex_.end()) {
      materials_.push_back(json);
      it = materialIndex_
               .emplace(json, static_cast<int>(materials_.size()) - 1)
               .first;
    }
    return it->second;
  }

  const GltfWriteOptions &options_;
  GltfWriteReport &report_;
  std::string bin_;
  std::vector<std::string> nodes_, meshes_, materials_, textures_, images_,
      samplers_, accessors_, views_;
  std::unordered_map<const MeshData *, MeshAccessors> meshAccessors_;
  std::map<std::pair<const MeshData *, int>, int> meshIndex_;
  std::unordered_map<std::string, int> materialIndex_;
  bool usesUnlit_ = false;
};

} // namespace

std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfWriteOptions &options, GltfWriteReport *report) {
  GltfWriteReport local;
  GltfWriteReport &r = report ? *report : local;
  r = GltfWriteReport{};
  Writer writer(options, r);
  for (const RenderItem *item : items)
    if (item) writer.add(*item);
  return writer.finish();
}

} // namespace x3d::runtime::extract
