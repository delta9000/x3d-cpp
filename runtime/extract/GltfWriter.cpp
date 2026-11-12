// GltfWriter.cpp — see GltfWriter.hpp for what is and is not exported.
#include "GltfWriter.hpp"

#include "X3DSceneBridge.hpp"
#include "x3d/nodes/GeoViewpoint.hpp"
#include "x3d/nodes/OrthoViewpoint.hpp"
#include "x3d/nodes/Viewpoint.hpp"

#include <algorithm>
#include <array>
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
  if (!std::isfinite(v) || v == 0.0) v = 0.0; // also folds -0 to 0
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

// ---- a minimal PNG encoder (stored, i.e. uncompressed, deflate blocks) ----

std::uint32_t crc32(const unsigned char *data, std::size_t n,
                    std::uint32_t crc = 0) {
  static const auto table = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      t[i] = c;
    }
    return t;
  }();
  crc = ~crc;
  for (std::size_t i = 0; i < n; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  return ~crc;
}

void be32(std::string &out, std::uint32_t v) {
  for (int shift = 24; shift >= 0; shift -= 8)
    out += static_cast<char>((v >> shift) & 0xFF);
}

void pngChunk(std::string &out, const char *type, const std::string &body) {
  be32(out, static_cast<std::uint32_t>(body.size()));
  const std::string tagged = std::string(type, 4) + body;
  out += tagged;
  be32(out, crc32(reinterpret_cast<const unsigned char *>(tagged.data()),
                  tagged.size()));
}

// `pixels` holds height rows of width*components bytes, bottom row first (the
// X3D and TexturePixels convention); PNG wants the top row first. Returns ""
// when the dimensions and the data do not agree.
std::string encodePng(std::size_t width, std::size_t height, int components,
                      const unsigned char *pixels, std::size_t size) {
  if (width == 0 || height == 0 || components < 1 || components > 4 ||
      width > 0x7FFFFFFFu || height > 0x7FFFFFFFu)
    return {};
  const std::size_t stride = width * static_cast<std::size_t>(components);
  if (size / stride < height) return {};

  std::string raw; // filter byte 0 + row, top row first
  raw.reserve((stride + 1) * height);
  for (std::size_t y = height; y-- > 0;) {
    raw += '\0';
    raw.append(reinterpret_cast<const char *>(pixels + y * stride), stride);
  }
  std::string z = "\x78\x01"; // zlib header, no compression
  std::uint32_t a = 1, b = 0;  // adler32
  for (unsigned char c : raw) {
    a = (a + c) % 65521u;
    b = (b + a) % 65521u;
  }
  for (std::size_t at = 0; at < raw.size();) {
    const std::size_t n = std::min<std::size_t>(65535, raw.size() - at);
    z += static_cast<char>(at + n == raw.size() ? 1 : 0);
    z += static_cast<char>(n & 0xFF);
    z += static_cast<char>(n >> 8);
    z += static_cast<char>(~n & 0xFF);
    z += static_cast<char>((~n >> 8) & 0xFF);
    z.append(raw, at, n);
    at += n;
  }
  be32(z, (b << 16) | a);

  static const int colorType[] = {0, 0, 4, 2, 6}; // grey, grey+a, rgb, rgba
  std::string ihdr;
  be32(ihdr, static_cast<std::uint32_t>(width));
  be32(ihdr, static_cast<std::uint32_t>(height));
  ihdr += '\x08';
  ihdr += static_cast<char>(colorType[components]);
  ihdr.append(3, '\0'); // deflate, adaptive filtering, no interlace
  std::string png("\x89PNG\r\n\x1a\n", 8);
  pngChunk(png, "IHDR", ihdr);
  pngChunk(png, "IDAT", z);
  pngChunk(png, "IEND", {});
  return png;
}

