// examples/x3d2svg/x3d2svg.hpp
//
// The reusable core of the x3d2svg example: pinhole projection, a view-all fit
// camera, flat-shaded facet collection, and SVG emission. Kept in a header (and
// out of main.cpp) so the projection math is unit-testable — see
// tests/project_test.cpp.
//
// Everything here consumes ONLY the public SDK façade (`x3d/sdk.hpp`), with the
// single documented exception of the `x3d::core::SFVec3f` value type, which the
// façade does not re-export but which `Mat4::transformPoint` requires.
#ifndef X3D2SVG_HPP
#define X3D2SVG_HPP

#include "x3d/sdk.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ostream>
#include <vector>

namespace x3d2svg {

namespace sdk = x3d::sdk;

struct Vec3 { double x = 0, y = 0, z = 0; };
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vec3 norm(Vec3 a) {
  double l = std::sqrt(dot(a, a));
  return l > 1e-9 ? Vec3{a.x / l, a.y / l, a.z / l} : a;
}

// Transform a world point through the SDK's own Mat4 so we match its
// column-major convention exactly. The SFVec3f value type is not re-exported
// into x3d::sdk, so this is the one place we reach into x3d::core.
inline Vec3 mul(const sdk::Mat4 &m, Vec3 p) {
  x3d::core::SFVec3f v{(float)p.x, (float)p.y, (float)p.z};
  auto o = m.transformPoint(v);
  return {o.x, o.y, o.z};
}

// A resolved camera: either the scene's bound Viewpoint (a view matrix), or a
// synthesized view-all fit basis. Both expose the same world→view mapping, with
// the camera looking down -Z in view space (the X3D/OpenGL convention).
struct Camera {
  bool useMatrix = false;
  sdk::Mat4 view;                              // when useMatrix
  Vec3 eye, right, up, back;                   // when !useMatrix (fit)
  double fov = 0.7854;                         // min-dimension FOV, radians

