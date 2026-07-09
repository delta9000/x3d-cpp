#include "usd_source.hpp"
#include "tinyusdz.hh"
#include "composition.hh"
#include "usdGeom.hh"
#include "value-types.hh"
#include "tydra/render-data.hh"
#include "io-util.hh"
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

namespace x3d::asset_import {

namespace {

// Fully compose a non-USDZ USD file (USDA/USDC) into a flattened Stage:
// subLayers, then iterate references/payloads/inherits/variants to a fixpoint
// (LIVRPS strength ordering), then bake the Layer into a Stage. tinyusdz'
// LoadUSDFromFile does NOT resolve these composition arcs, so a plain load of a
// composed scene (the production norm — external references + payloads) yields
// empty Xforms with no geometry. USDZ archives are self-contained and keep the
// direct LoadUSDFromFile path.
bool loadComposedStage(const std::string& path, tinyusdz::Stage& stage,
                       std::string& warn, std::string& err) {
  tinyusdz::Layer root;
  if (!tinyusdz::LoadLayerFromFile(path, &root, &warn, &err)) return false;

  const std::string base_dir = tinyusdz::io::GetBaseDir(path);
  tinyusdz::AssetResolutionResolver resolver;
  resolver.set_current_working_path(base_dir);
  resolver.set_search_paths({base_dir});

  tinyusdz::Layer src = root;
  {
    tinyusdz::Layer out;
    if (tinyusdz::CompositeSublayers(resolver, src, &out, &warn, &err)) {
      src = std::move(out);
    }
  }

  constexpr int kMaxIter = 64;
  for (int i = 0; i < kMaxIter; ++i) {
    bool unresolved = false;
    if (src.check_unresolved_references()) {
      unresolved = true;
      tinyusdz::Layer out;
      if (!tinyusdz::CompositeReferences(resolver, src, &out, &warn, &err))
        return false;
      src = std::move(out);
    }
    if (src.check_unresolved_payload()) {
      unresolved = true;
      tinyusdz::Layer out;
      if (!tinyusdz::CompositePayload(resolver, src, &out, &warn, &err))
        return false;
      src = std::move(out);
    }
    if (src.check_unresolved_inherits()) {
      unresolved = true;
      tinyusdz::Layer out;
      if (tinyusdz::CompositeInherits(src, &out, &warn, &err))
        src = std::move(out);
    }
    if (src.check_unresolved_variant()) {
      unresolved = true;
      tinyusdz::Layer out;
      if (tinyusdz::CompositeVariant(src, &out, &warn, &err))
        src = std::move(out);
    }
    if (!unresolved) break;
  }

  return tinyusdz::LayerToStage(src, &stage, &warn, &err);
}

// tinyusdz matrix4d is USD-convention: row-major with row-vector semantics
// (translation lives in m[3][0..2]; value-types.hh:650). Our Mat4 is
// column-major with column-vector semantics (translation in m[12..14]). The
// same transform for column vectors is the transpose of the USD matrix, so the
// column-major element [c*4+r] reads t.m[c][r] — NOT t.m[r][c]. Reading [r][c]
// transposes twice (a no-op on storage), which inverts every rotation and drops
// translation into the always-zero last column.
Mat4 toColumnMajor(const tinyusdz::value::matrix4d& t) {
  Mat4 m;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      m.m[c * 4 + r] = static_cast<float>(t.m[c][r]);
    }
  }
  return m;
}

// One resolved PointInstancer: a per-instance local transform (in the
// instancer's space) and the absolute path of the prototype it instantiates.
// tydra has no PointInstancer support, so we resolve instancing ourselves from
// the composed Stage and graft instances into the node tree below.
struct InstanceSet {
  std::vector<Mat4> transforms;
  std::vector<std::string> prototypePaths;
};

