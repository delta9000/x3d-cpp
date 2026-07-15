// TextureExtract.hpp — T-TEX v1: close textures end-to-end.
//
// namespace x3d::runtime::extract. Header-only, golden-untouched, node-as-truth
// (every read goes through the geombounds reflection helpers / nodeTypeName()
// dispatch; adds NO members to generated nodes). This is the "heavy logic" file
// the v1-closure design (2026-06-16 §3) keeps OUT of SceneExtractor/MeshBuilder so
// those take only a thin dispatch hook — see SceneExtractor::emit().
//
// RESPONSIBILITIES (the gaps the design's T-TEX track lists):
//   1. textureTransformParamsOf(appearance) — read the authored TextureTransform /
//      TextureTransform2D node from Appearance.textureTransform into the pure
//      TextureTransform2DParams field bag (§18.4.10). NULL / identity => identity.
//   2. applyTextureTransformToMesh(mesh, params) — BAKE that transform into every
//      MeshData.texcoord at extraction time (CPU-side §18.4.10 application). A
//      cheap identity early-exit keeps untransformed meshes byte-unchanged.
//   3. extendedSamplerOf(texNode) — full §18.4.9 sampler descriptor: repeatS/T,
//      TextureProperties boundary/filter modes, mipmaps, anisotropy. When no
//      TextureProperties is present the boundary modes derive from repeatS/T
//      (Repeat / ClampToEdge per §18.2.3).
//   4. texCoordGenOf(geom, &has) — when a geometry node's texCoord field is a
//      TextureCoordinateGenerator (not a TextureCoordinate), surface the
//      mode+parameter as a TexCoordGenDesc (the renderer computes UVs per §18.4.8).
//   5. resolveTextureRefs(refs, resolver) — thread the embedder's TextureResolver
//      onto each TextureRef.resolvedPixels (Url goes through the resolver after
//      applying the MFString fallback order; Inline / Movie do NOT).
//   6. enrichTextureRefs(refs, geom) — fill each ref's extSampler from its source
//      texture node and the geometry-borne TexCoordGenDesc (a single pass an
//      embedder/extractor calls after materialOf()).
//
// AUTHORED TEXTURECOORDINATE: extraction of explicit TextureCoordinate.point onto
// MeshData.texcoords (indexed by texCoordIndex, falling back to coordIndex per
// §13.3.6) already lives in MeshBuilder.hpp (buildAttrs/pickT) and ALWAYS wins
// over the default §13 bbox/grid projection. This file does NOT duplicate it; it
// adds the TextureTransform bake on top + the descriptor/resolver surface.
#ifndef X3D_RUNTIME_EXTRACT_TEXTURE_EXTRACT_HPP
#define X3D_RUNTIME_EXTRACT_TEXTURE_EXTRACT_HPP

#include "GeometryBounds.hpp"     // geombounds::getField/getNode/hasField
#include "RenderItem.hpp"         // MeshData / TextureRef
#include "TextureResolver.hpp"    // TextureResolver / TexturePixelResult
#include "TextureTransform2D.hpp" // TextureTransform2DParams / Extended sampler / TexCoordGen
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;

namespace texextract {

// Read an SFEnum field as its X3D token string via FieldInfo::getEnumString, so
// this stays decoupled from the generated enum-class type (same idiom as
// matsys::getEnumToken). Returns dflt if absent / not an enum / unreadable.
inline std::string enumToken(const x3d::nodes::X3DNode &n, const char *name,
                             const std::string &dflt) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) {
      if (!f.getEnumString) return dflt;
      try {
        std::string s = f.getEnumString(n);
        return s.empty() ? dflt : s;
      } catch (...) {
        return dflt;
      }
    }
  return dflt;
}

// §18.4.9 Table 18.7 — TextureProperties boundary-mode token -> BoundaryMode.
inline BoundaryMode boundaryModeFromToken(const std::string &t) {
  if (t == "CLAMP") return BoundaryMode::Clamp;
  if (t == "CLAMP_TO_EDGE") return BoundaryMode::ClampToEdge;
  if (t == "CLAMP_TO_BOUNDARY") return BoundaryMode::ClampToBoundary;
  if (t == "MIRRORED_REPEAT") return BoundaryMode::MirroredRepeat;
  return BoundaryMode::Repeat; // "REPEAT" / unknown.
}