// The rotation taking -Z (where a glTF light or camera points) to `dir`.
std::array<double, 4> aimQuaternion(const SFVec3f &dir) {
  double x = dir.x, y = dir.y, z = dir.z;
  const double len = std::sqrt(x * x + y * y + z * z);
  if (!(len > 1e-12)) return {0, 0, 0, 1};
  x /= len; y /= len; z /= len;
  // from = (0,0,-1): axis = from x dir = (y, -x, 0), cos = -z.
  if (z > 0.999999) return {0, 1, 0, 0}; // opposite: half turn about Y
  const double w = 1.0 - z;              // 1 + cos
  const double n = std::sqrt(y * y + x * x + w * w);
  return {y / n, -x / n, 0.0, w / n};
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

  void add(const LightDesc &light) {
    std::string l = "{\"type\":";
    l += light.type == LightDesc::Type::Directional ? "\"directional\""
         : light.type == LightDesc::Type::Point ? "\"point\"" : "\"spot\"";
    l += ",\"color\":" + array({std::clamp(light.color.r, 0.0f, 1.0f),
                               std::clamp(light.color.g, 0.0f, 1.0f),
                               std::clamp(light.color.b, 0.0f, 1.0f)});
    l += ",\"intensity\":" + num(std::max(0.0f, light.intensity));
    if (light.type != LightDesc::Type::Directional && light.radius > 0.0f)
      l += ",\"range\":" + num(light.radius);
    if (light.type == LightDesc::Type::Spot) {
      // glTF: 0 <= inner < outer <= pi/2. X3D allows beamWidth > cutOffAngle
      // (full intensity out to the cutoff).
      const double outer =
          std::clamp(double(light.cutOffAngle), 1e-4, 1.5707963267948966);
      const double inner =
          std::clamp(double(light.beamWidth), 0.0, outer - 1e-5);
      l += ",\"spot\":{\"innerConeAngle\":" + num(inner) +
           ",\"outerConeAngle\":" + num(outer) + "}";
    }
    lights_.push_back(l + "}");

    std::string node = "{\"extensions\":{\"KHR_lights_punctual\":{\"light\":" +
                       std::to_string(lights_.size() - 1) + "}}";
    if (light.type != LightDesc::Type::Directional) {
      const SFVec3f &p = light.worldLocation;
      if (p.x != 0 || p.y != 0 || p.z != 0)
        node += ",\"translation\":" + array({p.x, p.y, p.z});
    }
    if (light.type != LightDesc::Type::Point) {
      const auto q = aimQuaternion(light.worldDirection);
      if (q[3] < 1.0)
        node += ",\"rotation\":" + array({q[0], q[1], q[2], q[3]});
    }
    extraNodes_.push_back(node + "}");
  }

  void add(const GltfCamera &camera) {
    std::string c;
    if (camera.ortho) {
      const double znear = std::max(0.0f, camera.znear);
      const double zfar =
          camera.zfar > znear ? double(camera.zfar) : std::max(10000.0, znear * 2 + 1);
      c = "{\"type\":\"orthographic\",\"orthographic\":{\"xmag\":" +
          num(camera.xmag != 0 ? std::fabs(camera.xmag) : 1.0) + ",\"ymag\":" +
          num(camera.ymag != 0 ? std::fabs(camera.ymag) : 1.0) + ",\"znear\":" +
          num(znear) + ",\"zfar\":" + num(zfar) + "}";
    } else {
      const double znear = camera.znear > 0 ? double(camera.znear) : 0.1;
      const double yfov = std::clamp(double(camera.yfov), 1e-3, 3.14);
      c = "{\"type\":\"perspective\",\"perspective\":{\"yfov\":" + num(yfov) +
          ",\"znear\":" + num(znear);
      if (camera.zfar > znear) c += ",\"zfar\":" + num(camera.zfar);
      c += "}";
    }
    if (!camera.name.empty()) c += ",\"name\":" + quote(camera.name);
    cameras_.push_back(c + "}");

    std::string node =
        "{\"camera\":" + std::to_string(cameras_.size() - 1);
    if (!camera.name.empty()) node += ",\"name\":" + quote(camera.name);
    if (camera.cameraToWorld.m != Mat4::identity().m) {
      std::vector<std::string> cells;
      for (float v : camera.cameraToWorld.m) cells.push_back(num(v));
      node += ",\"matrix\":[" + join(cells) + "]";
    }
    extraNodes_.push_back(node + "}");
  }

  std::string finish() {
    report_.nodes = nodes_.size();
    report_.lights = lights_.size();
    report_.cameras = cameras_.size();
    nodes_.insert(nodes_.end(), extraNodes_.begin(), extraNodes_.end());
    report_.meshes = meshes_.size();
    report_.materials = materials_.size();
    report_.textures = textures_.size();

    std::vector<std::string> top;
    top.push_back("\"asset\":{\"version\":\"2.0\",\"generator\":" +
                  quote(options_.generator) + "}");
    std::vector<std::string> used;
    if (usesUnlit_) used.push_back("\"KHR_materials_unlit\"");
    if (!lights_.empty()) used.push_back("\"KHR_lights_punctual\"");
    if (!used.empty())
      top.push_back("\"extensionsUsed\":[" + join(used) + "]");
    if (!lights_.empty())
      top.push_back("\"extensions\":{\"KHR_lights_punctual\":{\"lights\":[" +
                    join(lights_) + "]}}");
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
    section("cameras", cameras_);
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
    auto intern = [](std::vector<std::string> &list, const std::string &v) {
      auto it = std::find(list.begin(), list.end(), v);
      if (it == list.end()) it = list.insert(list.end(), v);
      return static_cast<int>(it - list.begin());
    };
    int image = -1;
    if (!ref.hasTexCoordGen && !acc.texcoords.empty()) {
      const std::string url =
          ref.source == TextureRef::Source::Url ? imageUrl(ref.url) : "";
      if (!url.empty()) {
        image = intern(images_, "{\"uri\":" + quote(url) + "}");
      } else if (ref.source == TextureRef::Source::Inline) {
        const SFImage &img = ref.inlinePixels;
        if (img.width > 0 && img.height > 0)
          image = embed(encodePng(static_cast<std::size_t>(img.width),
                                  static_cast<std::size_t>(img.height),
                                  img.numComponents, img.data.data(),
                                  img.data.size()));
      } else if (ref.source == TextureRef::Source::Url &&
                 ref.resolvedPixels.ready() &&
                 ref.resolvedPixels.pixels->layers == 1) {
        // No url glTF can reference (a KTX, GIF, data: url, ...), but the
        // host decoded it: embed the decoded surface.
        const TexturePixels &px = *ref.resolvedPixels.pixels;
        auto cached = decodedImages_.find(&px);
        if (cached != decodedImages_.end()) {
          image = cached->second;
        } else {
          image = embed(encodePng(px.width, px.height, 4, px.rgba.data(),
                                  px.rgba.size()));
          decodedImages_.emplace(&px, image);
        }
      }
    }
    if (image < 0) {
      ++report_.droppedTextures;
      return {};
    }
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

  // Store PNG bytes in the buffer and return its image index; identical
  // images are stored once. -1 for an empty (unencodable) image.
  int embed(const std::string &png) {
    if (png.empty()) return -1;
    auto found = embedded_.find(png);
    if (found != embedded_.end()) return found->second;
    while (bin_.size() % 4) bin_ += '\0';
    const std::size_t offset = bin_.size();
    bin_ += png;
    while (bin_.size() % 4) bin_ += '\0';
    views_.push_back("{\"buffer\":0,\"byteOffset\":" + std::to_string(offset) +
                     ",\"byteLength\":" + std::to_string(png.size()) + "}");
    images_.push_back("{\"bufferView\":" + std::to_string(views_.size() - 1) +
                      ",\"mimeType\":\"image/png\"}");
    ++report_.embeddedImages;
    return embedded_.emplace(png, static_cast<int>(images_.size()) - 1)
        .first->second;
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
      samplers_, accessors_, views_, lights_, cameras_, extraNodes_;
  std::unordered_map<std::string, int> embedded_;
  std::unordered_map<const TexturePixels *, int> decodedImages_;
  std::unordered_map<const MeshData *, MeshAccessors> meshAccessors_;
  std::map<std::pair<const MeshData *, int>, int> meshIndex_;
  std::unordered_map<std::string, int> materialIndex_;
  bool usesUnlit_ = false;
};

} // namespace

std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfSceneExtras &extras,
                     const GltfWriteOptions &options, GltfWriteReport *report) {
  GltfWriteReport local;
  GltfWriteReport &r = report ? *report : local;
  const std::size_t skippedViewpoints = r.skippedViewpoints;
  r = GltfWriteReport{};
  if (report) r.skippedViewpoints = skippedViewpoints;
  Writer writer(options, r);
  for (const RenderItem *item : items)
    if (item) writer.add(*item);
  for (const LightDesc &light : extras.lights) writer.add(light);
  for (const GltfCamera &camera : extras.cameras) writer.add(camera);
  return writer.finish();
}