// Column-major TRS from an instance's translate/quaternion(w,x,y,z)/scale.
Mat4 instanceTransform(const Vec3& t, const float q[4], const Vec3& s) {
  const float w = q[0], x = q[1], y = q[2], z = q[3];
  // Rotation matrix (row r, col c) from the unit quaternion.
  const float r00 = 1 - 2 * (y * y + z * z), r01 = 2 * (x * y - w * z), r02 = 2 * (x * z + w * y);
  const float r10 = 2 * (x * y + w * z), r11 = 1 - 2 * (x * x + z * z), r12 = 2 * (y * z - w * x);
  const float r20 = 2 * (x * z - w * y), r21 = 2 * (y * z + w * x), r22 = 1 - 2 * (x * x + y * y);
  Mat4 m;  // column-major; M = Translate * Rotate * Scale
  m.m[0] = r00 * s.x; m.m[1] = r10 * s.x; m.m[2] = r20 * s.x; m.m[3] = 0;
  m.m[4] = r01 * s.y; m.m[5] = r11 * s.y; m.m[6] = r21 * s.y; m.m[7] = 0;
  m.m[8] = r02 * s.z; m.m[9] = r12 * s.z; m.m[10] = r22 * s.z; m.m[11] = 0;
  m.m[12] = t.x; m.m[13] = t.y; m.m[14] = t.z; m.m[15] = 1;
  return m;
}

// Walk the composed Stage and resolve every PointInstancer into an InstanceSet,
// keyed by the instancer's absolute prim path (matches tydra Node::abs_path).
void collectInstancers(const tinyusdz::Prim& prim, const std::string& parentPath,
                       std::map<std::string, InstanceSet>& out) {
  const std::string absPath = parentPath + "/" + prim.element_name();
  if (const auto* pi = prim.as<tinyusdz::PointInstancer>()) {
    std::vector<tinyusdz::value::point3f> positions;
    std::vector<tinyusdz::value::quath> orientations;
    std::vector<tinyusdz::value::float3> scales;
    std::vector<std::int32_t> protoIndices;
    if (auto a = pi->positions.get_value()) a.value().get(tinyusdz::value::TimeCode::Default(), &positions);
    if (auto a = pi->orientations.get_value()) a.value().get(tinyusdz::value::TimeCode::Default(), &orientations);
    if (auto a = pi->scales.get_value()) a.value().get(tinyusdz::value::TimeCode::Default(), &scales);
    if (auto a = pi->protoIndices.get_value()) a.value().get(tinyusdz::value::TimeCode::Default(), &protoIndices);

    std::vector<std::string> protoPaths;
    if (pi->prototypes) {
      const tinyusdz::Relationship& rel = pi->prototypes.value();
      if (rel.type == tinyusdz::Relationship::Type::Path) {
        protoPaths.push_back(rel.targetPath.full_path_name());
      } else if (rel.type == tinyusdz::Relationship::Type::PathVector) {
        for (const auto& tp : rel.targetPathVector) protoPaths.push_back(tp.full_path_name());
      }
    }

    InstanceSet set;
    for (std::size_t i = 0; i < positions.size(); ++i) {
      const std::int32_t pidx = i < protoIndices.size() ? protoIndices[i] : 0;
      if (pidx < 0 || static_cast<std::size_t>(pidx) >= protoPaths.size()) continue;
      Vec3 t{positions[i][0], positions[i][1], positions[i][2]};
      float q[4] = {1, 0, 0, 0};
      if (i < orientations.size()) {
        q[0] = tinyusdz::value::half_to_float(orientations[i].real);
        q[1] = tinyusdz::value::half_to_float(orientations[i].imag[0]);
        q[2] = tinyusdz::value::half_to_float(orientations[i].imag[1]);
        q[3] = tinyusdz::value::half_to_float(orientations[i].imag[2]);
      }
      Vec3 s{1, 1, 1};
      if (i < scales.size()) { s = Vec3{scales[i][0], scales[i][1], scales[i][2]}; }
      set.transforms.push_back(instanceTransform(t, q, s));
      set.prototypePaths.push_back(protoPaths[pidx]);
    }
    if (!set.transforms.empty()) out[absPath] = std::move(set);
  }
  for (const auto& child : prim.children()) collectInstancers(child, absPath, out);
}