// §18.4.9 Table 18.8 — magnification filter token -> MagFilter.
inline MagFilter magFilterFromToken(const std::string &t) {
  if (t == "AVG_PIXEL") return MagFilter::AvgPixel;
  if (t == "NEAREST_PIXEL") return MagFilter::NearestPixel;
  if (t == "FASTEST") return MagFilter::Fastest;
  if (t == "NICEST") return MagFilter::Nicest;
  return MagFilter::Default;
}

// §18.4.9 Table 18.9 — minification filter token -> MinFilter.
inline MinFilter minFilterFromToken(const std::string &t) {
  if (t == "AVG_PIXEL") return MinFilter::AvgPixel;
  if (t == "AVG_PIXEL_AVG_MIPMAP") return MinFilter::AvgPixelAvgMipmap;
  if (t == "AVG_PIXEL_NEAREST_MIPMAP") return MinFilter::AvgPixelNearestMipmap;
  if (t == "NEAREST_PIXEL") return MinFilter::NearestPixel;
  if (t == "NEAREST_PIXEL_AVG_MIPMAP") return MinFilter::NearestPixelAvgMipmap;
  if (t == "NEAREST_PIXEL_NEAREST_MIPMAP")
    return MinFilter::NearestPixelNearestMipmap;
  if (t == "FASTEST") return MinFilter::Fastest;
  if (t == "NICEST") return MinFilter::Nicest;
  return MinFilter::Default;
}

// §18.4.8 Table 18.6 — TextureCoordinateGenerator mode token -> TexCoordGenMode.
inline TexCoordGenMode genModeFromToken(const std::string &t) {
  if (t == "CAMERASPACENORMAL") return TexCoordGenMode::CameraSpaceNormal;
  if (t == "CAMERASPACEPOSITION") return TexCoordGenMode::CameraSpacePosition;
  if (t == "CAMERASPACEREFLECTIONVECTOR")
    return TexCoordGenMode::CameraSpaceReflectionVector;
  if (t == "SPHERE-LOCAL") return TexCoordGenMode::SphereLocal;
  if (t == "COORD") return TexCoordGenMode::Coord;
  if (t == "COORD-EYE") return TexCoordGenMode::CoordEye;
  if (t == "NOISE") return TexCoordGenMode::Noise;
  if (t == "NOISE-EYE") return TexCoordGenMode::NoiseEye;
  if (t == "SPHERE-REFLECT") return TexCoordGenMode::SphereReflect;
  if (t == "SPHERE-REFLECT-LOCAL") return TexCoordGenMode::SphereReflectLocal;
  return TexCoordGenMode::Sphere; // "SPHERE" / unknown.
}

} // namespace texextract

inline TextureTransform2DParams textureTransformParamsOfNode(const x3d::nodes::X3DNode &tt) {
  TextureTransform2DParams p; // identity by default.
  const std::string t = tt.nodeTypeName();
  if (t == "MultiTextureTransform") {
    const auto children =
        geombounds::getField<MFNode>(tt, "textureTransform", {});
    for (const auto &child : children) {
      if (!child) continue;
      const auto childType = child->nodeTypeName();
      if (childType == "TextureTransform" || childType == "TextureTransform2D" ||
          childType == "TextureTransformMatrix3D")
        return textureTransformParamsOfNode(*child);
    }
    return p;
  }

  if (t == "TextureTransformMatrix3D") {
    const SFMatrix4f m = geombounds::getField<SFMatrix4f>(tt, "matrix", {});
    p.hasMatrix = true;
    p.matrix = {
        m.matrix[0][0], m.matrix[0][1], m.matrix[0][3],
        m.matrix[1][0], m.matrix[1][1], m.matrix[1][3],
        m.matrix[3][0], m.matrix[3][1], m.matrix[3][3]};
    return p;
  }

  if (t != "TextureTransform" && t != "TextureTransform2D") return p;

  const SFVec2f center =
      geombounds::getField<SFVec2f>(tt, "center", SFVec2f{0.0f, 0.0f});
  const SFVec2f scale =
      geombounds::getField<SFVec2f>(tt, "scale", SFVec2f{1.0f, 1.0f});
  const SFVec2f translation =
      geombounds::getField<SFVec2f>(tt, "translation", SFVec2f{0.0f, 0.0f});
  p.centerS = center.x;
  p.centerT = center.y;
  p.rotation = geombounds::getField<float>(tt, "rotation", 0.0f);
  p.scaleS = scale.x;
  p.scaleT = scale.y;
  p.translationS = translation.x;
  p.translationT = translation.y;
  return p;
}

