// assimp_source.cpp — assimp ImportSource backend. Compiled ONLY when
// -DX3D_CPP_BUILD_ASSIMP=ON (gated in the subdir CMakeLists). Converts the
// assimp aiScene into the ImportScene IR (Task 3) that emit() consumes.
//
// assimp lives behind the ImportSource seam: emit/texture-pipeline/CLI never
// include assimp headers; only this TU does. The IR is the stable boundary,
// so swapping assimp for a native glTF reader (spec fast-follow) touches only
// this file.
#include "assimp_source.hpp"

#include <assimp/Importer.hpp>      // Assimp::Importer
#include <assimp/scene.h>           // aiScene, aiNode, aiMesh, aiMaterial, aiCamera, aiLight, aiTexture
#include <assimp/postprocess.h>     // aiProcess_*
#include <assimp/material.h>        // AI_MATKEY_*, aiTextureType_*
#include <assimp/matrix4x4.h>       // aiMatrix4x4 (row-major: a1..d4)
#include <assimp/vector3.h>          // aiVector3D
#include <assimp/color4.h>          // aiColor4D
#include <assimp/camera.h>         // aiCamera
#include <assimp/light.h>          // aiLight, aiLightSource_*

#include <cstring>
#include <stdexcept>
#include <string>

namespace x3d::asset_import {

namespace {

// assimp's aiMatrix4x4 is ROW-MAJOR (a1..a4 = row 0). Our IR Mat4 is
// COLUMN-MAJOR (m[0..3] = column 0). Transpose on copy.
Mat4 toColumnMajor(const aiMatrix4x4& t) {
  Mat4 m;
  // aiMatrix4x4 layout: a1 a2 a3 a4 | b1 b2 b3 b4 | c1 c2 c3 c4 | d1 d2 d3 d4
  // Column-major storage: m[col*4 + row].
  m.m[0] = t.a1;  m.m[1] = t.b1;  m.m[2] = t.c1;  m.m[3] = t.d1;   // col 0
  m.m[4] = t.a2;  m.m[5] = t.b2;  m.m[6] = t.c2;  m.m[7] = t.d2;   // col 1
  m.m[8] = t.a3;  m.m[9] = t.b3;  m.m[10] = t.c3;  m.m[11] = t.d3; // col 2
  m.m[12] = t.a4; m.m[13] = t.b4; m.m[14] = t.c4;  m.m[15] = t.d4; // col 3
  return m;
}

Vec3 toVec3(const aiVector3D& v) { return {v.x, v.y, v.z}; }
Vec2 toVec2(const aiVector3D& v) { return {v.x, v.y}; }

// Loads one aiMaterial into the IR. Phong fields come from the classic
// AI_MATKEY_COLOR_* keys; PBR is detected via AI_MATKEY_GLTF_PBRMETALLICROUGHNESS
// (metallic/roughness factors + base color) and recorded in ImportMaterial::pbr.
ImportMaterial toMaterial(const aiMaterial* mat) {
  ImportMaterial out;
  aiString name;
  if (mat->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS) {
    out.name = name.C_Str();
  }

  aiColor3D c;
  if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, c) == aiReturn_SUCCESS) {
    out.diffuse = {c.r, c.g, c.b};
  }
  if (mat->Get(AI_MATKEY_COLOR_SPECULAR, c) == aiReturn_SUCCESS) {
    out.specular = {c.r, c.g, c.b};
  }
  if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, c) == aiReturn_SUCCESS) {
    out.emissive = {c.r, c.g, c.b};
  }
  float f;
  if (mat->Get(AI_MATKEY_SHININESS, f) == aiReturn_SUCCESS) {
    out.shininess = f;
  }
  if (mat->Get(AI_MATKEY_OPACITY, f) == aiReturn_SUCCESS) {
    out.opacity = f;
  }

  // PBR (glTF 2.0): metallic/roughness factors + base color. assimp exposes
  // these under AI_MATKEY_METALLIC_FACTOR / _ROUGHNESS_FACTOR / _BASE_COLOR.
  ai_real metallic = 0, roughness = 0;
  bool hasMetal = mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS;
  bool hasRough = mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS;
  aiColor4D base(1, 1, 1, 1);
  bool hasBase = mat->Get(AI_MATKEY_BASE_COLOR, base) == aiReturn_SUCCESS;
  if (hasMetal && hasRough) {
    out.pbr = PbrParams{};
    out.pbr->metallic = static_cast<float>(metallic);
    out.pbr->roughness = static_cast<float>(roughness);
    if (hasBase) {
      out.pbr->baseColor = {base.r, base.g, base.b, base.a};
    }
  }

  return out;
}

