// TextureTransform2D.hpp — pure (s,t) TextureTransform math, §18.4.10.
//
// Normative reference: ISO/IEC 19775-1:2023 §18.4.10 TextureTransform
//   Tc' = −C × S × R × C × T × Tc
// Expanded scalar application order (per spec):
//   1. translate  (apply translation field)
//   2. pivot to center
//   3. scale (non-uniform)
//   4. rotate CCW by rotation (radians)
//   5. pivot back from center
//
// This header is PURE:
//   - no X3D node types, no generated bindings, no scene-graph includes.
//   - only <cmath> and <array>.
//   - all functions are inline / constexpr where possible.
//   - independently testable with a single g++ -std=c++20 -fsyntax-only invocation.
//
// namespace x3d::runtime::extract
#ifndef X3D_RUNTIME_EXTRACT_TEXTURETRANSFORM2D_HPP
#define X3D_RUNTIME_EXTRACT_TEXTURETRANSFORM2D_HPP

#include <array>
#include <cmath>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// TextureTransform2DParams — the four authored fields of an X3D TextureTransform
// node (§18.4.10). All fields carry X3D defaults so a default-constructed
// instance is the identity transform.
// ---------------------------------------------------------------------------
struct TextureTransform2DParams {
    float centerS     = 0.0f;  // TextureTransform.center.x
    float centerT     = 0.0f;  // TextureTransform.center.y
    float rotation    = 0.0f;  // TextureTransform.rotation (radians, CCW in UV space)
    float scaleS      = 1.0f;  // TextureTransform.scale.x
    float scaleT      = 1.0f;  // TextureTransform.scale.y
    float translationS = 0.0f; // TextureTransform.translation.x
    float translationT = 0.0f; // TextureTransform.translation.y
};

// ---------------------------------------------------------------------------
// applyTextureTransform — apply one X3D TextureTransform to a single (s,t) pair.
//
// Implements §18.4.10 normative scalar expansion verbatim:
//   Step 1: apply translation
//   Step 2: shift to center pivot
//   Step 3: non-uniform scale
//   Step 4: 2D rotation CCW about the pivot (positive rotation → CCW in UV space
//           → texture appears to rotate CW on geometry surface)
//   Step 5: shift back from center pivot
//
// Returns: transformed (s', t') as std::array<float,2>.
//
// Zero-overhead inline: no heap, no branches beyond the trig intrinsics.
// ---------------------------------------------------------------------------
inline std::array<float, 2> applyTextureTransform(
    float s, float t,
    const TextureTransform2DParams& p) noexcept
{
    // Step 1: apply translation
    float s1 = s + p.translationS;
    float t1 = t + p.translationT;

    // Step 2: shift to center pivot
    float ds = s1 - p.centerS;
    float dt = t1 - p.centerT;

    // Step 3: non-uniform scale
    float ds2 = ds * p.scaleS;
    float dt2 = dt * p.scaleT;

    // Step 4: 2D rotation (CCW for positive angle per §18.4.10)
    float cs = std::cos(p.rotation);
    float ss = std::sin(p.rotation);
    float ds3 = ds2 * cs - dt2 * ss;
    float dt3 = ds2 * ss + dt2 * cs;

    // Step 5: shift back from center pivot
    return { ds3 + p.centerS, dt3 + p.centerT };
}

// ---------------------------------------------------------------------------
// applyTextureTransformIdentity — returns true iff p is the identity transform
// (all default values). Cheap early-exit for extraction loops.
// ---------------------------------------------------------------------------
inline bool isIdentityTextureTransform(
    const TextureTransform2DParams& p) noexcept
{
    return p.centerS     == 0.0f && p.centerT     == 0.0f
        && p.rotation    == 0.0f
        && p.scaleS      == 1.0f && p.scaleT      == 1.0f
        && p.translationS == 0.0f && p.translationT == 0.0f;
}

