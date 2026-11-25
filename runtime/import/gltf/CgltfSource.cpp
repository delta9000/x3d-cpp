// CgltfSource.cpp — glTF 2.0 -> ImportScene.
//
// Read: every scene root, node TRS / matrices, EXT_mesh_gpu_instancing,
// triangle / strip / fan / line / point primitives, POSITION, NORMAL, every
// TEXCOORD_n (flipped to X3D's bottom-left origin), COLOR_0, JOINTS_n /
// WEIGHTS_n, morph targets, metallic-roughness and unlit materials with their
// extension values, samplers, KHR_texture_transform (baked into texture
// coordinates), perspective and orthographic cameras, KHR_lights_punctual,
// skins, and animations including KHR_animation_pointer channels.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define CGLTF_IMPLEMENTATION
#include "vendor/cgltf.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "CgltfSource.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace x3d::runtime::importer {
namespace {

std::string str(const char *s) { return s ? s : ""; }

std::vector<float> floats(const cgltf_accessor *a) {
  if (!a) return {};
  const cgltf_size n = cgltf_accessor_unpack_floats(a, nullptr, 0);
  std::vector<float> out(n);
  if (n) cgltf_accessor_unpack_floats(a, out.data(), n);
  return out;
}

/// glTF extensions this backend implements; a file that requires any other
/// is refused, as the glTF specification asks.
bool supportedRequired(const std::string &ext) {
  static const std::set<std::string> kSupported = {
      "KHR_lights_punctual",       "KHR_materials_unlit",
      "KHR_texture_transform",     "KHR_materials_emissive_strength",
      "KHR_mesh_quantization",     "KHR_animation_pointer",
      "KHR_materials_ior",         "KHR_materials_clearcoat",
      "KHR_materials_specular",    "KHR_materials_pbrSpecularGlossiness",
      "EXT_mesh_gpu_instancing"};
  return kSupported.count(ext) > 0;
}

/// Minimal base64 decode for data: URIs.
std::vector<std::uint8_t> b64decode(const char *s) {
  static const std::string T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::vector<std::uint8_t> out;
  int val = 0, bits = -8;
  for (; *s; ++s) {
    if (*s == '=') break;
    const auto p = T.find(*s);
    if (p == std::string::npos) continue;
    val = (val << 6) + static_cast<int>(p);
    bits += 6;
    if (bits >= 0) {
      out.push_back(static_cast<std::uint8_t>((val >> bits) & 0xFF));
      bits -= 8;
    }
  }
  return out;
}

TextureWrap wrapOf(cgltf_int w) {
  switch (w) {
    case 33071: return TextureWrap::ClampToEdge;
    case 33648: return TextureWrap::MirroredRepeat;
    default: return TextureWrap::Repeat;
  }
}

TextureFilter filterOf(cgltf_int f) {
  switch (f) {
    case 9728: return TextureFilter::Nearest;
    case 9729: return TextureFilter::Linear;
    case 9984: return TextureFilter::NearestMipmapNearest;
    case 9985: return TextureFilter::LinearMipmapNearest;
    case 9986: return TextureFilter::NearestMipmapLinear;
    case 9987: return TextureFilter::LinearMipmapLinear;
    default: return TextureFilter::Default;
  }
}

/// A texture coordinate source: TEXCOORD_n, optionally transformed.
struct UvSpec {
  int texcoord = 0;
  bool transformed = false;
  float offset[2] = {0, 0};
  float rotation = 0;
  float scale[2] = {1, 1};
  bool operator<(const UvSpec &o) const {
    return std::tie(texcoord, transformed, offset[0], offset[1], rotation, scale[0], scale[1]) <
           std::tie(o.texcoord, o.transformed, o.offset[0], o.offset[1], o.rotation, o.scale[0],
                    o.scale[1]);
  }
};

class Reader {
public:
  Reader(const cgltf_data *d, ImportScene &s) : d_(d), s_(s) {}