int flattenNode(const tinyusdz::tydra::Node& un, std::vector<ImportNode>& nodes,
                const std::map<std::string, InstanceSet>& instancers) {
  ImportNode n;
  n.name = un.prim_name;
  n.localTransform = toColumnMajor(un.local_matrix);

  if (un.nodeType == tinyusdz::tydra::NodeType::Mesh && un.id >= 0) {
    n.meshIndices.push_back(un.id);
  }

  int idx = static_cast<int>(nodes.size());
  nodes.push_back(n);

  // If this node is a PointInstancer, its prototype children are templates that
  // must be replicated once per instance (with the instance transform) rather
  // than rendered directly. Non-prototype children flatten normally.
  const auto instIt = instancers.find(un.abs_path);
  const InstanceSet* inst = (instIt != instancers.end()) ? &instIt->second : nullptr;

  std::vector<int> childIndices;
  for (const auto& child : un.children) {
    bool isPrototype = false;
    if (inst) {
      for (const auto& pp : inst->prototypePaths) {
        if (pp == child.abs_path) { isPrototype = true; break; }
      }
    }
    if (isPrototype) {
      for (std::size_t i = 0; i < inst->transforms.size(); ++i) {
        if (inst->prototypePaths[i] != child.abs_path) continue;
        ImportNode instNode;
        instNode.name = child.prim_name + "_inst";
        instNode.localTransform = inst->transforms[i];
        int instIdx = static_cast<int>(nodes.size());
        nodes.push_back(instNode);
        int protoIdx = flattenNode(child, nodes, instancers);
        nodes[instIdx].childIndices = {protoIdx};
        childIndices.push_back(instIdx);
      }
      continue;  // prototype itself is not rendered un-instanced
    }
    int childIdx = flattenNode(child, nodes, instancers);
    childIndices.push_back(childIdx);
  }

  nodes[idx].childIndices = childIndices;
  return idx;
}

template <typename T>
std::optional<TextureRef> resolveTexture(
    const tinyusdz::tydra::ShaderParam<T>& param,
    const tinyusdz::tydra::RenderScene& render_scene,
    ImportScene& scene) 
{
  if (!param.is_texture()) return std::nullopt;
  int32_t tex_idx = param.texture_id;
  if (tex_idx < 0 || tex_idx >= static_cast<int32_t>(render_scene.textures.size())) return std::nullopt;
  
  const auto& uv_tex = render_scene.textures[tex_idx];
  int64_t image_idx = uv_tex.texture_image_id;
  if (image_idx < 0 || image_idx >= static_cast<int64_t>(render_scene.images.size())) return std::nullopt;
  
  const auto& img = render_scene.images[image_idx];
  TextureRef ref;
  
  // A decoded buffer holds raw texels, not the original file — never embed it
  // as compressed bytes. Fall back to the external asset path instead.
  if (!img.decoded && img.buffer_id >= 0 &&
      img.buffer_id < static_cast<int64_t>(render_scene.buffers.size())) {
    const auto& buf = render_scene.buffers[img.buffer_id];
    EmbeddedTexture emb;
    emb.key = img.asset_identifier;
    emb.bytes = buf.data;
    size_t dot = img.asset_identifier.find_last_of('.');
    if (dot != std::string::npos) {
      emb.hintExt = img.asset_identifier.substr(dot + 1);
    }
    
    ref.embeddedIndex = static_cast<int>(scene.embedded.size());
    scene.embedded.push_back(emb);
  } else {
    ref.externalPath = img.asset_identifier;
  }
  
  return ref;
}

} // namespace

