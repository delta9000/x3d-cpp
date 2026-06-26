// Intersect.hpp — ray vs AABB / sphere / triangle / cone / cylinder. Each
// returns the entry parameter t>=0 along the ray (std::optional).
// namespace x3d::runtime.
#ifndef X3D_RUNTIME_INTERSECT_HPP
#define X3D_RUNTIME_INTERSECT_HPP

#include "Aabb.hpp"
#include "Ray.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace x3d::runtime {

inline std::optional<float> rayAabb(const Ray &ray, const Aabb &box) {
  if (box.empty) return std::nullopt;
  const float o[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
  const float d[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
  const float lo[3] = {box.min.x, box.min.y, box.min.z};
  const float hi[3] = {box.max.x, box.max.y, box.max.z};
  float tmin = -1e30f, tmax = 1e30f;
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(d[i]) < 1e-12f) {
      if (o[i] < lo[i] || o[i] > hi[i]) return std::nullopt;
    } else {
      float t1 = (lo[i] - o[i]) / d[i], t2 = (hi[i] - o[i]) / d[i];
      if (t1 > t2) std::swap(t1, t2);
      tmin = std::max(tmin, t1);
      tmax = std::min(tmax, t2);
    }
  }
  if (tmax < tmin || tmax < 0) return std::nullopt;
  return tmin >= 0 ? tmin : tmax; // entry, or (origin inside) the exit
}

inline std::optional<float> raySphere(const Ray &ray, float radius) {
  if (radius <= 0) return std::nullopt;
  const SFVec3f &o = ray.origin, &d = ray.direction;
  float a = d.x*d.x + d.y*d.y + d.z*d.z;
  if (a < 1e-20f) return std::nullopt;
  float b = 2.0f * (o.x*d.x + o.y*d.y + o.z*d.z);
  float c = o.x*o.x + o.y*o.y + o.z*o.z - radius*radius;
  float disc = b*b - 4*a*c;
  if (disc < 0) return std::nullopt;
  float s = std::sqrt(disc);
  float t1 = (-b - s) / (2*a), t2 = (-b + s) / (2*a);
  if (t1 >= 0) return t1;
  if (t2 >= 0) return t2;
  return std::nullopt;
}