  void run() {
    for (cgltf_size i = 0; i < d_->extensions_required_count; ++i) {
      const std::string ext = str(d_->extensions_required[i]);
      if (!supportedRequired(ext))
        throw std::runtime_error("glTF: required extension " + ext + " is not supported");
    }
    for (cgltf_size i = 0; i < d_->extensions_used_count; ++i) {
      const std::string ext = str(d_->extensions_used[i]);
      if (ext == "KHR_draco_mesh_compression" || ext == "EXT_meshopt_compression" ||
          ext == "KHR_texture_basisu" || ext == "EXT_texture_webp" || ext == "KHR_materials_variants")
        s_.warnings.push_back("glTF extension " + ext + " is not read; data that only it holds is lost");
    }
    texcoordSets_ = 1;
    for (cgltf_size m = 0; m < d_->meshes_count; ++m)
      for (cgltf_size p = 0; p < d_->meshes[m].primitives_count; ++p)
        for (cgltf_size a = 0; a < d_->meshes[m].primitives[p].attributes_count; ++a) {
          const auto &at = d_->meshes[m].primitives[p].attributes[a];
          if (at.type == cgltf_attribute_type_texcoord)
            texcoordSets_ = std::max(texcoordSets_, static_cast<int>(at.index) + 1);
        }
    readMaterials();
    readMeshes();
    readCameras();
    readLights();
    readNodes();
    readSkins();
    readAnimations();
    readRoots();
  }

private:
  const cgltf_data *d_;
  ImportScene &s_;
  int texcoordSets_ = 1;
  std::unordered_map<const cgltf_image *, int> images_;
  /// Per material: the distinct uv specs its textures use, in set order.
  std::vector<std::vector<std::pair<UvSpec, int>>> materialUvs_;
  std::unordered_map<const cgltf_mesh *, std::vector<int>> meshPrims_;

  template <class T> int indexOf(const T *base, const T *p) const {
    return p ? static_cast<int>(p - base) : -1;
  }

  std::optional<int> embedImage(const cgltf_image *img) {
    if (auto it = images_.find(img); it != images_.end()) return it->second;
    std::vector<std::uint8_t> bytes;
    std::string ext;
    const std::string mime = str(img->mime_type);
    if (img->uri && std::strncmp(img->uri, "data:", 5) == 0) {
      const std::string uri = img->uri;
      if (const auto comma = uri.find(','); comma != std::string::npos) {
        if (uri.find(";base64") < comma) bytes = b64decode(uri.c_str() + comma + 1);
        const std::string head = uri.substr(5, comma - 5);
        ext = head.find("jpeg") != std::string::npos ? "jpg"
              : head.find("png") != std::string::npos ? "png"
              : head.find("webp") != std::string::npos ? "webp" : "";
      }
    } else if (img->buffer_view && img->buffer_view->buffer && img->buffer_view->buffer->data) {
      const cgltf_buffer_view *bv = img->buffer_view;
      const auto *base = static_cast<const std::uint8_t *>(bv->buffer->data) + bv->offset;
      bytes.assign(base, base + bv->size);
      ext = mime.find("jpeg") != std::string::npos ? "jpg"
            : mime.find("png") != std::string::npos ? "png"
            : mime.find("webp") != std::string::npos ? "webp"
            : mime.find("ktx2") != std::string::npos ? "ktx2" : "";
    } else {
      return std::nullopt;
    }
    if (bytes.empty()) return std::nullopt;
    const int idx = static_cast<int>(s_.embedded.size());
    EmbeddedTexture e;
    e.key = img->name ? img->name : "image" + std::to_string(indexOf(d_->images, img));
    e.hintExt = ext;
    e.bytes = std::move(bytes);
    s_.embedded.push_back(std::move(e));
    images_[img] = idx;
    return idx;
  }

  std::optional<TextureRef> textureRef(const cgltf_texture_view &view, std::size_t materialIndex) {
    const cgltf_texture *t = view.texture;
    if (!t) return std::nullopt;
    const cgltf_image *img = t->image ? t->image : t->basisu_image;
    if (!img) {
      s_.warnings.push_back("glTF texture " + std::to_string(indexOf(d_->textures, t)) +
                            " has no image this backend reads");
      return std::nullopt;
    }
    TextureRef r;
    if (img->uri && std::strncmp(img->uri, "data:", 5) != 0) {
      r.externalPath = img->uri;
    } else if (auto idx = embedImage(img)) {
      r.embeddedIndex = *idx;
    } else {
      return std::nullopt;
    }
    if (t->sampler) {
      r.wrapS = wrapOf(t->sampler->wrap_s);
      r.wrapT = wrapOf(t->sampler->wrap_t);
      r.minFilter = filterOf(t->sampler->min_filter);
      r.magFilter = filterOf(t->sampler->mag_filter);
    }
    UvSpec spec;
    spec.texcoord = view.texcoord;
    if (view.has_transform) {
      const auto &tt = view.transform;
      if (tt.has_texcoord) spec.texcoord = tt.texcoord;
      spec.transformed = true;
      spec.offset[0] = tt.offset[0];
      spec.offset[1] = tt.offset[1];
      spec.rotation = tt.rotation;
      spec.scale[0] = tt.scale[0];
      spec.scale[1] = tt.scale[1];
    }
    auto &list = materialUvs_[materialIndex];
    int set = -1;
    for (const auto &[sp, idx] : list)
      if (!(sp < spec) && !(spec < sp)) set = idx;
    if (set < 0) {
      if (!spec.transformed) {
        set = spec.texcoord;
      } else {
        // Transformed coordinates get a set of their own past the file's sets.
        int extra = 0;
        for (const auto &[sp, idx] : list)
          if (sp.transformed) ++extra;
        set = texcoordSets_ + extra;
      }
      list.emplace_back(spec, set);
    }
    r.uvSet = set;
    return r;
  }