// ---------------------------------------------------------------------------
// textureTransformParamsOf — read Appearance.textureTransform into the pure
// TextureTransform2DParams field bag (§18.4.10).
//
//   * appearance == nullptr / no textureTransform / null node => identity.
//   * A 2D TextureTransform / TextureTransform2D node => its center/rotation/
//     scale/translation SFVec2f+SFFloat fields.
//   * MultiTextureTransform => first usable contained transform for legacy
//     callers; use textureTransformParamsListOf() for per-channel transforms.
//   * TextureTransformMatrix3D => project the 4x4 transform over (s,t,0,1) onto
//     the same 2D seam.
// ---------------------------------------------------------------------------
inline TextureTransform2DParams textureTransformParamsOf(const x3d::nodes::X3DNode *appearance) {
  TextureTransform2DParams p; // identity by default.
  if (!appearance) return p;
  auto tt = geombounds::getNode(*appearance, "textureTransform");
  if (!tt) return p;
  return textureTransformParamsOfNode(*tt);
}

inline std::vector<TextureTransform2DParams>
textureTransformParamsListOfNode(const x3d::nodes::X3DNode &tt) {
  if (tt.nodeTypeName() != "MultiTextureTransform")
    return {textureTransformParamsOfNode(tt)};
  std::vector<TextureTransform2DParams> out;
  const auto children = geombounds::getField<MFNode>(tt, "textureTransform", {});
  out.reserve(children.size());
  for (const auto &child : children)
    out.push_back(child ? textureTransformParamsOfNode(*child)
                        : TextureTransform2DParams{});
  return out;
}

inline std::vector<TextureTransform2DParams>
textureTransformParamsListOf(const x3d::nodes::X3DNode *appearance) {
  if (!appearance) return {};
  auto tt = geombounds::getNode(*appearance, "textureTransform");
  if (!tt) return {};
  return textureTransformParamsListOfNode(*tt);
}

// ---------------------------------------------------------------------------
// applyTextureTransformToMesh — BAKE a TextureTransform into mesh.texcoords
// (§18.4.10), in place. Identity transforms early-exit (no allocation, no write)
// so untransformed meshes stay byte-for-byte identical. Operates on whatever
// texcoords MeshBuilder already produced (authored TextureCoordinate or the
// default §13 projection) — both are in the same [0,1]-ish UV space.
// ---------------------------------------------------------------------------
inline void applyTextureTransformToMesh(MeshData &mesh,
                                        const TextureTransform2DParams &p) {
  if (isIdentityTextureTransform(p)) return;
  for (SFVec2f &uv : mesh.texcoords) {
    const std::array<float, 2> r = applyTextureTransform(uv.x, uv.y, p);
    uv.x = r[0];
    uv.y = r[1];
  }
}

inline void applyTextureTransformsToMesh(
    MeshData &mesh, const std::vector<TextureTransform2DParams> &params) {
  if (params.empty()) return;
  if (mesh.texcoordSets.empty() && !mesh.texcoords.empty())
    mesh.texcoordSets.push_back(mesh.texcoords);
  for (std::size_t i = 0; i < params.size() && i < mesh.texcoordSets.size(); ++i) {
    if (isIdentityTextureTransform(params[i])) continue;
    for (SFVec2f &uv : mesh.texcoordSets[i]) {
      const std::array<float, 2> r =
          applyTextureTransform(uv.x, uv.y, params[i]);
      uv.x = r[0];
      uv.y = r[1];
    }
  }
  if (!mesh.texcoordSets.empty())
    mesh.texcoords = mesh.texcoordSets[0];
}