  Vec3 toView(Vec3 world) const {
    if (useMatrix) return mul(view, world);
    Vec3 d = sub(world, eye);
    return {dot(d, right), dot(d, up), dot(d, back)};
  }
};

// Build a view-all camera framing `b` from a gentle 3/4 angle — the same
// fallback the cpu_raster reference uses when no Viewpoint is bound.
inline Camera fitCamera(const sdk::Aabb &b, double fov) {
  Camera c;
  c.fov = fov;
  Vec3 center{(b.min.x + b.max.x) / 2.0, (b.min.y + b.max.y) / 2.0,
              (b.min.z + b.max.z) / 2.0};
  auto sz = b.size();
  double radius = 0.5 * std::sqrt((double)sz.x * sz.x + (double)sz.y * sz.y +
                                  (double)sz.z * sz.z);
  if (radius < 1e-6) radius = 1.0;
  double dist = radius / std::sin(std::max(0.1, fov) / 2.0) * 1.15;
  Vec3 dir = norm({0.35, 0.28, 1.0});
  c.eye = {center.x + dir.x * dist, center.y + dir.y * dist,
           center.z + dir.z * dist};
  c.back = norm(sub(c.eye, center));           // camera +Z, toward the eye
  c.right = norm(cross({0, 1, 0}, c.back));
  c.up = cross(c.back, c.right);
  return c;
}

struct Facet {
  double sx[3], sy[3];
  double depth;                                // mean view-space -z (painter's)
  double shade;                                // 0..1 headlight
  unsigned char r, g, b;
  double alpha;
};

// Project + flat-shade every triangle of the current tick through `cam`. Sorted
// far-to-near so a plain painter's paint order resolves occlusion.
inline std::vector<Facet> collect(sdk::SceneExtractor &ex, const Camera &cam,
                                  int W, int H) {
  const double fpix = (std::min(W, H) / 2.0) / std::tan(cam.fov / 2.0);
  const double nearP = 0.02;
  std::vector<Facet> facets;
  for (sdk::RenderItemId id : ex.fullSnapshot().added) {
    const auto &it = ex.item(id);
    const auto &mesh = *it.mesh;               // never null by contract
    if (mesh.indices.size() < 3) continue;
    const auto rgba = it.material.toRGBA();
    for (size_t k = 0; k + 2 < mesh.indices.size(); k += 3) {
      Vec3 v[3];
      bool ok = true;
      for (int j = 0; j < 3; ++j) {
        const auto &lp = mesh.positions[mesh.indices[k + j]];
        v[j] = cam.toView(mul(it.worldTransform, {lp.x, lp.y, lp.z}));
        if (v[j].z > -nearP) ok = false;       // crude near-plane cull (no clip)
      }
      if (!ok) continue;
      Facet fc;
      for (int j = 0; j < 3; ++j) {
        const double d = -v[j].z;
        fc.sx[j] = W / 2.0 + fpix * v[j].x / d;
        fc.sy[j] = H / 2.0 - fpix * v[j].y / d;
      }
      Vec3 n = cross(sub(v[1], v[0]), sub(v[2], v[0]));
      double nl = std::sqrt(dot(n, n));
      double ndotv = nl > 1e-9 ? std::fabs(n.z / nl) : 0.0;
      fc.shade = 0.25 + 0.75 * ndotv;
      fc.depth = (-v[0].z - v[1].z - v[2].z) / 3.0;
      fc.r = (unsigned char)std::lround(std::clamp(rgba.r, 0.f, 1.f) * 255);
      fc.g = (unsigned char)std::lround(std::clamp(rgba.g, 0.f, 1.f) * 255);
      fc.b = (unsigned char)std::lround(std::clamp(rgba.b, 0.f, 1.f) * 255);
      fc.alpha = rgba.a;
      facets.push_back(fc);
    }
  }
  std::sort(facets.begin(), facets.end(),
            [](const Facet &a, const Facet &b) { return a.depth > b.depth; });
  return facets;
}

// One SVG <g> frame. For a multi-frame animation, wrap it in a SMIL opacity
// keyframe so exactly one frame is visible per 1/fps slot in a seamless loop.
inline void emitGroup(std::ostream &svg, const std::vector<Facet> &facets, int f,
                      int frames, double fps) {
  svg << "<g opacity='" << (frames > 1 ? "0" : "1") << "'>";
  if (frames > 1) {
    const double dur = frames / fps;
    const double on = f / (double)frames, off = (f + 1) / (double)frames;
    svg << "<animate attributeName='opacity' values='0;1;1;0' keyTimes='0;" << on
        << ";" << off << ";1' dur='" << dur << "s' repeatCount='indefinite'/>";
  }
  svg << "\n";
  for (const auto &fc : facets) {
    int r = (int)std::lround(fc.r * fc.shade);
    int g = (int)std::lround(fc.g * fc.shade);
    int b = (int)std::lround(fc.b * fc.shade);
    char fill[16];
    std::snprintf(fill, sizeof fill, "#%02x%02x%02x", r, g, b);
    svg << "<polygon points='" << fc.sx[0] << "," << fc.sy[0] << " " << fc.sx[1]
        << "," << fc.sy[1] << " " << fc.sx[2] << "," << fc.sy[2] << "' fill='"
        << fill << "' fill-opacity='" << fc.alpha
        << "' stroke='#00000030' stroke-width='0.4'/>\n";
  }
  svg << "</g>\n";
}

inline void emitSvgHeader(std::ostream &svg, int W, int H) {
  svg << "<svg xmlns='http://www.w3.org/2000/svg' width='" << W << "' height='"
      << H << "' viewBox='0 0 " << W << " " << H << "'>\n"
      << "<rect width='100%' height='100%' fill='#0b0e14'/>\n";
}

} // namespace x3d2svg

#endif // X3D2SVG_HPP