  void readMaterials() {
    materialUvs_.resize(d_->materials_count);
    for (cgltf_size i = 0; i < d_->materials_count; ++i) {
      const cgltf_material &gm = d_->materials[i];
      ImportMaterial m;
      m.name = gm.name ? gm.name : "material" + std::to_string(i);
      PbrParams p; // glTF defaults when pbrMetallicRoughness is absent
      if (gm.has_pbr_metallic_roughness) {
        const auto &pm = gm.pbr_metallic_roughness;
        p.baseColor = {pm.base_color_factor[0], pm.base_color_factor[1], pm.base_color_factor[2],
                       pm.base_color_factor[3]};
        p.metallic = pm.metallic_factor;
        p.roughness = pm.roughness_factor;
        m.textures.baseColor = textureRef(pm.base_color_texture, i);
        m.textures.metallicRoughness = textureRef(pm.metallic_roughness_texture, i);
      } else if (gm.has_pbr_specular_glossiness) {
        const auto &sg = gm.pbr_specular_glossiness;
        p.baseColor = {sg.diffuse_factor[0], sg.diffuse_factor[1], sg.diffuse_factor[2],
                       sg.diffuse_factor[3]};
        p.metallic = 0;
        p.roughness = 1.0f - sg.glossiness_factor;
        m.textures.baseColor = textureRef(sg.diffuse_texture, i);
        s_.warnings.push_back("material " + m.name +
                              ": specular-glossiness approximated as metallic-roughness");
      }
      m.pbr = p;
      m.diffuse = {p.baseColor.x, p.baseColor.y, p.baseColor.z};
      m.opacity = p.baseColor.w;
      m.unlit = gm.unlit != 0;
      float strength = gm.has_emissive_strength ? gm.emissive_strength.emissive_strength : 1.0f;
      m.emissive = {gm.emissive_factor[0] * strength, gm.emissive_factor[1] * strength,
                    gm.emissive_factor[2] * strength};
      m.textures.normal = textureRef(gm.normal_texture, i);
      if (gm.normal_texture.texture) m.normalScale = gm.normal_texture.scale;
      m.textures.emissive = textureRef(gm.emissive_texture, i);
      m.textures.occlusion = textureRef(gm.occlusion_texture, i);
      if (gm.occlusion_texture.texture) m.occlusionStrength = gm.occlusion_texture.scale;
      m.alphaCutoff = gm.alpha_cutoff;
      if (gm.alpha_mode == cgltf_alpha_mode_mask) {
        m.alpha = AlphaMode::Mask;
        m.opacityThreshold = gm.alpha_cutoff;
      } else if (gm.alpha_mode == cgltf_alpha_mode_blend) {
        m.alpha = AlphaMode::Blend;
        m.opacityMode = OpacityMode::Presence;
      }
      m.doubleSided = gm.double_sided != 0;
      if (gm.has_ior) m.ior = gm.ior.ior;
      if (gm.has_clearcoat) {
        m.clearcoat = gm.clearcoat.clearcoat_factor;
        m.clearcoatRoughness = gm.clearcoat.clearcoat_roughness_factor;
      }
      s_.materials.push_back(std::move(m));
    }
  }

  /// Derived uv set for one spec from the raw TEXCOORD_n sets (X3D origin).
  static std::vector<Vec2> deriveUv(const UvSpec &spec, const std::vector<std::vector<Vec2>> &raw) {
    if (spec.texcoord < 0 || static_cast<std::size_t>(spec.texcoord) >= raw.size()) return {};
    std::vector<Vec2> out = raw[static_cast<std::size_t>(spec.texcoord)];
    if (!spec.transformed) return out;
    const float c = std::cos(spec.rotation), s = std::sin(spec.rotation);
    for (auto &uv : out) {
      // Back to glTF's top-left space, apply T * R * S, then flip again.
      const float u = uv.x, v = 1.0f - uv.y;
      const float su = u * spec.scale[0], sv = v * spec.scale[1];
      const float ru = c * su + s * sv, rv = -s * su + c * sv;
      uv = {ru + spec.offset[0], 1.0f - (rv + spec.offset[1])};
    }
    return out;
  }