// ---------------------------------------------------------------------------
// makeTextureTransform3x3 — produce the 2D homogeneous matrix (row-major 3×3)
// equivalent to applyTextureTransform, for shader upload.
//
// Column-vector convention:
//   [s']   [M] [s]
//   [t'] = [M] [t]
//   [1 ]       [1]
//
// where the 3×3 entries are stored row-major in the returned array:
//   out[0..2] = row 0,  out[3..5] = row 1,  out[6..8] = row 2.
//
// Derivation: Tc' = Tc_back · R · Sc · Tc_to · Tl · Tc
//   where Tl = translate(translation), Tc_to = translate(-center),
//   Sc = scale(scale), R = rotate(rotation), Tc_back = translate(center).
//
// The combined column-vector form (using X3D ordering):
//   [cs*sx    -ss*sy    cx - cs*(cx+tx) + ss*(cy+ty)]
//   [ss*sx     cs*sy    cy - ss*(cx+tx) - cs*(cy+ty)]
//   [0         0        1                            ]
//
// (cx,cy) = center, (tx,ty) = translation, (sx,sy) = scale, angle = rotation.
// ---------------------------------------------------------------------------
inline std::array<float, 9> makeTextureTransform3x3(
    const TextureTransform2DParams& p) noexcept
{
    float cs = std::cos(p.rotation);
    float ss = std::sin(p.rotation);

    // Combined pivot+translate shift applied in step 1+2:
    //   shifted_center = center + translation
    float shifted_s = p.centerS + p.translationS;
    float shifted_t = p.centerT + p.translationT;

    // M = R(rot) * Sc(scale), then the full affine offset.
    float m00 = cs * p.scaleS;
    float m01 = -ss * p.scaleT;
    float m10 = ss * p.scaleS;
    float m11 = cs * p.scaleT;

    // Translation column: center - M * (center + translation)
    float tx = p.centerS - (m00 * shifted_s + m01 * shifted_t);
    float ty = p.centerT - (m10 * shifted_s + m11 * shifted_t);

    // Row-major 3×3:
    return {
        m00, m01, tx,
        m10, m11, ty,
        0.0f, 0.0f, 1.0f
    };
}

// ---------------------------------------------------------------------------
// texelCoordRepeated / texelCoordClamped — §18.2.3 legacy wrap modes.
//
// These implement the X3D normative texel-location formulas that a consumer
// applies when repeatS/repeatT is known (or resolved from SamplerParams) and
// the renderer wants to compute the texel address in a software path or shader
// preamble. GPU-side consumers will typically translate repeatS/T to GL
// wrap states instead, but the formulas are surfaced here for correctness tests
// and CPU-side bakes.
//
// N = texture dimension (width or height) in texels.
// ---------------------------------------------------------------------------

/// §18.2.3 — repeatS/T TRUE: fractional part → [0, N)
inline float texelCoordRepeated(float C, int N) noexcept {
    return (C - std::floor(C)) * static_cast<float>(N);
}

/// §18.2.3 — repeatS/T FALSE (clamp): hard clamp to [0, N]
inline float texelCoordClamped(float C, int N) noexcept {
    if (C > 1.0f) return static_cast<float>(N);
    if (C < 0.0f) return 0.0f;
    return C * static_cast<float>(N);
}

// ---------------------------------------------------------------------------
// BoundaryMode / MagFilter / MinFilter / SamplerParams — §18.4.9
//
// Extended SamplerParams covers both legacy repeatS/T and full TextureProperties
// boundary/filter modes. When textureProperties is non-null on the source texture
// node, boundaryModeS/T govern (repeatS/T on the texture node are ignored per
// §18.4.9). When textureProperties is null, derive:
//   repeatS==TRUE  → BoundaryMode::Repeat
//   repeatS==FALSE → BoundaryMode::ClampToEdge  (spec-compatible default)
// ---------------------------------------------------------------------------

enum class BoundaryMode {
    Repeat,           // "REPEAT"            → GL_REPEAT
    Clamp,            // "CLAMP"             → GL_CLAMP
    ClampToEdge,      // "CLAMP_TO_EDGE"     → GL_CLAMP_TO_EDGE
    ClampToBoundary,  // "CLAMP_TO_BOUNDARY" → GL_CLAMP_TO_BORDER
    MirroredRepeat    // "MIRRORED_REPEAT"   → GL_MIRRORED_REPEAT
};

