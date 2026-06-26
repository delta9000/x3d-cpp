// Rasterizer.hpp — the headless CPU triangle rasterizer.
//
// Pipeline (mirrors a GL forward renderer, so output matches the PoC):
//   1. Vertex stage: localPos -> eye space (view*model) + clip space
//      (proj*view*model); normals -> eye space via the inverse-transpose mat3.
//   2. Near-plane clip in CLIP space against z >= -w (GL clip-z in [-1,1]),
//      splitting a straddling triangle so nothing divides by w<=0.
//   3. Perspective divide -> NDC -> screen (origin bottom-left = GL).
//   4. Back-face cull from screen-space winding honoring MeshData.ccw/solid.
//   5. Scan-convert in 2x2 QUADS so screen-space derivatives dFdx/dFdy are
//      available to the fragment shader (needed for the normal-map TBN) — the
//      same "helper invocation" model a real GPU uses. Perspective-correct
//      barycentric interpolation of every varying; z-buffer test.
//
// The fragment shader is any callable (FragmentInput)->optional color; returning
// `false` is a GLSL `discard`. This is what MaterialShader.hpp (the Phong/PBR/
// Unlit ports) and GlslInterpreter.hpp (author ComposedShader) both plug into.
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_RASTERIZER_HPP
#define X3D_CPURASTER_RASTERIZER_HPP

#include "Framebuffer.hpp"
#include "glsl.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace x3d::cpuraster {

// Per-fragment inputs handed to a fragment shader — the eye-space varyings the
// PoC GLSL `in` block carries, plus screen-space derivatives and gl_FrontFacing.
struct FragmentInput {
  glsl::vec3 posEye{0, 0, 0};   // vPosEye
  glsl::vec3 normalEye{0, 0, 1}; // vNormalEye (not yet normalized, like GLSL)
  glsl::vec4 color{1, 1, 1, 1};  // vColor (per-vertex Color)
  glsl::vec2 texcoord{0, 0};     // vTexCoord
  glsl::vec3 dPosEyeDx{0, 0, 0}, dPosEyeDy{0, 0, 0}; // dFdx/dFdy(vPosEye)
  glsl::vec2 dTexDx{0, 0}, dTexDy{0, 0};             // dFdx/dFdy(vTexCoord)
  bool frontFacing = true;       // gl_FrontFacing
};

// Returns false to DISCARD the fragment; otherwise writes `out` (final color,
// already gamma-encoded by the shader as in GL).
using FragmentShader = std::function<bool(const FragmentInput &, glsl::vec4 &)>;

// One vertex of input geometry, in the geometry's LOCAL frame (MeshData order).
struct Vertex {
  glsl::vec3 pos{0, 0, 0};
  glsl::vec3 normal{0, 1, 0};
  glsl::vec4 color{1, 1, 1, 1};
  glsl::vec2 texcoord{0, 0};
};

enum class BlendMode { Opaque, Blend };

class Rasterizer {
public:
  explicit Rasterizer(Framebuffer &fb) : fb_(fb) {}

