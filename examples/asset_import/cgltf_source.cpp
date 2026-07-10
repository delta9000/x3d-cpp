#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf.h"

#include "cgltf_source.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace x3d::asset_import {
namespace {

// Minimal base64 decode for "data:...;base64,<payload>" image URIs. Stops at a
// terminating quote/'=' and skips non-alphabet bytes.
std::vector<std::uint8_t> b64decode(const char* s) {
  static const std::string T =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::vector<std::uint8_t> out;
  int val = 0, bits = -8;
  for (; *s && *s != '"'; ++s) {
    if (*s == '=') break;
    auto p = T.find(*s);
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

// glTF matrices are column-major, right-handed, Y-up — the same convention as
// our Mat4 — so cgltf's flat 16-float local transform copies in with no flip.
Mat4 nodeLocal(const cgltf_node* n) {
  Mat4 m;
  cgltf_node_transform_local(n, m.m.data());
  return m;
}

Vec3 readV3(const cgltf_float* f) { return {f[0], f[1], f[2]}; }

int matIndex(const cgltf_data* d, const cgltf_material* m) {
  return m ? static_cast<int>(m - d->materials) : -1;
}

// Resolve a texture's image to either an external path or an embedded-bytes
// index, de-duplicating repeated images by pointer.
std::optional<TextureRef> imageRef(const cgltf_texture* t, ImportScene& scene,
                                   std::unordered_map<const void*, int>& cache) {
  if (!t || !t->image) return std::nullopt;
  const cgltf_image* img = t->image;
  if (auto it = cache.find(img); it != cache.end()) {
    TextureRef r;
    r.embeddedIndex = it->second;
    return r;
  }
  // External URI (anything that is not a data: URI).
  if (img->uri && std::strncmp(img->uri, "data:", 5) != 0) {
    TextureRef r;
    r.externalPath = img->uri;
    return r;
  }
  // Embedded: base64 data: URI or a GLB buffer-view.
  std::vector<std::uint8_t> bytes;
  std::string ext = "png";
  if (img->uri) { // data: URI
    if (const char* comma = std::strchr(img->uri, ',')) bytes = b64decode(comma + 1);
    if (std::strstr(img->uri, "jpeg") || std::strstr(img->uri, "jpg")) ext = "jpg";
  } else if (img->buffer_view) {
    const cgltf_buffer_view* bv = img->buffer_view;
    const auto* base = static_cast<const std::uint8_t*>(bv->buffer->data) + bv->offset;
    bytes.assign(base, base + bv->size);
    if (img->mime_type && std::strstr(img->mime_type, "jpeg")) ext = "jpg";
  }
  if (bytes.empty()) return std::nullopt;
  int idx = static_cast<int>(scene.embedded.size());
  scene.embedded.push_back(
      {std::string("cgltf_img_") + std::to_string(idx), ext, std::move(bytes)});
  cache[img] = idx;
  TextureRef r;
  r.embeddedIndex = idx;
  return r;
}

} // namespace

ImportScene CgltfSource::load(const std::string& path) {
  cgltf_options opt{};
  cgltf_data* d = nullptr;
  if (cgltf_parse_file(&opt, path.c_str(), &d) != cgltf_result_success)
    throw std::runtime_error("cgltf: parse failed");
  struct Guard {
    cgltf_data* d;
    ~Guard() {
      if (d) cgltf_free(d);
    }
  } guard{d};
  if (cgltf_load_buffers(&opt, d, path.c_str()) != cgltf_result_success)
    throw std::runtime_error("cgltf: buffer load failed");

  ImportScene scene;
  std::unordered_map<const void*, int> imgCache;

  // --- Materials ---
  for (size_t i = 0; i < d->materials_count; ++i) {
    const cgltf_material& gm = d->materials[i];
    ImportMaterial m;
    m.name = gm.name ? gm.name : ("material_" + std::to_string(i));
    if (gm.has_pbr_metallic_roughness) {
      const auto& pm = gm.pbr_metallic_roughness;
      PbrParams p;
      p.baseColor = {pm.base_color_factor[0], pm.base_color_factor[1],
                     pm.base_color_factor[2], pm.base_color_factor[3]};
      p.metallic = pm.metallic_factor;
      p.roughness = pm.roughness_factor;
      m.pbr = p;
      m.diffuse = {p.baseColor.x, p.baseColor.y, p.baseColor.z};
      m.opacity = p.baseColor.w;
      if (auto r = imageRef(pm.base_color_texture.texture, scene, imgCache))
        m.textures.baseColor = r;
      if (auto r = imageRef(pm.metallic_roughness_texture.texture, scene, imgCache))
        m.textures.metallicRoughness = r;
    }
    m.emissive = readV3(gm.emissive_factor);
    if (auto r = imageRef(gm.normal_texture.texture, scene, imgCache)) m.textures.normal = r;
    if (auto r = imageRef(gm.emissive_texture.texture, scene, imgCache)) m.textures.emissive = r;
    if (auto r = imageRef(gm.occlusion_texture.texture, scene, imgCache)) m.textures.occlusion = r;
    if (gm.alpha_mode == cgltf_alpha_mode_mask) {
      m.alpha = AlphaMode::Mask;
      m.opacityThreshold = gm.alpha_cutoff;
    } else if (gm.alpha_mode == cgltf_alpha_mode_blend) {
      m.alpha = AlphaMode::Blend;
    }
    scene.materials.push_back(std::move(m));
  }

  // --- Meshes (one ImportMesh per triangle primitive) ---
  std::unordered_map<const cgltf_mesh*, std::vector<int>> meshPrims;
  for (size_t i = 0; i < d->meshes_count; ++i) {
    const cgltf_mesh& gmesh = d->meshes[i];
    for (size_t pi = 0; pi < gmesh.primitives_count; ++pi) {
      const cgltf_primitive& prim = gmesh.primitives[pi];
      if (prim.type != cgltf_primitive_type_triangles) continue;
      ImportMesh mesh;
      mesh.materialIndex = matIndex(d, prim.material);
      for (size_t a = 0; a < prim.attributes_count; ++a) {
        const cgltf_attribute& at = prim.attributes[a];
        size_t n = cgltf_accessor_unpack_floats(at.data, nullptr, 0);
        std::vector<float> buf(n);
        cgltf_accessor_unpack_floats(at.data, buf.data(), n);
        size_t comp = cgltf_num_components(at.data->type);
        if (comp == 0) continue;
        size_t count = n / comp;
        if (at.type == cgltf_attribute_type_position) {
          for (size_t k = 0; k < count; ++k)
            mesh.positions.push_back({buf[k * comp], buf[k * comp + 1], buf[k * comp + 2]});
        } else if (at.type == cgltf_attribute_type_normal) {
          for (size_t k = 0; k < count; ++k)
            mesh.normals.push_back({buf[k * comp], buf[k * comp + 1], buf[k * comp + 2]});
        } else if (at.type == cgltf_attribute_type_texcoord && at.index == 0) {
          for (size_t k = 0; k < count; ++k)
            mesh.uv.push_back({buf[k * comp], buf[k * comp + 1]});
        } else if (at.type == cgltf_attribute_type_color) {
          for (size_t k = 0; k < count; ++k)
            mesh.colors.push_back({buf[k * comp], buf[k * comp + 1], buf[k * comp + 2],
                                   comp > 3 ? buf[k * comp + 3] : 1.0f});
        }
      }
      if (prim.indices) {
        size_t ic = prim.indices->count;
        for (size_t k = 0; k < ic; ++k)
          mesh.indices.push_back(
              static_cast<std::uint32_t>(cgltf_accessor_read_index(prim.indices, k)));
      } else {
        for (std::uint32_t k = 0; k < static_cast<std::uint32_t>(mesh.positions.size()); ++k)
          mesh.indices.push_back(k);
      }
      meshPrims[&gmesh].push_back(static_cast<int>(scene.meshes.size()));
      scene.meshes.push_back(std::move(mesh));
    }
  }

  // --- Nodes (hierarchy + TRS), cameras, lights ---
  std::unordered_map<const cgltf_node*, int> nodeIdx;
  for (size_t i = 0; i < d->nodes_count; ++i) nodeIdx[&d->nodes[i]] = static_cast<int>(i);
  for (size_t i = 0; i < d->nodes_count; ++i) {
    const cgltf_node& gn = d->nodes[i];
    ImportNode node;
    node.name = gn.name ? gn.name : ("node_" + std::to_string(i));
    node.localTransform = nodeLocal(&gn);
    if (gn.mesh)
      for (int mp : meshPrims[gn.mesh]) node.meshIndices.push_back(mp);
    for (size_t c = 0; c < gn.children_count; ++c)
      node.childIndices.push_back(nodeIdx[gn.children[c]]);
    scene.nodes.push_back(std::move(node));

    if (gn.camera && gn.camera->type == cgltf_camera_type_perspective) {
      const auto& pc = gn.camera->data.perspective;
      ImportCamera cam;
      cam.world = nodeLocal(&gn);
      cam.yfov = pc.yfov;
      cam.znear = pc.znear;
      cam.zfar = pc.has_zfar ? pc.zfar : 1000.0f;
      scene.cameras.push_back(cam);
    }
    if (gn.light) {
      const cgltf_light* gl = gn.light;
      ImportLight L;
      L.color = readV3(gl->color);
      L.intensity = gl->intensity;
      L.radius = gl->range > 0 ? gl->range : 100.0f;
      if (gl->type == cgltf_light_type_directional) {
        L.kind = ImportLight::Kind::Dir;
      } else if (gl->type == cgltf_light_type_spot) {
        L.kind = ImportLight::Kind::Spot;
        L.beamWidth = gl->spot_inner_cone_angle;
        L.cutOffAngle = gl->spot_outer_cone_angle;
      } else {
        L.kind = ImportLight::Kind::Point;
      }
      Mat4 w = nodeLocal(&gn);
      L.position = {w.m[12], w.m[13], w.m[14]};
      scene.lights.push_back(L);
    }
  }

  // Root node = first root of the default scene, else the first node.
  if (d->scene && d->scene->nodes_count > 0)
    scene.rootNode = nodeIdx[d->scene->nodes[0]];
  else if (!scene.nodes.empty())
    scene.rootNode = 0;
  return scene;
}

} // namespace x3d::asset_import