// ---------------------------------------------------------------------------
// extendedSamplerOf — resolve a texture node's §18.4.9 sampler state.
//
//   * No TextureProperties: repeatS/T read off the texture node; boundary modes
//     DERIVE from them (Repeat <-> repeat==TRUE, ClampToEdge otherwise, §18.2.3);
//     filters Default; no mipmaps; isotropic.
//   * With TextureProperties (non-null): boundaryModeS/T + filters + generate
//     MipMaps + anisotropicDegree GOVERN; repeatS/T are kept for legacy consumers
//     but the boundary enums are authoritative (§18.4.9). repeatS/T are also
//     back-derived from the boundary modes so a legacy bool-only consumer still
//     sees a sensible REPEAT vs non-REPEAT value.
// ---------------------------------------------------------------------------
inline ExtendedSamplerParams extendedSamplerOf(const std::shared_ptr<x3d::nodes::X3DNode> &texNode) {
  using namespace texextract;
  ExtendedSamplerParams s;
  if (!texNode) return s;

  s.repeatS = geombounds::getField<bool>(*texNode, "repeatS", true);
  s.repeatT = geombounds::getField<bool>(*texNode, "repeatT", true);
  // repeatR exists only on 3D texture nodes (§33); absent on 2D, defaults true.
  s.repeatR = geombounds::getField<bool>(*texNode, "repeatR", true);

  auto tp = geombounds::getNode(*texNode, "textureProperties");
  if (!tp) {
    // Legacy path: derive boundary modes from repeatS/T/R (§18.2.3, §33).
    s.boundaryModeS = boundaryModeFromRepeat(s.repeatS);
    s.boundaryModeT = boundaryModeFromRepeat(s.repeatT);
    s.boundaryModeR = boundaryModeFromRepeat(s.repeatR);
    return s;
  }

  // TextureProperties governs (§18.4.9). repeatS/T/R on the texture node ignored.
  s.boundaryModeS = boundaryModeFromToken(enumToken(*tp, "boundaryModeS", "REPEAT"));
  s.boundaryModeT = boundaryModeFromToken(enumToken(*tp, "boundaryModeT", "REPEAT"));
  s.boundaryModeR = boundaryModeFromToken(enumToken(*tp, "boundaryModeR", "REPEAT"));
  s.magnificationFilter =
      magFilterFromToken(enumToken(*tp, "magnificationFilter", "DEFAULT"));
  s.minificationFilter =
      minFilterFromToken(enumToken(*tp, "minificationFilter", "DEFAULT"));
  s.generateMipmaps = geombounds::getField<bool>(*tp, "generateMipMaps", false);
  s.anisotropicDegree =
      geombounds::getField<float>(*tp, "anisotropicDegree", 1.0f);
  // Back-derive the legacy repeat bools so a bool-only consumer stays sane.
  s.repeatS = (s.boundaryModeS == BoundaryMode::Repeat ||
               s.boundaryModeS == BoundaryMode::MirroredRepeat);
  s.repeatT = (s.boundaryModeT == BoundaryMode::Repeat ||
               s.boundaryModeT == BoundaryMode::MirroredRepeat);
  return s;
}

// ---------------------------------------------------------------------------
// texCoordGenOf — when a geometry node's texCoord field is a
// TextureCoordinateGenerator, surface the mode+parameter as a TexCoordGenDesc
// (§18.4.8). The renderer computes the actual per-vertex UVs from view state.
// Sets *has=true only when a generator is present; otherwise *has=false and the
// returned (default-Sphere) descriptor must be ignored.
// ---------------------------------------------------------------------------
inline TexCoordGenDesc texCoordGenOf(const x3d::nodes::X3DNode *geom, bool *has) {
  using namespace texextract;
  TexCoordGenDesc d;
  if (has) *has = false;
  if (!geom) return d;
  auto tc = geombounds::getNode(*geom, "texCoord");
  if (!tc || tc->nodeTypeName() != "TextureCoordinateGenerator") return d;
  d.mode = genModeFromToken(enumToken(*tc, "mode", "SPHERE"));
  d.parameter = geombounds::getField<std::vector<float>>(*tc, "parameter", {});
  if (has) *has = true;
  return d;
}