  // Draw an indexed triangle list. `verts` are LOCAL-frame; matrices place them.
  // ccw/solid come from MeshData; blend selects the opaque (depth-write) or the
  // transparency (source-over, no depth-write) path.
  void drawTriangles(const std::vector<Vertex> &verts,
                     const std::vector<std::uint32_t> &indices,
                     const glsl::mat4 &model, const glsl::mat4 &view,
                     const glsl::mat4 &proj, const glsl::mat3 &normalMat,
                     bool ccw, bool solid, BlendMode blend,
                     const FragmentShader &fs) {
    const glsl::mat4 mv = view * model;
    const glsl::mat4 mvp = proj * mv;
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
      std::array<ClipVertex, 3> tri;
      for (int k = 0; k < 3; ++k) {
        const Vertex &v = verts[indices[i + k]];
        ClipVertex cv;
        cv.clip = mvp * glsl::vec4(v.pos, 1.0f);
        cv.posEye = (mv * glsl::vec4(v.pos, 1.0f)).xyz();
        cv.normalEye = normalMat * v.normal;
        cv.color = v.color;
        cv.texcoord = v.texcoord;
        tri[k] = cv;
      }
      // Near-plane clip -> 0,1, or 2 triangles (fan).
      std::vector<ClipVertex> poly = clipNear({tri[0], tri[1], tri[2]});
      for (std::size_t t = 0; t + 2 < poly.size(); ++t)
        rasterTriangle(poly[0], poly[t + 1], poly[t + 2], ccw, solid, blend, fs);
    }
  }

  // Unlit constant-color LINES (GL_LINES: index pairs) — the B4 topology path.
  void drawLines(const std::vector<Vertex> &verts,
                 const std::vector<std::uint32_t> &indices,
                 const glsl::mat4 &model, const glsl::mat4 &view,
                 const glsl::mat4 &proj, glsl::vec4 baseColor, bool hasColors) {
    const glsl::mat4 mvp = proj * view * model;
    for (std::size_t i = 0; i + 1 < indices.size(); i += 2) {
      const Vertex &a = verts[indices[i]];
      const Vertex &b = verts[indices[i + 1]];
      glsl::vec4 ca = mvp * glsl::vec4(a.pos, 1.0f);
      glsl::vec4 cb = mvp * glsl::vec4(b.pos, 1.0f);
      if (ca.w <= 1e-6f || cb.w <= 1e-6f) continue; // skip behind-near segments.
      drawLineNDC(ca, cb, hasColors ? a.color : baseColor,
                  hasColors ? b.color : baseColor);
    }
  }

  // Unlit POINTS (GL_POINTS: one vertex each) — a 1px splat with depth test.
  void drawPoints(const std::vector<Vertex> &verts,
                  const std::vector<std::uint32_t> &indices,
                  const glsl::mat4 &model, const glsl::mat4 &view,
                  const glsl::mat4 &proj, glsl::vec4 baseColor, bool hasColors) {
    const glsl::mat4 mvp = proj * view * model;
    for (std::uint32_t idx : indices) {
      const Vertex &v = verts[idx];
      glsl::vec4 c = mvp * glsl::vec4(v.pos, 1.0f);
      if (c.w <= 1e-6f) continue;
      float sx, sy, sz;
      toScreen(c, sx, sy, sz);
      int px = static_cast<int>(sx), py = static_cast<int>(sy);
      if (px < 0 || px >= fb_.width() || py < 0 || py >= fb_.height()) continue;
      if (sz < fb_.depth(px, py)) {
        fb_.setDepth(px, py, sz);
        glsl::vec4 col = hasColors ? v.color : baseColor;
        fb_.setColor(px, py, col);
      }
    }
  }