ImportScene UsdSource::load(const std::string& path) {
  ImportScene scene;
  
  std::string warn, err;
  tinyusdz::Stage stage;
  bool is_usdz = tinyusdz::IsUSDZ(path);
  bool ret;
  if (is_usdz) {
    // USDZ archives are self-contained; LoadUSDFromFile handles in-archive
    // asset resolution directly.
    ret = tinyusdz::LoadUSDFromFile(path, &stage, &warn, &err);
  } else {
    // USDA/USDC may pull geometry through external references/payloads/
    // subLayers that LoadUSDFromFile does not resolve — compose them explicitly.
    ret = loadComposedStage(path, stage, warn, err);
  }
  if (!ret) {
    throw std::runtime_error("failed to load USD file: " + path + ". Error: " + err);
  }
  
  tinyusdz::tydra::RenderScene render_scene;
  tinyusdz::tydra::RenderSceneConverter converter;
  tinyusdz::tydra::RenderSceneConverterEnv env(stage);
  
  env.mesh_config.triangulate = true;
  env.mesh_config.build_vertex_indices = true;
  // Keep textures as their ORIGINAL compressed file bytes (JPEG/PNG). With
  // load_texture_assets=true tinyusdz DECODES each image into a raw texel
  // buffer (e.g. 4096*4096*4 = 64 MB) and flags TextureImage::decoded; writing
  // that buffer to a `.jpg` produces a giant invalid file. The downstream
  // texture pipeline wants the compressed bytes, so read them undecoded.
  env.scene_config.load_texture_assets = false;
  
  tinyusdz::USDZAsset usdz_asset;
  if (is_usdz) {
    if (!tinyusdz::ReadUSDZAssetInfoFromFile(path, &usdz_asset, &warn, &err)) {
      throw std::runtime_error("failed to read USDZ asset info: " + err);
    }
    tinyusdz::AssetResolutionResolver arr;
    if (!tinyusdz::SetupUSDZAssetResolution(arr, &usdz_asset)) {
      throw std::runtime_error("failed to setup USDZ asset resolution");
    }
    env.asset_resolver = arr;
  } else {
    std::string usd_basedir = tinyusdz::io::GetBaseDir(path);
    env.set_search_paths({usd_basedir});
  }
  
  ret = converter.ConvertToRenderScene(env, &render_scene);
  if (!ret) {
    throw std::runtime_error("failed to convert USD to RenderScene: " + converter.GetError());
  }
  
  // Materials
  for (const auto& um : render_scene.materials) {
    ImportMaterial mat;
    mat.name = um.name;
    
    const auto& pbr = um.surfaceShader;
    mat.diffuse = Vec3{pbr.diffuseColor.value[0], pbr.diffuseColor.value[1], pbr.diffuseColor.value[2]};
    mat.emissive = Vec3{pbr.emissiveColor.value[0], pbr.emissiveColor.value[1], pbr.emissiveColor.value[2]};
    mat.specular = Vec3{pbr.specularColor.value[0], pbr.specularColor.value[1], pbr.specularColor.value[2]};
    mat.opacity = pbr.opacity.value;
    mat.shininess = pbr.roughness.value;

    // UsdPreviewSurface-grade fields tydra populates but the legacy loop dropped.
    // These cannot be expressed in the X3D PhysicalMaterial node; they reach a
    // renderer only through the specialized `--emit-glsl` path.
    mat.useSpecularWorkflow = pbr.useSpecularWorkflow;
    mat.ior = pbr.ior.value;
    mat.clearcoat = pbr.clearcoat.value;
    mat.clearcoatRoughness = pbr.clearcoatRoughness.value;
    mat.opacityThreshold = pbr.opacityThreshold.value;

    // AlphaMode from the UsdPreviewSurface opacity signals: a nonzero
    // opacityThreshold is a masked cutout; else a sub-1 opacity is a blend.
    if (mat.opacityThreshold > 0.0f) {
      mat.alpha = AlphaMode::Mask;
    } else if (mat.opacity < 1.0f) {
      mat.alpha = AlphaMode::Blend;
    }

    PbrParams pbrParams;
    pbrParams.baseColor = Vec4{pbr.diffuseColor.value[0], pbr.diffuseColor.value[1], pbr.diffuseColor.value[2], pbr.opacity.value};
    pbrParams.metallic = pbr.metallic.value;
    pbrParams.roughness = pbr.roughness.value;
    mat.pbr = pbrParams;
    
    // Texture slots mapping
    mat.textures.baseColor = resolveTexture(pbr.diffuseColor, render_scene, scene);
    mat.textures.normal = resolveTexture(pbr.normal, render_scene, scene);
    mat.textures.emissive = resolveTexture(pbr.emissiveColor, render_scene, scene);
    mat.textures.occlusion = resolveTexture(pbr.occlusion, render_scene, scene);
    mat.textures.specular = resolveTexture(pbr.specularColor, render_scene, scene);

    // UsdPreviewSurface authors roughness and metallic as independent scalar
    // inputs, each optionally texture-connected; glTF-derived USD packs them into
    // one ORM texture (G=rough, B=metal), which is the case this single-slot IR
    // (and the X3D metallicRoughnessTexture) can represent. If an asset connects
    // roughness and metallic to *different* textures, only roughness is carried.
    if (pbr.roughness.is_texture()) {
      mat.textures.metallicRoughness = resolveTexture(pbr.roughness, render_scene, scene);
    } else if (pbr.metallic.is_texture()) {
      mat.textures.metallicRoughness = resolveTexture(pbr.metallic, render_scene, scene);
    }
    
    scene.materials.push_back(mat);
  }
  
  // Meshes
  for (const auto& um : render_scene.meshes) {
    ImportMesh mesh;
    
    mesh.positions.reserve(um.points.size());
    for (const auto& p : um.points) {
      mesh.positions.push_back(Vec3{p[0], p[1], p[2]});
    }
    
    const auto& indices = um.faceVertexIndices();
    mesh.indices.assign(indices.begin(), indices.end());
    
    if (!um.normals.empty()) {
      size_t norm_count = um.normals.vertex_count();
      mesh.normals.reserve(norm_count);
      const float* src = reinterpret_cast<const float*>(um.normals.buffer());
      if (um.normals.format == tinyusdz::tydra::VertexAttributeFormat::Vec3) {
        for (size_t i = 0; i < norm_count; ++i) {
          mesh.normals.push_back(Vec3{src[3 * i], src[3 * i + 1], src[3 * i + 2]});
        }
      }
    }
    
    if (um.texcoords.count(0)) {
      const auto& uv_attr = um.texcoords.at(0);
      size_t uv_count = uv_attr.vertex_count();
      mesh.uv.reserve(uv_count);
      const float* src = reinterpret_cast<const float*>(uv_attr.buffer());
      if (uv_attr.format == tinyusdz::tydra::VertexAttributeFormat::Vec2) {
        for (size_t i = 0; i < uv_count; ++i) {
          mesh.uv.push_back(Vec2{src[2 * i], src[2 * i + 1]});
        }
      }
    }
    
    mesh.materialIndex = um.material_id;
    scene.meshes.push_back(mesh);
  }
  
  // Resolve PointInstancers from the composed stage (tydra has no instancing),
  // keyed by absolute prim path so flattenNode can graft instances into the tree.
  std::map<std::string, InstanceSet> instancers;
  for (const auto& prim : stage.root_prims()) {
    collectInstancers(prim, "", instancers);
  }

  // Node tree flattening
  if (!render_scene.nodes.empty()) {
    uint32_t rootIdx = render_scene.default_root_node;
    if (rootIdx < render_scene.nodes.size()) {
      scene.rootNode = flattenNode(render_scene.nodes[rootIdx], scene.nodes, instancers);
    } else {
      scene.rootNode = flattenNode(render_scene.nodes[0], scene.nodes, instancers);
    }
  }
  
  // Cameras
  for (auto& uc : render_scene.cameras) {
    ImportCamera cam;
    cam.znear = uc.znear;
    cam.zfar = uc.zfar;
    cam.yfov = uc.yfov();
    cam.world = Mat4{};
    scene.cameras.push_back(cam);
  }
  
  return scene;
}

} // namespace x3d::asset_import
