// GltfWriter.cpp — see GltfWriter.hpp for what is and is not exported.
#include "GltfWriter.hpp"

#include "Interpolation.hpp"
#include "NurbsInterpolatorSystem.hpp"
#include "SplineInterpolation.hpp"
#include "X3DProtoClone.hpp"
#include "HAnimMotionSystem.hpp"

#include "TransformSystem.hpp"
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
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime::extract {
namespace {

// glTF constants.
constexpr int kFloat = 5126, kUint32 = 5125;
constexpr int kArrayBuffer = 34962, kElementArrayBuffer = 34963;
constexpr int kRepeat = 10497, kClampToEdge = 33071;
constexpr float kMinRange = 1e-6f; // glTF light range must be positive

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

// Axis-angle to a unit quaternion (x, y, z, w); a zero axis is no rotation.
std::array<double, 4> quaternion(const SFRotation &r) {
  const double len = std::sqrt(double(r.x) * r.x + double(r.y) * r.y +
                               double(r.z) * r.z);
  if (!(len > 1e-12) || !std::isfinite(len) || !std::isfinite(r.angle))
    return {0, 0, 0, 1};
  const double s = std::sin(r.angle * 0.5) / len;
  return {r.x * s, r.y * s, r.z * s, std::cos(r.angle * 0.5)};
}

bool isIdentity(const Mat4 &m) { return m.m == Mat4::identity().m; }

// An SFNode field's value, or null when `n` has no SFNode field `name` (some
// nodes have an MFNode field of that name, e.g. RigidBody.geometry).
std::shared_ptr<X3DNode> sfNode(const X3DNode &n, const char *name) {
  for (const FieldInfo &f : n.fields())
    if (f.x3dName == name && f.type == X3DFieldType::SFNode && f.get)
      return geombounds::getNode(n, name);
  return nullptr;
}

// ---- interpolator sampling ----

// What an interpolator emits, as glTF sees it: a rotation is a quaternion.
enum class SampleKind { None, Scalar, Color, Vec3, Rotation };

// Samples of one interpolator: fractions (the X3D key domain) and values.
struct Samples {
  SampleKind kind = SampleKind::None;
  std::vector<float> fractions;
  std::vector<float> values;
};

// Steps per key interval when glTF's LINEAR between keys is not what the
// interpolator does (HSV colours, splines, squad) or the value is mapped
// non-linearly on the way to glTF.
constexpr int kSubsteps = 8;

void push(Samples &out, float fraction, float v) {
  out.fractions.push_back(fraction);
  out.values.push_back(v);
}
void push(Samples &out, float fraction, const SFColor &c) {
  out.fractions.push_back(fraction);
  out.values.insert(out.values.end(), {c.r, c.g, c.b});
}
void push(Samples &out, float fraction, const SFVec3f &v) {
  out.fractions.push_back(fraction);
  out.values.insert(out.values.end(), {v.x, v.y, v.z});
}
void push(Samples &out, float fraction, const SFRotation &r) {
  const auto q = quaternion(r);
  out.fractions.push_back(fraction);
  out.values.insert(out.values.end(),
                    {float(q[0]), float(q[1]), float(q[2]), float(q[3])});
}

// Every key with its value; with `densify`, also kSubsteps - 1 evenly spaced
// fractions inside each key interval, valued by `eval`. Keys are taken as
// authored, so a repeated key (a jump) stays a pair of samples.
template <typename T, typename Eval>
bool sampleKeys(const std::vector<float> &key, const std::vector<T> &keyValue,
                bool densify, Eval eval, Samples &out) {
  if (key.empty() || keyValue.size() != key.size()) return false;
  for (std::size_t i = 0; i < key.size(); ++i) {
    push(out, key[i], keyValue[i]);
    if (!densify || i + 1 == key.size() || !(key[i + 1] > key[i])) continue;
    for (int k = 1; k < kSubsteps; ++k) {
      const float f = key[i] + (key[i + 1] - key[i]) * float(k) / kSubsteps;
      push(out, f, eval(f));
    }
  }
  return true;
}

// Sample the interpolators whose output glTF can carry. LINEAR keys are
// exact for the linear ones (and for OrientationInterpolator, since glTF
// slerps rotations); the others are densified.
bool sampleInterpolator(const X3DNode &n, bool densify,
                        const geo::GeoProjection &projection, Samples &out) {
  using geombounds::getField;
  const std::string type = n.nodeTypeName();
  const auto key = getField<std::vector<float>>(n, "key", {});
  const bool closed = getField<bool>(n, "closed", false);
  const bool normalize = getField<bool>(n, "normalizeVelocity", false);
  if (type == "ScalarInterpolator" || type == "SplineScalarInterpolator") {
    out.kind = SampleKind::Scalar;
    const auto kv = getField<std::vector<float>>(n, "keyValue", {});
    if (type == "ScalarInterpolator")
      return sampleKeys(key, kv, densify, [&](float f) {
        return interpolateValue(key, kv, f, lerpf);
      }, out);
    const auto vel = getField<std::vector<float>>(n, "keyVelocity", {});
    return sampleKeys(key, kv, true, [&](float f) {
      return hermiteSpline<float>(key, kv, vel, closed, normalize, f);
    }, out);
  }
  if (type == "ColorInterpolator") {
    // X3D interpolates colours in HSV.
    out.kind = SampleKind::Color;
    const auto kv = getField<std::vector<SFColor>>(n, "keyValue", {});
    return sampleKeys(key, kv, true, [&](float f) {
      return interpolateValue(key, kv, f, lerpColorHsv);
    }, out);
  }
  if (type == "PositionInterpolator" || type == "SplinePositionInterpolator") {
    out.kind = SampleKind::Vec3;
    const auto kv = getField<std::vector<SFVec3f>>(n, "keyValue", {});
    if (type == "PositionInterpolator")
      return sampleKeys(key, kv, densify, [&](float f) {
        return interpolateValue(key, kv, f, lerpVec3);
      }, out);
    const auto vel = getField<std::vector<SFVec3f>>(n, "keyVelocity", {});
    return sampleKeys(key, kv, true, [&](float f) {
      return hermiteSpline<SFVec3f>(key, kv, vel, closed, normalize, f);
    }, out);
  }
  if (type == "OrientationInterpolator" ||
      type == "SquadOrientationInterpolator") {
    out.kind = SampleKind::Rotation;
    const auto kv = getField<std::vector<SFRotation>>(n, "keyValue", {});
    const bool squad = type == "SquadOrientationInterpolator";
    return sampleKeys(key, kv, squad || densify, [&](float f) {
      return squad ? squadOrientation(key, kv, f, normalize)
                   : interpolateValue(key, kv, f, slerpRotation);
    }, out);
  }
  if (type == "GeoPositionInterpolator") {
    // Interpolated in geographic coordinates, then projected (§25.3.7): not
    // linear in the local frame, so densified.
    out.kind = SampleKind::Vec3;
    const auto kv = getField<std::vector<SFVec3d>>(n, "keyValue", {});
    if (key.empty() || kv.size() != key.size()) return false;
    auto local = [&](float f, SFVec3f &world) {
      const SFVec3d g = interpolateValue(key, kv, f, [](const SFVec3d &a,
                                                        const SFVec3d &b, float t) {
        return SFVec3d{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                       a.z + (b.z - a.z) * t};
      });
      return geo::toWorld(n, g, world, projection);
    };
    for (std::size_t i = 0; i < key.size(); ++i) {
      SFVec3f world;
      if (!geo::toWorld(n, kv[i], world, projection)) return false;
      push(out, key[i], world);
      if (i + 1 == key.size() || !(key[i + 1] > key[i])) continue;
      for (int k = 1; k < kSubsteps; ++k) {
        const float f = key[i] + (key[i + 1] - key[i]) * float(k) / kSubsteps;
        if (!local(f, world)) return false;
        push(out, f, world);
      }
    }
    return true;
  }
  if (type == "NurbsPositionInterpolator" ||
      type == "NurbsOrientationInterpolator") {
    // No keys: the fraction runs over the whole curve. Sample it evenly.
    const auto *pos = dynamic_cast<const x3d::nodes::NurbsPositionInterpolator *>(&n);
    const auto *ori = dynamic_cast<const x3d::nodes::NurbsOrientationInterpolator *>(&n);
    nurbs::CurveDef c;
    if (pos) {
      c.cp = nurbsPoints(pos->getControlPoint());
      c.w = pos->getWeight();
      c.knot = pos->getKnot();
      c.order = pos->getOrder();
    } else if (ori) {
      c.cp = nurbsPoints(ori->getControlPoint());
      c.w = ori->getWeight();
      c.knot = ori->getKnot();
      c.order = ori->getOrder();
    } else {
      return false;
    }
    out.kind = pos ? SampleKind::Vec3 : SampleKind::Rotation;
    constexpr int kCurveSamples = 64;
    for (int i = 0; i <= kCurveSamples; ++i) {
      const double f = double(i) / kCurveSamples;
      if (pos) {
        SFVec3f p;
        if (nurbs::evalCurve(c, f, p)) push(out, float(f), p);
      } else {
        // The orientation turns +Z onto the curve's tangent.
        SFVec3f t;
        if (nurbsTangent(c, f, t)) push(out, float(f), rotationFromPositiveZ(t));
      }
    }
    return !out.fractions.empty();
  }
  return false;
}

// An interpolator writing an MF field inside a geometry (Coordinate.point,
// Normal.vector, TextureCoordinate.point, Extrusion.spine, ...).
struct GeometrySource {
  std::shared_ptr<X3DNode> interpolator;
  const X3DNode *target;
  std::string field;
  // The target is a TextureTransform on the shape's appearance, not a node
  // in the geometry: its values go to GltfMorph::textureFields.
  bool texture = false;
};

// The value of a single-value interpolator at a key fraction `f` (`right`:
// after a jump at exactly that key), or `eval(f)` between keys.
template <typename T, typename Eval>
T valueAtKey(const std::vector<float> &key, const std::vector<T> &kv, float f,
             bool right, Eval eval) {
  std::size_t first = key.size(), last = key.size();
  for (std::size_t i = 0; i < key.size(); ++i)
    if (key[i] == f) {
      if (first == key.size()) first = i;
      last = i;
    }
  if (first < key.size()) return kv[right ? last : first];
  return eval(f);
}

// What can drive a TextureTransform field: the field's X3D type and whether
// the interpolator emits it.
bool drivesTextureTransform(const std::string &interpolator,
                            const std::string &field) {
  const bool vec2 = interpolator == "PositionInterpolator2D" ||
                    interpolator == "SplinePositionInterpolator2D";
  const bool scalar = interpolator == "ScalarInterpolator" ||
                      interpolator == "SplineScalarInterpolator";
  if (field == "rotation") return scalar;
  return vec2 && (field == "translation" || field == "center" ||
                  field == "scale");
}

// The keyValue row of a multi-value interpolator at `fraction` (`right`:
// after a jump at exactly that key).
template <typename T, typename Lerp>
std::vector<T> rowAt(const std::vector<float> &key, const std::vector<T> &kv,
                     std::size_t m, float fraction, bool right, Lerp lerp) {
  auto row = [&](std::size_t i) {
    return std::vector<T>(kv.begin() + std::ptrdiff_t(i * m),
                          kv.begin() + std::ptrdiff_t((i + 1) * m));
  };
  std::size_t first = key.size(), last = key.size();
  for (std::size_t i = 0; i < key.size(); ++i)
    if (key[i] == fraction) {
      if (first == key.size()) first = i;
      last = i;
    }
  if (first < key.size()) return row(right ? last : first);
  return interpolateMulti(key, kv, fraction, lerp);
}

// Re-tessellate `geometry` at every key of `sources` (the union of their
// keys, a jump giving two targets). Fills `morph` and the fraction of each
// target; false if a source is malformed or the vertex count changes.
bool morphTargets(const std::shared_ptr<X3DNode> &geometry,
                  const std::vector<GeometrySource> &sources,
                  const MeshBuildOptions &options,
                  const geo::GeoProjection &projection, GltfMorph &morph,
                  std::vector<float> &fractions) {
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
  const auto clone = deepClone(geometry, cloneMap);
  if (!clone) return false;
  struct Source {
    X3DNode *node = nullptr;
    const FieldInfo *field = nullptr;
    std::vector<float> key;
    std::vector<SFVec3f> kv3;
    std::vector<SFVec2f> kv2;
    std::size_t m = 0;
    bool normal = false, twoD = false;
  };
  // A TextureTransform field: one value per key, from a linear or spline
  // interpolator.
  struct TextureSource {
    const X3DNode *node = nullptr;
    std::string field;
    std::vector<float> key, kv1, vel1;
    std::vector<SFVec2f> kv2, vel2;
    bool vec2 = false, spline = false, closed = false, normalize = false;
    std::any at(float f, bool right) const {
      if (vec2)
        return std::any(valueAtKey(key, kv2, f, right, [&](float x) {
          return spline ? hermiteSpline<SFVec2f>(key, kv2, vel2, closed, normalize, x)
                        : interpolateValue(key, kv2, x, lerpVec2);
        }));
      return std::any(valueAtKey(key, kv1, f, right, [&](float x) {
        return spline ? hermiteSpline<float>(key, kv1, vel1, closed, normalize, x)
                      : interpolateValue(key, kv1, x, lerpf);
      }));
    }
  };
  std::vector<Source> in;
  std::vector<TextureSource> textures;
  std::set<float> keys, jumps;
  auto addKeys = [&](const std::vector<float> &key, bool densify) {
    for (std::size_t i = 0; i < key.size(); ++i) {
      if (i && key[i] < key[i - 1]) return false;
      if (i && key[i] == key[i - 1]) jumps.insert(key[i]);
      keys.insert(key[i]);
      // texture coordinates are not linear in a TextureTransform's fields
      // taken together (rotation, and products of scale with the others), and
      // splines are not linear at all: sample inside each interval too.
      if (densify && i + 1 < key.size() && key[i + 1] > key[i])
        for (int k = 1; k < kSubsteps; ++k)
          keys.insert(key[i] + (key[i + 1] - key[i]) * float(k) / kSubsteps);
    }
    return true;
  };
  for (const GeometrySource &g : sources) {
    const std::string type = g.interpolator->nodeTypeName();
    if (g.texture) {
      using geombounds::getField;
      TextureSource t;
      t.node = g.target;
      t.field = g.field;
      t.vec2 = g.field != "rotation";
      t.spline = type.rfind("Spline", 0) == 0;
      t.closed = getField<bool>(*g.interpolator, "closed", false);
      t.normalize = getField<bool>(*g.interpolator, "normalizeVelocity", false);
      t.key = getField<std::vector<float>>(*g.interpolator, "key", {});
      if (t.vec2) {
        t.kv2 = getField<std::vector<SFVec2f>>(*g.interpolator, "keyValue", {});
        t.vel2 = getField<std::vector<SFVec2f>>(*g.interpolator, "keyVelocity", {});
      } else {
        t.kv1 = getField<std::vector<float>>(*g.interpolator, "keyValue", {});
        t.vel1 = getField<std::vector<float>>(*g.interpolator, "keyVelocity", {});
      }
      const std::size_t total = t.vec2 ? t.kv2.size() : t.kv1.size();
      if (t.key.empty() || total != t.key.size() || !addKeys(t.key, true))
        return false;
      textures.push_back(std::move(t));
      continue;
    }
    Source src;
    const auto found = cloneMap.find(g.target);
    if (found == cloneMap.end() || !found->second) return false;
    src.node = found->second.get();
    for (const FieldInfo &f : src.node->fields())
      if (f.x3dName == g.field && f.set) src.field = &f;
    src.twoD = type == "CoordinateInterpolator2D";
    src.normal = type == "NormalInterpolator";
    if (!src.field || src.field->type != (src.twoD ? X3DFieldType::MFVec2f
                                                    : X3DFieldType::MFVec3f))
      return false;
    src.key = geombounds::getField<std::vector<float>>(*g.interpolator, "key", {});
    if (src.twoD)
      src.kv2 = geombounds::getField<std::vector<SFVec2f>>(*g.interpolator, "keyValue", {});
    else
      src.kv3 = geombounds::getField<std::vector<SFVec3f>>(*g.interpolator, "keyValue", {});
    const std::size_t total = src.twoD ? src.kv2.size() : src.kv3.size();
    if (src.key.empty() || total == 0 || total % src.key.size() != 0) return false;
    src.m = total / src.key.size();
    if (!addKeys(src.key, false)) return false;
    in.push_back(std::move(src));
  }
  // With only TextureTransform sources the geometry does not change: one
  // tessellation serves every target.
  std::optional<MeshData> still;
  for (float f : keys) {
    for (bool right : {false, true}) {
      if (right && !jumps.count(f)) continue;
      for (Source &src : in) {
        if (src.twoD)
          src.field->set(*src.node, std::any(rowAt(src.key, src.kv2, src.m, f, right,
                                                   lerpVec2)));
        else if (src.normal)
          src.field->set(*src.node, std::any(rowAt(src.key, src.kv3, src.m, f, right,
                                                   slerpNormal)));
        else
          src.field->set(*src.node, std::any(rowAt(src.key, src.kv3, src.m, f, right,
                                                   lerpVec3)));
      }
      MeshData mesh;
      if (in.empty()) {
        if (!still) still = buildLocalMesh(clone.get(), projection, options);
        mesh = *still;
      } else {
        mesh = buildLocalMesh(clone.get(), projection, options);
      }
      if (mesh.positions.empty() ||
          (!morph.targets.empty() &&
           mesh.positions.size() != morph.targets.front().positions.size()))
        return false;
      std::vector<GltfTextureFieldValue> fields;
      for (const TextureSource &t : textures)
        fields.push_back({t.node, t.field, t.at(f, right)});
      morph.targets.push_back(std::move(mesh));
      morph.textureFields.push_back(std::move(fields));
      fractions.push_back(f);
    }
  }
  morph.geometry = geometry.get();
  return !morph.targets.empty();
}

// Where an interpolator's value goes in glTF.
struct Destination {
  SampleKind kind = SampleKind::None; // what the field takes; None: nowhere
  GltfAnimationChannel::Path path = GltfAnimationChannel::Path::Property;
  std::string field;    // Path::Property: the X3D field
  bool densify = false; // the glTF value is not linear in the X3D value
  bool aim = false;     // a direction, written as the rotation aiming -Z
};

Destination destinationOf(const X3DNode &to, const std::string &field) {
  using Path = GltfAnimationChannel::Path;
  const std::string type = to.nodeTypeName();
  Destination d;
  auto property = [&](SampleKind kind, bool densify = false) {
    d.kind = kind;
    d.path = Path::Property;
    d.field = field;
    d.densify = densify;
  };
  auto node = [&](SampleKind kind, Path path) {
    d.kind = kind;
    d.path = path;
  };
  // Nodes with the Transform fields and composition (HAnim 19774-1 §6.3,
  // §6.5; CAD §32.4.2).
  if (type == "Transform" || type == "HAnimHumanoid" || type == "HAnimJoint" ||
      type == "HAnimSite" || type == "CADPart") {
    if (field == "translation") node(SampleKind::Vec3, Path::Translation);
    else if (field == "scale") node(SampleKind::Vec3, Path::Scale);
    else if (field == "rotation") node(SampleKind::Rotation, Path::Rotation);
    else if (field == "center") property(SampleKind::Vec3);
    else if (field == "scaleOrientation") property(SampleKind::Rotation);
  } else if (type == "Viewpoint" || type == "OrthoViewpoint") {
    if (field == "position") node(SampleKind::Vec3, Path::Translation);
    else if (field == "orientation") node(SampleKind::Rotation, Path::Rotation);
    else if (field == "fieldOfView" && type == "Viewpoint")
      property(SampleKind::Scalar);
  } else if (type == "DirectionalLight" || type == "PointLight" ||
             type == "SpotLight") {
    if (field == "color") property(SampleKind::Color);
    else if (field == "intensity") property(SampleKind::Scalar);
    else if (field == "location" && type != "DirectionalLight")
      node(SampleKind::Vec3, Path::Translation);
    else if (field == "radius" && type != "DirectionalLight")
      property(SampleKind::Scalar);
    else if ((field == "beamWidth" || field == "cutOffAngle") &&
             type == "SpotLight")
      property(SampleKind::Scalar);
    else if (field == "direction" && type != "PointLight") {
      node(SampleKind::Vec3, Path::Rotation);
      d.densify = d.aim = true;
    }
  } else if (type == "Material") {
    if (field == "diffuseColor" || field == "emissiveColor")
      property(SampleKind::Color);
    else if (field == "transparency") property(SampleKind::Scalar);
    else if (field == "shininess") property(SampleKind::Scalar, true);
  } else if (type == "PhysicalMaterial") {
    if (field == "baseColor" || field == "emissiveColor")
      property(SampleKind::Color);
    else if (field == "transparency" || field == "metallic" ||
             field == "roughness")
      property(SampleKind::Scalar);
  } else if (type == "UnlitMaterial") {
    if (field == "emissiveColor") property(SampleKind::Color);
    else if (field == "transparency") property(SampleKind::Scalar);
  }
  return d;
}


std::string matrixJson(const Mat4 &m) {
  std::vector<std::string> cells;
  for (float v : m.m) cells.push_back(num(v));
  return "\"matrix\":[" + join(cells) + "]";
}

// A glTF node under construction: `body` is the object's members without the
// closing brace, so children can be appended when everything is known.
struct Node {
  std::string body;
  std::vector<int> children;
};

// One placement of an animated Transform, as the glTF nodes that realise it.
struct TransformInstance {
  int trs = -1;    // translation and rotation live here
  int scale = -1;  // the node carrying scale (trs unless scaleOrientation is used)
  int tail = -1;   // parent for content below the Transform
  bool oriented = false; // scale and tail are helper nodes (scaleOrientation)
  Mat4 after;      // estimated world matrix below this Transform
};

// Static glTF values of a material made from an animated X3D material, for
// the components a channel does not drive.
struct MaterialStatic {
  bool unlit = false;       // KHR_materials_unlit: base colour = emissive
  bool vertexColor = false; // COLOR_0 replaces the base colour's RGB
  MaterialModel model = MaterialModel::Phong;
  std::array<float, 4> base{1, 1, 1, 1};
  std::array<float, 3> emissive{0, 0, 0};
  float metallic = 0, roughness = 1;
};

struct LightStatic {
  LightDesc::Type type = LightDesc::Type::Point;
  SFColor color{1, 1, 1};
  float intensity = 1, radius = 0, beamWidth = 0, cutOffAngle = 0;
};

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
    if (item.shaderProgram) ++report_.droppedShaders;
    if (item.skin && item.skin->binding && !item.skin->binding->empty()) {
      if (addSkinned(item)) return;
      ++report_.staticSkins;
    }

    const MeshAccessors &acc = accessorsFor(item.mesh);
    const int material =
        materialFor(item.material, mesh, acc, materialNodeOf(item));
    // Morph targets of an animated geometry, when they fit this mesh.
    const X3DNode *geometry = geometryNodeOf(item);
    const GltfMorph *morph = nullptr;
    if (const auto m = morphs_.find(geometry);
        m != morphs_.end() &&
        m->second->targets.front().positions.size() == mesh.positions.size() &&
        (!m->second->segment ||
         std::find(item.path.begin(), item.path.end(), m->second->segment) !=
             item.path.end()))
      morph = m->second;
    // Morph targets carry this placement's TextureTransform, which lives on
    // the appearance, so a morphed mesh is only shared by one appearance.
    const X3DNode *appearance = nullptr;
    if (morph && !item.path.empty() && item.path.back())
      appearance = sfNode(*item.path.back(), "appearance").get();
    const auto key = std::make_tuple(item.mesh.get(), material, appearance);
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
      std::string targets, weights;
      if (morph) {
        const std::vector<std::string> &t = targetsFor(item, *morph, acc);
        targets = ",\"targets\":[" + join(t) + "]";
        std::vector<std::string> w(t.size(), "0");
        if (morph->weights.size() == t.size())
          for (std::size_t k = 0; k < t.size(); ++k) w[k] = num(morph->weights[k]);
        weights = ",\"weights\":[" + join(w) + "]";
      }
      meshes_.push_back("{\"primitives\":[{\"attributes\":{" + join(attrs) +
                        "},\"indices\":" + std::to_string(acc.indices) +
                        ",\"material\":" + std::to_string(material) +
                        ",\"mode\":" + std::to_string(mode) + targets + "}]" +
                        weights + "}");
      found = meshIndex_.emplace(key, static_cast<int>(meshes_.size()) - 1)
                  .first;
    }

    std::string node = "{\"mesh\":" + std::to_string(found->second);
    std::string name;
    for (const X3DNode *n : item.path)
      if (n && !n->getDEF().empty()) name = n->getDEF();
    if (!name.empty()) node += ",\"name\":" + quote(name);

    // Without an animated Transform above it the item is a root carrying its
    // world matrix. Under one, it hangs off that Transform's node with
    // whatever is left of its world matrix, so the composed matrix is still
    // exactly item.worldTransform at export time.
    Mat4 below = Mat4::identity();
    const int parent = animatedParent(item.path, below);
    const Mat4 local =
        parent < 0 ? item.worldTransform : below.inverse() * item.worldTransform;
    if (!isIdentity(local)) node += "," + matrixJson(local);
    const int index = attach(parent, node);
    if (morph) morphNodes_[geometry].push_back(index);
    ++itemNodes_;
    report_.triangles += acc.triangles;
  }

  void setMorphs(const std::vector<GltfMorph> &morphs) {
    for (const GltfMorph &m : morphs)
      if (m.geometry && !m.targets.empty()) morphs_[m.geometry] = &m;
  }

  // Keep the humanoid and palette joints of every skinned item as glTF
  // nodes, before any item is placed, so rigid parts and skins hang from
  // the same joint nodes.
  void prepareSkins(const std::vector<const RenderItem *> &items) {
    for (const RenderItem *item : items) {
      if (!item || !item->skin || !item->skin->binding) continue;
      const hanim::SkinBinding &b = *item->skin->binding;
      if (b.empty() || !b.humanoid) continue;
      keptTransforms_.insert(b.humanoid);
      keptTransforms_.insert(b.joints.begin(), b.joints.end());
    }
  }

  // Every node below `humanoid`'s skeleton, as the chain from the skeleton
  // root down to it (the path the extractor walks; the first chain wins).
  const std::unordered_map<const X3DNode *, PathKey> &
  skeletonOf(const X3DNode *humanoid) {
    if (const auto found = skeletons_.find(humanoid); found != skeletons_.end())
      return found->second;
    auto &out = skeletons_[humanoid];
    PathKey chain;
    std::function<void(const X3DNode *)> rec = [&](const X3DNode *n) {
      if (!n || chain.size() >= kMaxNestingDepth || out.count(n)) return;
      chain.push_back(n);
      out.emplace(n, chain);
      forEachChildNode(*n, [&](const FieldInfo &f, const std::shared_ptr<X3DNode> &c) {
        if (f.x3dName == "children") rec(c.get());
      });
      chain.pop_back();
    };
    forEachChildNode(*humanoid, [&](const FieldInfo &f,
                                    const std::shared_ptr<X3DNode> &c) {
      if (f.x3dName == "skeleton") rec(c.get());
    });
    return out;
  }

  // The glTF skin of one placement of a humanoid's skin (`frame`: the
  // shape's ancestors). Joint 0 is a node with the frame's world matrix and
  // an identity inverse bind, for vertices no joint weights; joint j+1 is
  // palette joint j with B_j^-1 from the binding.
  int skinFor(const hanim::SkinBinding &b, const PathKey &frame) {
    const auto key = std::make_pair(&b, frame);
    if (const auto found = skinIndex_.find(key); found != skinIndex_.end())
      return found->second;
    Mat4 below = Mat4::identity();
    const int anchor = animatedParent(frame, below);
    std::vector<int> joints{frameNode(anchor, below, frame)};
    const Mat4 identity = Mat4::identity();
    std::vector<float> inverse(identity.m.begin(), identity.m.end());
    const auto &skeleton = skeletonOf(b.humanoid);
    for (std::size_t j = 0; j < b.joints.size(); ++j) {
      PathKey path = frame;
      if (const auto chain = skeleton.find(b.joints[j]); chain != skeleton.end())
        path.insert(path.end(), chain->second.begin(), chain->second.end());
      else
        path.push_back(b.joints[j]); // posed alone, as evaluatePose does
      joints.push_back(animatedParent(path, below));
      const Mat4 &m = j < b.inverseBind.size() ? b.inverseBind[j] : Mat4::identity();
      inverse.insert(inverse.end(), m.m.begin(), m.m.end());
    }
    std::vector<std::string> list;
    for (int node : joints) list.push_back(std::to_string(node));
    std::string json = "{";
    if (!b.humanoid->getDEF().empty())
      json += "\"name\":" + quote(b.humanoid->getDEF()) + ",";
    json += "\"joints\":[" + join(list) + "],\"inverseBindMatrices\":" +
            std::to_string(floatAccessor(inverse, 16, "MAT4", false, 0)) + "}";
    skins_.push_back(json);
    report_.skins = skins_.size();
    return skinIndex_.emplace(key, static_cast<int>(skins_.size()) - 1).first->second;
  }

  // A skinned item as a glTF skinned mesh: the binding's bind-pose
  // coordinates and normals, JOINTS_n/WEIGHTS_n from its influences (every
  // influence, four per set), and its Joint displacers as morph targets.
  // glTF composes joint world * inverse bind, which is the humanoid's world
  // matrix times D_j = C_j B_j^-1, so the posed mesh matches deformedMesh().
  // False when the corners do not map to skin coordinates.
  bool addSkinned(const RenderItem &item) {
    const hanim::SkinBinding &b = *item.skin->binding;
    const MeshData &mesh = *item.mesh;
    const std::size_t n = mesh.positions.size();
    if (mesh.sourceCoordIndex.size() != n || item.path.size() < 2 ||
        b.influenceOffset.size() != b.bindPositions.size() + 1 ||
        std::find(item.path.begin(), item.path.end(), b.humanoid) == item.path.end())
      return false;
    for (std::uint32_t source : mesh.sourceCoordIndex)
      if (source >= b.bindPositions.size()) return false;

    const PathKey frame(item.path.begin(), item.path.end() - 1);
    const int skin = skinFor(b, frame);
    const auto meshKey = std::make_pair(item.mesh.get(), &b);
    auto bound = skinMeshes_.find(meshKey);
    if (bound == skinMeshes_.end()) {
      auto bind = std::make_shared<MeshData>(mesh);
      for (std::size_t i = 0; i < n; ++i)
        bind->positions[i] = b.bindPositions[mesh.sourceCoordIndex[i]];
      if (!b.bindNormals.empty() && mesh.sourceNormalIndex.size() == n &&
          bind->normals.size() == n)
        for (std::size_t i = 0; i < n; ++i)
          if (mesh.sourceNormalIndex[i] < b.bindNormals.size())
            bind->normals[i] = b.bindNormals[mesh.sourceNormalIndex[i]];
      bound = skinMeshes_.emplace(meshKey, SkinMesh{MeshRef(std::move(bind)), {}, {}}).first;
      SkinMesh &sm = bound->second;
      sm.attributes = skinAttributes(b, mesh.sourceCoordIndex);
      sm.targets = displacerTargets(b, mesh.sourceCoordIndex);
    }
    const SkinMesh &sm = bound->second;
    const MeshAccessors &acc = accessorsFor(sm.mesh);
    const int material = materialFor(item.material, *sm.mesh, acc, materialNodeOf(item));
    const auto key = std::make_tuple(sm.mesh.get(), material);
    auto found = skinMeshIndex_.find(key);
    if (found == skinMeshIndex_.end()) {
      std::vector<std::string> attrs{"\"POSITION\":" + std::to_string(acc.position)};
      if (acc.normal >= 0) attrs.push_back("\"NORMAL\":" + std::to_string(acc.normal));
      for (std::size_t i = 0; i < acc.texcoords.size(); ++i)
        attrs.push_back("\"TEXCOORD_" + std::to_string(i) +
                        "\":" + std::to_string(acc.texcoords[i]));
      if (acc.color >= 0) attrs.push_back("\"COLOR_0\":" + std::to_string(acc.color));
      attrs.insert(attrs.end(), sm.attributes.begin(), sm.attributes.end());
      const int mode = sm.mesh->topology == Topology::Triangles ? 4
                       : sm.mesh->topology == Topology::Lines ? 1 : 0;
      std::string targets, weights;
      if (!sm.targets.empty()) {
        targets = ",\"targets\":[" + join(sm.targets) + "]";
        std::vector<std::string> w;
        for (const hanim::JointDisplacer &d : b.displacers)
          w.push_back(num(geombounds::getField<float>(*d.node, "weight", 0.0f)));
        weights = ",\"weights\":[" + join(w) + "]";
      }
      meshes_.push_back("{\"primitives\":[{\"attributes\":{" + join(attrs) +
                        "},\"indices\":" + std::to_string(acc.indices) +
                        ",\"material\":" + std::to_string(material) +
                        ",\"mode\":" + std::to_string(mode) + targets + "}]" +
                        weights + "}");
      found = skinMeshIndex_.emplace(key, static_cast<int>(meshes_.size()) - 1).first;
    }

    // glTF ignores a skinned mesh node's transform: a root with none.
    std::string node = "{\"mesh\":" + std::to_string(found->second) +
                       ",\"skin\":" + std::to_string(skin);
    std::string name;
    for (const X3DNode *p : item.path)
      if (p && !p->getDEF().empty()) name = p->getDEF();
    if (!name.empty()) node += ",\"name\":" + quote(name);
    const int index = attach(-1, node);
    if (!sm.targets.empty()) {
      skinMeshNodes_[b.humanoid].push_back(index);
      for (std::size_t d = 0; d < b.displacers.size(); ++d)
        skinDisplacers_[b.displacers[d].node] = {&b, d};
    }
    ++itemNodes_;
    report_.triangles += acc.triangles;
    return true;
  }

  // JOINTS_n / WEIGHTS_n attributes ("NAME":accessor) for corners mapped to
  // skin coordinates by `sources`. Joint 0 holds unweighted vertices.
  std::vector<std::string> skinAttributes(const hanim::SkinBinding &b,
                                          const std::vector<std::uint32_t> &sources) {
    std::size_t width = 1;
    for (std::uint32_t s : sources)
      width = std::max<std::size_t>(width, b.influenceOffset[s + 1] - b.influenceOffset[s]);
    const std::size_t sets = (width + 3) / 4;
    const bool wide = b.joints.size() + 1 > 256;
    std::vector<std::string> out;
    for (std::size_t set = 0; set < sets; ++set) {
      std::vector<std::uint16_t> joints;
      std::vector<float> weights;
      for (std::uint32_t s : sources) {
        const std::uint32_t begin = b.influenceOffset[s], end = b.influenceOffset[s + 1];
        for (std::size_t k = 0; k < 4; ++k) {
          const std::size_t at = set * 4 + k;
          if (begin == end) {
            joints.push_back(0);
            weights.push_back(at == 0 ? 1.0f : 0.0f);
          } else if (begin + at < end && b.influences[begin + at].joint < b.joints.size()) {
            joints.push_back(static_cast<std::uint16_t>(b.influences[begin + at].joint + 1));
            weights.push_back(b.influences[begin + at].weight);
          } else {
            joints.push_back(0);
            weights.push_back(0.0f);
          }
        }
      }
      int v;
      if (wide) {
        v = view(joints.data(), joints.size() * sizeof(std::uint16_t), kArrayBuffer);
      } else {
        std::vector<std::uint8_t> narrow(joints.begin(), joints.end());
        v = view(narrow.data(), narrow.size(), kArrayBuffer);
      }
      accessors_.push_back("{\"bufferView\":" + std::to_string(v) +
                           ",\"componentType\":" + (wide ? "5123" : "5121") +
                           ",\"count\":" + std::to_string(sources.size()) +
                           ",\"type\":\"VEC4\"}");
      out.push_back("\"JOINTS_" + std::to_string(set) +
                    "\":" + std::to_string(accessors_.size() - 1));
      out.push_back("\"WEIGHTS_" + std::to_string(set) + "\":" +
                    std::to_string(floatAccessor(weights, 4, "VEC4", false)));
    }
    return out;
  }

  // One morph target per Joint displacer. X3D adds weight * C_owner *
  // displacement after skinning; a glTF target is added before, so the
  // offset is stored as B_owner * displacement, which skinning turns into
  // C_owner * displacement for a vertex the owning joint fully weights.
  std::vector<std::string> displacerTargets(const hanim::SkinBinding &b,
                                            const std::vector<std::uint32_t> &sources) {
    std::vector<std::string> out;
    for (const hanim::JointDisplacer &d : b.displacers) {
      std::vector<SFVec3f> offset(b.bindPositions.size(), SFVec3f{0, 0, 0});
      const Mat4 bind = d.joint < b.inverseBind.size() ? b.inverseBind[d.joint].inverse()
                                                       : Mat4::identity();
      for (std::size_t k = 0; k < d.index.size() && k < d.displacement.size(); ++k) {
        if (d.index[k] >= offset.size()) continue;
        const SFVec3f v = bind.transformDirection(d.displacement[k]);
        SFVec3f &o = offset[d.index[k]];
        o = {o.x + v.x, o.y + v.y, o.z + v.z};
      }
      std::vector<float> flat;
      for (std::uint32_t s : sources) {
        const SFVec3f &o = offset[s];
        flat.insert(flat.end(), {std::isfinite(o.x) ? o.x : 0.0f,
                                 std::isfinite(o.y) ? o.y : 0.0f,
                                 std::isfinite(o.z) ? o.z : 0.0f});
      }
      out.push_back("{\"POSITION\":" +
                    std::to_string(floatAccessor(flat, 3, "VEC3", true)) + "}");
    }
    return out;
  }

  // Declare the animations before any item is added: their targets decide
  // which Transforms are kept as nodes.
  void setAnimations(const std::vector<GltfAnimation> &animations) {
    animations_ = &animations;
    for (const GltfAnimation &animation : animations)
      for (const GltfAnimationChannel &channel : animation.channels) {
        if (!channel.target) continue;
        animatedFields_[channel.target].insert(channel.field);
        // Nodes whose own glTF node moves: Transforms, viewpoints and lights
        // with a translation/rotation/scale channel, and Transforms whose
        // centre or scaleOrientation moves.
        if ((channel.path != GltfAnimationChannel::Path::Property &&
             channel.path != GltfAnimationChannel::Path::Weights) ||
            channel.field == "center" || channel.field == "scaleOrientation")
          animatedTargets_.insert(channel.target);
      }
  }

  void add(const LightDesc &light) {
    std::string l = "{\"type\":";
    l += light.type == LightDesc::Type::Directional ? "\"directional\""
         : light.type == LightDesc::Type::Point ? "\"point\"" : "\"spot\"";
    l += ",\"color\":" + array({std::clamp(light.color.r, 0.0f, 1.0f),
                               std::clamp(light.color.g, 0.0f, 1.0f),
                               std::clamp(light.color.b, 0.0f, 1.0f)});
    l += ",\"intensity\":" + num(std::max(0.0f, light.intensity));
    // An animated radius needs a range to animate, even a non-positive one.
    if (light.type != LightDesc::Type::Directional &&
        (light.radius > 0.0f || fieldAnimated(light.node, "radius")))
      l += ",\"range\":" + num(std::max(light.radius, kMinRange));
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
    if (light.node) {
      lightsByNode_[light.node].push_back(static_cast<int>(lights_.size()) - 1);
      LightStatic st;
      st.type = light.type;
      st.color = light.color;
      st.intensity = light.intensity;
      st.radius = light.radius;
      st.beamWidth = light.beamWidth;
      st.cutOffAngle = light.cutOffAngle;
      lightStatics_.push_back(st);
    } else {
      lightStatics_.emplace_back();
    }

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
    // Under an animated Transform, or with an animated location, the light
    // is placed in its own frame so it moves with what drives it.
    const bool moving = light.node && animatedTargets_.count(light.node);
    Mat4 below = Mat4::identity();
    const int parent = light.node ? animatedParent(light.path, below) : -1;
    if (parent < 0 && !moving) {
      attach(-1, node);
      return;
    }
    node = "{\"extensions\":{\"KHR_lights_punctual\":{\"light\":" +
           std::to_string(lights_.size() - 1) + "}}";
    if (!light.node->getDEF().empty())
      node += ",\"name\":" + quote(light.node->getDEF());
    if (light.type != LightDesc::Type::Directional) {
      const SFVec3f p =
          geombounds::getVec3fLenient(*light.node, "location", {0, 0, 0});
      node += ",\"translation\":" + array({p.x, p.y, p.z});
    }
    if (light.type != LightDesc::Type::Point) {
      const auto q = aimQuaternion(
          geombounds::getVec3fLenient(*light.node, "direction", {0, 0, -1}));
      node += ",\"rotation\":" + array({q[0], q[1], q[2], q[3]});
    }
    const int index = attach(frameNode(parent, below, light.path), node);
    if (moving) registerTarget(light.node, index);
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
    if (camera.node && !camera.ortho)
      camerasByNode_[camera.node].push_back(static_cast<int>(cameras_.size()) - 1);

    std::string node =
        "{\"camera\":" + std::to_string(cameras_.size() - 1);
    if (!camera.name.empty()) node += ",\"name\":" + quote(camera.name);
    const bool moving = camera.node && animatedTargets_.count(camera.node);
    Mat4 below = Mat4::identity();
    const int parent = camera.node ? animatedParent(camera.path, below) : -1;
    if (moving) {
      // Its own position and orientation are the animated properties.
      const SFVec3f p =
          geombounds::getVec3fLenient(*camera.node, "position", {0, 0, 10});
      const auto q = quaternion(geombounds::getField<SFRotation>(
          *camera.node, "orientation", {0, 0, 1, 0}));
      node += ",\"translation\":" + array({p.x, p.y, p.z}) +
              ",\"rotation\":" + array({q[0], q[1], q[2], q[3]});
      registerTarget(camera.node,
                     attach(frameNode(parent, below, camera.path), node));
      return;
    }
    const Mat4 local = parent < 0 ? camera.cameraToWorld
                                  : below.inverse() * camera.cameraToWorld;
    if (!isIdentity(local)) node += "," + matrixJson(local);
    attach(parent, node);
  }

  std::string finish() {
    writeAnimations();
    report_.nodes = itemNodes_;
    report_.lights = lights_.size();
    report_.cameras = cameras_.size();
    std::vector<std::string> nodes;
    for (const Node &n : nodes_) {
      std::string json = n.body;
      if (!n.children.empty()) {
        std::vector<std::string> kids;
        for (int c : n.children) kids.push_back(std::to_string(c));
        json += ",\"children\":[" + join(kids) + "]";
      }
      nodes.push_back(json + "}");
    }
    report_.meshes = meshes_.size();
    report_.materials = materials_.size();
    report_.textures = textures_.size();

    std::vector<std::string> top;
    top.push_back("\"asset\":{\"version\":\"2.0\",\"generator\":" +
                  quote(options_.generator) + "}");
    std::vector<std::string> used;
    if (usesUnlit_) used.push_back("\"KHR_materials_unlit\"");
    if (usesPointer_) used.push_back("\"KHR_animation_pointer\"");
    if (!lights_.empty()) used.push_back("\"KHR_lights_punctual\"");
    if (!used.empty())
      top.push_back("\"extensionsUsed\":[" + join(used) + "]");
    if (!lights_.empty())
      top.push_back("\"extensions\":{\"KHR_lights_punctual\":{\"lights\":[" +
                    join(lights_) + "]}}");
    std::vector<std::string> roots;
    for (int r : roots_) roots.push_back(std::to_string(r));
    top.push_back("\"scene\":0");
    top.push_back(roots.empty() ? "\"scenes\":[{}]"
                                : "\"scenes\":[{\"nodes\":[" + join(roots) +
                                      "]}]");
    auto section = [&](const char *name, const std::vector<std::string> &v) {
      if (!v.empty())
        top.push_back(std::string("\"") + name + "\":[" + join(v) + "]");
    };
    section("nodes", nodes);
    section("meshes", meshes_);
    section("skins", skins_);
    section("materials", materials_);
    section("textures", textures_);
    section("images", images_);
    section("samplers", samplers_);
    section("cameras", cameras_);
    section("animations", animationJson_);
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
  // `target` 0 marks data that is not vertex data (animation samples).
  int view(const void *data, std::size_t bytes, int target) {
    while (bin_.size() % 4) bin_ += '\0';
    const std::size_t offset = bin_.size();
    bin_.append(static_cast<const char *>(data), bytes);
    while (bin_.size() % 4) bin_ += '\0';
    views_.push_back("{\"buffer\":0,\"byteOffset\":" + std::to_string(offset) +
                     ",\"byteLength\":" + std::to_string(bytes) +
                     (target ? ",\"target\":" + std::to_string(target) : "") +
                     "}");
    return static_cast<int>(views_.size()) - 1;
  }

  // Add a node under `parent` (-1: a scene root) and return its index.
  int attach(int parent, const std::string &body) {
    nodes_.push_back(Node{body, {}});
    const int index = static_cast<int>(nodes_.size()) - 1;
    if (parent < 0) roots_.push_back(index);
    else nodes_[static_cast<std::size_t>(parent)].children.push_back(index);
    return index;
  }

  // The parent for a leaf (light, camera) that keeps its own local
  // translation/rotation: `parent`, plus a matrix node for whatever plain
  // Transforms lie between it and the leaf.
  int frameNode(int parent, const Mat4 &below, const PathKey &path) {
    const geo::GeoProjection &projection = *geo::builtinProjectionOwner();
    Mat4 world = Mat4::identity();
    for (const X3DNode *n : path)
      if (TransformSystem::isTransform(n))
        world = world * TransformSystem::localMatrix(n, projection);
    const Mat4 frame = parent < 0 ? world : below.inverse() * world;
    // The inverse leaves float noise on an identity remainder.
    bool identity = true;
    for (std::size_t i = 0; i < 16; ++i)
      identity = identity && std::fabs(frame.m[i] - Mat4::identity().m[i]) < 1e-6f;
    return identity ? parent : attach(parent, "{" + matrixJson(frame));
  }

  // A leaf node that is itself an animation target.
  void registerTarget(const X3DNode *target, int node) {
    TransformInstance inst;
    inst.trs = inst.scale = inst.tail = node;
    inst.after = Mat4::identity();
    targetInstances_[target].push_back(inst);
  }

  // Walk `path` and make sure every animated Transform on it exists as glTF
  // nodes. Returns the node content below the last one hangs from (-1 if the
  // path has none) and, in `below`, the estimated world matrix just below it.
  //
  // X3D: M = T * C * R * SR * S * SR^-1 * C^-1. glTF nodes are T*R*S, so:
  //   trs   { translation T+C, rotation R, scale S }   and, if C != 0,
  //   tail  { translation -C }
  // or, with a scaleOrientation,
  //   trs   { translation T+C, rotation R }
  //   scale { rotation SR, scale S }
  //   tail  { translation SR^-1(-C), rotation SR^-1 }
  int animatedParent(const PathKey &path, Mat4 &below) {
    if (animatedTargets_.empty() && keptTransforms_.empty()) return -1;
    const geo::GeoProjection &projection = *geo::builtinProjectionOwner();
    int parent = -1;
    Mat4 world = Mat4::identity(); // estimate at the current path node
    Mat4 since = Mat4::identity(); // accumulated since the last animated node
    Mat4 anchor = Mat4::identity(); // world just below the last animated node
    PathKey prefix;
    for (const X3DNode *n : path) {
      prefix.push_back(n);
      if (!TransformSystem::isTransform(n)) continue;
      if (!animatedTargets_.count(n) && !keptTransforms_.count(n)) {
        const Mat4 local = TransformSystem::localMatrix(n, projection);
        world = world * local;
        since = since * local;
        continue;
      }
      auto found = instances_.find(prefix);
      if (found == instances_.end()) {
        if (!isIdentity(since)) parent = attach(parent, "{" + matrixJson(since));
        const SFVec3f t = geombounds::getVec3fLenient(*n, "translation", {0, 0, 0});
        const SFVec3f c = geombounds::getVec3fLenient(*n, "center", {0, 0, 0});
        const SFVec3f sc = geombounds::getVec3fLenient(*n, "scale", {1, 1, 1});
        const SFRotation r =
            geombounds::getField<SFRotation>(*n, "rotation", {0, 0, 1, 0});
        const SFRotation so =
            geombounds::getField<SFRotation>(*n, "scaleOrientation", {0, 0, 1, 0});
        const SFRotation soInverse{so.x, so.y, so.z, -so.angle};
        // An animated centre or scaleOrientation needs its helper nodes even
        // when it is zero at export time.
        const bool oriented = quaternion(so)[3] < 1.0 - 1e-12 ||
                              fieldAnimated(n, "scaleOrientation");
        const bool centred = c.x != 0 || c.y != 0 || c.z != 0 ||
                             fieldAnimated(n, "center");
        const auto q = quaternion(r);

        std::string trs = "{";
        if (!n->getDEF().empty()) trs += "\"name\":" + quote(n->getDEF()) + ",";
        trs += "\"translation\":" + array({double(t.x) + c.x, double(t.y) + c.y,
                                           double(t.z) + c.z}) +
               ",\"rotation\":" + array({q[0], q[1], q[2], q[3]});
        const std::string scale = "\"scale\":" + array({sc.x, sc.y, sc.z});
        TransformInstance inst;
        if (!oriented) {
          inst.trs = inst.scale = inst.tail = attach(parent, trs + "," + scale);
          if (centred)
            inst.tail = attach(inst.trs, "{\"translation\":" +
                                             array({-double(c.x), -double(c.y),
                                                    -double(c.z)}));
        } else {
          inst.oriented = true;
          inst.trs = attach(parent, trs);
          const auto qs = quaternion(so), qi = quaternion(soInverse);
          inst.scale = attach(inst.trs, "{\"rotation\":" +
                                            array({qs[0], qs[1], qs[2], qs[3]}) +
                                            "," + scale);
          const SFVec3f back = Mat4::rotation(soInverse)
                                   .transformDirection({-c.x, -c.y, -c.z});
          inst.tail = attach(inst.scale,
                             "{\"translation\":" + array({back.x, back.y, back.z}) +
                                 ",\"rotation\":" +
                                 array({qi[0], qi[1], qi[2], qi[3]}));
        }
        // Exactly what those nodes compose to.
        inst.after = world * Mat4::translation(t) * Mat4::translation(c) *
                     Mat4::rotation(r) * Mat4::rotation(so) * Mat4::scale(sc) *
                     Mat4::rotation(soInverse) *
                     Mat4::translation({-c.x, -c.y, -c.z});
        found = instances_.emplace(prefix, inst).first;
        targetInstances_[n].push_back(inst);
      }
      parent = found->second.tail;
      world = anchor = found->second.after;
      since = Mat4::identity();
    }
    // Content hangs from the last animated node; any plain Transform between
    // it and the item stays folded into the item's own matrix.
    below = anchor;
    return parent;
  }

  bool fieldAnimated(const X3DNode *node, const char *field) const {
    if (!node) return false;
    const auto found = animatedFields_.find(node);
    return found != animatedFields_.end() && found->second.count(field);
  }

  // The X3D material node of an item's shape, or null.
  static const X3DNode *materialNodeOf(const RenderItem &item) {
    if (item.path.empty() || !item.path.back()) return nullptr;
    const auto appearance = sfNode(*item.path.back(), "appearance");
    if (!appearance) return nullptr;
    return sfNode(*appearance, "material").get();
  }

  static const X3DNode *geometryNodeOf(const RenderItem &item) {
    if (item.path.empty() || !item.path.back()) return nullptr;
    return sfNode(*item.path.back(), "geometry").get();
  }

  // The TextureTransforms baked into a target's texture coordinates for a
  // placement with `appearance`: the appearance's own, with the fields an
  // animation drives at that target (on a clone, so the scene is untouched).
  static std::vector<TextureTransform2DParams>
  textureTransformsAt(const std::shared_ptr<X3DNode> &appearance,
                      const X3DNode *geometry,
                      const std::vector<GltfTextureFieldValue> &fields) {
    if (!appearance || fields.empty())
      return textureTransformsForGeometry(appearance.get(), geometry);
    std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
    const auto clone = deepClone(appearance, cloneMap);
    if (!clone) return textureTransformsForGeometry(appearance.get(), geometry);
    for (const GltfTextureFieldValue &v : fields) {
      const auto node = cloneMap.find(v.node);
      if (node == cloneMap.end() || !node->second) continue; // not this appearance
      for (const FieldInfo &f : node->second->fields())
        if (f.x3dName == v.field && f.set) f.set(*node->second, v.value);
    }
    return textureTransformsForGeometry(clone.get(), geometry);
  }

  // The morph targets of `item`'s mesh as glTF target objects: each a
  // difference from the base mesh in the attributes the base mesh has. The
  // base mesh has the placement's TextureTransform baked in, so each target
  // gets it too, at that target's animated values.
  const std::vector<std::string> &targetsFor(const RenderItem &item,
                                             const GltfMorph &morph,
                                             const MeshAccessors &acc) {
    std::shared_ptr<X3DNode> appearance;
    if (!item.path.empty() && item.path.back())
      appearance = sfNode(*item.path.back(), "appearance");
    const auto cacheKey = std::make_pair(item.mesh.get(), appearance.get());
    auto found = morphTargets_.find(cacheKey);
    if (found != morphTargets_.end()) return found->second;
    const MeshData &base = *item.mesh;
    const std::size_t n = base.positions.size();
    auto finite = [](float v) { return std::isfinite(v) ? v : 0.0f; };
    auto unitNormal = [](const SFVec3f &v) {
      const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
      if (len > 1e-20f && std::isfinite(len))
        return SFVec3f{v.x / len, v.y / len, v.z / len};
      return SFVec3f{0, 0, 1};
    };
    std::vector<std::string> out;
    std::vector<float> flat;
    static const std::vector<GltfTextureFieldValue> kNoFields;
    for (std::size_t k = 0; k < morph.targets.size(); ++k) {
      MeshData target = morph.targets[k];
      applyTextureTransformsToMesh(
          target, textureTransformsAt(appearance, morph.geometry,
                                      k < morph.textureFields.size()
                                          ? morph.textureFields[k]
                                          : kNoFields));
      std::vector<std::string> attrs;
      flat.clear();
      for (std::size_t i = 0; i < n; ++i) {
        const SFVec3f &a = base.positions[i], &b = target.positions[i];
        flat.insert(flat.end(), {finite(b.x) - finite(a.x), finite(b.y) - finite(a.y),
                                 finite(b.z) - finite(a.z)});
      }
      // POSITION accessors, targets included, need bounds.
      attrs.push_back("\"POSITION\":" + std::to_string(floatAccessor(flat, 3, "VEC3", true)));
      if (acc.normal >= 0 && target.normals.size() == n) {
        flat.clear();
        for (std::size_t i = 0; i < n; ++i) {
          const SFVec3f a = unitNormal(base.normals[i]), b = unitNormal(target.normals[i]);
          flat.insert(flat.end(), {b.x - a.x, b.y - a.y, b.z - a.z});
        }
        attrs.push_back("\"NORMAL\":" + std::to_string(floatAccessor(flat, 3, "VEC3", false)));
      }
      for (std::size_t set = 0; set < acc.texcoords.size(); ++set) {
        const std::vector<SFVec2f> &a = set ? base.texcoordSets[set] : base.texcoords;
        const std::vector<SFVec2f> *b =
            set ? (set < target.texcoordSets.size() ? &target.texcoordSets[set] : nullptr)
                : &target.texcoords;
        if (!b || b->size() != n || a.size() != n) continue;
        flat.clear();
        for (std::size_t i = 0; i < n; ++i)
          // t is flipped to glTF's top-left origin, so its difference is too.
          flat.insert(flat.end(), {finite((*b)[i].x) - finite(a[i].x),
                                   finite(a[i].y) - finite((*b)[i].y)});
        attrs.push_back("\"TEXCOORD_" + std::to_string(set) +
                        "\":" + std::to_string(floatAccessor(flat, 2, "VEC2", false)));
      }
      out.push_back("{" + join(attrs) + "}");
    }
    return morphTargets_.emplace(cacheKey, std::move(out)).first->second;
  }

  // ---- animation ----

  // One output sampler's keyframes: times may repeat (an X3D jump).
  struct Keys {
    std::vector<float> times;
    std::vector<float> values;
    std::size_t width = 0;
    bool rotation = false; // unit quaternions, kept in one hemisphere
    bool step = false;     // glTF STEP: each key holds until the next
  };

  // The union of the channels' times, each repeated time once per side of
  // the jump: (time, right side?).
  static std::vector<std::pair<float, bool>>
  timeline(const std::vector<const GltfAnimationChannel *> &channels,
           bool densify) {
    std::vector<float> all;
    std::set<float> jumps;
    for (const GltfAnimationChannel *ch : channels) {
      all.insert(all.end(), ch->times.begin(), ch->times.end());
      for (std::size_t i = 1; i < ch->times.size(); ++i)
        if (ch->times[i] == ch->times[i - 1]) jumps.insert(ch->times[i]);
    }
    std::sort(all.begin(), all.end());
    all.erase(std::unique(all.begin(), all.end()), all.end());
    std::vector<std::pair<float, bool>> out;
    for (std::size_t i = 0; i < all.size(); ++i) {
      out.emplace_back(all[i], false);
      if (jumps.count(all[i])) out.emplace_back(all[i], true);
      if (densify && i + 1 < all.size())
        for (int k = 1; k < kSubsteps; ++k)
          out.emplace_back(all[i] + (all[i + 1] - all[i]) * float(k) / kSubsteps,
                           false);
    }
    return out;
  }

  // A channel's value at `time` (`right`: after a jump there), linear between
  // keys, spherical for quaternions, held before the first and after the last.
  static void valueAt(const GltfAnimationChannel &ch, std::size_t width,
                      bool rotation, float time, bool right, float *out) {
    const std::vector<float> &t = ch.times;
    const auto lo = std::lower_bound(t.begin(), t.end(), time);
    const auto hi = std::upper_bound(t.begin(), t.end(), time);
    std::size_t at;
    if (lo != hi) {
      at = static_cast<std::size_t>((right ? hi - 1 : lo) - t.begin());
    } else if (hi == t.begin()) {
      at = 0;
    } else if (hi == t.end()) {
      at = t.size() - 1;
    } else if (ch.step) {
      at = static_cast<std::size_t>(hi - t.begin()) - 1;
    } else {
      const std::size_t i = static_cast<std::size_t>(hi - t.begin()) - 1;
      const float u = (time - t[i]) / (t[i + 1] - t[i]);
      const float *a = &ch.values[i * width], *b = &ch.values[(i + 1) * width];
      if (!rotation) {
        for (std::size_t k = 0; k < width; ++k) out[k] = a[k] + (b[k] - a[k]) * u;
        return;
      }
      const Quat q = slerp(Quat{a[0], a[1], a[2], a[3]}, Quat{b[0], b[1], b[2], b[3]}, u);
      out[0] = float(q.x); out[1] = float(q.y); out[2] = float(q.z); out[3] = float(q.w);
      return;
    }
    for (std::size_t k = 0; k < width; ++k) out[k] = ch.values[at * width + k];
  }

  // Sample `fn(time, right, out)` over the channels' common timeline.
  template <typename Fn>
  static Keys resample(const std::vector<const GltfAnimationChannel *> &channels,
                       std::size_t width, bool rotation, bool densify, Fn fn) {
    Keys keys;
    keys.width = width;
    keys.rotation = rotation;
    keys.step = !channels.empty() &&
                std::all_of(channels.begin(), channels.end(),
                            [](const GltfAnimationChannel *c) { return c->step; });
    std::vector<float> v(width);
    for (const auto &[time, right] : timeline(channels, densify)) {
      fn(time, right, v.data());
      keys.times.push_back(time);
      keys.values.insert(keys.values.end(), v.begin(), v.end());
    }
    return keys;
  }

  // Write `keys` as a LINEAR (or STEP) sampler of `samplers`; returns its
  // index.
  int sampler(Keys keys, std::vector<std::string> &samplers) {
    // glTF wants strictly increasing times; X3D allows a repeated key for a
    // jump. Nudge repeats forward by the smallest float step.
    std::vector<float> &times = keys.times;
    for (std::size_t i = 0; i < times.size(); ++i) {
      if (!std::isfinite(times[i]) || times[i] < 0) times[i] = 0;
      if (i && times[i] <= times[i - 1])
        times[i] = std::nextafter(times[i - 1], std::numeric_limits<float>::max());
    }
    if (keys.rotation) {
      // Unit length, and each key in the hemisphere of the one before.
      for (std::size_t i = 0; i < times.size(); ++i) {
        float *q = &keys.values[i * 4];
        const float len =
            std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (!(len > 1e-12f) || !std::isfinite(len)) {
          q[0] = q[1] = q[2] = 0;
          q[3] = 1;
        } else {
          for (int k = 0; k < 4; ++k) q[k] /= len;
        }
        if (i) {
          const float *p = q - 4;
          if (p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3] < 0)
            for (int k = 0; k < 4; ++k) q[k] = -q[k];
        }
      }
    }
    for (float &v : keys.values)
      if (!std::isfinite(v)) v = 0;
    static const char *const kTypes[] = {"", "SCALAR", "VEC2", "VEC3", "VEC4"};
    const int input = floatAccessor(times, 1, "SCALAR", true, 0);
    const int output = floatAccessor(keys.values, static_cast<int>(keys.width),
                                     kTypes[keys.width], false, 0);
    samplers.push_back("{\"input\":" + std::to_string(input) + ",\"output\":" +
                       std::to_string(output) + ",\"interpolation\":\"" +
                       (keys.step ? "STEP" : "LINEAR") + "\"}");
    return static_cast<int>(samplers.size()) - 1;
  }

  void nodeChannel(std::vector<std::string> &channels, int sampler, int node,
                   const char *path) {
    channels.push_back("{\"sampler\":" + std::to_string(sampler) +
                       ",\"target\":{\"node\":" + std::to_string(node) +
                       ",\"path\":\"" + path + "\"}}");
    ++report_.animationChannels;
  }

  void pointerChannel(std::vector<std::string> &channels, int sampler,
                      const std::string &pointer) {
    channels.push_back(
        "{\"sampler\":" + std::to_string(sampler) +
        ",\"target\":{\"path\":\"pointer\",\"extensions\":{"
        "\"KHR_animation_pointer\":{\"pointer\":" + quote(pointer) + "}}}}");
    ++report_.animationChannels;
    ++report_.pointerChannels;
    usesPointer_ = true;
  }

  // The channels of one target in one animation, by X3D field (node paths
  // by their glTF name).
  using FieldChannels = std::map<std::string, const GltfAnimationChannel *>;

  static std::string fieldKey(const GltfAnimationChannel &ch) {
    switch (ch.path) {
      case GltfAnimationChannel::Path::Translation: return "translation";
      case GltfAnimationChannel::Path::Rotation: return "rotation";
      case GltfAnimationChannel::Path::Scale: return "scale";
      case GltfAnimationChannel::Path::Weights: return "weights";
      default: return ch.field;
    }
  }

  // Translation, rotation, scale, center and scaleOrientation of a node kept
  // as glTF nodes (see animatedParent for the layout).
  void writeNodeChannels(const X3DNode &target, const FieldChannels &f,
                         std::vector<std::string> &samplers,
                         std::vector<std::string> &channels) {
    const auto instances = targetInstances_.find(&target);
    if (instances == targetInstances_.end()) return;
    auto get = [&](const char *name) -> const GltfAnimationChannel * {
      const auto it = f.find(name);
      return it == f.end() ? nullptr : it->second;
    };
    const GltfAnimationChannel *T = get("translation"), *R = get("rotation"),
                               *S = get("scale"), *C = get("center"),
                               *SO = get("scaleOrientation");
    const SFVec3f c0 = geombounds::getVec3fLenient(target, "center", {0, 0, 0});
    const SFVec3f t0 = geombounds::getVec3fLenient(target, "translation", {0, 0, 0});
    const auto so0 = quaternion(
        geombounds::getField<SFRotation>(target, "scaleOrientation", {0, 0, 1, 0}));
    auto vec3 = [](const GltfAnimationChannel *ch, const SFVec3f &dflt, float time,
                   bool right) {
      std::array<float, 3> v{dflt.x, dflt.y, dflt.z};
      if (ch) valueAt(*ch, 3, false, time, right, v.data());
      return v;
    };
    auto quat = [&](float time, bool right) {
      std::array<float, 4> q{float(so0[0]), float(so0[1]), float(so0[2]),
                             float(so0[3])};
      if (SO) valueAt(*SO, 4, true, time, right, q.data());
      return q;
    };

    int trsT = -1, trsR = -1, scl = -1, soR = -1, tailT = -1, tailR = -1;
    if (T || C) // the trs node's translation is T + centre
      trsT = sampler(resample({T ? T : C, C ? C : T}, 3, false, false,
                              [&](float time, bool right, float *out) {
                                const auto t = vec3(T, t0, time, right);
                                const auto c = vec3(C, c0, time, right);
                                for (int k = 0; k < 3; ++k) out[k] = t[k] + c[k];
                              }),
                     samplers);
    if (R)
      trsR = sampler(resample({R}, 4, true, false,
                              [&](float time, bool right, float *out) {
                                valueAt(*R, 4, true, time, right, out);
                              }),
                     samplers);
    if (S)
      scl = sampler(resample({S}, 3, false, false,
                             [&](float time, bool right, float *out) {
                               valueAt(*S, 3, false, time, right, out);
                             }),
                    samplers);
    if (SO && std::any_of(instances->second.begin(), instances->second.end(),
                          [](const TransformInstance &i) { return i.oriented; })) {
      soR = sampler(resample({SO}, 4, true, false,
                             [&](float time, bool right, float *out) {
                               const auto q = quat(time, right);
                               std::copy(q.begin(), q.end(), out);
                             }),
                    samplers);
      tailR = sampler(resample({SO}, 4, true, false,
                               [&](float time, bool right, float *out) {
                                 const auto q = quat(time, right);
                                 out[0] = -q[0]; out[1] = -q[1]; out[2] = -q[2];
                                 out[3] = q[3];
                               }),
                      samplers);
    }
    // The tail undoes the centre: -C, or SR^-1(-C) under a scaleOrientation
    // (a rotated vector, so densified when SR moves).
    bool anyOriented = false, anyPlain = false;
    for (const TransformInstance &inst : instances->second)
      (inst.oriented ? anyOriented : anyPlain) = true;
    int tailOriented = -1;
    if ((C || SO) && anyOriented)
      tailOriented = sampler(
          resample({C ? C : SO, SO ? SO : C}, 3, false, SO != nullptr,
                   [&](float time, bool right, float *out) {
                     const auto c = vec3(C, c0, time, right);
                     const auto q = quat(time, right);
                     // Rotate -C by the conjugate of q.
                     const Quat inv{-q[0], -q[1], -q[2], q[3]};
                     const Quat r = quatMul(quatMul(inv, Quat{-c[0], -c[1], -c[2], 0}),
                                            Quat{q[0], q[1], q[2], q[3]});
                     out[0] = float(r.x); out[1] = float(r.y); out[2] = float(r.z);
                   }),
          samplers);
    if (C && anyPlain)
      tailT = sampler(resample({C}, 3, false, false,
                               [&](float time, bool right, float *out) {
                                 valueAt(*C, 3, false, time, right, out);
                                 for (int k = 0; k < 3; ++k) out[k] = -out[k];
                               }),
                      samplers);

    for (const TransformInstance &inst : instances->second) {
      if (trsT >= 0) nodeChannel(channels, trsT, inst.trs, "translation");
      if (trsR >= 0) nodeChannel(channels, trsR, inst.trs, "rotation");
      if (scl >= 0) nodeChannel(channels, scl, inst.scale, "scale");
      if (inst.oriented) {
        if (soR >= 0) nodeChannel(channels, soR, inst.scale, "rotation");
        if (tailR >= 0) nodeChannel(channels, tailR, inst.tail, "rotation");
        if (tailOriented >= 0)
          nodeChannel(channels, tailOriented, inst.tail, "translation");
      } else if (tailT >= 0 && inst.tail != inst.trs) {
        nodeChannel(channels, tailT, inst.tail, "translation");
      }
    }
  }

  // Material, light and camera properties, through KHR_animation_pointer.
  // Returns false when the target has nothing exported to animate.
  bool writePointerChannels(const X3DNode &target, const FieldChannels &f,
                            std::vector<std::string> &samplers,
                            std::vector<std::string> &channels) {
    auto get = [&](const char *name) -> const GltfAnimationChannel * {
      const auto it = f.find(name);
      return it == f.end() ? nullptr : it->second;
    };
    auto unit = [](float v) { return std::clamp(v, 0.0f, 1.0f); };
    bool wrote = false;

    if (const auto mats = materialsBySource_.find(&target);
        mats != materialsBySource_.end()) {
      for (int index : mats->second) {
        const MaterialStatic &st = materialStatics_.at(index);
        const std::string at = "/materials/" + std::to_string(index);
        // The base colour: RGB from the lit colour field (or the emissive
        // colour when unlit), alpha from transparency.
        const char *rgbField = st.unlit ? "emissiveColor"
                               : st.model == MaterialModel::Physical ? "baseColor"
                               : st.model == MaterialModel::Unlit ? "emissiveColor"
                                                                    : "diffuseColor";
        const GltfAnimationChannel *rgb = st.vertexColor ? nullptr : get(rgbField);
        const GltfAnimationChannel *tr = get("transparency");
        if (rgb || tr) {
          std::vector<const GltfAnimationChannel *> in;
          if (rgb) in.push_back(rgb);
          if (tr) in.push_back(tr);
          pointerChannel(channels,
                         sampler(resample(in, 4, false, false,
                                          [&](float time, bool right, float *out) {
                                            std::copy(st.base.begin(), st.base.end(), out);
                                            if (rgb) valueAt(*rgb, 3, false, time, right, out);
                                            float a = 1.0f - st.base[3];
                                            if (tr) valueAt(*tr, 1, false, time, right, &a);
                                            for (int k = 0; k < 3; ++k) out[k] = unit(out[k]);
                                            out[3] = unit(1.0f - a);
                                          }),
                                 samplers),
                         at + "/pbrMetallicRoughness/baseColorFactor");
          wrote = true;
        }
        if (const auto *e = get("emissiveColor"); e && !st.unlit &&
                                                   st.model != MaterialModel::Unlit) {
          pointerChannel(channels,
                         sampler(resample({e}, 3, false, false,
                                          [&](float time, bool right, float *out) {
                                            valueAt(*e, 3, false, time, right, out);
                                            for (int k = 0; k < 3; ++k) out[k] = unit(out[k]);
                                          }),
                                 samplers),
                         at + "/emissiveFactor");
          wrote = true;
        }
        auto scalar = [&](const GltfAnimationChannel *ch, const char *pointer,
                          float (*map)(float)) {
          if (!ch) return;
          pointerChannel(channels,
                         sampler(resample({ch}, 1, false, false,
                                          [&](float time, bool right, float *out) {
                                            valueAt(*ch, 1, false, time, right, out);
                                            out[0] = map(out[0]);
                                          }),
                                 samplers),
                         at + pointer);
          wrote = true;
        };
        if (!st.unlit && st.model == MaterialModel::Physical) {
          scalar(get("metallic"), "/pbrMetallicRoughness/metallicFactor",
                 [](float v) { return std::clamp(v, 0.0f, 1.0f); });
          scalar(get("roughness"), "/pbrMetallicRoughness/roughnessFactor",
                 [](float v) { return std::clamp(v, 0.0f, 1.0f); });
        } else if (!st.unlit && st.model == MaterialModel::Phong) {
          // Blinn-Phong exponent (shininess * 128) to perceptual roughness,
          // as for the static value.
          scalar(get("shininess"), "/pbrMetallicRoughness/roughnessFactor",
                 [](float v) {
                   const double exponent = std::max(0.0, double(v)) * 128.0;
                   return float(std::pow(2.0 / (exponent + 2.0), 0.25));
                 });
        }
      }
      return wrote;
    }

    if (const auto lights = lightsByNode_.find(&target);
        lights != lightsByNode_.end()) {
      for (int index : lights->second) {
        const LightStatic &st = lightStatics_[static_cast<std::size_t>(index)];
        const std::string at =
            "/extensions/KHR_lights_punctual/lights/" + std::to_string(index);
        if (const auto *ch = get("color")) {
          pointerChannel(channels,
                         sampler(resample({ch}, 3, false, false,
                                          [&](float time, bool right, float *out) {
                                            valueAt(*ch, 3, false, time, right, out);
                                            for (int k = 0; k < 3; ++k) out[k] = unit(out[k]);
                                          }),
                                 samplers),
                         at + "/color");
          wrote = true;
        }
        auto scalar = [&](const GltfAnimationChannel *ch, const std::string &pointer,
                          auto map) {
          if (!ch) return;
          pointerChannel(channels,
                         sampler(resample({ch}, 1, false, false,
                                          [&](float time, bool right, float *out) {
                                            valueAt(*ch, 1, false, time, right, out);
                                            out[0] = map(out[0]);
                                          }),
                                 samplers),
                         pointer);
          wrote = true;
        };
        scalar(get("intensity"), at + "/intensity",
               [](float v) { return std::max(0.0f, v); });
        if (st.type != LightDesc::Type::Directional)
          scalar(get("radius"), at + "/range",
                 [](float v) { return std::max(v, kMinRange); });
        if (st.type == LightDesc::Type::Spot) {
          // glTF: 0 <= inner < outer <= pi/2, so the two clamp together.
          const auto *beam = get("beamWidth");
          const auto *cut = get("cutOffAngle");
          auto cone = [&](float time, bool right) {
            float b = st.beamWidth, c = st.cutOffAngle;
            if (beam) valueAt(*beam, 1, false, time, right, &b);
            if (cut) valueAt(*cut, 1, false, time, right, &c);
            const float outer = std::clamp(c, 1e-4f, 1.5707963f);
            return std::make_pair(std::clamp(b, 0.0f, outer - 1e-5f), outer);
          };
          std::vector<const GltfAnimationChannel *> in;
          if (beam) in.push_back(beam);
          if (cut) in.push_back(cut);
          if (!in.empty()) {
            pointerChannel(channels,
                           sampler(resample(in, 1, false, false,
                                            [&](float time, bool right, float *out) {
                                              out[0] = cone(time, right).first;
                                            }),
                                   samplers),
                           at + "/spot/innerConeAngle");
            pointerChannel(channels,
                           sampler(resample(in, 1, false, false,
                                            [&](float time, bool right, float *out) {
                                              out[0] = cone(time, right).second;
                                            }),
                                   samplers),
                           at + "/spot/outerConeAngle");
            wrote = true;
          }
        }
      }
      return wrote;
    }

    if (const auto cams = camerasByNode_.find(&target);
        cams != camerasByNode_.end()) {
      if (const auto *ch = get("fieldOfView")) {
        const int s = sampler(resample({ch}, 1, false, false,
                                       [&](float time, bool right, float *out) {
                                         valueAt(*ch, 1, false, time, right, out);
                                         out[0] = std::clamp(out[0], 1e-3f, 3.14f);
                                       }),
                              samplers);
        for (int index : cams->second)
          pointerChannel(channels, s,
                         "/cameras/" + std::to_string(index) + "/perspective/yfov");
        wrote = true;
      }
    }
    return wrote;
  }

  void writeAnimations() {
    if (!animations_) return;
    for (const GltfAnimation &animation : *animations_) {
      // Group the channels by target; within a target the last channel for
      // a field wins, like the last ROUTE event.
      std::vector<const X3DNode *> order;
      std::unordered_map<const X3DNode *, FieldChannels> byTarget;
      for (const GltfAnimationChannel &ch : animation.channels) {
        const bool rotation =
            ch.path == GltfAnimationChannel::Path::Rotation ||
            (ch.path == GltfAnimationChannel::Path::Property &&
             ch.field == "scaleOrientation");
        const auto morph = ch.path == GltfAnimationChannel::Path::Weights
                               ? morphs_.find(ch.target) : morphs_.end();
        const std::size_t width =
            ch.path == GltfAnimationChannel::Path::Weights
                ? (morph == morphs_.end() ? 0 : morph->second->targets.size())
            : rotation ? 4
            : ch.path != GltfAnimationChannel::Path::Property ? 3
            : (ch.field == "center" || ch.field.find("Color") != std::string::npos ||
               ch.field == "color")
                ? 3
                : 1;
        if (!ch.target || ch.times.empty() || width == 0 ||
            ch.values.size() != ch.times.size() * width ||
            !std::is_sorted(ch.times.begin(), ch.times.end())) {
          ++report_.skippedAnimationChannels;
          continue;
        }
        auto [it, fresh] = byTarget.try_emplace(ch.target);
        if (fresh) order.push_back(ch.target);
        it->second[fieldKey(ch)] = &ch;
      }
      std::vector<std::string> samplers, channels;
      // Joint displacer weights, gathered per skin: glTF animates a mesh's
      // weights as one array.
      std::map<const hanim::SkinBinding *,
               std::vector<std::pair<std::size_t, const GltfAnimationChannel *>>>
          displacerWeights;
      for (const X3DNode *target : order) {
        const FieldChannels &f = byTarget[target];
        if (const auto d = skinDisplacers_.find(target); d != skinDisplacers_.end()) {
          for (const auto &[name, ch] : f) {
            if (name == "weight") displacerWeights[d->second.first].emplace_back(d->second.second, ch);
            else ++report_.skippedAnimationChannels;
          }
          continue;
        }
        FieldChannels nodeFields, pointerFields;
        for (const auto &[name, ch] : f) {
          const bool node = ch->path != GltfAnimationChannel::Path::Property ||
                            name == "center" || name == "scaleOrientation";
          (node ? nodeFields : pointerFields).emplace(name, ch);
        }
        if (const auto w = nodeFields.find("weights"); w != nodeFields.end()) {
          // Morph weights: every exported shape using the geometry.
          const auto nodes = morphNodes_.find(target);
          if (nodes == morphNodes_.end()) {
            ++report_.skippedAnimationChannels;
          } else {
            Keys keys;
            keys.times = w->second->times;
            keys.values = w->second->values;
            keys.width = 1;
            const int s = sampler(std::move(keys), samplers);
            for (int node : nodes->second) nodeChannel(channels, s, node, "weights");
          }
          nodeFields.erase(w);
        }
        const std::size_t before = channels.size();
        if (!nodeFields.empty())
          writeNodeChannels(*target, nodeFields, samplers, channels);
        if (channels.size() == before && !nodeFields.empty())
          report_.skippedAnimationChannels += nodeFields.size();
        if (!pointerFields.empty() &&
            !writePointerChannels(*target, pointerFields, samplers, channels))
          report_.skippedAnimationChannels += pointerFields.size();
      }
      for (const auto &[binding, list] : displacerWeights) {
        const std::size_t n = binding->displacers.size();
        std::vector<float> rest;
        for (const hanim::JointDisplacer &d : binding->displacers)
          rest.push_back(geombounds::getField<float>(*d.node, "weight", 0.0f));
        std::vector<const GltfAnimationChannel *> in;
        for (const auto &entry : list) in.push_back(entry.second);
        Keys keys = resample(in, n, false, false, [&](float time, bool right, float *out) {
          std::copy(rest.begin(), rest.end(), out);
          for (const auto &[k, ch] : list) valueAt(*ch, 1, false, time, right, &out[k]);
        });
        keys.width = 1; // glTF weights: one SCALAR per target per key
        const int s = sampler(std::move(keys), samplers);
        for (int node : skinMeshNodes_[binding->humanoid])
          nodeChannel(channels, s, node, "weights");
      }
      if (channels.empty()) continue;
      std::string json = "{";
      if (!animation.name.empty()) json += "\"name\":" + quote(animation.name) + ",";
      animationJson_.push_back(json + "\"channels\":[" + join(channels) +
                               "],\"samplers\":[" + join(samplers) + "]}");
    }
    report_.animations = animationJson_.size();
  }

  int floatAccessor(const std::vector<float> &data, int components,
                    const char *type, bool bounds, int target = kArrayBuffer) {
    const int v = view(data.data(), data.size() * sizeof(float), target);
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
        // Prefer the file's own bytes (a GLB is then self-contained and does
        // not depend on where it is written); else reference the url.
        image = embedFile(url, ref.baseUrl);
        if (image < 0) {
          image = intern(images_, "{\"uri\":" + quote(url) + "}");
          ++report_.externalImages;
        }
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

  // Embed the PNG or JPEG file behind `url` through options_.readImage. -1
  // when there is no reader, it returns nothing, or the bytes are neither.
  int embedFile(const std::string &url, const std::string &baseUrl) {
    if (!options_.readImage) return -1;
    const std::string key = std::to_string(baseUrl.size()) + ":" + baseUrl + url;
    auto cached = fileImages_.find(key);
    if (cached != fileImages_.end()) return cached->second;
    const std::string bytes = options_.readImage(url, baseUrl);
    int image = -1;
    if (bytes.compare(0, 8, std::string("\x89PNG\r\n\x1a\n", 8)) == 0)
      image = embed(bytes, "image/png");
    else if (bytes.compare(0, 3, "\xFF\xD8\xFF") == 0)
      image = embed(bytes, "image/jpeg");
    fileImages_.emplace(key, image);
    return image;
  }

  // Store image bytes in the buffer and return the image index; identical
  // images are stored once. -1 for an empty (unencodable) image.
  int embed(const std::string &png, const char *mime = "image/png") {
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
                      ",\"mimeType\":\"" + mime + "\"}");
    ++report_.embeddedImages;
    return embedded_.emplace(png, static_cast<int>(images_.size()) - 1)
        .first->second;
  }

  int materialFor(const MaterialDesc &m, const MeshData &mesh,
                  const MeshAccessors &acc, const X3DNode *source) {
    // A material an animation drives gets glTF materials of its own, so a
    // channel aimed at it moves nothing else.
    if (source && !animatedFields_.count(source)) source = nullptr;
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
    // glTF factors must lie in [0,1]; the lenient readers keep out-of-range
    // authored colours (validateRanges reports them).
    auto unit = [](float v) { return std::clamp(v, 0.0f, 1.0f); };
    base = {unit(base.r), unit(base.g), unit(base.b), unit(base.a)};
    double metallic = 0.0, roughness = 1.0;
    if (m.model == MaterialModel::Physical) {
      metallic = unit(m.physical.metallic);
      roughness = unit(m.physical.roughness);
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
    // Each glTF texture property is written at most once: the first texture
    // that maps to it wins and any further one is counted as unmapped.
    std::vector<std::string> written;
    auto slot = [&](std::vector<std::string> &into, const char *key,
                    const TextureRef &ref, const std::string &extra = {}) {
      if (std::find(written.begin(), written.end(), key) != written.end()) {
        ++report_.unmappedTextureSlots;
        return false;
      }
      const std::string info = textureInfo(ref, acc, extra);
      if (info.empty()) return false;
      written.push_back(key);
      into.push_back(std::string("\"") + key + "\":" + info);
      return true;
    };
    bool emissiveTexture = false;
    const bool phong = m.model == MaterialModel::Phong;
    for (const TextureRef &ref : m.textures) {
      if (unlit) {
        // KHR_materials_unlit shows the base colour, and an unlit X3D
        // surface's colour is its emissive colour and texture: an Appearance
        // with a texture and no Material, or UnlitMaterial.emissiveTexture.
        if (ref.slot == Slot::Emissive)
          slot(pbr, "baseColorTexture", ref);
        else
          ++report_.unmappedTextureSlots; // lighting inputs of an unlit surface
      } else if (ref.slot == Slot::BaseColor ||
                 (phong && ref.slot == Slot::Diffuse)) {
        slot(pbr, "baseColorTexture", ref);
      } else if (ref.slot == Slot::Normal) {
        slot(fields, "normalTexture", ref, ",\"scale\":" + num(m.normalScale));
      } else if (ref.slot == Slot::Emissive) {
        emissiveTexture = slot(fields, "emissiveTexture", ref);
      } else if (ref.slot == Slot::Occlusion) {
        const double strength = phong ? m.phong.occlusionStrength
                                      : m.physical.occlusionStrength;
        slot(fields, "occlusionTexture", ref,
             ",\"strength\":" + num(std::clamp(strength, 0.0, 1.0)));
      } else if (ref.slot == Slot::MetallicRoughness && !phong) {
        slot(pbr, "metallicRoughnessTexture", ref);
      } else {
        ++report_.unmappedTextureSlots; // specular, shininess, ambient
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
      if (e.r != 0 || e.g != 0 || e.b != 0 ||
          fieldAnimated(source, "emissiveColor"))
        fields.push_back("\"emissiveFactor\":" +
                         array({std::clamp(e.r, 0.0f, 1.0f),
                                std::clamp(e.g, 0.0f, 1.0f),
                                std::clamp(e.b, 0.0f, 1.0f)}));
    }
    if (m.alphaMode == AlphaMode::Mask) {
      fields.push_back("\"alphaMode\":\"MASK\"");
      fields.push_back("\"alphaCutoff\":" + num(std::max(0.0f, m.alphaCutoff)));
    } else if (m.alphaMode == AlphaMode::Blend || base.a < 1.0f ||
               fieldAnimated(source, "transparency")) {
      fields.push_back("\"alphaMode\":\"BLEND\"");
    }
    if (m.doubleSided || !mesh.solid)
      fields.push_back("\"doubleSided\":true");
    if (unlit) {
      fields.push_back("\"extensions\":{\"KHR_materials_unlit\":{}}");
      usesUnlit_ = true;
    }
    const std::string json = "{" + join(fields) + "}";
    const auto key = std::make_pair(json, source);
    auto it = materialIndex_.find(key);
    if (it == materialIndex_.end()) {
      materials_.push_back(json);
      const int index = static_cast<int>(materials_.size()) - 1;
      it = materialIndex_.emplace(key, index).first;
      if (source) {
        MaterialStatic st;
        st.unlit = unlit;
        st.vertexColor = acc.color >= 0;
        st.model = m.model;
        st.base = {base.r, base.g, base.b, base.a};
        st.emissive = {unit(m.emissive.r), unit(m.emissive.g), unit(m.emissive.b)};
        st.metallic = float(metallic);
        st.roughness = float(roughness);
        materialStatics_.emplace(index, st);
        materialsBySource_[source].push_back(index);
      }
    }
    return it->second;
  }

  const GltfWriteOptions &options_;
  GltfWriteReport &report_;
  std::string bin_;
  std::vector<Node> nodes_;
  std::vector<int> roots_;
  std::size_t itemNodes_ = 0;
  std::vector<std::string> meshes_, materials_, textures_, images_, samplers_,
      accessors_, views_, lights_, cameras_, animationJson_;
  const std::vector<GltfAnimation> *animations_ = nullptr;
  std::unordered_set<const X3DNode *> animatedTargets_;
  std::map<PathKey, TransformInstance> instances_;
  std::unordered_map<const X3DNode *, std::vector<TransformInstance>>
      targetInstances_;
  std::unordered_map<std::string, int> embedded_, fileImages_;
  std::unordered_map<const TexturePixels *, int> decodedImages_;
  std::unordered_map<const MeshData *, MeshAccessors> meshAccessors_;
  std::map<std::tuple<const MeshData *, int, const X3DNode *>, int> meshIndex_;
  std::map<std::pair<std::string, const X3DNode *>, int> materialIndex_;
  // Animation targets: every field driven on a node, over all animations.
  std::unordered_map<const X3DNode *, std::set<std::string>> animatedFields_;
  std::unordered_map<const X3DNode *, std::vector<int>> materialsBySource_,
      lightsByNode_, camerasByNode_;
  std::unordered_map<int, MaterialStatic> materialStatics_;
  std::vector<LightStatic> lightStatics_; // one per glTF light
  std::unordered_map<const X3DNode *, const GltfMorph *> morphs_;
  // Skins: kept joint nodes, skeleton chains, the skins and bind meshes.
  struct SkinMesh {
    MeshRef mesh;                        // bind-pose copy of the item mesh
    std::vector<std::string> attributes; // JOINTS_n / WEIGHTS_n
    std::vector<std::string> targets;    // Joint displacer morph targets
  };
  std::unordered_set<const X3DNode *> keptTransforms_;
  std::unordered_map<const X3DNode *, std::unordered_map<const X3DNode *, PathKey>>
      skeletons_;
  std::map<std::pair<const hanim::SkinBinding *, PathKey>, int> skinIndex_;
  std::vector<std::string> skins_;
  std::map<std::pair<const MeshData *, const hanim::SkinBinding *>, SkinMesh> skinMeshes_;
  std::map<std::tuple<const MeshData *, int>, int> skinMeshIndex_;
  // Mesh nodes of each humanoid's skins with displacer targets, and each
  // Joint displacer's binding and target index.
  std::unordered_map<const X3DNode *, std::vector<int>> skinMeshNodes_;
  std::unordered_map<const X3DNode *, std::pair<const hanim::SkinBinding *, std::size_t>>
      skinDisplacers_;
  std::unordered_map<const X3DNode *, std::vector<int>> morphNodes_;
  std::map<std::pair<const MeshData *, const X3DNode *>, std::vector<std::string>>
      morphTargets_;
  bool usesPointer_ = false;
  bool usesUnlit_ = false;
};

} // namespace