// Resolves one material texture slot into a TextureRef. Embedded textures
// (assimp `*N` paths) become embeddedIndex refs; file paths stay external.
// Returns true and fills `ref` when the slot has at least one texture.
bool toTextureRef(const aiMaterial* mat, aiTextureType type,
                  const aiScene* scene, TextureRef& ref) {
  if (mat->GetTextureCount(type) == 0) return false;
  aiString path;
  if (mat->GetTexture(type, 0, &path) != aiReturn_SUCCESS) return false;
  const std::string p = path.C_Str();
  if (p.empty()) return false;

  // Embedded textures are referenced as "*N" (compressed) in assimp 5+.
  if (p[0] == '*') {
    auto [tex, idx] = scene->GetEmbeddedTextureAndIndex(p.c_str());
    if (idx >= 0) {
      ref.embeddedIndex = idx;
      return true;
    }
  }
  ref.externalPath = p;
  return true;
}

void toTextureSlots(const aiMaterial* mat, const aiScene* scene,
                    TextureSlots& slots) {
  TextureRef ref;
  if (toTextureRef(mat, aiTextureType_DIFFUSE, scene, ref)) slots.baseColor = ref;
  ref = {};
  if (toTextureRef(mat, aiTextureType_BASE_COLOR, scene, ref)) slots.baseColor = ref;
  ref = {};
  if (toTextureRef(mat, aiTextureType_NORMALS, scene, ref)) slots.normal = ref;
  ref = {};
  if (toTextureRef(mat, aiTextureType_EMISSIVE, scene, ref)) slots.emissive = ref;
  ref = {};
  if (toTextureRef(mat, aiTextureType_AMBIENT_OCCLUSION, scene, ref))
    slots.occlusion = ref;
  ref = {};
  // glTF packs metallic+roughness into one texture on the roughness slot.
  if (toTextureRef(mat, aiTextureType_DIFFUSE_ROUGHNESS, scene, ref) ||
      toTextureRef(mat, aiTextureType_METALNESS, scene, ref)) {
    slots.metallicRoughness = ref;
  }
  ref = {};
  if (toTextureRef(mat, aiTextureType_SPECULAR, scene, ref)) slots.specular = ref;
}

// Loads one aiMesh into the IR. assimp has already triangulated (aiProcess_
// Triangulate) and generated normals (GenSmoothNormals); we copy positions,
// normals, uv[0], colors[0], and flatten the face index array.
ImportMesh toMesh(const aiMesh* mesh) {
  ImportMesh out;
  out.materialIndex =
      mesh->mMaterialIndex < 0xFFFFFFFF ? static_cast<int>(mesh->mMaterialIndex) : -1;

  out.positions.reserve(mesh->mNumVertices);
  for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
    out.positions.push_back(toVec3(mesh->mVertices[i]));
  }

  if (mesh->mNormals) {
    out.normals.reserve(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      out.normals.push_back(toVec3(mesh->mNormals[i]));
    }
  }

  if (mesh->mTextureCoords[0]) {
    out.uv.reserve(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      out.uv.push_back(toVec2(mesh->mTextureCoords[0][i]));
    }
  }

  if (mesh->mColors[0]) {
    out.colors.reserve(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      const aiColor4D& c = mesh->mColors[0][i];
      out.colors.push_back({c.r, c.g, c.b, c.a});
    }
  }

  out.indices.reserve(mesh->mNumFaces * 3);
  for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    for (unsigned j = 0; j < face.mNumIndices; ++j) {
      out.indices.push_back(face.mIndices[j]);
    }
  }
  return out;
}

ImportCamera toCamera(const aiCamera* cam) {
  ImportCamera out;
  // assimp gives world-space position + look-at + up; build a camera-to-world
  // matrix (right = up x lookat for a left-handed view... here we just place
  // position and encode orientation as identity — emit() derives orientation
  // from decomposeTRS, and the fixture/round-trip path only needs position +
  // fov, which the Viewpoint setters carry). For full orientation, a future
  // task can build the basis explicitly; this keeps the IR honest.
  out.world = Mat4{};  // identity; position encoded below.
  out.world.m[12] = cam->mPosition.x;
  out.world.m[13] = cam->mPosition.y;
  out.world.m[14] = cam->mPosition.z;
  // assimp's mHorizontalFOV is horizontal; X3D Viewpoint fieldOfView is also
  // a single angle (commonly horizontal). Pass it through directly.
  out.yfov = cam->mHorizontalFOV;
  out.znear = cam->mClipPlaneNear;
  out.zfar = cam->mClipPlaneFar;
  return out;
}