// ---------------------------------------------------------------------------
// resolveTextureRefs — thread the embedder's TextureResolver onto each ref's
// resolvedPixels. The caller applies the MFString fallback order: try each URL
// in order and keep the FIRST Ready (or Pending) result; only when every URL
// Fails does the ref stay Failed. Source::Inline (PixelTexture) and Source::Movie
// NEVER go through the resolver (Inline is decoded from inlinePixels; Movie is
// descriptor-only) — their resolvedPixels stays makeFailed().
//
// `resolver` must be a valid callable (use makeNullTextureResolver() for the
// default white-fallback PoC path; a null std::function is NOT legal to call).
//
// `memo` (optional, ADR-0045) de-duplicates resolves BY URL. Decoding is the most
// expensive thing at this seam and one URL is routinely shared by many
// placements, so the extractor threads a per-snapshot memo through and N
// placements of one textured DEF'd Shape cost ONE embedder call and ONE shared
// buffer instead of N of each.
//
// ONLY Ready results are memoized, deliberately. Pending means "not decoded yet,
// retry on a later frame" — caching it would freeze the retry and the texture
// would never appear. Failed is likewise re-offered, so a resolver that recovers
// (a late mount, a warmed cache) is not permanently written off. Neither carries
// pixel payload, so re-offering them costs nothing that matters.
// ---------------------------------------------------------------------------
inline void resolveTextureRefs(
    std::vector<TextureRef> &refs, const TextureResolver &resolver,
    std::unordered_map<std::string, TexturePixelResult> *memo = nullptr) {
  if (!resolver) return; // defensive: never call a null std::function.
  for (TextureRef &ref : refs) {
    if (ref.source != TextureRef::Source::Url) continue; // Inline/Movie: skip.
    TexturePixelResult result = TexturePixelResult::makeFailed();
    for (const std::string &url : ref.url) {
      if (url.empty()) continue;
      if (memo) {
        auto hit = memo->find(url);
        if (hit != memo->end()) {
          // Ready-only memo, so a hit always wins the MFString fallback race.
          result = hit->second; // shares the pixel buffer, copies no bytes.
          break;
        }
      }
      TexturePixelResult r = resolver(url);
      if (r.ready() || r.pending()) {
        if (memo && r.ready()) memo->emplace(url, r);
        result = std::move(r);
        break; // first non-failed URL wins (MFString fallback order).
      }
    }
    ref.resolvedPixels = std::move(result);
  }
}

// ---------------------------------------------------------------------------
// enrichTextureRefs — single pass an extractor calls after materialOf(): fills
// each ref's extended sampler from its SOURCE texture node, and attaches the
// geometry-borne TexCoordGenDesc (one generator per geometry applies to every
// texture channel that uses default coords). MaterialSystem already populated the
// legacy SamplerParams + URLs; this layers the §18.4.9/§18.4.8 descriptor surface.
//
// `texNodes` is parallel to `refs` (the concrete texture node each ref came from,
// or nullptr); pass an empty vector to skip the sampler enrichment (the legacy
// SamplerParams on each ref still stands).
// ---------------------------------------------------------------------------
inline void enrichTextureRefs(std::vector<TextureRef> &refs,
                              const std::vector<std::shared_ptr<x3d::nodes::X3DNode>> &texNodes,
                              const x3d::nodes::X3DNode *geom) {
  bool hasGen = false;
  TexCoordGenDesc gen = texCoordGenOf(geom, &hasGen);
  for (std::size_t i = 0; i < refs.size(); ++i) {
    if (i < texNodes.size() && texNodes[i])
      refs[i].extSampler = extendedSamplerOf(texNodes[i]);
    if (hasGen) {
      refs[i].hasTexCoordGen = true;
      refs[i].texCoordGen = gen;
    }
  }
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_TEXTURE_EXTRACT_HPP
