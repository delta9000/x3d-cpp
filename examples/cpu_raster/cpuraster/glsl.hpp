// glsl.hpp — a GLSL-emulation math layer for the headless CPU rasterizer.
//
// This is the "GLSL emulation" substrate: C++ vector/matrix types and free
// functions whose names and semantics mirror GLSL (vec3, dot, normalize, mix,
// clamp, pow, reflect, texture(), …). It lets the fixed-function material
// shaders (MaterialShader.hpp) be written as near-verbatim CPU ports of the
// PoC's lit.frag / pbr.frag / unlit.frag GLSL, and it is the value runtime the
// author-shader interpreter (GlslInterpreter.hpp) evaluates against.
//
// This file is OUT-OF-SDK consumer code (examples/cpu_raster/) — it never lands
// in the x3d_cpp SDK. It depends ONLY on the SDK's POD value types (SFVec*,
// SFColor*) and Mat4 across the extraction seam, plus <cmath>.
//
// CONVENTIONS (match the SDK + the PoC GLSL):
//   * mat4 is COLUMN-MAJOR, element (row r, col c) = m[c*4+r] — identical to
//     x3d::runtime::Mat4 so a Mat4 converts in with no transpose.
//   * A column vector p transforms as out = M * p (right-to-left), GL style.
//   * texture() origin is bottom-left (X3D LOCAL == GL); NO implicit V-flip.
#ifndef X3D_CPURASTER_GLSL_HPP
#define X3D_CPURASTER_GLSL_HPP

#include "Mat4.hpp"     // x3d::runtime::Mat4 (column-major, GL-native)
#include "X3Dtypes.hpp" // SFVec2f/3f/4f, SFColor, SFColorRGBA

#include <algorithm>
#include <array>
#include <cmath>

namespace x3d::cpuraster::glsl {

// ---------------------------------------------------------------------------
// Scalar helpers (GLSL-named so ports read 1:1). These also overload for the
// vector types below via component-wise application.
// ---------------------------------------------------------------------------
inline float clampf(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}
inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
inline float stepf(float edge, float x) { return x < edge ? 0.0f : 1.0f; }
inline float fractf(float x) { return x - std::floor(x); }
inline float signf(float x) { return (x > 0.0f) - (x < 0.0f); }

// ---------------------------------------------------------------------------
// vec2 / vec3 / vec4 — POD-ish value types with GLSL component access (.x/.y/.z
// /.w and .r/.g/.b/.a aliases via accessor methods) and the arithmetic GLSL
// uses. Kept deliberately small: only what the shaders + interpreter need.
// ---------------------------------------------------------------------------
struct vec2 {
  float x = 0.0f, y = 0.0f;
  vec2() = default;
  vec2(float s) : x(s), y(s) {}
  vec2(float x_, float y_) : x(x_), y(y_) {}
  vec2(const SFVec2f &v) : x(v.x), y(v.y) {}
  float &operator[](int i) { return i == 0 ? x : y; }
  float operator[](int i) const { return i == 0 ? x : y; }
  // .r/.g aliases (GLSL color swizzle on a vec2).
  float r() const { return x; }
  float g() const { return y; }
};

struct vec3 {
  float x = 0.0f, y = 0.0f, z = 0.0f;
  vec3() = default;
  vec3(float s) : x(s), y(s), z(s) {}
  vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  vec3(const vec2 &v, float z_) : x(v.x), y(v.y), z(z_) {}
  vec3(const SFVec3f &v) : x(v.x), y(v.y), z(v.z) {}
  vec3(const SFColor &c) : x(c.r), y(c.g), z(c.b) {}
  float &operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }
  float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
  vec2 xy() const { return {x, y}; }
  // color aliases.
  float r() const { return x; }
  float g() const { return y; }
  float b() const { return z; }
  SFVec3f toSF() const { return {x, y, z}; }
  SFColor toColor() const { return {x, y, z}; }
};