std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfSceneExtras &extras,
                     const GltfWriteOptions &options, GltfWriteReport *report) {
  GltfWriteReport local;
  GltfWriteReport &r = report ? *report : local;
  // gltfExtrasFrom reports what it skipped before the writer runs; keep it.
  const std::size_t skippedViewpoints = report ? r.skippedViewpoints : 0;
  const std::size_t skippedRoutes = report ? r.skippedAnimationRoutes : 0;
  const std::size_t skippedMotions = report ? r.skippedMotions : 0;
  r = GltfWriteReport{};
  r.skippedViewpoints = skippedViewpoints;
  r.skippedAnimationRoutes = skippedRoutes;
  r.skippedMotions = skippedMotions;
  Writer writer(options, r);
  writer.setMorphs(extras.morphs);
  writer.setAnimations(extras.animations);
  writer.prepareSkins(items);
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
  // First placement of every node, as its ancestors (roots first).
  std::unordered_map<const X3DNode *, PathKey> ancestors;
  {
    PathKey stack;
    std::function<void(const X3DNode *)> rec = [&](const X3DNode *n) {
      if (!n || stack.size() >= kMaxNestingDepth ||
          !ancestors.emplace(n, stack).second)
        return;
      stack.push_back(n);
      forEachChildNode(*n, [&](const FieldInfo &,
                               const std::shared_ptr<X3DNode> &c) {
        rec(c.get());
      });
      stack.pop_back();
    };
    for (const auto &root : extractor.scene().rootNodes) rec(root.get());
  }
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
    if (const auto placed = ancestors.find(n); placed != ancestors.end()) {
      cam.node = n;
      cam.path = placed->second;
    }
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

  // ---- animations: TimeSensor -> interpolator -> Transform/viewpoint/light ----
  const Scene &scene = extractor.scene();
  auto endpoint = [&](const std::weak_ptr<X3DNode> &resolved,
                      const std::string &def) {
    auto node = resolved.lock();
    return node ? node : scene.resolve(def);
  };
  // "set_x" and "x_changed" both name field x.
  auto fieldName = [](std::string f) {
    if (f.rfind("set_", 0) == 0) f.erase(0, 4);
    const std::string suffix = "_changed";
    if (f.size() > suffix.size() &&
        f.compare(f.size() - suffix.size(), suffix.size(), suffix) == 0)
      f.erase(f.size() - suffix.size());
    return f;
  };
  auto isInterpolator = [](const X3DNode &n) {
    const std::string type = n.nodeTypeName();
    // PositionInterpolator2D and CoordinateInterpolator2D end in "2D".
    for (const std::string tail : {"Interpolator", "Interpolator2D"})
      if (type.size() > tail.size() &&
          type.compare(type.size() - tail.size(), tail.size(), tail) == 0)
        return true;
    return false;
  };

  // The TimeSensor driving an interpolator sets its time scale.
  auto clockOf = [&](const std::shared_ptr<X3DNode> &interpolator) {
    for (const Route &drive : scene.routes) {
      const auto sink = endpoint(drive.to, drive.toNode);
      if (sink != interpolator || fieldName(drive.toField) != "fraction") continue;
      const auto source = endpoint(drive.from, drive.fromNode);
      if (source && source->nodeTypeName() == "TimeSensor" &&
          fieldName(drive.fromField) == "fraction")
        return static_cast<const X3DNode *>(source.get());
    }
    return static_cast<const X3DNode *>(nullptr);
  };
  auto durationOf = [](const X3DNode *clock) {
    double duration = 1.0;
    if (clock) {
      duration = geombounds::getField<double>(*clock, "cycleInterval", 1.0);
      if (!(duration > 0) || !std::isfinite(duration)) duration = 1.0;
    }
    return duration;
  };

  // Every node inside a shape's geometry, mapped to the geometry nodes it
  // is part of: an interpolator driving one of them changes those meshes.
  std::unordered_map<const X3DNode *, std::vector<std::shared_ptr<X3DNode>>>
      geometriesOf;
  x3d::runtime::detail::forEachNode(scene, [&](X3DNode *n) {
    auto geometry = sfNode(*n, "geometry");
    if (!geometry) return;
    std::unordered_set<const X3DNode *> seen;
    std::function<void(const X3DNode *, std::size_t)> rec =
        [&](const X3DNode *m, std::size_t depth) {
          if (!m || depth > kMaxNestingDepth || !seen.insert(m).second) return;
          auto &list = geometriesOf[m];
          if (std::find(list.begin(), list.end(), geometry) == list.end())
            list.push_back(geometry);
          forEachChildNode(*m, [&](const FieldInfo &,
                                   const std::shared_ptr<X3DNode> &c) {
            rec(c.get(), depth + 1);
          });
        };
    rec(geometry.get(), 0);
  });
  // Every TextureTransform on a shape's appearance (directly, or inside a
  // MultiTextureTransform), mapped to the geometry nodes of those shapes: it
  // is baked into their texture coordinates.
  std::unordered_map<const X3DNode *, std::vector<std::shared_ptr<X3DNode>>>
      texturedGeometriesOf;
  x3d::runtime::detail::forEachNode(scene, [&](X3DNode *n) {
    auto geometry = sfNode(*n, "geometry");
    auto appearance = sfNode(*n, "appearance");
    if (!geometry || !appearance) return;
    auto transform = sfNode(*appearance, "textureTransform");
    if (!transform) return;
    std::vector<std::shared_ptr<X3DNode>> transforms{transform};
    if (transform->nodeTypeName() == "MultiTextureTransform")
      for (const auto &child :
           geombounds::getField<MFNode>(*transform, "textureTransform", {}))
        if (child) transforms.push_back(child);
    for (const auto &t : transforms) {
      auto &list = texturedGeometriesOf[t.get()];
      if (std::find(list.begin(), list.end(), geometry) == list.end())
        list.push_back(geometry);
    }
  });

  std::map<std::pair<const X3DNode *, const X3DNode *>,
           std::vector<GeometrySource>> bySurface; // (geometry, clock)
  std::vector<std::pair<const X3DNode *, const X3DNode *>> surfaceOrder;
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> geometryNode;

  // Shared handles of every node (a Segment is cloned for its morph
  // targets) and the Joint or Segment owning each HAnimDisplacer.
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> shared;
  std::unordered_map<const X3DNode *, const X3DNode *> displacerOwner;
  for (const auto &root : scene.rootNodes)
    if (root) shared.emplace(root.get(), root);
  x3d::runtime::detail::forEachNode(scene, [&](X3DNode *n) {
    forEachChildNode(*n, [&](const FieldInfo &f, const std::shared_ptr<X3DNode> &c) {
      shared.emplace(c.get(), c);
      if (f.x3dName == "displacers") displacerOwner.emplace(c.get(), n);
    });
  });
  struct DisplacerSource {
    const X3DNode *displacer;
    Samples samples;
  };
  std::map<std::pair<const X3DNode *, const X3DNode *>, std::vector<DisplacerSource>>
      bySegment; // (Segment, clock)
  std::vector<std::pair<const X3DNode *, const X3DNode *>> segmentOrder;

  std::size_t skippedRoutes = 0;
  std::vector<const X3DNode *> clocks; // animation order: first use
  std::unordered_map<const X3DNode *, GltfAnimation> byClock;
  auto animationFor = [&](const X3DNode *clock) -> GltfAnimation & {
    auto found = byClock.find(clock);
    if (found == byClock.end()) {
      clocks.push_back(clock);
      found = byClock.emplace(clock, GltfAnimation{}).first;
      if (clock) found->second.name = clock->getDEF();
    }
    return found->second;
  };
  for (const Route &route : scene.routes) {
    const auto from = endpoint(route.from, route.fromNode);
    const auto to = endpoint(route.to, route.toNode);
    if (!from || !to || !isInterpolator(*from)) continue;
    if (fieldName(route.fromField) != "value") continue;

    const std::string type = from->nodeTypeName();
    const std::string toType = to->nodeTypeName();
    if (toType == "TextureTransform" || toType == "TextureTransform2D") {
      // A TextureTransform is baked into the texture coordinates of every
      // shape using it, so it animates as morph targets of their geometry.
      const std::string field = fieldName(route.toField);
      const auto geometries = texturedGeometriesOf.find(to.get());
      if (!drivesTextureTransform(type, field) ||
          geometries == texturedGeometriesOf.end()) {
        ++skippedRoutes;
        continue;
      }
      const X3DNode *clock = clockOf(from);
      for (const auto &geometry : geometries->second) {
        const auto key = std::make_pair(static_cast<const X3DNode *>(geometry.get()), clock);
        auto &sources = bySurface[key];
        if (sources.empty()) surfaceOrder.push_back(key);
        sources.push_back({from, to.get(), field, true});
        geometryNode[geometry.get()] = geometry;
      }
      continue;
    }
    if (type == "CoordinateInterpolator" || type == "CoordinateInterpolator2D" ||
        type == "NormalInterpolator") {
      const auto geometries = geometriesOf.find(to.get());
      if (geometries == geometriesOf.end()) {
        ++skippedRoutes;
        continue;
      }
      const X3DNode *clock = clockOf(from);
      for (const auto &geometry : geometries->second) {
        const auto key = std::make_pair(static_cast<const X3DNode *>(geometry.get()), clock);
        auto &sources = bySurface[key];
        if (sources.empty()) surfaceOrder.push_back(key);
        sources.push_back({from, to.get(), fieldName(route.toField)});
        geometryNode[geometry.get()] = geometry;
      }
      continue;
    }

    if (type == "ScalarInterpolator" || type == "SplineScalarInterpolator") {
      if (to->nodeTypeName() == "HAnimDisplacer" && fieldName(route.toField) == "weight") {
        const auto owner = displacerOwner.find(to.get());
        Samples samples;
        if (owner == displacerOwner.end() ||
            !sampleInterpolator(*from, false, ctx.geoProjection(), samples) ||
            samples.kind != SampleKind::Scalar) {
          ++skippedRoutes;
          continue;
        }
        const X3DNode *clock = clockOf(from);
        if (owner->second->nodeTypeName() == "HAnimSegment") {
          // Segment displacers move the Segment's own mesh: morph targets.
          const auto key = std::make_pair(owner->second, clock);
          auto &sources = bySegment[key];
          if (sources.empty()) segmentOrder.push_back(key);
          sources.push_back({to.get(), std::move(samples)});
          continue;
        }
        // Joint displacers: the humanoid's skin morph weights (the writer).
        GltfAnimationChannel channel;
        channel.target = to.get();
        channel.path = GltfAnimationChannel::Path::Property;
        channel.field = "weight";
        channel.values = std::move(samples.values);
        const double duration = durationOf(clock);
        for (float key : samples.fractions) channel.times.push_back(float(key * duration));
        animationFor(clock).channels.push_back(std::move(channel));
        continue;
      }
    }

    const Destination dest = destinationOf(*to, fieldName(route.toField));
    Samples samples;
    if (dest.kind == SampleKind::None ||
        !sampleInterpolator(*from, dest.densify, ctx.geoProjection(), samples) ||
        samples.kind != dest.kind) {
      ++skippedRoutes;
      continue;
    }
    GltfAnimationChannel channel;
    channel.target = to.get();
    channel.path = dest.path;
    channel.field = dest.field;
    if (dest.aim) {
      // A light's direction becomes the rotation aiming -Z along it.
      for (std::size_t i = 0; i + 3 <= samples.values.size(); i += 3) {
        const auto q = aimQuaternion(
            {samples.values[i], samples.values[i + 1], samples.values[i + 2]});
        channel.values.insert(channel.values.end(),
                              {float(q[0]), float(q[1]), float(q[2]), float(q[3])});
      }
    } else {
      channel.values = std::move(samples.values);
    }
    const X3DNode *clock = clockOf(from);
    const double duration = durationOf(clock);
    for (float key : samples.fractions)
      channel.times.push_back(float(key * duration));
    animationFor(clock).channels.push_back(std::move(channel));
  }

  // Morph targets: re-tessellate each driven geometry at every key of its
  // interpolators (the union, when several drive it), on a clone so the
  // scene is untouched. glTF gives a mesh one set of targets, so only the
  // first TimeSensor driving a geometry is exported.
  std::unordered_set<const X3DNode *> morphed;
  for (const auto &surface : surfaceOrder) {
    const auto &sources = bySurface[surface];
    if (!morphed.insert(surface.first).second) {
      skippedRoutes += sources.size();
      continue;
    }
    GltfMorph morph;
    std::vector<float> fractions;
    if (!morphTargets(geometryNode[surface.first], sources,
                      extractor.meshOptions(), ctx.geoProjection(), morph,
                      fractions)) {
      skippedRoutes += sources.size();
      continue;
    }
    GltfAnimationChannel channel;
    channel.target = surface.first;
    channel.path = GltfAnimationChannel::Path::Weights;
    const double duration = durationOf(surface.second);
    const std::size_t n = morph.targets.size();
    for (std::size_t i = 0; i < n; ++i) {
      channel.times.push_back(float(fractions[i] * duration));
      for (std::size_t k = 0; k < n; ++k) channel.values.push_back(k == i ? 1.0f : 0.0f);
    }
    extras.morphs.push_back(std::move(morph));
    animationFor(surface.second).channels.push_back(std::move(channel));
  }
  // Segment displacers: per Segment, the meshes it displaces (a geometry
  // whose coord is the Segment's coord) get targets [no displacement, each
  // displacer at weight 1] on a clone of the Segment. Positions are linear
  // in the weights, so weights [1 - sum(w), w...] give the X3D mesh exactly.
  auto lerpFloat = [](float a, float b, float t) { return a + (b - a) * t; };
  std::unordered_set<const X3DNode *> displacedSegments;
  for (const auto &segmentKey : segmentOrder) {
    const auto &sources = bySegment[segmentKey];
    const X3DNode *segment = segmentKey.first;
    const auto segmentPtr = shared.find(segment);
    const auto coord = sfNode(*segment, "coord");
    if (!displacedSegments.insert(segment).second || segmentPtr == shared.end() || !coord) {
      skippedRoutes += sources.size();
      continue;
    }
    std::vector<const X3DNode *> displacers;
    forEachChildNode(*segment, [&](const FieldInfo &f, const std::shared_ptr<X3DNode> &c) {
      if (f.x3dName == "displacers" && c) displacers.push_back(c.get());
    });
    std::vector<std::shared_ptr<X3DNode>> geometries;
    {
      std::unordered_set<const X3DNode *> seen;
      std::function<void(const X3DNode *, std::size_t)> rec = [&](const X3DNode *n,
                                                                  std::size_t depth) {
        if (!n || depth > kMaxNestingDepth || !seen.insert(n).second) return;
        if (const auto geometry = sfNode(*n, "geometry")) {
          if (sfNode(*geometry, "coord") == coord &&
              std::find(geometries.begin(), geometries.end(), geometry) == geometries.end())
            geometries.push_back(geometry);
          return;
        }
        const std::string t = n->nodeTypeName();
        if (depth > 0 && (t == "HAnimJoint" || t == "HAnimSegment")) return;
        forEachChildNode(*n, [&](const FieldInfo &f, const std::shared_ptr<X3DNode> &c) {
          if (f.x3dName == "children") rec(c.get(), depth + 1);
        });
      };
      rec(segment, 0);
    }
    // The union of the driven displacers' keys, both sides of a jump.
    std::set<float> keys, jumps;
    for (const DisplacerSource &src : sources)
      for (std::size_t i = 0; i < src.samples.fractions.size(); ++i) {
        keys.insert(src.samples.fractions[i]);
        if (i && src.samples.fractions[i] == src.samples.fractions[i - 1])
          jumps.insert(src.samples.fractions[i]);
      }
    std::vector<float> rest;
    for (const X3DNode *d : displacers)
      rest.push_back(geombounds::getField<float>(*d, "weight", 0.0f));
    auto weightsAt = [&](float f, bool right) {
      std::vector<float> w = rest;
      for (const DisplacerSource &src : sources) {
        const auto at = std::find(displacers.begin(), displacers.end(), src.displacer);
        if (at == displacers.end()) continue;
        const auto row = rowAt(src.samples.fractions, src.samples.values, 1, f, right, lerpFloat);
        if (!row.empty()) w[std::size_t(at - displacers.begin())] = row.front();
      }
      return w;
    };
    auto withRest = [](const std::vector<float> &w) {
      float sum = 0;
      for (float v : w) sum += v;
      std::vector<float> out{1.0f - sum};
      out.insert(out.end(), w.begin(), w.end());
      return out;
    };
    bool any = false;
    for (const auto &geometry : geometries) {
      if (morphed.count(geometry.get())) continue;
      std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
      const auto clone = deepClone(segmentPtr->second, cloneMap);
      const auto geometryClone = cloneMap.find(geometry.get());
      if (!clone || geometryClone == cloneMap.end()) continue;
      std::vector<X3DNode *> weightClones;
      for (const X3DNode *d : displacers) {
        const auto c = cloneMap.find(d);
        weightClones.push_back(c == cloneMap.end() ? nullptr : c->second.get());
      }
      auto build = [&](std::ptrdiff_t only) {
        for (std::size_t k = 0; k < weightClones.size(); ++k)
          if (weightClones[k])
            for (const FieldInfo &f : weightClones[k]->fields())
              if (f.x3dName == "weight" && f.set)
                f.set(*weightClones[k], std::any(float(std::ptrdiff_t(k) == only ? 1 : 0)));
        MeshBuildOptions options = extractor.meshOptions();
        options.hanimSegment = clone.get();
        return buildLocalMesh(geometryClone->second.get(), ctx.geoProjection(), options);
      };
      GltfMorph morph;
      morph.geometry = geometry.get();
      morph.segment = segment;
      morph.targets.push_back(build(-1));
      bool ok = !morph.targets.back().positions.empty();
      for (std::size_t k = 0; ok && k < displacers.size(); ++k) {
        morph.targets.push_back(build(std::ptrdiff_t(k)));
        ok = morph.targets.back().positions.size() == morph.targets.front().positions.size();
      }
      if (!ok) continue;
      morph.weights = withRest(rest);
      GltfAnimationChannel channel;
      channel.target = geometry.get();
      channel.path = GltfAnimationChannel::Path::Weights;
      const double duration = durationOf(segmentKey.second);
      for (float f : keys)
        for (bool right : {false, true}) {
          if (right && !jumps.count(f)) continue;
          const auto w = withRest(weightsAt(f, right));
          channel.times.push_back(float(f * duration));
          channel.values.insert(channel.values.end(), w.begin(), w.end());
        }
      morphed.insert(geometry.get());
      extras.morphs.push_back(std::move(morph));
      animationFor(segmentKey.second).channels.push_back(std::move(channel));
      any = true;
    }
    if (!any) skippedRoutes += sources.size();
  }

  // HAnimMotion: one animation per enabled motion, its frames as STEP keys
  // frameDuration apart, from the playback start (frameIndex) through one
  // pass of [startFrame, endFrame] by frameIncrement (one cycle when it
  // loops). A last key holds the final frame for its full duration.
  using Motion = hanim::HAnimMotionSystem;
  std::size_t skippedMotions = 0;
  x3d::runtime::detail::forEachNode(scene, [&](X3DNode *n) {
    auto *humanoid = dynamic_cast<x3d::nodes::HAnimHumanoid *>(n);
    if (!humanoid) return;
    const auto &motions = humanoid->getMotions();
    for (std::size_t slot = 0; slot < motions.size(); ++slot) {
      const auto *motion = dynamic_cast<const x3d::nodes::HAnimMotion *>(motions[slot].get());
      if (!motion) continue;
      if (!Motion::motionEnabled(*humanoid, *motion, slot)) {
        ++skippedMotions;
        continue;
      }
      std::size_t width = 0;
      const auto groups = Motion::decode(*humanoid, *motion, width);
      const int count = width ? int(motion->getValues().size() / width) : 0;
      if (count == 0) {
        ++skippedMotions;
        continue;
      }
      const auto [lo, hi] = Motion::frameRange(*motion, count);
      int index = std::clamp(std::clamp(int(motion->getFrameIndex()), 0, count - 1), lo, hi);
      const int increment = motion->getFrameIncrement();
      const double duration = motion->getFrameDuration();
      std::vector<int> frames{index};
      if (increment != 0 && duration > 0) {
        const int span = hi - lo + 1;
        for (int guard = 0; guard < span; ++guard) {
          int next = index + increment;
          if (next > hi || next < lo) {
            if (!motion->getLoop()) break;
            next = lo + ((next - lo) % span + span) % span;
          }
          if (next == frames.front()) break;
          frames.push_back(next);
          index = next;
        }
      }
      GltfAnimation animation;
      animation.name = motion->getDEF();
      if (animation.name.empty())
        animation.name = geombounds::getField<std::string>(*motion, "name", {});
      std::vector<std::pair<GltfAnimationChannel, GltfAnimationChannel>> perJoint;
      const double step = duration > 0 ? duration : 0.0;
      for (std::size_t k = 0; k <= frames.size(); ++k) {
        if (k == frames.size() && (step <= 0 || frames.empty())) break;
        const int frame = frames[std::min(k, frames.size() - 1)];
        const auto poses = Motion::poseAt(groups, *motion, width, frame);
        if (perJoint.empty()) perJoint.resize(poses.size());
        for (std::size_t j = 0; j < poses.size() && j < perJoint.size(); ++j) {
          auto &[t, r] = perJoint[j];
          const float time = float(double(k) * step);
          if (poses[j].translated) {
            t.target = poses[j].joint;
            t.path = GltfAnimationChannel::Path::Translation;
            t.step = true;
            t.times.push_back(time);
            t.values.insert(t.values.end(), {poses[j].translation.x, poses[j].translation.y,
                                             poses[j].translation.z});
          }
          if (poses[j].rotated) {
            const auto q = quaternion(poses[j].rotation);
            r.target = poses[j].joint;
            r.path = GltfAnimationChannel::Path::Rotation;
            r.step = true;
            r.times.push_back(time);
            r.values.insert(r.values.end(),
                            {float(q[0]), float(q[1]), float(q[2]), float(q[3])});
          }
        }
      }
      for (auto &[t, r] : perJoint) {
        if (t.target) animation.channels.push_back(std::move(t));
        if (r.target) animation.channels.push_back(std::move(r));
      }
      if (animation.channels.empty()) ++skippedMotions;
      else extras.animations.push_back(std::move(animation));
    }
  });
  for (const X3DNode *clock : clocks)
    extras.animations.push_back(std::move(byClock[clock]));
  if (report) {
    report->skippedAnimationRoutes = skippedRoutes;
    report->skippedMotions = skippedMotions;
  }
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