GltfSceneExtras gltfExtrasFrom(const SceneExtractor &extractor,
                               GltfWriteReport *report) {
  GltfSceneExtras extras;
  extras.lights = extractor.lights();

  const X3DExecutionContext &ctx = extractor.context();
  const X3DNode *bound = ctx.boundViewpoint();
  std::size_t skipped = 0;
  x3d::runtime::detail::forEachNode(extractor.scene(), [&](X3DNode *n) {
    if (dynamic_cast<const x3d::nodes::GeoViewpoint *>(n)) {
      ++skipped;
      return;
    }
    const bool ortho = dynamic_cast<const x3d::nodes::OrthoViewpoint *>(n);
    if (!ortho && !dynamic_cast<const x3d::nodes::Viewpoint *>(n)) return;

    GltfCamera cam;
    cam.name = geombounds::getField<std::string>(*n, "description", {});
    if (cam.name.empty()) cam.name = n->getDEF();
    cam.cameraToWorld =
        ctx.worldOf(n) *
        Mat4::translation(geombounds::getVec3fLenient(*n, "position", {0, 0, 10})) *
        Mat4::rotation(
            geombounds::getField<SFRotation>(*n, "orientation", {0, 0, 1, 0}));
    const float nearDistance = geombounds::getField<float>(*n, "nearDistance", -1.0f);
    const float farDistance = geombounds::getField<float>(*n, "farDistance", -1.0f);
    cam.znear = nearDistance > 0 ? nearDistance : (ortho ? 0.0f : 0.1f);
    cam.zfar = farDistance > 0 ? farDistance : 0.0f;
    cam.ortho = ortho;
    if (ortho) {
      const auto fov = geombounds::getField<std::vector<float>>(
          *n, "fieldOfView", {-1, -1, 1, 1});
      if (fov.size() >= 4) {
        cam.xmag = (fov[2] - fov[0]) * 0.5f;
        cam.ymag = (fov[3] - fov[1]) * 0.5f;
      }
    } else {
      cam.yfov = geombounds::getField<float>(*n, "fieldOfView", 0.7854f);
    }
    if (n == bound)
      extras.cameras.insert(extras.cameras.begin(), std::move(cam));
    else
      extras.cameras.push_back(std::move(cam));
  });
  if (report) report->skippedViewpoints = skipped;
  return extras;
}

std::string writeGlb(const SceneExtractor &extractor,
                     const std::vector<RenderItemId> &ids,
                     const GltfWriteOptions &options, GltfWriteReport *report) {
  std::vector<const RenderItem *> items;
  items.reserve(ids.size());
  for (RenderItemId id : ids) items.push_back(&extractor.item(id));
  GltfWriteReport local;
  GltfWriteReport &r = report ? *report : local;
  const GltfSceneExtras extras = gltfExtrasFrom(extractor, &r);
  return writeGlb(items, extras, options, &r);
}

} // namespace x3d::runtime::extract