  static void readVec3(const cgltf_accessor *a, std::vector<Vec3> &out) {
    const auto f = floats(a);
    const cgltf_size comp = cgltf_num_components(a->type);
    if (comp < 3) return;
    out.resize(f.size() / comp);
    for (std::size_t k = 0; k < out.size(); ++k)
      out[k] = {f[k * comp], f[k * comp + 1], f[k * comp + 2]};
  }

  void readMeshes() {
    for (cgltf_size i = 0; i < d_->meshes_count; ++i) {
      const cgltf_mesh &gmesh = d_->meshes[i];
      for (cgltf_size pi = 0; pi < gmesh.primitives_count; ++pi) {
        const cgltf_primitive &prim = gmesh.primitives[pi];
        const std::string meshName = (gmesh.name ? std::string(gmesh.name) : "mesh" + std::to_string(i)) +
                                     (gmesh.primitives_count > 1 ? "_" + std::to_string(pi) : "");
        if (prim.has_draco_mesh_compression)
          throw std::runtime_error("glTF: mesh " + meshName +
                                   " is Draco-compressed (KHR_draco_mesh_compression), which is not supported");
        ImportMesh mesh;
        mesh.name = meshName;
        mesh.materialIndex = indexOf(d_->materials, prim.material);
        std::vector<std::vector<Vec2>> raw;
        std::vector<std::pair<int, std::vector<float>>> joints, weights; // by set
        for (cgltf_size a = 0; a < prim.attributes_count; ++a) {
          const cgltf_attribute &at = prim.attributes[a];
          if (!at.data) continue;
          const cgltf_size comp = cgltf_num_components(at.data->type);
          switch (at.type) {
            case cgltf_attribute_type_position: readVec3(at.data, mesh.positions); break;
            case cgltf_attribute_type_normal: readVec3(at.data, mesh.normals); break;
            case cgltf_attribute_type_texcoord: {
              const auto f = floats(at.data);
              if (static_cast<int>(raw.size()) <= at.index) raw.resize(static_cast<std::size_t>(at.index) + 1);
              auto &uv = raw[static_cast<std::size_t>(at.index)];
              for (std::size_t k = 0; k + 1 < f.size(); k += comp) uv.push_back({f[k], 1.0f - f[k + 1]});
              break;
            }
            case cgltf_attribute_type_color:
              if (at.index == 0) {
                const auto f = floats(at.data);
                mesh.colorsHaveAlpha = comp == 4;
                for (std::size_t k = 0; k + comp - 1 < f.size(); k += comp)
                  mesh.colors.push_back({f[k], f[k + 1], f[k + 2], comp == 4 ? f[k + 3] : 1.0f});
              }
              break;
            case cgltf_attribute_type_joints: {
              // Joint indices are integers; read_uint keeps them exact.
              const cgltf_size n = at.data->count;
              std::vector<float> j(n * 4, 0.0f);
              for (cgltf_size k = 0; k < n; ++k) {
                cgltf_uint v[4] = {0, 0, 0, 0};
                cgltf_accessor_read_uint(at.data, k, v, 4);
                for (int c = 0; c < 4; ++c) j[k * 4 + static_cast<cgltf_size>(c)] = static_cast<float>(v[c]);
              }
              joints.emplace_back(at.index, std::move(j));
              break;
            }
            case cgltf_attribute_type_weights: weights.emplace_back(at.index, floats(at.data)); break;
            default: break;
          }
        }
        const std::size_t nv = mesh.positions.size();
        // Derived texture coordinate sets (identity sets keep their index).
        std::size_t maxSet = raw.size();
        if (mesh.materialIndex >= 0)
          for (const auto &[spec, set] : materialUvs_[static_cast<std::size_t>(mesh.materialIndex)])
            maxSet = std::max(maxSet, static_cast<std::size_t>(set) + 1);
        mesh.uvSets.resize(maxSet);
        for (std::size_t k = 0; k < raw.size(); ++k)
          if (raw[k].size() == nv) mesh.uvSets[k] = raw[k];
        if (mesh.materialIndex >= 0)
          for (const auto &[spec, set] : materialUvs_[static_cast<std::size_t>(mesh.materialIndex)])
            if (spec.transformed) mesh.uvSets[static_cast<std::size_t>(set)] = deriveUv(spec, raw);
        if (!mesh.normals.empty() && mesh.normals.size() != nv) mesh.normals.clear();
        if (!mesh.colors.empty() && mesh.colors.size() != nv) mesh.colors.clear();
        // Skin influences.
        if (!joints.empty()) {
          mesh.influences.assign(nv, {});
          for (const auto &[set, j] : joints) {
            const std::vector<float> *w = nullptr;
            for (const auto &[ws, wv] : weights)
              if (ws == set) w = &wv;
            if (!w) continue;
            for (std::size_t v = 0; v < nv; ++v)
              for (int c = 0; c < 4; ++c) {
                const std::size_t k = v * 4 + static_cast<std::size_t>(c);
                if (k >= j.size() || k >= w->size()) continue;
                if ((*w)[k] > 0) mesh.influences[v].push_back({static_cast<int>(j[k]), (*w)[k]});
              }
          }
        }
        // Morph targets.
        for (cgltf_size t = 0; t < prim.targets_count; ++t) {
          MorphTarget mt;
          for (cgltf_size a = 0; a < prim.targets[t].attributes_count; ++a) {
            const cgltf_attribute &at = prim.targets[t].attributes[a];
            if (at.type == cgltf_attribute_type_position) readVec3(at.data, mt.positions);
            else if (at.type == cgltf_attribute_type_normal) readVec3(at.data, mt.normals);
          }
          if (mt.positions.size() != nv) mt.positions.assign(nv, Vec3{});
          if (!mt.normals.empty() && mt.normals.size() != mesh.normals.size()) mt.normals.clear();
          mesh.morphTargets.push_back(std::move(mt));
        }
        mesh.morphWeights.assign(prim.targets_count, 0.0f);
        for (cgltf_size t = 0; t < prim.targets_count && t < gmesh.weights_count; ++t)
          mesh.morphWeights[t] = gmesh.weights[t];
        // Topology.
        std::vector<std::uint32_t> idx;
        if (prim.indices) {
          idx.resize(prim.indices->count);
          for (cgltf_size k = 0; k < prim.indices->count; ++k)
            idx[k] = static_cast<std::uint32_t>(cgltf_accessor_read_index(prim.indices, k));
        } else {
          idx.resize(nv);
          for (std::size_t k = 0; k < nv; ++k) idx[k] = static_cast<std::uint32_t>(k);
        }
        switch (prim.type) {
          case cgltf_primitive_type_triangles:
            mesh.indices = idx;
            mesh.indices.resize(idx.size() - idx.size() % 3);
            break;
          case cgltf_primitive_type_triangle_strip:
            for (std::size_t k = 2; k < idx.size(); ++k) {
              if (k % 2 == 0) mesh.indices.insert(mesh.indices.end(), {idx[k - 2], idx[k - 1], idx[k]});
              else mesh.indices.insert(mesh.indices.end(), {idx[k - 1], idx[k - 2], idx[k]});
            }
            break;
          case cgltf_primitive_type_triangle_fan:
            for (std::size_t k = 2; k < idx.size(); ++k)
              mesh.indices.insert(mesh.indices.end(), {idx[0], idx[k - 1], idx[k]});
            break;
          case cgltf_primitive_type_lines:
            mesh.topology = ImportMesh::Topology::Lines;
            mesh.indices = idx;
            mesh.indices.resize(idx.size() - idx.size() % 2);
            break;
          case cgltf_primitive_type_line_strip:
          case cgltf_primitive_type_line_loop:
            mesh.topology = ImportMesh::Topology::Lines;
            for (std::size_t k = 1; k < idx.size(); ++k) mesh.indices.insert(mesh.indices.end(), {idx[k - 1], idx[k]});
            if (prim.type == cgltf_primitive_type_line_loop && idx.size() > 2)
              mesh.indices.insert(mesh.indices.end(), {idx.back(), idx.front()});
            break;
          case cgltf_primitive_type_points:
            mesh.topology = ImportMesh::Topology::Points;
            break;
          default: break;
        }
        meshPrims_[&gmesh].push_back(static_cast<int>(s_.meshes.size()));
        s_.meshes.push_back(std::move(mesh));
      }
    }
  }