private:
  struct ClipVertex {
    glsl::vec4 clip{0, 0, 0, 1};
    glsl::vec3 posEye{0, 0, 0};
    glsl::vec3 normalEye{0, 0, 1};
    glsl::vec4 color{1, 1, 1, 1};
    glsl::vec2 texcoord{0, 0};
  };

  static ClipVertex lerpCV(const ClipVertex &a, const ClipVertex &b, float t) {
    ClipVertex r;
    auto L4 = [t](glsl::vec4 x, glsl::vec4 y) { return x + (y - x) * t; };
    auto L3 = [t](glsl::vec3 x, glsl::vec3 y) { return x + (y - x) * t; };
    auto L2 = [t](glsl::vec2 x, glsl::vec2 y) { return x + (y - x) * t; };
    r.clip = L4(a.clip, b.clip);
    r.posEye = L3(a.posEye, b.posEye);
    r.normalEye = L3(a.normalEye, b.normalEye);
    r.color = L4(a.color, b.color);
    r.texcoord = L2(a.texcoord, b.texcoord);
    return r;
  }

  // Clip a triangle against the near plane (clip.z >= -clip.w). Sutherland-
  // Hodgman against one plane -> a convex polygon (3 or 4 verts), or empty.
  static std::vector<ClipVertex> clipNear(const std::array<ClipVertex, 3> &in) {
    std::vector<ClipVertex> out;
    auto dist = [](const ClipVertex &v) { return v.clip.z + v.clip.w; };
    for (int i = 0; i < 3; ++i) {
      const ClipVertex &cur = in[i];
      const ClipVertex &nxt = in[(i + 1) % 3];
      float dc = dist(cur), dn = dist(nxt);
      bool inCur = dc >= 0.0f, inNxt = dn >= 0.0f;
      if (inCur) out.push_back(cur);
      if (inCur != inNxt) {
        float t = dc / (dc - dn);
        out.push_back(lerpCV(cur, nxt, t));
      }
    }
    return out;
  }

  // clip -> screen. sx,sy in pixels (origin bottom-left); sz is NDC z in [-1,1].
  void toScreen(const glsl::vec4 &clip, float &sx, float &sy, float &sz) const {
    const float invw = 1.0f / clip.w;
    const float nx = clip.x * invw, ny = clip.y * invw, nz = clip.z * invw;
    sx = (nx * 0.5f + 0.5f) * fb_.width();
    sy = (ny * 0.5f + 0.5f) * fb_.height();
    sz = nz;
  }

  void rasterTriangle(const ClipVertex &a, const ClipVertex &b,
                      const ClipVertex &c, bool ccw, bool solid, BlendMode blend,
                      const FragmentShader &fs) {
    // Screen positions + per-vertex 1/w for perspective-correct interpolation.
    float x[3], y[3], z[3], invw[3];
    const ClipVertex *V[3] = {&a, &b, &c};
    for (int k = 0; k < 3; ++k) {
      toScreen(V[k]->clip, x[k], y[k], z[k]);
      invw[k] = 1.0f / V[k]->clip.w;
    }

    // Twice the signed screen area. >0 == CCW in our bottom-left screen frame.
    const float area2 = (x[1] - x[0]) * (y[2] - y[0]) -
                        (x[2] - x[0]) * (y[1] - y[0]);
    if (std::fabs(area2) < 1e-9f) return; // degenerate.

    // Front-facing: CCW screen winding is front when ccw==true (GL default).
    const bool frontIsPositive = ccw;
    const bool frontFacing = (area2 > 0.0f) == frontIsPositive;
    if (solid && !frontFacing) return; // back-face cull.

    const float invArea = 1.0f / area2;

    // Bounding box clamped to the viewport, snapped to even coords so 2x2 quads
    // align (helper invocations cover the same quad grid a GPU would use).
    int minX = std::max(0, (int)std::floor(std::min({x[0], x[1], x[2]})));
    int maxX = std::min(fb_.width() - 1, (int)std::ceil(std::max({x[0], x[1], x[2]})));
    int minY = std::max(0, (int)std::floor(std::min({y[0], y[1], y[2]})));
    int maxY = std::min(fb_.height() - 1, (int)std::ceil(std::max({y[0], y[1], y[2]})));
    if (minX > maxX || minY > maxY) return;
    minX &= ~1; minY &= ~1; // snap quad origin to even pixel.

    // Per-quad scratch for the 4 fragments.
    struct QuadPix {
      bool inTri = false, inView = false;
      float depth = 0.0f;
      FragmentInput frag;
    };

    for (int qy = minY; qy <= maxY; qy += 2) {
      for (int qx = minX; qx <= maxX; qx += 2) {
        std::array<QuadPix, 4> q; // (0,0)(1,0)(0,1)(1,1)
        for (int sub = 0; sub < 4; ++sub) {
          const int px = qx + (sub & 1);
          const int py = qy + (sub >> 1);
          QuadPix &p = q[sub];
          p.inView = (px < fb_.width() && py < fb_.height());
          const float fx = px + 0.5f, fy = py + 0.5f;
          // Barycentric via edge functions, normalized by the signed area.
          float b0 = edge(x[1], y[1], x[2], y[2], fx, fy) * invArea;
          float b1 = edge(x[2], y[2], x[0], y[0], fx, fy) * invArea;
          float b2 = edge(x[0], y[0], x[1], y[1], fx, fy) * invArea;
          p.inTri = (b0 >= 0.0f && b1 >= 0.0f && b2 >= 0.0f) ||
                    (b0 <= 0.0f && b1 <= 0.0f && b2 <= 0.0f);
          // Perspective-correct varyings (helper pixels extrapolate — fine for
          // derivatives). Linear depth in screen space.
          p.depth = b0 * z[0] + b1 * z[1] + b2 * z[2];
          const float iw = b0 * invw[0] + b1 * invw[1] + b2 * invw[2];
          const float rw = (std::fabs(iw) > 1e-12f) ? 1.0f / iw : 0.0f;
          auto pc3 = [&](glsl::vec3 A0, glsl::vec3 A1, glsl::vec3 A2) {
            return (A0 * (b0 * invw[0]) + A1 * (b1 * invw[1]) +
                    A2 * (b2 * invw[2])) * rw;
          };
          auto pc4 = [&](glsl::vec4 A0, glsl::vec4 A1, glsl::vec4 A2) {
            return (A0 * (b0 * invw[0]) + A1 * (b1 * invw[1]) +
                    A2 * (b2 * invw[2])) * rw;
          };
          auto pc2 = [&](glsl::vec2 A0, glsl::vec2 A1, glsl::vec2 A2) {
            return (A0 * (b0 * invw[0]) + A1 * (b1 * invw[1]) +
                    A2 * (b2 * invw[2])) * rw;
          };
          FragmentInput &f = p.frag;
          f.posEye = pc3(a.posEye, b.posEye, c.posEye);
          f.normalEye = pc3(a.normalEye, b.normalEye, c.normalEye);
          f.color = pc4(a.color, b.color, c.color);
          f.texcoord = pc2(a.texcoord, b.texcoord, c.texcoord);
          f.frontFacing = frontFacing;
        }

        // Coarse screen-space derivatives (one pair per quad, as GL coarse mode):
        // ddx = right - left (subpix 1 - 0); ddy = top - bottom (subpix 2 - 0).
        const glsl::vec3 dPosX = q[1].frag.posEye - q[0].frag.posEye;
        const glsl::vec3 dPosY = q[2].frag.posEye - q[0].frag.posEye;
        const glsl::vec2 dTexX = q[1].frag.texcoord - q[0].frag.texcoord;
        const glsl::vec2 dTexY = q[2].frag.texcoord - q[0].frag.texcoord;

        for (int sub = 0; sub < 4; ++sub) {
          QuadPix &p = q[sub];
          if (!p.inView || !p.inTri) continue;
          const int px = qx + (sub & 1);
          const int py = qy + (sub >> 1);
          // Depth test (smaller == nearer).
          if (!(p.depth < fb_.depth(px, py))) continue;
          p.frag.dPosEyeDx = dPosX; p.frag.dPosEyeDy = dPosY;
          p.frag.dTexDx = dTexX;   p.frag.dTexDy = dTexY;
          glsl::vec4 out;
          if (!fs(p.frag, out)) continue; // discard.
          if (blend == BlendMode::Opaque) {
            fb_.setDepth(px, py, p.depth);
            fb_.setColor(px, py, out);
          } else {
            // Transparency: test against opaque depth, do NOT write depth.
            fb_.blendColor(px, py, out);
          }
        }
      }
    }
  }

  static float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
  }

  // Depth-tested constant-color line in NDC->screen (Bresenham-ish DDA).
  void drawLineNDC(const glsl::vec4 &ca, const glsl::vec4 &cb, glsl::vec4 cola,
                   glsl::vec4 colb) {
    float ax, ay, az, bx, by, bz;
    toScreen(ca, ax, ay, az);
    toScreen(cb, bx, by, bz);
    const int steps =
        std::max(1, (int)std::ceil(std::max(std::fabs(bx - ax), std::fabs(by - ay))));
    for (int s = 0; s <= steps; ++s) {
      const float t = (float)s / steps;
      const int px = (int)std::lround(ax + (bx - ax) * t);
      const int py = (int)std::lround(ay + (by - ay) * t);
      const float pz = az + (bz - az) * t;
      if (px < 0 || px >= fb_.width() || py < 0 || py >= fb_.height()) continue;
      if (pz < fb_.depth(px, py)) {
        fb_.setDepth(px, py, pz);
        fb_.setColor(px, py, cola + (colb - cola) * t);
      }
    }
  }

  Framebuffer &fb_;
};

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_RASTERIZER_HPP