ImportLight toLight(const aiLight* l) {
  ImportLight out;
  switch (l->mType) {
    case aiLightSource_DIRECTIONAL: out.kind = ImportLight::Kind::Dir; break;
    case aiLightSource_POINT:       out.kind = ImportLight::Kind::Point; break;
    case aiLightSource_SPOT:        out.kind = ImportLight::Kind::Spot; break;
    default:                        out.kind = ImportLight::Kind::Dir; break;
  }
  out.color = {l->mColorDiffuse.r, l->mColorDiffuse.g, l->mColorDiffuse.b};
  out.position = toVec3(l->mPosition);
  out.direction = toVec3(l->mDirection);
  out.attenuation = {l->mAttenuationConstant, l->mAttenuationLinear,
                     l->mAttenuationQuadratic};
  out.intensity = 1.0f;
  out.cutOffAngle = l->mAngleOuterCone;
  out.beamWidth = l->mAngleInnerCone;
  out.radius = 100.0f;
  return out;
}

// Recursively walks the aiNode tree into the IR's flat nodes vector, recording
// parent->child via childIndices and returning this node's index.
int walkNode(const aiNode* node, ImportScene& scene) {
  ImportNode ir;
  ir.name = node->mName.C_Str();
  ir.localTransform = toColumnMajor(node->mTransformation);

  const int myIndex = static_cast<int>(scene.nodes.size());
  scene.nodes.push_back(std::move(ir));

  ImportNode& me = scene.nodes[myIndex];
  for (unsigned i = 0; i < node->mNumMeshes; ++i) {
    me.meshIndices.push_back(static_cast<int>(node->mMeshes[i]));
  }
  for (unsigned i = 0; i < node->mNumChildren; ++i) {
    const int childIndex = walkNode(node->mChildren[i], scene);
    me.childIndices.push_back(childIndex);
  }
  return myIndex;
}

}  // namespace

ImportScene AssimpSource::load(const std::string& path) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(path,
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices);
  if (scene == nullptr) {
    throw std::runtime_error("assimp failed to read: " + path + " — " +
                            importer.GetErrorString());
  }

  ImportScene out;

  for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
    out.meshes.push_back(toMesh(scene->mMeshes[i]));
  }
  for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
    ImportMaterial m = toMaterial(scene->mMaterials[i]);
    toTextureSlots(scene->mMaterials[i], scene, m.textures);
    out.materials.push_back(std::move(m));
  }
  for (unsigned i = 0; i < scene->mNumCameras; ++i) {
    out.cameras.push_back(toCamera(scene->mCameras[i]));
  }
  for (unsigned i = 0; i < scene->mNumLights; ++i) {
    out.lights.push_back(toLight(scene->mLights[i]));
  }
  for (unsigned i = 0; i < scene->mNumTextures; ++i) {
    const aiTexture* t = scene->mTextures[i];
    EmbeddedTexture et;
    et.key = t->mFilename.C_Str();
    if (et.key.empty()) et.key = "*" + std::to_string(i);
    // achFormatHint is a C string like "png" / "jpg" when compressed.
    et.hintExt = t->achFormatHint[0] ? std::string(t->achFormatHint) : "bin";
    if (t->mHeight == 0) {
      // Compressed: pcData is mWidth bytes of encoded image data.
      const auto* bytes = reinterpret_cast<const std::uint8_t*>(t->pcData);
      et.bytes.assign(bytes, bytes + t->mWidth);
    } else {
      // Uncompressed texels (rare for model files); leave bytes empty — the
      // texture pipeline will re-encode from RGBA. (Handled in Task 9's
      // re-encode path; for now mark as needing decode.)
      et.hintExt = "bin";
    }
    out.embedded.push_back(std::move(et));
  }

  if (scene->mRootNode) {
    out.rootNode = walkNode(scene->mRootNode, out);
  }

  return out;
}

}  // namespace x3d::asset_import