  void readCameras() {
    for (cgltf_size i = 0; i < d_->cameras_count; ++i) {
      const cgltf_camera &gc = d_->cameras[i];
      ImportCamera c;
      c.name = str(gc.name);
      if (gc.type == cgltf_camera_type_orthographic) {
        c.kind = ImportCamera::Kind::Orthographic;
        c.xmag = gc.data.orthographic.xmag;
        c.ymag = gc.data.orthographic.ymag;
        c.znear = gc.data.orthographic.znear;
        c.zfar = gc.data.orthographic.zfar;
      } else {
        const auto &p = gc.data.perspective;
        c.yfov = p.yfov;
        c.aspectRatio = p.has_aspect_ratio ? p.aspect_ratio : 0;
        c.znear = p.znear;
        c.zfar = p.has_zfar ? p.zfar : 0;
      }
      s_.cameras.push_back(c);
    }
  }

  void readLights() {
    for (cgltf_size i = 0; i < d_->lights_count; ++i) {
      const cgltf_light &gl = d_->lights[i];
      ImportLight l;
      l.name = str(gl.name);
      l.color = {gl.color[0], gl.color[1], gl.color[2]};
      l.intensity = gl.intensity;
      l.range = gl.range;
      if (gl.type == cgltf_light_type_directional) {
        l.kind = ImportLight::Kind::Directional;
      } else if (gl.type == cgltf_light_type_spot) {
        l.kind = ImportLight::Kind::Spot;
        l.innerConeAngle = gl.spot_inner_cone_angle;
        l.outerConeAngle = gl.spot_outer_cone_angle;
      } else {
        l.kind = ImportLight::Kind::Point;
      }
      s_.lights.push_back(l);
    }
  }

