// AssimpSource.cpp — reads anything Assimp reads into the ImportScene IR.
//
// Assimp does the format work: triangulation, smooth normals where the file
// has none, texture-transform baking (aiProcess_TransformUVCoords) and
// primitive sorting, so every aiMesh here is one topology. This file maps
// the aiScene onto the IR: nodes, meshes with every uv and colour set,
// Phong / PBR / unlit materials, bones as skins, anim meshes as morph
// targets, cameras and lights placed by their node, and node and morph
// animations.
#include "AssimpSource.hpp"

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::runtime::importer {

namespace {

// aiMatrix4x4 is row-major (a1..a4 is row 0); the IR is column-major.
Mat4 toMat4(const aiMatrix4x4 &t) {
  Mat4 m;
  m.m = {t.a1, t.b1, t.c1, t.d1, t.a2, t.b2, t.c2, t.d2,
         t.a3, t.b3, t.c3, t.d3, t.a4, t.b4, t.c4, t.d4};
  return m;
}

Vec3 toVec3(const aiVector3D &v) { return {v.x, v.y, v.z}; }

Vec3 normalized(Vec3 v) {
  const float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  if (l <= 0) return v;
  return {v.x / l, v.y / l, v.z / l};
}
Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

class Reader {
public:
  Reader(const aiScene &s, ImportScene &out) : s_(s), out_(out) {}

  void run() {
    for (unsigned i = 0; i < s_.mNumMaterials; ++i) out_.materials.push_back(material(*s_.mMaterials[i]));
    for (unsigned i = 0; i < s_.mNumMeshes; ++i) out_.meshes.push_back(mesh(*s_.mMeshes[i]));
    embedded();
    if (s_.mRootNode) {
      const int root = walk(*s_.mRootNode);
      const ImportNode &r = out_.nodes[static_cast<std::size_t>(root)];
      const bool identity = r.localTransform.m == Mat4{}.m;
      // Assimp always adds one root; skip it when it carries nothing.
      if (identity && r.meshIndices.empty() && r.camera < 0 && r.light < 0 && !animated_.count(root))
        out_.roots = r.childIndices;
      else
        out_.roots = {root};
    }
    cameras();
    lights();
    skins();
    animations();
  }

private:
  const aiScene &s_;
  ImportScene &out_;
  std::unordered_map<std::string, int> nodeByName_;
  std::map<int, bool> animated_;
  std::vector<int> meshSkin_; ///< skin index each mesh's influences use

  void warn(const std::string &w) {
    if (std::find(out_.warnings.begin(), out_.warnings.end(), w) == out_.warnings.end())
      out_.warnings.push_back(w);
  }

  int nodeNamed(const std::string &n) const {
    auto it = nodeByName_.find(n);
    return it == nodeByName_.end() ? -1 : it->second;
  }

  // ---- materials ---------------------------------------------------------

  std::optional<TextureRef> texture(const aiMaterial &m, aiTextureType type, unsigned index = 0) {
    if (m.GetTextureCount(type) <= index) return std::nullopt;
    aiString path;
    aiTextureMapping mapping = aiTextureMapping_UV;
    unsigned uv = 0;
    aiTextureMapMode modes[3] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
    if (m.GetTexture(type, index, &path, &mapping, &uv, nullptr, nullptr, modes) != aiReturn_SUCCESS)
      return std::nullopt;
    const std::string p = path.C_Str();
    if (p.empty()) return std::nullopt;
    if (mapping != aiTextureMapping_UV)
      warn("a texture uses a non-UV mapping Assimp could not convert; it reads uv set " +
           std::to_string(uv));
    TextureRef ref;
    auto [tex, idx] = s_.GetEmbeddedTextureAndIndex(p.c_str());
    if (tex && idx >= 0)
      ref.embeddedIndex = idx;
    else
      ref.externalPath = p;
    ref.uvSet = static_cast<int>(uv);
    auto wrap = [&](aiTextureMapMode mode) {
      switch (mode) {
        case aiTextureMapMode_Clamp: return TextureWrap::ClampToEdge;
        case aiTextureMapMode_Mirror: return TextureWrap::MirroredRepeat;
        case aiTextureMapMode_Decal:
          warn("decal texture wrap mapped to clamp-to-edge");
          return TextureWrap::ClampToEdge;
        default: return TextureWrap::Repeat;
      }
    };
    ref.wrapS = wrap(modes[0]);
    ref.wrapT = wrap(modes[1]);
    return ref;
  }

  static bool sameImage(const std::optional<TextureRef> &a, const std::optional<TextureRef> &b) {
    return a && b && a->externalPath == b->externalPath && a->embeddedIndex == b->embeddedIndex;
  }

  ImportMaterial material(const aiMaterial &m) {
    ImportMaterial out;
    aiString name;
    if (m.Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS) out.name = name.C_Str();

    aiColor3D c;
    float f = 0;
    int shading = 0;
    m.Get(AI_MATKEY_SHADING_MODEL, shading);
    if (m.Get(AI_MATKEY_COLOR_DIFFUSE, c) == aiReturn_SUCCESS) out.diffuse = {c.r, c.g, c.b};
    if (m.Get(AI_MATKEY_COLOR_SPECULAR, c) == aiReturn_SUCCESS) out.specular = {c.r, c.g, c.b};
    if (m.Get(AI_MATKEY_SHININESS_STRENGTH, f) == aiReturn_SUCCESS && f >= 0)
      out.specular = {out.specular.x * f, out.specular.y * f, out.specular.z * f};
    if (m.Get(AI_MATKEY_COLOR_EMISSIVE, c) == aiReturn_SUCCESS) out.emissive = {c.r, c.g, c.b};
    if (m.Get(AI_MATKEY_EMISSIVE_INTENSITY, f) == aiReturn_SUCCESS)
      out.emissive = {out.emissive.x * f, out.emissive.y * f, out.emissive.z * f};
    if (m.Get(AI_MATKEY_COLOR_AMBIENT, c) == aiReturn_SUCCESS) {
      const float d = (out.diffuse.x + out.diffuse.y + out.diffuse.z) / 3.0f;
      const float a = (c.r + c.g + c.b) / 3.0f;
      out.ambientIntensity = d > 0 ? std::clamp(a / d, 0.0f, 1.0f) : std::clamp(a, 0.0f, 1.0f);
    }
    // X3D shininess * 128 is the Phong exponent.
    if (m.Get(AI_MATKEY_SHININESS, f) == aiReturn_SUCCESS) out.shininess = std::clamp(f / 128.0f, 0.0f, 1.0f);
    if (m.Get(AI_MATKEY_OPACITY, f) == aiReturn_SUCCESS) out.opacity = std::clamp(f, 0.0f, 1.0f);
    if (m.Get(AI_MATKEY_REFRACTI, f) == aiReturn_SUCCESS && f >= 1.0f) out.ior = f;
    int two = 0;
    if (m.Get(AI_MATKEY_TWOSIDED, two) == aiReturn_SUCCESS) out.doubleSided = two != 0;

    // PBR: metallic-roughness factors (glTF and the Assimp PBR keys of other
    // formats). A base colour alone also means a PBR material.
    ai_real metallic = 0, roughness = 0;
    aiColor4D base(1, 1, 1, 1);
    const bool hasMetal = m.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS;
    const bool hasRough = m.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS;
    const bool hasBase = m.Get(AI_MATKEY_BASE_COLOR, base) == aiReturn_SUCCESS;
    const bool pbr = hasMetal || hasRough || shading == aiShadingMode_PBR_BRDF;
    if (pbr) {
      PbrParams p;
      p.baseColor = hasBase ? Vec4{base.r, base.g, base.b, base.a}
                            : Vec4{out.diffuse.x, out.diffuse.y, out.diffuse.z, out.opacity};
      p.metallic = hasMetal ? static_cast<float>(metallic) : 0.0f;
      p.roughness = hasRough ? static_cast<float>(roughness) : 0.5f;
      out.pbr = p;
    } else {
      ai_real gloss = 0;
      if (m.Get(AI_MATKEY_GLOSSINESS_FACTOR, gloss) == aiReturn_SUCCESS) {
        out.shininess = std::clamp(static_cast<float>(gloss), 0.0f, 1.0f);
        warn("specular-glossiness material '" + out.name + "' approximated as Phong");
      }
    }
    if (shading == aiShadingMode_Unlit) out.unlit = true;
    if (m.Get(AI_MATKEY_CLEARCOAT_FACTOR, f) == aiReturn_SUCCESS) out.clearcoat = f;
    if (m.Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, f) == aiReturn_SUCCESS) out.clearcoatRoughness = f;

    // glTF alpha (Assimp's glTF reader keeps the mode as a string).
    aiString alphaMode;
    if (m.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == aiReturn_SUCCESS) {
      const std::string a = alphaMode.C_Str();
      out.alpha = a == "MASK" ? AlphaMode::Mask : a == "BLEND" ? AlphaMode::Blend : AlphaMode::Opaque;
      if (m.Get(AI_MATKEY_GLTF_ALPHACUTOFF, f) == aiReturn_SUCCESS) out.alphaCutoff = f;
    } else if ((out.pbr ? out.pbr->baseColor.w : out.opacity) < 1.0f) {
      out.alpha = AlphaMode::Blend;
    }

    // Textures.
    TextureSlots &t = out.textures;
    t.baseColor = texture(m, aiTextureType_BASE_COLOR);
    if (!t.baseColor) t.baseColor = texture(m, aiTextureType_DIFFUSE);
    t.normal = texture(m, aiTextureType_NORMALS);
    if (!t.normal) t.normal = texture(m, aiTextureType_NORMAL_CAMERA);
    t.emissive = texture(m, aiTextureType_EMISSION_COLOR);
    if (!t.emissive) t.emissive = texture(m, aiTextureType_EMISSIVE);
    // Assimp's glTF reader stores occlusion as the lightmap.
    t.occlusion = texture(m, aiTextureType_AMBIENT_OCCLUSION);
    if (!t.occlusion) t.occlusion = texture(m, aiTextureType_LIGHTMAP);
    t.specular = texture(m, aiTextureType_SPECULAR);
    t.shininess = texture(m, aiTextureType_SHININESS);
    t.ambient = texture(m, aiTextureType_AMBIENT);
    // glTF packs metallic (B) and roughness (G) in one image.
    t.metallicRoughness = texture(m, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);
    if (!t.metallicRoughness) {
      auto metal = texture(m, aiTextureType_METALNESS);
      auto rough = texture(m, aiTextureType_DIFFUSE_ROUGHNESS);
      if (metal && rough && !sameImage(metal, rough))
        warn("material '" + out.name +
             "' has separate metallic and roughness images; X3D needs them packed, the "
             "roughness image is used as the packed one");
      t.metallicRoughness = rough ? rough : metal;
    }
    if (m.GetTextureCount(aiTextureType_HEIGHT) > 0 && !t.normal)
      warn("material '" + out.name + "' has a height (bump) map; X3D has no bump slot, it is dropped");
    if (m.GetTextureCount(aiTextureType_OPACITY) > 0)
      warn("material '" + out.name + "' has an opacity map; X3D reads alpha from the base texture, it is dropped");
    if (m.GetTextureCount(aiTextureType_DISPLACEMENT) > 0)
      warn("material '" + out.name + "' has a displacement map; dropped");
    if (m.GetTextureCount(aiTextureType_REFLECTION) > 0)
      warn("material '" + out.name + "' has a reflection map; dropped");
    return out;
  }

  // ---- meshes ------------------------------------------------------------

  ImportMesh mesh(const aiMesh &am) {
    ImportMesh out;
    out.name = am.mName.C_Str();
    out.materialIndex = static_cast<int>(am.mMaterialIndex);
    if (am.mPrimitiveTypes & aiPrimitiveType_TRIANGLE)
      out.topology = ImportMesh::Topology::Triangles;
    else if (am.mPrimitiveTypes & aiPrimitiveType_LINE)
      out.topology = ImportMesh::Topology::Lines;
    else if (am.mPrimitiveTypes & aiPrimitiveType_POINT)
      out.topology = ImportMesh::Topology::Points;
    const unsigned n = am.mNumVertices;
    out.positions.reserve(n);
    for (unsigned i = 0; i < n; ++i) out.positions.push_back(toVec3(am.mVertices[i]));
    if (am.mNormals && out.topology == ImportMesh::Topology::Triangles)
      for (unsigned i = 0; i < n; ++i) out.normals.push_back(toVec3(am.mNormals[i]));
    for (unsigned k = 0; k < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++k) {
      if (!am.mTextureCoords[k]) break;
      std::vector<Vec2> uv(n);
      for (unsigned i = 0; i < n; ++i) uv[i] = {am.mTextureCoords[k][i].x, am.mTextureCoords[k][i].y};
      out.uvSets.push_back(std::move(uv));
    }
    if (am.mColors[0]) {
      for (unsigned i = 0; i < n; ++i) {
        const aiColor4D &c = am.mColors[0][i];
        out.colors.push_back({c.r, c.g, c.b, c.a});
        if (c.a < 1.0f) out.colorsHaveAlpha = true;
      }
      if (am.mColors[1]) warn("mesh '" + out.name + "' has more than one colour set; only the first is kept");
    }
    if (out.topology != ImportMesh::Topology::Points) {
      const unsigned per = out.topology == ImportMesh::Topology::Triangles ? 3 : 2;
      out.indices.reserve(am.mNumFaces * per);
      for (unsigned f = 0; f < am.mNumFaces; ++f) {
        const aiFace &face = am.mFaces[f];
        if (face.mNumIndices != per) continue;
        for (unsigned j = 0; j < per; ++j) out.indices.push_back(face.mIndices[j]);
      }
    }
    // Anim meshes hold whole replacement attributes; the IR wants deltas.
    for (unsigned a = 0; a < am.mNumAnimMeshes; ++a) {
      const aiAnimMesh &t = *am.mAnimMeshes[a];
      if (t.mNumVertices != n) continue;
      MorphTarget mt;
      if (t.mVertices) {
        mt.positions.resize(n);
        for (unsigned i = 0; i < n; ++i)
          mt.positions[i] = {t.mVertices[i].x - am.mVertices[i].x, t.mVertices[i].y - am.mVertices[i].y,
                             t.mVertices[i].z - am.mVertices[i].z};
      } else {
        mt.positions.assign(n, Vec3{});
      }
      if (t.mNormals && am.mNormals && !out.normals.empty()) {
        mt.normals.resize(n);
        for (unsigned i = 0; i < n; ++i)
          mt.normals[i] = {t.mNormals[i].x - am.mNormals[i].x, t.mNormals[i].y - am.mNormals[i].y,
                           t.mNormals[i].z - am.mNormals[i].z};
      }
      out.morphTargets.push_back(std::move(mt));
      out.morphWeights.push_back(t.mWeight);
    }
    if (am.mNumAnimMeshes && am.mMethod == aiMorphingMethod_VERTEX_BLEND)
      warn("mesh '" + out.name + "' uses vertex-blend morphing; imported as relative morph targets");
    return out;
  }

  void embedded() {
    for (unsigned i = 0; i < s_.mNumTextures; ++i) {
      const aiTexture &t = *s_.mTextures[i];
      EmbeddedTexture e;
      e.key = t.mFilename.length ? std::string(t.mFilename.C_Str()) : "*" + std::to_string(i);
      std::string hint = t.achFormatHint;
      std::transform(hint.begin(), hint.end(), hint.begin(),
                     [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
      if (t.mHeight == 0) {
        // Compressed: pcData holds mWidth bytes of the encoded file.
        const auto *b = reinterpret_cast<const std::uint8_t *>(t.pcData);
        e.bytes.assign(b, b + t.mWidth);
        e.hintExt = hint.empty() ? "bin" : hint;
      } else {
        // Raw aiTexel (BGRA8) rows, top row first: hand on as RGBA8.
        e.width = t.mWidth;
        e.height = t.mHeight;
        e.bytes.resize(static_cast<std::size_t>(t.mWidth) * t.mHeight * 4);
        for (std::size_t p = 0; p < static_cast<std::size_t>(t.mWidth) * t.mHeight; ++p) {
          const aiTexel &x = t.pcData[p];
          e.bytes[p * 4 + 0] = x.r;
          e.bytes[p * 4 + 1] = x.g;
          e.bytes[p * 4 + 2] = x.b;
          e.bytes[p * 4 + 3] = x.a;
        }
        e.hintExt = "png";
      }
      out_.embedded.push_back(std::move(e));
    }
  }

  // ---- nodes -------------------------------------------------------------

  int walk(const aiNode &n) {
    const int me = static_cast<int>(out_.nodes.size());
    out_.nodes.emplace_back();
    {
      ImportNode &ir = out_.nodes.back();
      ir.name = n.mName.C_Str();
      ir.localTransform = toMat4(n.mTransformation);
      for (unsigned i = 0; i < n.mNumMeshes; ++i) ir.meshIndices.push_back(static_cast<int>(n.mMeshes[i]));
    }
    nodeByName_.emplace(n.mName.C_Str(), me);
    for (unsigned i = 0; i < n.mNumChildren; ++i) {
      const int c = walk(*n.mChildren[i]);
      out_.nodes[static_cast<std::size_t>(me)].childIndices.push_back(c);
    }
    return me;
  }

  // ---- cameras and lights ------------------------------------------------

  /// A frame at `pos` looking along `look` with `up` (camera: -Z forward).
  static Mat4 frame(Vec3 pos, Vec3 look, Vec3 up) {
    Vec3 z = normalized({-look.x, -look.y, -look.z});
    if (dot(z, z) == 0) z = {0, 0, 1};
    Vec3 x = cross(up, z);
    if (dot(x, x) < 1e-12f) x = cross(std::fabs(z.y) < 0.9f ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, z);
    x = normalized(x);
    const Vec3 y = cross(z, x);
    Mat4 m;
    m.m = {x.x, x.y, x.z, 0, y.x, y.y, y.z, 0, z.x, z.y, z.z, 0, pos.x, pos.y, pos.z, 1};
    return m;
  }

  void cameras() {
    for (unsigned i = 0; i < s_.mNumCameras; ++i) {
      const aiCamera &c = *s_.mCameras[i];
      ImportCamera cam;
      cam.name = c.mName.C_Str();
      cam.aspectRatio = c.mAspect > 0 ? c.mAspect : 0;
      const float aspect = c.mAspect > 0 ? c.mAspect : 1.0f;
      if (c.mOrthographicWidth > 0) {
        // Assimp stores the half width.
        cam.kind = ImportCamera::Kind::Orthographic;
        cam.xmag = c.mOrthographicWidth;
        cam.ymag = c.mOrthographicWidth / aspect;
      } else {
        // aiCamera documents mHorizontalFOV as the half angle, but the
        // glTF, FBX and Collada importers all store the full angle.
        cam.yfov = 2.0f * std::atan(std::tan(0.5f * c.mHorizontalFOV) / aspect);
      }
      cam.znear = c.mClipPlaneNear;
      cam.zfar = c.mClipPlaneFar;
      cam.local = frame(toVec3(c.mPosition), toVec3(c.mLookAt), toVec3(c.mUp));
      out_.cameras.push_back(cam);
      const int idx = static_cast<int>(out_.cameras.size()) - 1;
      if (const int node = nodeNamed(cam.name); node >= 0 && out_.nodes[static_cast<std::size_t>(node)].camera < 0)
        out_.nodes[static_cast<std::size_t>(node)].camera = idx;
    }
  }

  void lights() {
    for (unsigned i = 0; i < s_.mNumLights; ++i) {
      const aiLight &l = *s_.mLights[i];
      ImportLight out;
      out.name = l.mName.C_Str();
      switch (l.mType) {
        case aiLightSource_DIRECTIONAL: out.kind = ImportLight::Kind::Directional; break;
        case aiLightSource_POINT: out.kind = ImportLight::Kind::Point; break;
        case aiLightSource_SPOT: out.kind = ImportLight::Kind::Spot; break;
        case aiLightSource_AREA:
          out.kind = ImportLight::Kind::Point;
          warn("area light '" + out.name + "' approximated as a point light");
          break;
        case aiLightSource_AMBIENT:
          warn("ambient light '" + out.name + "' dropped; X3D lights carry ambientIntensity per light");
          continue;
        default: warn("light '" + out.name + "' of unknown type dropped"); continue;
      }
      // Assimp folds intensity into the colour; split it back out.
      const aiColor3D &c = l.mColorDiffuse;
      const float k = std::max({c.r, c.g, c.b});
      out.intensity = k > 0 ? k : 0;
      out.color = k > 0 ? Vec3{c.r / k, c.g / k, c.b / k} : Vec3{1, 1, 1};
      out.attenuation = {l.mAttenuationConstant, l.mAttenuationLinear, l.mAttenuationQuadratic};
      // Assimp cone angles are full angles; the IR wants half angles.
      out.innerConeAngle = l.mAngleInnerCone * 0.5f;
      out.outerConeAngle = l.mAngleOuterCone * 0.5f;
      out.position = toVec3(l.mPosition);
      if (out.kind != ImportLight::Kind::Point) {
        const Vec3 d = normalized(toVec3(l.mDirection));
        if (dot(d, d) > 0) out.direction = d;
      }
      out_.lights.push_back(out);
      const int idx = static_cast<int>(out_.lights.size()) - 1;
      if (const int node = nodeNamed(out.name); node >= 0 && out_.nodes[static_cast<std::size_t>(node)].light < 0)
        out_.nodes[static_cast<std::size_t>(node)].light = idx;
    }
  }

  // ---- skins -------------------------------------------------------------

  // Every node whose meshes carry bones gets one skin: the union of its
  // meshes' bones. A mesh shared by nodes with different skins is copied.
  void skins() {
    meshSkin_.assign(out_.meshes.size(), -1);
    const std::size_t nodeCount = out_.nodes.size();
    for (std::size_t ni = 0; ni < nodeCount; ++ni) {
      std::vector<int> meshes = out_.nodes[ni].meshIndices;
      bool any = false;
      for (int mi : meshes)
        if (mi >= 0 && static_cast<unsigned>(mi) < s_.mNumMeshes && s_.mMeshes[mi]->mNumBones) any = true;
      if (!any) continue;
      ImportSkin skin;
      skin.name = out_.nodes[ni].name;
      std::unordered_map<std::string, int> jointByName;
      for (int mi : meshes) {
        const aiMesh &am = *s_.mMeshes[mi];
        for (unsigned b = 0; b < am.mNumBones; ++b) {
          const aiBone &bone = *am.mBones[b];
          const std::string bn = bone.mName.C_Str();
          if (jointByName.count(bn)) continue;
          const int node = nodeNamed(bn);
          if (node < 0) {
            warn("bone '" + bn + "' has no node; its weights are dropped");
            continue;
          }
          jointByName.emplace(bn, static_cast<int>(skin.joints.size()));
          skin.joints.push_back(node);
          skin.inverseBind.push_back(toMat4(bone.mOffsetMatrix));
        }
      }
      if (skin.joints.empty()) continue;
      out_.skins.push_back(std::move(skin));
      const int si = static_cast<int>(out_.skins.size()) - 1;
      out_.nodes[ni].skin = si;
      for (std::size_t k = 0; k < meshes.size(); ++k) {
        int mi = meshes[k];
        const aiMesh &am = *s_.mMeshes[mi];
        if (meshSkin_[static_cast<std::size_t>(mi)] >= 0 && meshSkin_[static_cast<std::size_t>(mi)] != si) {
          ImportMesh copy = out_.meshes[static_cast<std::size_t>(mi)];
          copy.influences.clear();
          out_.meshes.push_back(std::move(copy));
          meshSkin_.push_back(-1);
          mi = static_cast<int>(out_.meshes.size()) - 1;
          out_.nodes[ni].meshIndices[k] = mi;
        }
        if (meshSkin_[static_cast<std::size_t>(mi)] == si) continue;
        meshSkin_[static_cast<std::size_t>(mi)] = si;
        ImportMesh &m = out_.meshes[static_cast<std::size_t>(mi)];
        m.influences.assign(m.positions.size(), {});
        for (unsigned b = 0; b < am.mNumBones; ++b) {
          const aiBone &bone = *am.mBones[b];
          auto it = jointByName.find(bone.mName.C_Str());
          if (it == jointByName.end()) continue;
          for (unsigned w = 0; w < bone.mNumWeights; ++w) {
            const aiVertexWeight &vw = bone.mWeights[w];
            if (vw.mVertexId < m.influences.size() && vw.mWeight > 0)
              m.influences[vw.mVertexId].emplace_back(it->second, vw.mWeight);
          }
        }
      }
    }
  }

  // ---- animations --------------------------------------------------------

  template <class Key, class Push>
  static void keys(const Key *k, unsigned count, double tps, AnimationChannel &ch, Push push) {
    double last = -1e300;
    for (unsigned i = 0; i < count; ++i) {
      const double t = k[i].mTime / tps;
      if (t <= last) continue; // Assimp can repeat a key; times must increase.
      last = t;
      ch.times.push_back(static_cast<float>(t));
      push(k[i]);
    }
  }

  int morphNode(const std::string &name) const {
    if (const int n = nodeNamed(name); n >= 0) return n;
    for (std::size_t ni = 0; ni < out_.nodes.size(); ++ni)
      for (int mi : out_.nodes[ni].meshIndices)
        if (out_.meshes[static_cast<std::size_t>(mi)].name == name) return static_cast<int>(ni);
    return -1;
  }

  void animations() {
    for (unsigned a = 0; a < s_.mNumAnimations; ++a) {
      const aiAnimation &an = *s_.mAnimations[a];
      ImportAnimation out;
      out.name = an.mName.C_Str();
      const double tps = an.mTicksPerSecond > 0 ? an.mTicksPerSecond : 25.0;
      for (unsigned c = 0; c < an.mNumChannels; ++c) {
        const aiNodeAnim &na = *an.mChannels[c];
        const int node = nodeNamed(na.mNodeName.C_Str());
        if (node < 0) {
          warn(std::string("animation channel for missing node '") + na.mNodeName.C_Str() + "' dropped");
          continue;
        }
        animated_[node] = true;
        if (na.mNumPositionKeys) {
          AnimationChannel ch;
          ch.path = AnimationChannel::Path::Translation;
          ch.target = node;
          keys(na.mPositionKeys, na.mNumPositionKeys, tps, ch, [&](const aiVectorKey &k) {
            ch.values.insert(ch.values.end(), {k.mValue.x, k.mValue.y, k.mValue.z});
          });
          out.channels.push_back(std::move(ch));
        }
        if (na.mNumRotationKeys) {
          AnimationChannel ch;
          ch.path = AnimationChannel::Path::Rotation;
          ch.target = node;
          keys(na.mRotationKeys, na.mNumRotationKeys, tps, ch, [&](const aiQuatKey &k) {
            ch.values.insert(ch.values.end(), {k.mValue.x, k.mValue.y, k.mValue.z, k.mValue.w});
          });
          out.channels.push_back(std::move(ch));
        }
        if (na.mNumScalingKeys) {
          AnimationChannel ch;
          ch.path = AnimationChannel::Path::Scale;
          ch.target = node;
          keys(na.mScalingKeys, na.mNumScalingKeys, tps, ch, [&](const aiVectorKey &k) {
            ch.values.insert(ch.values.end(), {k.mValue.x, k.mValue.y, k.mValue.z});
          });
          out.channels.push_back(std::move(ch));
        }
        if (na.mPreState != aiAnimBehaviour_DEFAULT || na.mPostState != aiAnimBehaviour_DEFAULT)
          warn("animation '" + out.name + "' pre/post behaviour ignored; it loops");
      }
      for (unsigned c = 0; c < an.mNumMorphMeshChannels; ++c) {
        const aiMeshMorphAnim &ma = *an.mMorphMeshChannels[c];
        const int node = morphNode(ma.mName.C_Str());
        if (node < 0) {
          warn(std::string("morph channel '") + ma.mName.C_Str() + "' has no node; dropped");
          continue;
        }
        std::size_t targets = 0;
        for (int mi : out_.nodes[static_cast<std::size_t>(node)].meshIndices)
          targets = std::max(targets, out_.meshes[static_cast<std::size_t>(mi)].morphTargets.size());
        if (targets == 0) continue;
        AnimationChannel ch;
        ch.path = AnimationChannel::Path::Weights;
        ch.target = node;
        keys(ma.mKeys, ma.mNumKeys, tps, ch, [&](const aiMeshMorphKey &k) {
          std::vector<float> w(targets, 0.0f);
          for (unsigned v = 0; v < k.mNumValuesAndWeights; ++v)
            if (k.mValues[v] < targets) w[k.mValues[v]] = static_cast<float>(k.mWeights[v]);
          ch.values.insert(ch.values.end(), w.begin(), w.end());
        });
        out.channels.push_back(std::move(ch));
      }
      if (an.mNumMeshChannels)
        warn("animation '" + out.name + "' swaps whole meshes; mesh channels dropped");
      if (!out.channels.empty()) out_.animations.push_back(std::move(out));
    }
  }
};

/// FBX files state their axes in metadata that Assimp reads but does not
/// apply. Rotate the root into X3D's +Y up, +Z front.
void applyFbxAxes(aiScene &s) {
  if (!s.mMetaData || !s.mRootNode) return;
  int up = 1, upSign = 1, front = 2, frontSign = 1, coord = 0, coordSign = 1;
  if (!s.mMetaData->Get("UpAxis", up)) return;
  s.mMetaData->Get("UpAxisSign", upSign);
  s.mMetaData->Get("FrontAxis", front);
  s.mMetaData->Get("FrontAxisSign", frontSign);
  s.mMetaData->Get("CoordAxis", coord);
  s.mMetaData->Get("CoordAxisSign", coordSign);
  auto axis = [](int a, int sign) {
    aiVector3D v(0, 0, 0);
    v[static_cast<unsigned>(std::clamp(a, 0, 2))] = static_cast<ai_real>(sign < 0 ? -1 : 1);
    return v;
  };
  const aiVector3D r = axis(coord, coordSign), u = axis(up, upSign), f = axis(front, frontSign);
  const aiMatrix4x4 m(r.x, r.y, r.z, 0, u.x, u.y, u.z, 0, f.x, f.y, f.z, 0, 0, 0, 0, 1);
  if (m.IsIdentity()) return;
  s.mRootNode->mTransformation = m * s.mRootNode->mTransformation;
}

} // namespace

ImportScene AssimpSource::load(const std::string &path) {
  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);
  const unsigned flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                         aiProcess_GenSmoothNormals | aiProcess_SortByPType |
                         aiProcess_ValidateDataStructure | aiProcess_TransformUVCoords |
                         aiProcess_GenUVCoords;
  const aiScene *scene = importer.ReadFile(path, flags);
  if (!scene) throw std::runtime_error("assimp could not read '" + path + "': " + importer.GetErrorString());
  if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    throw std::runtime_error("assimp read '" + path + "' only partly (incomplete scene)");
  if (hasExtension(path, {".fbx"})) applyFbxAxes(*const_cast<aiScene *>(scene));
  ImportScene out;
  Reader(*scene, out).run();
  return out;
}

void addAssimpBackend(BackendRegistry &registry) {
  std::vector<std::string> exts;
  {
    Assimp::Importer probe;
    aiString list;
    probe.GetExtensionList(list);
    std::string s = list.C_Str(); // "*.3ds;*.obj;..."
    std::size_t p = 0;
    while (p < s.size()) {
      const std::size_t q = s.find(';', p);
      std::string e = s.substr(p, q == std::string::npos ? std::string::npos : q - p);
      if (e.rfind("*", 0) == 0) e.erase(0, 1);
      if (!e.empty()) exts.push_back(lowerExtension("x" + e));
      if (q == std::string::npos) break;
      p = q + 1;
    }
  }
  registry.add({"assimp", exts,
                [exts](const std::string &p) {
                  const std::string e = lowerExtension(p);
                  if (e == ".gltf" || e == ".glb") return 10;
                  return std::find(exts.begin(), exts.end(), e) != exts.end() ? 50 : -1;
                },
                [] { return std::unique_ptr<ImportSource>(std::make_unique<AssimpSource>()); }});
}

} // namespace x3d::runtime::importer