struct vec4 {
  float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
  vec4() = default;
  vec4(float s) : x(s), y(s), z(s), w(s) {}
  vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
  vec4(const vec3 &v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
  vec4(const vec2 &a, const vec2 &b) : x(a.x), y(a.y), z(b.x), w(b.y) {}
  vec4(const SFVec4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
  vec4(const SFColorRGBA &c) : x(c.r), y(c.g), z(c.b), w(c.a) {}
  float &operator[](int i) {
    return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }
  float operator[](int i) const {
    return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
  }
  vec3 xyz() const { return {x, y, z}; }
  vec2 xy() const { return {x, y}; }
  float r() const { return x; }
  float g() const { return y; }
  float b() const { return z; }
  float a() const { return w; }
  SFColorRGBA toRGBA() const { return {x, y, z, w}; }
};

// ---- vec2 arithmetic -------------------------------------------------------
inline vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline vec2 operator*(vec2 a, vec2 b) { return {a.x * b.x, a.y * b.y}; }
inline vec2 operator*(vec2 a, float s) { return {a.x * s, a.y * s}; }
inline vec2 operator*(float s, vec2 a) { return {a.x * s, a.y * s}; }
inline vec2 operator/(vec2 a, float s) { return {a.x / s, a.y / s}; }
inline vec2 operator-(vec2 a) { return {-a.x, -a.y}; }

// ---- vec3 arithmetic -------------------------------------------------------
inline vec3 operator+(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline vec3 operator-(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline vec3 operator*(vec3 a, vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
inline vec3 operator*(vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline vec3 operator*(float s, vec3 a) { return {a.x * s, a.y * s, a.z * s}; }
inline vec3 operator/(vec3 a, vec3 b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }
inline vec3 operator/(vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline vec3 operator-(vec3 a) { return {-a.x, -a.y, -a.z}; }
inline vec3 &operator+=(vec3 &a, vec3 b) { a = a + b; return a; }
inline vec3 &operator*=(vec3 &a, vec3 b) { a = a * b; return a; }
inline vec3 &operator*=(vec3 &a, float s) { a = a * s; return a; }

// ---- vec4 arithmetic -------------------------------------------------------
inline vec4 operator+(vec4 a, vec4 b) { return {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}; }
inline vec4 operator-(vec4 a, vec4 b) { return {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w}; }
inline vec4 operator*(vec4 a, vec4 b) { return {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; }
inline vec4 operator*(vec4 a, float s) { return {a.x*s, a.y*s, a.z*s, a.w*s}; }
inline vec4 operator*(float s, vec4 a) { return {a.x*s, a.y*s, a.z*s, a.w*s}; }
inline vec4 &operator*=(vec4 &a, vec4 b) { a = a * b; return a; }

// ---------------------------------------------------------------------------
// GLSL builtins — scalar + vector overloads.
// ---------------------------------------------------------------------------
inline float dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
inline float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float dot(vec4 a, vec4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

inline vec3 cross(vec3 a, vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(vec2 v) { return std::sqrt(dot(v, v)); }
inline float length(vec3 v) { return std::sqrt(dot(v, v)); }

inline vec2 normalize(vec2 v) {
  float L = length(v);
  return L > 1e-12f ? v / L : vec2{0, 0};
}
inline vec3 normalize(vec3 v) {
  float L = length(v);
  return L > 1e-12f ? v / L : vec3{0, 0, 0};
}

// reflect(I, N) = I - 2*dot(N,I)*N (GLSL).
inline vec3 reflect(vec3 I, vec3 N) { return I - 2.0f * dot(N, I) * N; }

// clamp.
inline float clamp(float x, float lo, float hi) { return clampf(x, lo, hi); }
inline vec3 clamp(vec3 v, float lo, float hi) {
  return {clampf(v.x, lo, hi), clampf(v.y, lo, hi), clampf(v.z, lo, hi)};
}
inline vec4 clamp(vec4 v, float lo, float hi) {
  return {clampf(v.x, lo, hi), clampf(v.y, lo, hi), clampf(v.z, lo, hi),
          clampf(v.w, lo, hi)};
}

// mix.
inline float mix(float a, float b, float t) { return mixf(a, b, t); }
inline vec3 mix(vec3 a, vec3 b, float t) {
  return {mixf(a.x, b.x, t), mixf(a.y, b.y, t), mixf(a.z, b.z, t)};
}
inline vec3 mix(vec3 a, vec3 b, vec3 t) {
  return {mixf(a.x, b.x, t.x), mixf(a.y, b.y, t.y), mixf(a.z, b.z, t.z)};
}

// max/min (scalar + vector-vs-scalar, the common GLSL forms).
inline float maxf(float a, float b) { return a > b ? a : b; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline vec3 maxv(vec3 v, float s) { return {maxf(v.x, s), maxf(v.y, s), maxf(v.z, s)}; }

// pow (component-wise vec3 — used by linearToSRGB).
inline float pow_(float x, float y) { return std::pow(x, y); }
inline vec3 pow_(vec3 v, float y) {
  return {std::pow(v.x, y), std::pow(v.y, y), std::pow(v.z, y)};
}
inline vec3 pow_(vec3 v, vec3 y) {
  return {std::pow(v.x, y.x), std::pow(v.y, y.y), std::pow(v.z, y.z)};
}

inline vec3 absv(vec3 v) { return {std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)}; }

// ---------------------------------------------------------------------------
// mat3 / mat4 — column-major, GL-native. mat3 multiplies a vec3 (M*v); mat4
// multiplies a vec4. from(Mat4) imports the SDK matrix with no transpose.
// ---------------------------------------------------------------------------
struct mat3 {
  std::array<float, 9> m{}; // column-major: (r,c) = m[c*3+r]
  mat3() = default;
  static mat3 identity() {
    mat3 r;
    r.m[0] = r.m[4] = r.m[8] = 1.0f;
    return r;
  }
  // columns as vec3.
  vec3 col(int c) const { return {m[c * 3 + 0], m[c * 3 + 1], m[c * 3 + 2]}; }
};

inline vec3 operator*(const mat3 &M, vec3 v) {
  return {M.m[0]*v.x + M.m[3]*v.y + M.m[6]*v.z,
          M.m[1]*v.x + M.m[4]*v.y + M.m[7]*v.z,
          M.m[2]*v.x + M.m[5]*v.y + M.m[8]*v.z};
}

struct mat4 {
  std::array<float, 16> m{}; // column-major: (r,c) = m[c*4+r]
  mat4() = default;
  explicit mat4(const x3d::runtime::Mat4 &M) : m(M.m) {}
  static mat4 identity() {
    mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }
  mat4 operator*(const mat4 &b) const {
    mat4 c;
    for (int col = 0; col < 4; ++col)
      for (int row = 0; row < 4; ++row) {
        float s = 0.0f;
        for (int k = 0; k < 4; ++k) s += m[k * 4 + row] * b.m[col * 4 + k];
        c.m[col * 4 + row] = s;
      }
    return c;
  }
};

inline vec4 operator*(const mat4 &M, vec4 v) {
  return {M.m[0]*v.x + M.m[4]*v.y + M.m[8]*v.z  + M.m[12]*v.w,
          M.m[1]*v.x + M.m[5]*v.y + M.m[9]*v.z  + M.m[13]*v.w,
          M.m[2]*v.x + M.m[6]*v.y + M.m[10]*v.z + M.m[14]*v.w,
          M.m[3]*v.x + M.m[7]*v.y + M.m[11]*v.z + M.m[15]*v.w};
}

// The eye-space normal matrix = transpose(inverse(upper-left 3x3 of view*model)),
// returned column-major. Identical math to the PoC's normalMatrix3() so shading
// normals match the GL renderer under non-uniform scale.
inline mat3 normalMatrix(const x3d::runtime::Mat4 &viewModel) {
  const x3d::runtime::Mat4 inv = viewModel.inverse();
  mat3 out;
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r)
      out.m[c * 3 + r] = inv.m[r * 4 + c]; // transpose of inv's UL 3x3.
  return out;
}

// ---------------------------------------------------------------------------
// linearToSRGB — the exact piecewise encode the PoC shaders use at output.
// ---------------------------------------------------------------------------
inline vec3 linearToSRGB(vec3 lin) {
  auto enc = [](float c) {
    return c < 0.0031308f ? c * 12.92f
                          : 1.055f * std::pow(clampf(c, 0.0f, 1.0f), 1.0f / 2.4f) -
                                0.055f;
  };
  return {enc(lin.x), enc(lin.y), enc(lin.z)};
}

} // namespace x3d::cpuraster::glsl

#endif // X3D_CPURASTER_GLSL_HPP