  void readNodes() {
    s_.nodes.resize(d_->nodes_count);
    for (cgltf_size i = 0; i < d_->nodes_count; ++i) {
      const cgltf_node &gn = d_->nodes[i];
      ImportNode &n = s_.nodes[i];
      n.name = gn.name ? gn.name : "";
      cgltf_node_transform_local(&gn, n.localTransform.m.data());
      if (!gn.has_matrix) {
        Trs t;
        if (gn.has_translation) t.translation = {gn.translation[0], gn.translation[1], gn.translation[2]};
        if (gn.has_rotation) t.rotation = {gn.rotation[0], gn.rotation[1], gn.rotation[2], gn.rotation[3]};
        if (gn.has_scale) t.scale = {gn.scale[0], gn.scale[1], gn.scale[2]};
        n.trs = t;
      }
      n.camera = indexOf(d_->cameras, gn.camera);
      n.light = indexOf(d_->lights, gn.light);
      n.skin = indexOf(d_->skins, gn.skin);
      n.morphWeights.assign(gn.weights, gn.weights + gn.weights_count);
      for (cgltf_size c = 0; c < gn.children_count; ++c)
        n.childIndices.push_back(indexOf(d_->nodes, gn.children[c]));
      if (!gn.mesh) continue;
      const auto &prims = meshPrims_[gn.mesh];
      if (!gn.has_mesh_gpu_instancing) {
        n.meshIndices = prims;
        continue;
      }
      // EXT_mesh_gpu_instancing: one child node per instance.
      std::vector<float> tr, rot, sc;
      for (cgltf_size a = 0; a < gn.mesh_gpu_instancing.attributes_count; ++a) {
        const auto &at = gn.mesh_gpu_instancing.attributes[a];
        const std::string name = str(at.name);
        if (name == "TRANSLATION") tr = floats(at.data);
        else if (name == "ROTATION") rot = floats(at.data);
        else if (name == "SCALE") sc = floats(at.data);
      }
      const std::size_t count = std::max({tr.size() / 3, rot.size() / 4, sc.size() / 3});
      for (std::size_t k = 0; k < count; ++k) {
        ImportNode inst;
        inst.name = n.name.empty() ? "" : n.name + "_instance" + std::to_string(k);
        Trs t;
        if (tr.size() >= 3 * (k + 1)) t.translation = {tr[3 * k], tr[3 * k + 1], tr[3 * k + 2]};
        if (rot.size() >= 4 * (k + 1)) t.rotation = {rot[4 * k], rot[4 * k + 1], rot[4 * k + 2], rot[4 * k + 3]};
        if (sc.size() >= 3 * (k + 1)) t.scale = {sc[3 * k], sc[3 * k + 1], sc[3 * k + 2]};
        inst.trs = t;
        inst.meshIndices = prims;
        inst.skin = n.skin;
        inst.morphWeights = n.morphWeights;
        s_.nodes[i].childIndices.push_back(static_cast<int>(s_.nodes.size()));
        s_.nodes.push_back(std::move(inst));
      }
    }
  }