enum class MagFilter {
    Default,
    AvgPixel,      // "AVG_PIXEL"      → bilinear
    NearestPixel,  // "NEAREST_PIXEL"  → nearest
    Fastest,
    Nicest
};

enum class MinFilter {
    Default,
    AvgPixel,                  // "AVG_PIXEL"
    AvgPixelAvgMipmap,         // "AVG_PIXEL_AVG_MIPMAP"        → trilinear
    AvgPixelNearestMipmap,     // "AVG_PIXEL_NEAREST_MIPMAP"
    NearestPixel,              // "NEAREST_PIXEL"
    NearestPixelAvgMipmap,     // "NEAREST_PIXEL_AVG_MIPMAP"
    NearestPixelNearestMipmap, // "NEAREST_PIXEL_NEAREST_MIPMAP"
    Fastest,
    Nicest
};

// Extended sampler descriptor (replaces the minimal SamplerParams in RenderItem.hpp
// for consumers that read TextureProperties). Carry BOTH the legacy bools and the
// extended boundary enums so existing code that only touches repeatS/T is unaffected.
struct ExtendedSamplerParams {
    bool            repeatS              = true;  // legacy; overridden by TextureProperties
    bool            repeatT              = true;  // legacy; overridden by TextureProperties
    bool            repeatR              = true;  // 3D textures (§33); legacy R-axis wrap
    BoundaryMode    boundaryModeS        = BoundaryMode::Repeat;
    BoundaryMode    boundaryModeT        = BoundaryMode::Repeat;
    BoundaryMode    boundaryModeR        = BoundaryMode::Repeat;
    MagFilter       magnificationFilter  = MagFilter::Default;
    MinFilter       minificationFilter   = MinFilter::Default;
    bool            generateMipmaps      = false;
    float           anisotropicDegree    = 1.0f; // [1, ∞); 1 = isotropic
};

// Derive BoundaryMode from the legacy repeatS/repeatT bool (§18.2.3).
inline BoundaryMode boundaryModeFromRepeat(bool repeat) noexcept {
    return repeat ? BoundaryMode::Repeat : BoundaryMode::ClampToEdge;
}

// ---------------------------------------------------------------------------
// TexCoordGenMode / TexCoordGenDesc — §18.4.8 TextureCoordinateGenerator
//
// When a geometry node's texCoord field is a TextureCoordinateGenerator rather
// than a TextureCoordinate, the renderer computes UVs per-vertex using view state.
// Surface this descriptor on TextureRef (alongside or instead of explicit texcoords)
// so the renderer can dispatch to the correct GPU-side generation path.
// ---------------------------------------------------------------------------
enum class TexCoordGenMode {
    Sphere,                     // u=Nx/2+0.5, v=Ny/2+0.5 (camera-space normal)
    CameraSpaceNormal,          // (Nx,Ny,Nz) camera-space normal → [−1,1]
    CameraSpacePosition,        // (Px,Py,Pz) camera-space position
    CameraSpaceReflectionVector,// R = 2·dot(E,N)·N − E; output [−1,1]
    SphereLocal,                // SPHERE in local coordinates
    Coord,                      // raw vertex coordinates
    CoordEye,                   // vertex coords in camera space
    Noise,                      // Perlin solid noise; parameter=[sx,sy,sz,tx,ty,tz]
    NoiseEye,                   // same but camera-space vertex coords
    SphereReflect,              // reflection-based sphere map; parameter[0]=IOR
    SphereReflectLocal          // as above; parameter[1..3]=eye point in local coords
};

struct TexCoordGenDesc {
    TexCoordGenMode    mode      = TexCoordGenMode::Sphere;
    std::vector<float> parameter; // mode-dependent; see §18.4.8 Table 18.6
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_TEXTURETRANSFORM2D_HPP
