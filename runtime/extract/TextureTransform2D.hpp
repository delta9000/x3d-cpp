// TextureTransform2D.hpp — pure (s,t) TextureTransform math, §18.4.10.
//
// Normative reference: ISO/IEC 19775-1:2023 §18.4.10 TextureTransform
//   Tc' = C × S × R × C⁻¹ × T × Tc
// (C = translate-to-center, C⁻¹ its inverse.) Applied right-to-left, the scalar
// order is the spec's "(in order)" list — translation, rotation, scaling — with
// R and S both about the pivot:
//   1. translate  (apply translation field)
//   2. pivot to center            (C⁻¹, −center)
//   3. rotate CCW by rotation (radians)   (R)
//   4. scale (non-uniform)                (S)
//   5. pivot back from center      (C, +center)
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
    bool hasMatrix = false;
    std::array<float, 9> matrix{
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f};
};

// ---------------------------------------------------------------------------
// applyTextureTransform — apply one X3D TextureTransform to a single (s,t) pair.
//
// Implements §18.4.10's normative matrix  Tc' = C · S · R · C⁻¹ · T · Tc, whose
// right-to-left application order is exactly the spec's "(in order)" list —
// translation, then rotation, then scaling — with rotation and scaling both
// about `center` (C = translate-to-center, C⁻¹ = translate-from-center):
//   Step 1: apply translation                      (T)
//   Step 2: translate the pivot to the origin      (C⁻¹, −center)
//   Step 3: 2D rotation CCW about the pivot        (R)
//   Step 4: non-uniform scale about the pivot      (S)
//   Step 5: translate the pivot back               (C, +center)
//
// §18.4.10's prose "rotation ... after the scaling operation has been applied"
// contradicts this order; the numbered list + the normative matrix (rotate
// BEFORE scale) are followed here — see finding TXF-1.
//
// Returns: transformed (s', t') as std::array<float,2>.
//
// Zero-overhead inline: no heap, no branches beyond the trig intrinsics.
// ---------------------------------------------------------------------------
inline std::array<float, 2> applyTextureTransform(
    float s, float t,
    const TextureTransform2DParams& p) noexcept
{
    if (p.hasMatrix) {
        const float w = p.matrix[6] * s + p.matrix[7] * t + p.matrix[8];
        const float invW = (w != 0.0f) ? (1.0f / w) : 1.0f;
        return {
            (p.matrix[0] * s + p.matrix[1] * t + p.matrix[2]) * invW,
            (p.matrix[3] * s + p.matrix[4] * t + p.matrix[5]) * invW};
    }

    // Step 1: apply translation.
    float s1 = s + p.translationS;
    float t1 = t + p.translationT;

    // Step 2: translate the pivot to the origin (subtract center).
    float ds = s1 - p.centerS;
    float dt = t1 - p.centerT;

    // Step 3: 2D rotation CCW about the pivot (positive rotation → CCW in UV
    // space → texture appears to rotate CW on the geometry surface).
    const float cs = std::cos(p.rotation);
    const float ss = std::sin(p.rotation);
    const float ds2 = ds * cs - dt * ss;
    const float dt2 = ds * ss + dt * cs;

    // Step 4: non-uniform scale about the pivot.
    const float ds3 = ds2 * p.scaleS;
    const float dt3 = dt2 * p.scaleT;

    // Step 5: translate the pivot back (add center).
    return {ds3 + p.centerS, dt3 + p.centerT};
}

// ---------------------------------------------------------------------------
// applyTextureTransformIdentity — returns true iff p is the identity transform
// (all default values). Cheap early-exit for extraction loops.
// ---------------------------------------------------------------------------
inline bool isIdentityTextureTransform(
    const TextureTransform2DParams& p) noexcept
{
    return !p.hasMatrix
        && p.centerS     == 0.0f && p.centerT     == 0.0f
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
// Derivation: Tc' = C · S · R · C⁻¹ · T · Tc
//   where C = translate(center), C⁻¹ = translate(-center),
//   T = translate(translation), R = rotate(rotation), S = scale(scale).
//   Applied right-to-left the order is T, C⁻¹, R, S, C (translate, pivot to the
//   origin, rotate, scale, un-pivot) — §18.4.10's "(in order)" list.
//
// With M = S · R the combined column-vector form is:
//   [sx*cs    -sx*ss    M·(c-t) + c ]
//   [sy*ss     sy*cs    M·(c-t) + c ]
//   [0         0        1           ]
//
// (cx,cy) = center, (tx,ty) = translation, (sx,sy) = scale, angle = rotation.
// ---------------------------------------------------------------------------
inline std::array<float, 9> makeTextureTransform3x3(
    const TextureTransform2DParams& p) noexcept
{
    if (p.hasMatrix) return p.matrix;

    float cs = std::cos(p.rotation);
    float ss = std::sin(p.rotation);

    // M = S(scale) * R(rot) — scale AFTER rotate (right-to-left of S·R).
    float m00 = p.scaleS * cs;
    float m01 = -p.scaleS * ss;
    float m10 = p.scaleT * ss;
    float m11 = p.scaleT * cs;

    // Offset = M * (translation - center) + center.
    float shifted_s = p.translationS - p.centerS;
    float shifted_t = p.translationT - p.centerT;
    float tx = (m00 * shifted_s + m01 * shifted_t) + p.centerS;
    float ty = (m10 * shifted_s + m11 * shifted_t) + p.centerT;

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
    SFColorRGBA     borderColor          {0, 0, 0, 0};
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