  void readSkins() {
    for (cgltf_size i = 0; i < d_->skins_count; ++i) {
      const cgltf_skin &gs = d_->skins[i];
      ImportSkin sk;
      sk.name = str(gs.name);
      for (cgltf_size j = 0; j < gs.joints_count; ++j) sk.joints.push_back(indexOf(d_->nodes, gs.joints[j]));
      sk.inverseBind.assign(gs.joints_count, Mat4{});
      if (gs.inverse_bind_matrices) {
        const auto f = floats(gs.inverse_bind_matrices);
        for (cgltf_size j = 0; j < gs.joints_count && 16 * (j + 1) <= f.size(); ++j)
          std::copy(f.begin() + static_cast<std::ptrdiff_t>(16 * j),
                    f.begin() + static_cast<std::ptrdiff_t>(16 * (j + 1)), sk.inverseBind[j].m.begin());
      }
      sk.skeleton = indexOf(d_->nodes, gs.skeleton);
      s_.skins.push_back(std::move(sk));
    }
  }

  /// "/materials/3/pbrMetallicRoughness/baseColorFactor" -> path + index.
  bool pointerTarget(const std::string &ptr, AnimationChannel &ch) {
    using P = AnimationChannel::Path;
    auto parse = [&](const char *prefix, int &idx, std::string &rest) {
      const std::string p = prefix;
      if (ptr.compare(0, p.size(), p) != 0) return false;
      std::size_t end = 0;
      try {
        idx = std::stoi(ptr.substr(p.size()), &end);
      } catch (...) {
        return false;
      }
      rest = ptr.substr(p.size() + end);
      return true;
    };
    int idx = -1;
    std::string rest;
    if (parse("/nodes/", idx, rest)) {
      ch.target = idx;
      if (rest == "/translation") ch.path = P::Translation;
      else if (rest == "/rotation") ch.path = P::Rotation;
      else if (rest == "/scale") ch.path = P::Scale;
      else if (rest == "/weights") ch.path = P::Weights;
      else return false;
      return true;
    }
    if (parse("/materials/", idx, rest)) {
      ch.target = idx;
      if (rest == "/pbrMetallicRoughness/baseColorFactor") ch.path = P::MaterialBaseColor;
      else if (rest == "/emissiveFactor") ch.path = P::MaterialEmissive;
      else if (rest == "/pbrMetallicRoughness/metallicFactor") ch.path = P::MaterialMetallic;
      else if (rest == "/pbrMetallicRoughness/roughnessFactor") ch.path = P::MaterialRoughness;
      else if (rest == "/alphaCutoff") ch.path = P::MaterialAlphaCutoff;
      else return false;
      return true;
    }
    if (parse("/extensions/KHR_lights_punctual/lights/", idx, rest)) {
      ch.target = idx;
      if (rest == "/color") ch.path = P::LightColor;
      else if (rest == "/intensity") ch.path = P::LightIntensity;
      else if (rest == "/range") ch.path = P::LightRange;
      else if (rest == "/spot/innerConeAngle") ch.path = P::LightInnerCone;
      else if (rest == "/spot/outerConeAngle") ch.path = P::LightOuterCone;
      else return false;
      return true;
    }
    if (parse("/cameras/", idx, rest)) {
      ch.target = idx;
      if (rest == "/perspective/yfov") ch.path = P::CameraYfov;
      else return false;
      return true;
    }
    return false;
  }

  static std::string pointerOf(const cgltf_animation_channel &c) {
    for (cgltf_size e = 0; e < c.extensions_count; ++e) {
      if (str(c.extensions[e].name) != "KHR_animation_pointer" || !c.extensions[e].data) continue;
      const std::string data = c.extensions[e].data;
      const auto key = data.find("\"pointer\"");
      if (key == std::string::npos) continue;
      const auto q0 = data.find('"', data.find(':', key) + 1);
      const auto q1 = data.find('"', q0 + 1);
      if (q0 == std::string::npos || q1 == std::string::npos) continue;
      std::string out = data.substr(q0 + 1, q1 - q0 - 1);
      // JSON Pointer escapes.
      for (std::size_t p; (p = out.find("~1")) != std::string::npos;) out.replace(p, 2, "/");
      for (std::size_t p; (p = out.find("~0")) != std::string::npos;) out.replace(p, 2, "~");
      return out;
    }
    return "";
  }