inline std::optional<float> rayTriangle(const Ray &ray, const SFVec3f &v0,
                                        const SFVec3f &v1, const SFVec3f &v2) {
  auto sub = [](const SFVec3f &a, const SFVec3f &b) { return SFVec3f{a.x-b.x, a.y-b.y, a.z-b.z}; };
  auto cross = [](const SFVec3f &a, const SFVec3f &b) {
    return SFVec3f{a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
  };
  auto dot = [](const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
  SFVec3f e1 = sub(v1, v0), e2 = sub(v2, v0);
  SFVec3f p = cross(ray.direction, e2);
  float det = dot(e1, p);
  if (std::fabs(det) < 1e-12f) return std::nullopt;
  float inv = 1.0f / det;
  SFVec3f tv = sub(ray.origin, v0);
  float u = dot(tv, p) * inv;
  if (u < 0 || u > 1) return std::nullopt;
  SFVec3f q = cross(tv, e1);
  float v = dot(ray.direction, q) * inv;
  if (v < 0 || u + v > 1) return std::nullopt;
  float t = dot(e2, q) * inv;
  return t < 0 ? std::nullopt : std::optional<float>(t);
}

// rayTriangleBary — like rayTriangle, but also returns the barycentric
// coordinates (u, v) of the hit relative to (v0, v1, v2): the hit point is
// (1-u-v)·v0 + u·v1 + v·v2. Used by the pick narrow-phase to interpolate
// per-vertex texcoords at the hit (M2.5 TouchSensor seam).
inline std::optional<float> rayTriangleBary(const Ray &ray, const SFVec3f &v0,
                                            const SFVec3f &v1, const SFVec3f &v2,
                                            float &u, float &v) {
  auto sub = [](const SFVec3f &a, const SFVec3f &b) { return SFVec3f{a.x-b.x, a.y-b.y, a.z-b.z}; };
  auto cross = [](const SFVec3f &a, const SFVec3f &b) {
    return SFVec3f{a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
  };
  auto dot = [](const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
  SFVec3f e1 = sub(v1, v0), e2 = sub(v2, v0);
  SFVec3f p = cross(ray.direction, e2);
  float det = dot(e1, p);
  if (std::fabs(det) < 1e-12f) return std::nullopt;
  float inv = 1.0f / det;
  SFVec3f tv = sub(ray.origin, v0);
  u = dot(tv, p) * inv;
  if (u < 0 || u > 1) return std::nullopt;
  SFVec3f q = cross(tv, e1);
  v = dot(ray.direction, q) * inv;
  if (v < 0 || u + v > 1) return std::nullopt;
  float t = dot(e2, q) * inv;
  return t < 0 ? std::nullopt : std::optional<float>(t);
}

// rayCone — X3D §13.3.2: axis=Y, apex at (0,+height/2,0), base circle radius
// bottomRadius at y=-height/2. `side` enables the lateral surface; `bottom`
// enables the base disk cap. Returns the minimum non-negative t, or nullopt.
// Conservative: respects side/bottom flags so a purely-cap-less or purely-
// side-less cone won't over-accept. Degenerate inputs (r<=0, h<=0) return
// nullopt. The lateral quadric is double-sheeted; we reject the mirror sheet
// (hit y > height/2) to stay on the real cone.
inline std::optional<float> rayCone(const Ray &ray, float bottomRadius,
                                    float height, bool side = true,
                                    bool bottom = true) {
  if (bottomRadius <= 0.0f || height <= 0.0f) return std::nullopt;
  const float ox = ray.origin.x, oy = ray.origin.y, oz = ray.origin.z;
  const float dx = ray.direction.x, dy = ray.direction.y, dz = ray.direction.z;
  const float halfH = height * 0.5f;
  // k = bottomRadius / height (slope of the cone, tangent of half-angle)
  const float k = bottomRadius / height;
  const float k2 = k * k;

  std::optional<float> best;
  auto keep = [&](float t) {
    if (t >= 0.0f && (!best || t < *best)) best = t;
  };

  // --- Lateral surface ---
  if (side) {
    // Signed y-distance from apex (apex is at y = +halfH in local space).
    const float ay = halfH - oy; // = apex_y - oy
    // Quadratic: (dx²+dz² - k²*dy²)*t² + 2*(ox*dx+oz*dz + k²*ay*dy)*t
    //            + (ox²+oz² - k²*ay²) = 0
    const float A = dx*dx + dz*dz - k2*dy*dy;
    const float B = ox*dx + oz*dz + k2*ay*dy;
    const float C = ox*ox + oz*oz - k2*ay*ay;
    if (std::fabs(A) < 1e-12f) {
      // Linear (ray nearly parallel to cone slope on xz projection).
      // 2*B*t + C = 0  =>  t = -C/(2B)
      if (std::fabs(B) > 1e-12f) {
        float t = -C / (2.0f * B);
        float hy = oy + t * dy;
        if (t >= 0.0f && hy >= -halfH && hy <= halfH) keep(t);
      }
    } else {
      const float disc = B*B - A*C;
      if (disc >= 0.0f) {
        const float sq = std::sqrt(disc);
        const float t1 = (-B - sq) / A;
        const float t2 = (-B + sq) / A;
        for (float t : {t1, t2}) {
          float hy = oy + t * dy;
          // Must be within the valid cone segment and NOT on the mirror sheet
          // (y must be <= halfH — the apex).
          if (t >= 0.0f && hy >= -halfH && hy <= halfH) keep(t);
        }
      }
    }
  }

  // --- Bottom disk cap (y = -halfH, x²+z² <= bottomRadius²) ---
  if (bottom && std::fabs(dy) > 1e-12f) {
    float t = (-halfH - oy) / dy;
    if (t >= 0.0f) {
      float hx = ox + t * dx, hz = oz + t * dz;
      if (hx*hx + hz*hz <= bottomRadius*bottomRadius) keep(t);
    }
  }

  return best;
}

// rayCylinder — X3D §13.3.3: axis=Y, radius `radius`,
// caps at y = ±height/2. `side` enables the lateral tube; `top` the top disk
// (y=+halfH); `bottom` the bottom disk (y=-halfH). Returns min non-negative t.
inline std::optional<float> rayCylinder(const Ray &ray, float radius,
                                        float height, bool side = true,
                                        bool top = true, bool bottom = true) {
  if (radius <= 0.0f || height <= 0.0f) return std::nullopt;
  const float ox = ray.origin.x, oy = ray.origin.y, oz = ray.origin.z;
  const float dx = ray.direction.x, dy = ray.direction.y, dz = ray.direction.z;
  const float halfH = height * 0.5f;
  const float r2 = radius * radius;

  std::optional<float> best;
  auto keep = [&](float t) {
    if (t >= 0.0f && (!best || t < *best)) best = t;
  };

  // --- Lateral surface: x²+z² = r², quadratic in xz ---
  if (side) {
    const float A = dx*dx + dz*dz;
    if (A > 1e-12f) {
      const float B = ox*dx + oz*dz;
      const float C = ox*ox + oz*oz - r2;
      const float disc = B*B - A*C;
      if (disc >= 0.0f) {
        const float sq = std::sqrt(disc);
        const float t1 = (-B - sq) / A;
        const float t2 = (-B + sq) / A;
        for (float t : {t1, t2}) {
          float hy = oy + t * dy;
          if (t >= 0.0f && hy >= -halfH && hy <= halfH) keep(t);
        }
      }
    }
  }

  // --- Cap intersections (ray vs horizontal disk) ---
  if (std::fabs(dy) > 1e-12f) {
    if (top) {
      float t = (halfH - oy) / dy;
      if (t >= 0.0f) {
        float hx = ox + t * dx, hz = oz + t * dz;
        if (hx*hx + hz*hz <= r2) keep(t);
      }
    }
    if (bottom) {
      float t = (-halfH - oy) / dy;
      if (t >= 0.0f) {
        float hx = ox + t * dx, hz = oz + t * dz;
        if (hx*hx + hz*hz <= r2) keep(t);
      }
    }
  }

  return best;
}

} // namespace x3d::runtime
#endif