  void readAnimations() {
    for (cgltf_size i = 0; i < d_->animations_count; ++i) {
      const cgltf_animation &ga = d_->animations[i];
      ImportAnimation a;
      a.name = ga.name ? ga.name : "animation" + std::to_string(i);
      for (cgltf_size c = 0; c < ga.channels_count; ++c) {
        const cgltf_animation_channel &gc = ga.channels[c];
        if (!gc.sampler || !gc.sampler->input || !gc.sampler->output) continue;
        AnimationChannel ch;
        if (gc.target_node && gc.target_path != cgltf_animation_path_type_invalid) {
          ch.target = indexOf(d_->nodes, gc.target_node);
          switch (gc.target_path) {
            case cgltf_animation_path_type_translation: ch.path = AnimationChannel::Path::Translation; break;
            case cgltf_animation_path_type_rotation: ch.path = AnimationChannel::Path::Rotation; break;
            case cgltf_animation_path_type_scale: ch.path = AnimationChannel::Path::Scale; break;
            case cgltf_animation_path_type_weights: ch.path = AnimationChannel::Path::Weights; break;
            default: continue;
          }
        } else {
          const std::string ptr = pointerOf(gc);
          if (ptr.empty() || !pointerTarget(ptr, ch)) {
            s_.warnings.push_back("animation " + a.name + ": channel " + std::to_string(c) +
                                  (ptr.empty() ? " has no target" : " targets " + ptr + ", which has no X3D field"));
            continue;
          }
        }
        switch (gc.sampler->interpolation) {
          case cgltf_interpolation_type_step: ch.interpolation = AnimationChannel::Interpolation::Step; break;
          case cgltf_interpolation_type_cubic_spline:
            ch.interpolation = AnimationChannel::Interpolation::CubicSpline;
            break;
          default: ch.interpolation = AnimationChannel::Interpolation::Linear; break;
        }
        ch.times = floats(gc.sampler->input);
        ch.values = floats(gc.sampler->output);
        // Sample times must increase; drop a malformed channel rather than
        // build a broken interpolator.
        bool increasing = !ch.times.empty();
        for (std::size_t k = 1; k < ch.times.size(); ++k) increasing = increasing && ch.times[k] > ch.times[k - 1];
        if (!increasing) {
          s_.warnings.push_back("animation " + a.name + ": channel " + std::to_string(c) +
                                " has times that do not increase; dropped");
          continue;
        }
        a.channels.push_back(std::move(ch));
      }
      s_.animations.push_back(std::move(a));
    }
  }

  void readRoots() {
    const cgltf_scene *scene = d_->scene ? d_->scene : (d_->scenes_count ? &d_->scenes[0] : nullptr);
    if (scene) {
      for (cgltf_size i = 0; i < scene->nodes_count; ++i) s_.roots.push_back(indexOf(d_->nodes, scene->nodes[i]));
      if (d_->scenes_count > 1)
        s_.warnings.push_back("glTF file has " + std::to_string(d_->scenes_count) +
                              " scenes; only the default scene is imported");
    } else {
      for (cgltf_size i = 0; i < d_->nodes_count; ++i)
        if (!d_->nodes[i].parent) s_.roots.push_back(static_cast<int>(i));
    }
  }
};

} // namespace

ImportScene CgltfSource::load(const std::string &path) {
  cgltf_options opt{};
  cgltf_data *d = nullptr;
  const cgltf_result pr = cgltf_parse_file(&opt, path.c_str(), &d);
  if (pr != cgltf_result_success) {
    const char *why = pr == cgltf_result_file_not_found   ? "file not found"
                      : pr == cgltf_result_io_error       ? "read error"
                      : pr == cgltf_result_legacy_gltf    ? "glTF 1.0 is not supported"
                      : pr == cgltf_result_unknown_format ? "not a glTF file"
                                                          : "invalid glTF";
    throw std::runtime_error(std::string("glTF: ") + why + ": " + path);
  }
  struct Guard {
    cgltf_data *d;
    ~Guard() { cgltf_free(d); }
  } guard{d};
  if (cgltf_load_buffers(&opt, d, path.c_str()) != cgltf_result_success)
    throw std::runtime_error("glTF: cannot load the buffers of " + path);
  if (cgltf_validate(d) != cgltf_result_success)
    throw std::runtime_error("glTF: invalid accessor or buffer data in " + path);
  ImportScene scene;
  Reader(d, scene).run();
  return scene;
}

} // namespace x3d::runtime::importer
