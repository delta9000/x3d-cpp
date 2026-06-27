# NURBS Curve + Patch Surface Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `NurbsCurve` and `NurbsPatchSurface` produce renderable geometry (line + triangle mesh with analytic normals) through the existing extraction pipeline.

**Architecture:** A node-free math unit `runtime/extract/NurbsEval.hpp` (namespace `x3d::runtime::extract::nurbs`) implements Cox–de Boor evaluation, rational weighting, periodic/closed handling, and analytic surface normals (quotient rule). Two thin arms in `MeshBuilder.hpp` read X3D fields and call it. `GeometryBounds.hpp` gets convex-hull (control-point AABB) bounds. The existing `externalGeometryResolver` stays the unrecognized-geometry fallback (now serving the deferred trimmed/swept/swung nodes).

**Tech Stack:** C++17/20 header-only, doctest, CMake (Ninja), mise tasks.

**Spec:** `docs/superpowers/specs/2026-06-27-nurbs-curve-patch-design.md`. All math in this plan was machine-verified against a computer-algebra system; golden values below are exact.

## Global Constraints

- Header-only under `runtime/`; installed automatically via the `runtime/` directory install rule — no CMake `install()` edits needed.
- Math unit `nurbs` namespace must NOT depend on X3D nodes or `MeshBuilder` — plain arrays only (independently unit-testable).
- Field reads use `geombounds::getField<T>`, `geombounds::getNode`, `geombounds::getPointsLenient` (the last accepts MFVec3f AND MFVec3d, so CoordinateDouble control points work for free).
- Types: `SFInt32 = int`, `SFBool = bool`, `MFDouble = std::vector<double>` (`generated_cpp_bindings/x3d/core/X3Dtypes.hpp`). `SFVec3f`/`SFVec2f` from the same header.
- `Topology::Lines` is GL_LINES (index **pairs**): emit expanded positions two-per-segment with trivial `0..N-1` indices (matches the existing IndexedLineSet arm).
- `recognizedGeometryType()` and the `buildLocalMesh()` dispatch must change in lockstep (the file says so at line ~1257).
- Keep all artifacts tool-agnostic: do not name the verification tool in code/docs/commits (verify with it; write results plainly).
- New test `.cpp` files only run once added to their grouped doctest target's source list in `CMakeLists.txt`. Extract tests → `x3d_extract_tests` (list ends ~line 2003). Bounds tests → `x3d_geometry_scene_tests` (`geometry_bounds_test.cpp` at line 1862).
- Run `mise run build` to compile + ctest the dev preset; full gate is `mise run ci`.

---

## File Structure

- **Create** `runtime/extract/NurbsEval.hpp` — the math unit (Tasks 1–4 build it up).
- **Create** `runtime/extract/tests/nurbs_eval_test.cpp` — pure-math tests (Tasks 1–4).
- **Modify** `runtime/extract/MeshBuilder.hpp` — two arms + `recognizedGeometryType` (Tasks 5–6).
- **Create** `runtime/extract/tests/mesh_builder_nurbs_test.cpp` — integration tests (Tasks 5–6).
- **Modify** `runtime/scene/GeometryBounds.hpp` — two bounds arms (Task 7).
- **Modify** `runtime/scene/tests/geometry_bounds_test.cpp` — bounds assertions (Task 7).
- **Modify** `runtime/extract/tests/external_geom_seam_test.cpp` + `runtime/extract/tests/scene_extractor_audit_test.cpp` — swap unrecognized specimen (Task 8).
- **Modify** `CMakeLists.txt` — register the two new test files (Tasks 1, 5).
- **Docs** — new ADR, `findings.yaml`, `extract.md`, `v1-capabilities.md` (Task 9); full CI (Task 10).

---

## Task 1: NurbsEval — open curves (knots, span, basis, rational eval)

**Files:**
- Create: `runtime/extract/NurbsEval.hpp`
- Create: `runtime/extract/tests/nurbs_eval_test.cpp`
- Modify: `CMakeLists.txt` (add the test to `x3d_extract_tests` source list, ~line 2003)

**Interfaces:**
- Produces: `namespace x3d::runtime::extract::nurbs` with `struct CurveDef { std::vector<SFVec3f> cp; std::vector<double> w, knot; int order=3; bool closed=false; };` and `std::vector<SFVec3f> tessellateCurve(const CurveDef&, int segments);` (returns `segments+1` points across the valid domain `[knot[order-1], knot[numCP]]`). Detail helpers: `detail::clampedKnots`, `detail::findSpan`, `detail::basisFuns`, `detail::prepareCurve`.

- [ ] **Step 1: Write the failing test**

Create `runtime/extract/tests/nurbs_eval_test.cpp`:

```cpp
// nurbs_eval_test.cpp — pure-math NURBS evaluation (no X3D nodes).
#include "NurbsEval.hpp"
#include "doctest/doctest.h"
#include <cmath>
#include <vector>

using namespace x3d::core;
using namespace x3d::runtime::extract;

static bool feq(double a, double b, double e = 1e-6) { return std::fabs(a - b) < e; }

TEST_CASE("nurbs_curve_degree1_is_polyline") {
  // order 2 (degree 1), uniform weights => samples lie ON the control polygon.
  nurbs::CurveDef c;
  c.cp = {{0,0,0},{1,0,0},{1,1,0}};
  c.order = 2;
  auto pts = nurbs::tessellateCurve(c, 4);
  CHECK(pts.size() == 5);
  // endpoints interpolated (clamped)
  CHECK((feq(pts.front().x,0) && feq(pts.front().y,0)));
  CHECK((feq(pts.back().x,1)  && feq(pts.back().y,1)));
  // midpoint of a degree-1 spline at t=0.5 of domain is the middle control point
  CHECK((feq(pts[2].x,1) && feq(pts[2].y,0)));
}

TEST_CASE("nurbs_curve_bezier_reduction") {
  // order 4, 4 control points, no interior knots => single cubic Bezier.
  nurbs::CurveDef c;
  c.cp = {{0,0,0},{1,2,0},{3,3,0},{4,0,0}};
  c.order = 4;
  auto pts = nurbs::tessellateCurve(c, 8);
  // compare to direct de Casteljau at the same evenly-spaced t in [0,1]
  auto bez = [&](double t){
    double mt=1-t;
    double b0=mt*mt*mt, b1=3*mt*mt*t, b2=3*mt*t*t, b3=t*t*t;
    return SFVec3f{ (float)(b0*0+b1*1+b2*3+b3*4),
                    (float)(b0*0+b1*2+b2*3+b3*0), 0.f };
  };
  for (int i=0;i<=8;++i){
    auto e = bez(i/8.0);
    CHECK((feq(pts[i].x,e.x,1e-5) && feq(pts[i].y,e.y,1e-5)));
  }
}

TEST_CASE("nurbs_curve_exact_unit_circle") {
  // Rational quadratic, 9 control points, weights cos45 on the corners.
  // Machine-verified: every sample satisfies x^2+y^2 == 1.
  const double s = std::sqrt(2.0)/2.0;
  nurbs::CurveDef c;
  c.cp = {{1,0,0},{1,1,0},{0,1,0},{-1,1,0},{-1,0,0},
          {-1,-1,0},{0,-1,0},{1,-1,0},{1,0,0}};
  c.w  = {1,s,1,s,1,s,1,s,1};
  c.knot = {0,0,0, 0.5,0.5, 1,1, 1.5,1.5, 2,2,2};
  c.order = 3;
  auto pts = nurbs::tessellateCurve(c, 64);
  CHECK(pts.size() == 65);
  for (auto& p : pts) CHECK(feq((double)p.x*p.x + (double)p.y*p.y, 1.0, 1e-9));
}
```

- [ ] **Step 2: Register the test in CMake**

In `CMakeLists.txt`, in the `x3d_extract_tests` source list (the block ending around line 2003 with `castshadow_extract_test.cpp`), add after that line:

```cmake
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/nurbs_eval_test.cpp")
```
(Move the closing `)` from the previous line onto this new entry — i.e. the new file becomes the last element of the list.)

- [ ] **Step 3: Run the test to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — compile error, `NurbsEval.hpp` not found.

- [ ] **Step 4: Create the implementation**

Create `runtime/extract/NurbsEval.hpp`:

```cpp
#ifndef X3D_RUNTIME_EXTRACT_NURBSEVAL_HPP
#define X3D_RUNTIME_EXTRACT_NURBSEVAL_HPP

// Node-free NURBS evaluation: Cox-de Boor basis, rational (weighted) curves and
// surfaces with analytic normals. Operates on plain arrays — no X3D-node or
// MeshBuilder dependency. All formulas machine-verified against a CAS.
#include "x3d/core/X3Dtypes.hpp"
#include <vector>
#include <cmath>

namespace x3d::runtime::extract::nurbs {

using x3d::core::SFVec3f;
using x3d::core::SFVec2f;

struct CurveDef {
  std::vector<SFVec3f> cp;     // control points
  std::vector<double>  w;      // weights; empty => all 1.0
  std::vector<double>  knot;   // len cp.size()+order; empty/wrong => generated
  int  order = 3;              // = degree + 1 (min 2)
  bool closed = false;
};

namespace detail {

// Clamped uniform knots: len n+order, first `order`=0, last `order`=1, interior uniform.
inline std::vector<double> clampedKnots(int n, int order) {
  std::vector<double> k(n + order, 0.0);
  int interior = n - order;               // = n - order interior knots
  for (int i = 0; i < order; ++i) k[i] = 0.0;
  for (int i = 0; i < order; ++i) k[n + i] = 1.0;
  for (int i = 1; i <= interior; ++i) k[order - 1 + i] = double(i) / double(interior + 1);
  return k;
}

// Uniform unclamped (periodic) knots: 0,1,...,M+order-1 (length M+order).
inline std::vector<double> periodicKnots(int M, int order) {
  std::vector<double> k(M + order);
  for (int i = 0; i < M + order; ++i) k[i] = double(i);
  return k;
}

// Knot span containing u, for `numCP` control points. Clamps to [knot[p], knot[numCP]].
inline int findSpan(int numCP, int order, double u, const std::vector<double>& knot) {
  int p = order - 1, nHigh = numCP - 1;
  if (u >= knot[nHigh + 1]) return nHigh;
  if (u <= knot[p]) return p;
  int lo = p, hi = nHigh + 1, mid = (lo + hi) / 2;
  while (u < knot[mid] || u >= knot[mid + 1]) {
    if (u < knot[mid]) hi = mid; else lo = mid;
    mid = (lo + hi) / 2;
  }
  return mid;
}

// Nonzero basis functions N[0..p] at span i (Cox-de Boor, PT A2.2). The
// `denom != 0` guard implements the 0/0 -> 0 convention at repeated knots.
inline void basisFuns(int i, double u, int order, const std::vector<double>& knot,
                      double* N) {
  int p = order - 1;
  std::vector<double> left(order, 0.0), right(order, 0.0);
  N[0] = 1.0;
  for (int j = 1; j <= p; ++j) {
    left[j]  = u - knot[i + 1 - j];
    right[j] = knot[i + j] - u;
    double saved = 0.0;
    for (int r = 0; r < j; ++r) {
      double denom = right[r + 1] + left[j - r];
      double temp = denom != 0.0 ? N[r] / denom : 0.0;
      N[r] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    N[j] = saved;
  }
}

// Resolve weights + knots; apply closed/periodic wrap (X3D: wrap only when the
// first and last control points DIFFER, else it is already a closed clamped loop).
inline CurveDef prepareCurve(const CurveDef& in) {
  CurveDef c = in;
  int n = (int)c.cp.size();
  if ((int)c.w.size() != n) c.w.assign(n, 1.0);
  const bool endsDiffer = n >= 2 &&
      (c.cp.front().x != c.cp.back().x || c.cp.front().y != c.cp.back().y ||
       c.cp.front().z != c.cp.back().z);
  if (c.closed && endsDiffer) {
    int p = c.order - 1;
    for (int i = 0; i < p; ++i) { c.cp.push_back(c.cp[i]); c.w.push_back(c.w[i]); }
    c.knot = periodicKnots((int)c.cp.size(), c.order);
  } else if ((int)c.knot.size() != n + c.order) {
    c.knot = clampedKnots(n, c.order);
  }
  return c;
}

} // namespace detail

// segments+1 samples across the valid domain [knot[order-1], knot[numCP]].
inline std::vector<SFVec3f> tessellateCurve(const CurveDef& in, int segments) {
  std::vector<SFVec3f> out;
  CurveDef c = detail::prepareCurve(in);
  int numCP = (int)c.cp.size();
  if (numCP < c.order || segments < 1) return out;
  int p = c.order - 1;
  double u0 = c.knot[p], u1 = c.knot[numCP];
  out.reserve(segments + 1);
  std::vector<double> N(c.order);
  for (int s = 0; s <= segments; ++s) {
    double u = u0 + (u1 - u0) * double(s) / double(segments);
    int span = detail::findSpan(numCP, c.order, u, c.knot);
    detail::basisFuns(span, u, c.order, c.knot, N.data());
    double x=0,y=0,z=0,w=0;
    for (int a = 0; a <= p; ++a) {
      int idx = span - p + a;
      double wi = c.w[idx], nb = N[a] * wi;
      x += nb*c.cp[idx].x; y += nb*c.cp[idx].y; z += nb*c.cp[idx].z; w += nb;
    }
    out.push_back(SFVec3f{ (float)(x/w), (float)(y/w), (float)(z/w) });
  }
  return out;
}

} // namespace x3d::runtime::extract::nurbs
#endif
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — `nurbs_eval_test` cases green (polyline, Bézier, circle).

- [ ] **Step 6: Commit**

```bash
git add runtime/extract/NurbsEval.hpp runtime/extract/tests/nurbs_eval_test.cpp CMakeLists.txt
git commit -m "feat(extract): NURBS open-curve evaluation (Cox-de Boor + rational)"
```

---

## Task 2: NurbsEval — closed / periodic curves

**Files:**
- Modify: `runtime/extract/NurbsEval.hpp` (already done in Task 1: `prepareCurve` handles `closed`; this task adds the test that proves it)
- Modify: `runtime/extract/tests/nurbs_eval_test.cpp`

**Interfaces:**
- Consumes: `nurbs::tessellateCurve` with `CurveDef::closed = true`.

- [ ] **Step 1: Write the failing test**

Append to `runtime/extract/tests/nurbs_eval_test.cpp`:

```cpp
TEST_CASE("nurbs_curve_closed_periodic") {
  // Square control net, degree 2, closed. Verified against a reference closed
  // spline: wrapping order-1 points + uniform knots traces a smooth closed loop.
  nurbs::CurveDef c;
  c.cp = {{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
  c.order = 3;
  c.closed = true;
  auto pts = nurbs::tessellateCurve(c, 12);
  CHECK(pts.size() == 13);
  // closed loop: first sample == last sample
  CHECK((feq(pts.front().x, pts.back().x) && feq(pts.front().y, pts.back().y)));
  // golden start points (machine-verified): (0.5,0), (0.7778,0.0556), (0.9444,0.2222)
  CHECK((feq(pts[0].x,0.5,1e-4)    && feq(pts[0].y,0.0,1e-4)));
  CHECK((feq(pts[1].x,0.77778,1e-4) && feq(pts[1].y,0.05556,1e-4)));
  CHECK((feq(pts[2].x,0.94444,1e-4) && feq(pts[2].y,0.22222,1e-4)));
  // C1 continuity across the seam: forward tangent at end ~ tangent at start
  SFVec3f tStart{pts[1].x-pts[0].x, pts[1].y-pts[0].y, 0};
  SFVec3f tEnd  {pts[pts.size()-1].x-pts[pts.size()-2].x,
                 pts[pts.size()-1].y-pts[pts.size()-2].y, 0};
  double ls=std::sqrt(tStart.x*tStart.x+tStart.y*tStart.y);
  double le=std::sqrt(tEnd.x*tEnd.x+tEnd.y*tEnd.y);
  double dot=(tStart.x*tEnd.x+tStart.y*tEnd.y)/(ls*le);
  CHECK(dot > 0.99); // tangents aligned at the seam
}
```

- [ ] **Step 2: Run to verify it passes (prepareCurve already implements this)**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS. If the closed branch in `prepareCurve` were wrong this fails on the golden checks.

- [ ] **Step 3: Commit**

```bash
git add runtime/extract/tests/nurbs_eval_test.cpp
git commit -m "test(extract): NURBS closed/periodic curve seam continuity"
```

---

## Task 3: NurbsEval — open surfaces with analytic normals

**Files:**
- Modify: `runtime/extract/NurbsEval.hpp`
- Modify: `runtime/extract/tests/nurbs_eval_test.cpp`

**Interfaces:**
- Produces: `struct SurfaceDef { std::vector<SFVec3f> cp; std::vector<double> w, uKnot, vKnot; int uDim=0,vDim=0,uOrder=3,vOrder=3; bool uClosed=false,vClosed=false; };`, `struct SurfaceSample { SFVec3f p, n; SFVec2f uv; };`, `std::vector<SurfaceSample> tessellateSurface(const SurfaceDef&, int uSeg, int vSeg);` returning a `(uSeg+1)*(vSeg+1)` grid in v-major / u-fastest order (`idx = b*(uSeg+1)+a`). Control net is `cp[i + j*uDim]`, u-index fastest. Detail: `dersBasisFuns1`, `evalSurface`, `prepareSurface`.

- [ ] **Step 1: Write the failing test**

Append to `runtime/extract/tests/nurbs_eval_test.cpp`:

```cpp
TEST_CASE("nurbs_surface_planar_normals") {
  // Bilinear (order 2x2) patch over a planar z=0 grid => every normal is +Z,
  // every point lies on z=0.
  nurbs::SurfaceDef s;
  s.uDim = 2; s.vDim = 2; s.uOrder = 2; s.vOrder = 2;
  s.cp = {{0,0,0},{2,0,0},   // v=0 row (u fastest)
          {0,3,0},{2,3,0}};  // v=1 row
  auto g = nurbs::tessellateSurface(s, 3, 3);
  CHECK(g.size() == 16);
  for (auto& sm : g) {
    CHECK(feq(sm.p.z, 0.0, 1e-6));
    CHECK((feq(std::fabs(sm.n.z), 1.0, 1e-6))); // unit +/-Z
    CHECK((feq(sm.n.x,0,1e-6) && feq(sm.n.y,0,1e-6)));
  }
}

TEST_CASE("nurbs_surface_analytic_normal_matches_finite_difference") {
  // Curved bicubic-ish patch: analytic normal must match a central-difference
  // normal computed from the point evaluator alone.
  nurbs::SurfaceDef s;
  s.uDim = 3; s.vDim = 3; s.uOrder = 3; s.vOrder = 3;
  s.cp = {{0,0,0},{1,0,1},{2,0,0},
          {0,1,1},{1,1,2},{2,1,1},
          {0,2,0},{1,2,1},{2,2,0}};
  auto g = nurbs::tessellateSurface(s, 8, 8);
  // central-difference point sampler over the SAME prepared surface
  auto P = [&](double u,double v){
    return nurbs::detail::evalSurface(
      nurbs::detail::prepareSurface(s).cp, nurbs::detail::prepareSurface(s).w,
      nurbs::detail::clampedKnots(3,3), nurbs::detail::clampedKnots(3,3),
      3,3,3,3,u,v).p;
  };
  const double h=1e-4;
  // check an interior sample (a=4,b=4 in a 9x9 grid => u=v=0.5)
  auto an = g[4*9 + 4].n;
  SFVec3f du{ (P(0.5+h,0.5).x-P(0.5-h,0.5).x)/(2*h),
              (P(0.5+h,0.5).y-P(0.5-h,0.5).y)/(2*h),
              (P(0.5+h,0.5).z-P(0.5-h,0.5).z)/(2*h) };
  SFVec3f dv{ (P(0.5,0.5+h).x-P(0.5,0.5-h).x)/(2*h),
              (P(0.5,0.5+h).y-P(0.5,0.5-h).y)/(2*h),
              (P(0.5,0.5+h).z-P(0.5,0.5-h).z)/(2*h) };
  SFVec3f fd{ du.y*dv.z-du.z*dv.y, du.z*dv.x-du.x*dv.z, du.x*dv.y-du.y*dv.x };
  double l=std::sqrt(fd.x*fd.x+fd.y*fd.y+fd.z*fd.z);
  fd.x/=l; fd.y/=l; fd.z/=l;
  double dot = an.x*fd.x + an.y*fd.y + an.z*fd.z;
  CHECK(std::fabs(dot) > 0.999); // same direction (sign may differ)
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — `SurfaceDef`/`tessellateSurface`/`detail::evalSurface` undefined.

- [ ] **Step 3: Add the surface implementation**

In `runtime/extract/NurbsEval.hpp`, add the `SurfaceDef`/`SurfaceSample` structs after `CurveDef` (top-level), and add the detail functions inside `namespace detail` (after `prepareCurve`), and `tessellateSurface` at top level (after `tessellateCurve`):

```cpp
// --- after CurveDef (top level) ---
struct SurfaceDef {
  std::vector<SFVec3f> cp;     // uDim*vDim, u-index fastest: cp[i + j*uDim]
  std::vector<double>  w;
  std::vector<double>  uKnot, vKnot;
  int  uDim = 0, vDim = 0, uOrder = 3, vOrder = 3;
  bool uClosed = false, vClosed = false;
};
struct SurfaceSample { SFVec3f p; SFVec3f n; SFVec2f uv; };
```

```cpp
// --- inside namespace detail, after prepareCurve ---

// Basis funcs N[0..p] AND first derivatives dN[0..p] at span i (PT A2.3, d=1).
// Verified against symbolic D[BSplineBasis] to machine epsilon.
inline void dersBasisFuns1(int i, double u, int order,
                           const std::vector<double>& knot, double* N, double* dN) {
  int p = order - 1;
  std::vector<std::vector<double>> ndu(order, std::vector<double>(order, 0.0));
  std::vector<double> left(order, 0.0), right(order, 0.0);
  ndu[0][0] = 1.0;
  for (int j = 1; j <= p; ++j) {
    left[j]  = u - knot[i + 1 - j];
    right[j] = knot[i + j] - u;
    double saved = 0.0;
    for (int r = 0; r < j; ++r) {
      double denom = right[r + 1] + left[j - r];
      ndu[j][r] = denom;
      double temp = denom != 0.0 ? ndu[r][j - 1] / denom : 0.0;
      ndu[r][j] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    ndu[j][j] = saved;
  }
  for (int j = 0; j <= p; ++j) N[j] = ndu[j][p];
  for (int r = 0; r <= p; ++r) {
    double der = 0.0; int rk = r - 1, pk = p - 1;
    if (r >= 1 && ndu[pk + 1][rk] != 0.0) der += (1.0 / ndu[pk + 1][rk]) * ndu[rk][pk];
    if (r <= pk && ndu[pk + 1][r]  != 0.0) der += (-1.0 / ndu[pk + 1][r]) * ndu[r][pk];
    dN[r] = der * double(p);
  }
}

// Rational surface point + analytic normal at (u,v). Homogeneous accumulation
// then quotient rule: dP = (dS.xyz - dS.w * P) / S.w ; normal = dP/du x dP/dv.
inline SurfaceSample evalSurface(const std::vector<SFVec3f>& cp,
                                 const std::vector<double>& w,
                                 const std::vector<double>& uKnot,
                                 const std::vector<double>& vKnot,
                                 int uDim, int vDim, int uOrder, int vOrder,
                                 double u, double v) {
  int pu = uOrder - 1, pv = vOrder - 1;
  int su = findSpan(uDim, uOrder, u, uKnot);
  int sv = findSpan(vDim, vOrder, v, vKnot);
  std::vector<double> Nu(uOrder), dNu(uOrder), Nv(vOrder), dNv(vOrder);
  dersBasisFuns1(su, u, uOrder, uKnot, Nu.data(), dNu.data());
  dersBasisFuns1(sv, v, vOrder, vKnot, Nv.data(), dNv.data());
  double Sx=0,Sy=0,Sz=0,Sw=0, Ux=0,Uy=0,Uz=0,Uw=0, Vx=0,Vy=0,Vz=0,Vw=0;
  for (int b = 0; b <= pv; ++b) {
    int jv = sv - pv + b;
    for (int a = 0; a <= pu; ++a) {
      int iu = su - pu + a, idx = iu + jv * uDim;
      double wi = w.empty() ? 1.0 : w[idx];
      const SFVec3f& P = cp[idx];
      double hx=wi*P.x, hy=wi*P.y, hz=wi*P.z, hw=wi;
      double nN=Nu[a]*Nv[b], nU=dNu[a]*Nv[b], nV=Nu[a]*dNv[b];
      Sx+=nN*hx; Sy+=nN*hy; Sz+=nN*hz; Sw+=nN*hw;
      Ux+=nU*hx; Uy+=nU*hy; Uz+=nU*hz; Uw+=nU*hw;
      Vx+=nV*hx; Vy+=nV*hy; Vz+=nV*hz; Vw+=nV*hw;
    }
  }
  double inv = 1.0 / Sw;
  SFVec3f Pt{ (float)(Sx*inv), (float)(Sy*inv), (float)(Sz*inv) };
  double dUx=(Ux-Uw*Sx*inv)*inv, dUy=(Uy-Uw*Sy*inv)*inv, dUz=(Uz-Uw*Sz*inv)*inv;
  double dVx=(Vx-Vw*Sx*inv)*inv, dVy=(Vy-Vw*Sy*inv)*inv, dVz=(Vz-Vw*Sz*inv)*inv;
  SFVec3f nrm{ (float)(dUy*dVz-dUz*dVy),
               (float)(dUz*dVx-dUx*dVz),
               (float)(dUx*dVy-dUy*dVx) };
  double len = std::sqrt((double)nrm.x*nrm.x + (double)nrm.y*nrm.y + (double)nrm.z*nrm.z);
  if (len > 1e-20) { nrm.x/=(float)len; nrm.y/=(float)len; nrm.z/=(float)len; }
  return SurfaceSample{ Pt, nrm, SFVec2f{0,0} };
}

// Resolve weights + u/v knots (open clamped here; closed wrap added in Task 4).
inline SurfaceDef prepareSurface(const SurfaceDef& in) {
  SurfaceDef s = in;
  if ((int)s.w.size() != s.uDim * s.vDim) s.w.assign(s.uDim * s.vDim, 1.0);
  if ((int)s.uKnot.size() != s.uDim + s.uOrder) s.uKnot = clampedKnots(s.uDim, s.uOrder);
  if ((int)s.vKnot.size() != s.vDim + s.vOrder) s.vKnot = clampedKnots(s.vDim, s.vOrder);
  return s;
}
```

```cpp
// --- top level, after tessellateCurve ---
inline std::vector<SurfaceSample> tessellateSurface(const SurfaceDef& in,
                                                    int uSeg, int vSeg) {
  std::vector<SurfaceSample> out;
  SurfaceDef s = detail::prepareSurface(in);
  if (s.uDim < s.uOrder || s.vDim < s.vOrder || uSeg < 1 || vSeg < 1) return out;
  if ((int)s.cp.size() < s.uDim * s.vDim) return out;
  int pu = s.uOrder - 1, pv = s.vOrder - 1;
  double u0 = s.uKnot[pu], u1 = s.uKnot[s.uDim];
  double v0 = s.vKnot[pv], v1 = s.vKnot[s.vDim];
  out.reserve((uSeg + 1) * (vSeg + 1));
  for (int b = 0; b <= vSeg; ++b) {
    double v = v0 + (v1 - v0) * double(b) / double(vSeg);
    for (int a = 0; a <= uSeg; ++a) {
      double u = u0 + (u1 - u0) * double(a) / double(uSeg);
      auto sm = detail::evalSurface(s.cp, s.w, s.uKnot, s.vKnot,
                                    s.uDim, s.vDim, s.uOrder, s.vOrder, u, v);
      sm.uv = SFVec2f{ (float)(double(a)/uSeg), (float)(double(b)/vSeg) };
      out.push_back(sm);
    }
  }
  return out;
}
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — planar-normal + analytic-vs-finite-difference cases green.

- [ ] **Step 5: Commit**

```bash
git add runtime/extract/NurbsEval.hpp runtime/extract/tests/nurbs_eval_test.cpp
git commit -m "feat(extract): NURBS open-surface eval with analytic normals"
```

---

## Task 4: NurbsEval — closed surfaces (uClosed / vClosed wrap)

**Files:**
- Modify: `runtime/extract/NurbsEval.hpp` (extend `prepareSurface`)
- Modify: `runtime/extract/tests/nurbs_eval_test.cpp`

**Interfaces:**
- Consumes: `tessellateSurface` with `SurfaceDef::uClosed/vClosed = true`.

- [ ] **Step 1: Write the failing test**

Append to `runtime/extract/tests/nurbs_eval_test.cpp`:

```cpp
TEST_CASE("nurbs_surface_closed_cylinder") {
  // Open square profile in XZ, swept along +Y, uClosed => a closed tube. The
  // u-seam vertices must coincide and normals stay continuous across it.
  nurbs::SurfaceDef s;
  s.uDim = 4; s.vDim = 2; s.uOrder = 3; s.vOrder = 2; s.uClosed = true;
  // v=0 ring (y=0), then v=1 ring (y=4); u fastest
  s.cp = {{1,0,0},{0,0,1},{-1,0,0},{0,0,-1},
          {1,4,0},{0,4,1},{-1,4,0},{0,4,-1}};
  auto g = nurbs::tessellateSurface(s, 16, 1);
  CHECK(g.size() == (16+1)*(1+1));
  int gw = 16 + 1;
  // u-seam: first and last column of each v-row coincide (closed in u)
  for (int b=0;b<=1;++b){
    auto& a0 = g[b*gw + 0];
    auto& a1 = g[b*gw + 16];
    CHECK((feq(a0.p.x,a1.p.x,1e-5) && feq(a0.p.y,a1.p.y,1e-5) && feq(a0.p.z,a1.p.z,1e-5)));
    double ndot = a0.n.x*a1.n.x + a0.n.y*a1.n.y + a0.n.z*a1.n.z;
    CHECK(ndot > 0.99); // continuous normal across the seam
  }
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — the u-seam points do NOT coincide (open clamped surface, no wrap yet).

- [ ] **Step 3: Extend `prepareSurface` with the closed wrap**

Replace the body of `detail::prepareSurface` in `runtime/extract/NurbsEval.hpp` with:

```cpp
inline SurfaceDef prepareSurface(const SurfaceDef& in) {
  SurfaceDef s = in;
  if ((int)s.w.size() != s.uDim * s.vDim) s.w.assign(s.uDim * s.vDim, 1.0);

  // u-direction periodic wrap: append first (uOrder-1) columns of every row.
  if (s.uClosed && s.uDim >= 2) {
    int pu = s.uOrder - 1, nu = s.uDim + pu;
    std::vector<SFVec3f> cp2(nu * s.vDim);
    std::vector<double>  w2(nu * s.vDim);
    for (int j = 0; j < s.vDim; ++j)
      for (int i = 0; i < nu; ++i) {
        int src = (i < s.uDim ? i : i - s.uDim) + j * s.uDim;
        cp2[i + j * nu] = s.cp[src];
        w2[i + j * nu]  = s.w[src];
      }
    s.cp = std::move(cp2); s.w = std::move(w2); s.uDim = nu;
    s.uKnot = periodicKnots(s.uDim, s.uOrder);
  } else if ((int)s.uKnot.size() != s.uDim + s.uOrder) {
    s.uKnot = clampedKnots(s.uDim, s.uOrder);
  }

  // v-direction periodic wrap: append first (vOrder-1) rows (uDim may have grown).
  if (s.vClosed && s.vDim >= 2) {
    int pv = s.vOrder - 1, nv = s.vDim + pv;
    std::vector<SFVec3f> cp2(s.uDim * nv);
    std::vector<double>  w2(s.uDim * nv);
    for (int j = 0; j < nv; ++j) {
      int srcj = (j < s.vDim ? j : j - s.vDim);
      for (int i = 0; i < s.uDim; ++i) {
        cp2[i + j * s.uDim] = s.cp[i + srcj * s.uDim];
        w2[i + j * s.uDim]  = s.w[i + srcj * s.uDim];
      }
    }
    s.cp = std::move(cp2); s.w = std::move(w2); s.vDim = nv;
    s.vKnot = periodicKnots(s.vDim, s.vOrder);
  } else if ((int)s.vKnot.size() != s.vDim + s.vOrder) {
    s.vKnot = clampedKnots(s.vDim, s.vOrder);
  }
  return s;
}
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — cylinder u-seam coincident + normal-continuous. (Re-run Task 3's tests too; open surfaces are unaffected since the wrap branches are gated on `uClosed`/`vClosed`.)

- [ ] **Step 5: Commit**

```bash
git add runtime/extract/NurbsEval.hpp runtime/extract/tests/nurbs_eval_test.cpp
git commit -m "feat(extract): NURBS closed-surface (uClosed/vClosed) periodic wrap"
```

---

## Task 5: MeshBuilder NurbsCurve arm

**Files:**
- Modify: `runtime/extract/MeshBuilder.hpp`
- Create: `runtime/extract/tests/mesh_builder_nurbs_test.cpp`
- Modify: `CMakeLists.txt` (register the new test in `x3d_extract_tests`)

**Interfaces:**
- Consumes: `nurbs::tessellateCurve`, `nurbs::tessellationToSegments`.
- Produces: `recognizedGeometryType("NurbsCurve") == true`; `buildLocalMesh` on a `NurbsCurve` yields `Topology::Lines`, `solid=false`, expanded line-pair positions.

- [ ] **Step 1: Write the failing test**

Create `runtime/extract/tests/mesh_builder_nurbs_test.cpp`:

```cpp
// mesh_builder_nurbs_test.cpp — NurbsCurve/NurbsPatchSurface -> MeshData.
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("nurbs_curve_arm_emits_lines") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,0,0},{1,1,0},{2,1,0}}));
  auto curve = createX3DNode("NurbsCurve");
  setF(curve, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(curve, "order", std::any(SFInt32{3}));
  setF(curve, "tessellation", std::any(SFInt32{8})); // 8 segments => 9 sample points
  bool rec = false;
  auto mesh = buildLocalMesh(curve.get(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(mesh.topology == Topology::Lines);
  CHECK(mesh.solid == false);
  // 9 samples => 8 segments => 16 expanded line-pair positions
  CHECK(mesh.positions.size() == 16);
  CHECK(mesh.indices.size() == 16);
}

TEST_CASE("nurbs_curve_recognized_oracle") {
  CHECK(recognizedGeometryType("NurbsCurve"));
}

TEST_CASE("nurbs_curve_degenerate_recognized_but_empty") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0}})); // 1 pt < order
  auto curve = createX3DNode("NurbsCurve");
  setF(curve, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(curve, "order", std::any(SFInt32{3}));
  bool rec = false;
  auto mesh = buildLocalMesh(curve.get(), MeshBuildOptions{}, &rec);
  CHECK(rec);                      // recognized type...
  CHECK(mesh.positions.empty());   // ...but legitimately empty
}
```

- [ ] **Step 2: Register the test in CMake**

In `CMakeLists.txt`, append to the `x3d_extract_tests` source list (after the `nurbs_eval_test.cpp` entry added in Task 1):

```cmake
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract/tests/mesh_builder_nurbs_test.cpp")
```
(Again, the new entry becomes the list's last element — move the closing `)`.)

- [ ] **Step 3: Run to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — `recognizedGeometryType("NurbsCurve")` is false; arm not present.

- [ ] **Step 4: Add the include, the segments helper, the arm, and the oracle entry**

In `runtime/extract/NurbsEval.hpp`, add inside `namespace x3d::runtime::extract::nurbs` (top level, after `tessellateSurface`):

```cpp
// X3D tessellation field -> segment count. >0: that many; <0: |t|*numCP; 0: 2*numCP.
inline int tessellationToSegments(int tess, int numCP) {
  int segs = tess > 0 ? tess : (tess < 0 ? (-tess) * numCP : 2 * numCP);
  return segs < 1 ? 1 : segs;
}
```

In `runtime/extract/MeshBuilder.hpp`, add the include near the other extract includes (top of file, with the other `#include "..."`):

```cpp
#include "NurbsEval.hpp"
```

In `recognizedGeometryType()` (line ~1257), add `NurbsCurve` to the returned disjunction (a new line before the closing `;`):

```cpp
      // NURBS (T5): curve -> Lines, patch surface -> Triangles
      || t == "NurbsCurve" || t == "NurbsPatchSurface";
```

In `buildLocalMesh()`, immediately after the `mesh.solid = ...;` line (~line 1295), insert the curve arm:

```cpp
  if (t == "NurbsCurve") {
    auto cpNode = geombounds::getNode(*geom, "controlPoint");
    if (!cpNode) return mesh;                       // recognized, empty
    nurbs::CurveDef c;
    c.cp     = geombounds::getPointsLenient(*cpNode, "point");
    c.w      = geombounds::getField<std::vector<double>>(*geom, "weight", {});
    c.knot   = geombounds::getField<std::vector<double>>(*geom, "knot", {});
    c.order  = geombounds::getField<SFInt32>(*geom, "order", 3);
    c.closed = geombounds::getField<SFBool>(*geom, "closed", false);
    if ((int)c.cp.size() < c.order) return mesh;    // recognized, empty
    int segs = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "tessellation", 0), (int)c.cp.size());
    auto pts = nurbs::tessellateCurve(c, segs);
    if (pts.size() < 2) return mesh;
    for (std::size_t k = 0; k + 1 < pts.size(); ++k) {
      mesh.positions.push_back(pts[k]);
      mesh.positions.push_back(pts[k + 1]);
    }
    mesh.indices.resize(mesh.positions.size());
    for (std::uint32_t i = 0; i < mesh.indices.size(); ++i) mesh.indices[i] = i;
    mesh.topology = Topology::Lines;
    mesh.solid = false;
    return mesh;
  }
```

- [ ] **Step 5: Run to verify it passes**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — curve arm emits 16 line-pair positions, recognized, degenerate-empty.

- [ ] **Step 6: Commit**

```bash
git add runtime/extract/MeshBuilder.hpp runtime/extract/NurbsEval.hpp \
        runtime/extract/tests/mesh_builder_nurbs_test.cpp CMakeLists.txt
git commit -m "feat(extract): MeshBuilder NurbsCurve arm (-> Topology::Lines)"
```

---

## Task 6: MeshBuilder NurbsPatchSurface arm

**Files:**
- Modify: `runtime/extract/MeshBuilder.hpp`
- Modify: `runtime/extract/tests/mesh_builder_nurbs_test.cpp`

**Interfaces:**
- Consumes: `nurbs::tessellateSurface`, `nurbs::tessellationToSegments`.
- Produces: `buildLocalMesh` on a `NurbsPatchSurface` yields `Topology::Triangles`, populated normals, implicit `(u,v)` texcoords, `uSeg*vSeg*6` positions.

- [ ] **Step 1: Write the failing test**

Append to `runtime/extract/tests/mesh_builder_nurbs_test.cpp`:

```cpp
TEST_CASE("nurbs_patch_arm_emits_triangles") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{
      {0,0,0},{1,0,0},{2,0,0},
      {0,1,1},{1,1,2},{2,1,1},
      {0,2,0},{1,2,0},{2,2,0}}));
  auto patch = createX3DNode("NurbsPatchSurface");
  setF(patch, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(patch, "uDimension", std::any(SFInt32{3}));
  setF(patch, "vDimension", std::any(SFInt32{3}));
  setF(patch, "uOrder", std::any(SFInt32{3}));
  setF(patch, "vOrder", std::any(SFInt32{3}));
  setF(patch, "uTessellation", std::any(SFInt32{4}));
  setF(patch, "vTessellation", std::any(SFInt32{4}));
  bool rec = false;
  auto mesh = buildLocalMesh(patch.get(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(mesh.topology == Topology::Triangles);
  CHECK(mesh.hasNormals);
  // 4x4 cells * 2 tris * 3 verts = 96 expanded positions
  CHECK(mesh.positions.size() == 96);
  CHECK(mesh.normals.size() == 96);
  CHECK(mesh.texcoords.size() == 96);
}

TEST_CASE("nurbs_patch_recognized_oracle") {
  CHECK(recognizedGeometryType("NurbsPatchSurface"));
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — patch arm not present; `mesh.positions.size()` is 0.

- [ ] **Step 3: Add the patch arm**

In `runtime/extract/MeshBuilder.hpp`, immediately after the `NurbsCurve` arm added in Task 5, insert:

```cpp
  if (t == "NurbsPatchSurface") {
    auto cpNode = geombounds::getNode(*geom, "controlPoint");
    if (!cpNode) return mesh;
    nurbs::SurfaceDef s;
    s.cp      = geombounds::getPointsLenient(*cpNode, "point");
    s.w       = geombounds::getField<std::vector<double>>(*geom, "weight", {});
    s.uKnot   = geombounds::getField<std::vector<double>>(*geom, "uKnot", {});
    s.vKnot   = geombounds::getField<std::vector<double>>(*geom, "vKnot", {});
    s.uDim    = geombounds::getField<SFInt32>(*geom, "uDimension", 0);
    s.vDim    = geombounds::getField<SFInt32>(*geom, "vDimension", 0);
    s.uOrder  = geombounds::getField<SFInt32>(*geom, "uOrder", 3);
    s.vOrder  = geombounds::getField<SFInt32>(*geom, "vOrder", 3);
    s.uClosed = geombounds::getField<SFBool>(*geom, "uClosed", false);
    s.vClosed = geombounds::getField<SFBool>(*geom, "vClosed", false);
    if (s.uDim < s.uOrder || s.vDim < s.vOrder ||
        (int)s.cp.size() < s.uDim * s.vDim) return mesh;   // recognized, empty
    int uSeg = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "uTessellation", 0), s.uDim);
    int vSeg = nurbs::tessellationToSegments(
        geombounds::getField<SFInt32>(*geom, "vTessellation", 0), s.vDim);
    auto grid = nurbs::tessellateSurface(s, uSeg, vSeg);
    if (grid.empty()) return mesh;
    int gw = uSeg + 1;
    for (int j = 0; j < vSeg; ++j)
      for (int i = 0; i < uSeg; ++i) {
        const nurbs::SurfaceSample* quad[6] = {
            &grid[j*gw + i],         &grid[j*gw + (i+1)],     &grid[(j+1)*gw + (i+1)],
            &grid[j*gw + i],         &grid[(j+1)*gw + (i+1)], &grid[(j+1)*gw + i] };
        for (auto* v : quad) {
          mesh.positions.push_back(v->p);
          mesh.normals.push_back(v->n);
          mesh.texcoords.push_back(v->uv);
        }
      }
    mesh.indices.resize(mesh.positions.size());
    for (std::uint32_t k = 0; k < mesh.indices.size(); ++k) mesh.indices[k] = k;
    mesh.topology = Topology::Triangles;
    mesh.hasNormals = true;
    return mesh;   // mesh.solid was carried from getField at the top
  }
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — 96 positions/normals/texcoords, Triangles, recognized.

- [ ] **Step 5: Commit**

```bash
git add runtime/extract/MeshBuilder.hpp runtime/extract/tests/mesh_builder_nurbs_test.cpp
git commit -m "feat(extract): MeshBuilder NurbsPatchSurface arm (-> Triangles + analytic normals)"
```

---

## Task 7: GeometryBounds — convex-hull (control-point) AABB

**Files:**
- Modify: `runtime/scene/GeometryBounds.hpp`
- Modify: `runtime/scene/tests/geometry_bounds_test.cpp`

**Interfaces:**
- Consumes: existing `geombounds::pointsBounds(std::shared_ptr<X3DNode>)` and `geombounds::getNode`.
- Produces: `localGeometryBounds` returns the control-point AABB for `NurbsCurve`/`NurbsPatchSurface`.

- [ ] **Step 1: Write the failing test**

Append inside the existing `TEST_CASE("geometry_bounds_test")` in `runtime/scene/tests/geometry_bounds_test.cpp` (before its closing brace):

```cpp
  // NurbsCurve / NurbsPatchSurface: AABB over control points (convex hull).
  {
    auto cc = createX3DNode("Coordinate");
    setF(cc, "point", std::any(std::vector<SFVec3f>{{-2,0,0},{0,5,0},{3,0,1}}));
    auto nc = createX3DNode("NurbsCurve");
    setF(nc, "controlPoint", std::any(std::shared_ptr<X3DNode>(cc)));
    Aabb b = localGeometryBounds(nc.get());
    CHECK((feq(b.min.x,-2) && feq(b.max.x,3) && feq(b.max.y,5) && feq(b.max.z,1)));

    auto cs = createX3DNode("Coordinate");
    setF(cs, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,0,0},{0,1,4}}));
    auto np = createX3DNode("NurbsPatchSurface");
    setF(np, "controlPoint", std::any(std::shared_ptr<X3DNode>(cs)));
    Aabb p = localGeometryBounds(np.get());
    CHECK((feq(p.min.x,0) && feq(p.max.x,1) && feq(p.max.y,1) && feq(p.max.z,4)));
  }
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -20`
Expected: FAIL — NURBS nodes fall through `localGeometryBounds`, returning an empty AABB.

- [ ] **Step 3: Add the bounds arms**

In `runtime/scene/GeometryBounds.hpp`, inside `localGeometryBounds`, add before the final fall-through (e.g. after the existing primitive arms, before the generic `coord` handling):

```cpp
  if (t == "NurbsCurve" || t == "NurbsPatchSurface")
    return pointsBounds(getNode(*geom, "controlPoint"));
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — both NURBS AABBs match the control-point extents.

- [ ] **Step 5: Commit**

```bash
git add runtime/scene/GeometryBounds.hpp runtime/scene/tests/geometry_bounds_test.cpp
git commit -m "feat(scene): convex-hull bounds for NurbsCurve/NurbsPatchSurface"
```

---

## Task 8: Move the unrecognized-geometry test specimen off NurbsPatchSurface

**Files:**
- Modify: `runtime/extract/tests/external_geom_seam_test.cpp`
- Modify: `runtime/extract/tests/scene_extractor_audit_test.cpp`

**Interfaces:**
- Both tests use `NurbsTrimmedSurface` (a still-unrecognized stub) as the resolver/skip specimen.

- [ ] **Step 1: Swap the specimen in `external_geom_seam_test.cpp`**

Replace every `createX3DNode("NurbsPatchSurface")` with `createX3DNode("NurbsTrimmedSurface")` (4 occurrences: section 0 and sections 1–3). Update the section-0 comment to read `// A NurbsTrimmedSurface (still unrecognized) triggers the resolver`.

- [ ] **Step 2: Swap the specimen in `scene_extractor_audit_test.cpp`**

At line ~82 replace `createX3DNode("NurbsPatchSurface")` with `createX3DNode("NurbsTrimmedSurface")`, and update the two `skippedGeometryCounts` assertions (lines ~97–98) from `"NurbsPatchSurface"` to `"NurbsTrimmedSurface"`.

- [ ] **Step 3: Run to verify both pass**

Run: `mise run build 2>&1 | tail -20`
Expected: PASS — resolver still fires for the unrecognized `NurbsTrimmedSurface`; the now-recognized `NurbsPatchSurface` is unaffected.

- [ ] **Step 4: Commit**

```bash
git add runtime/extract/tests/external_geom_seam_test.cpp \
        runtime/extract/tests/scene_extractor_audit_test.cpp
git commit -m "test(extract): move unrecognized-geometry specimen to NurbsTrimmedSurface"
```

---

## Task 9: Docs — ADR, conformance, subsystem, capabilities

**Files:**
- Create: `docs/wiki/decisions/NNNN-nurbs-tessellation-first-party.md` (next free ADR number)
- Modify: `docs/wiki/decisions/` coverage row in `docs/wiki/coverage.md`
- Modify: `docs/conformance/findings.yaml`
- Modify: `docs/wiki/subsystems/extract.md`
- Modify: `docs/sdk/v1-capabilities.md`

- [ ] **Step 1: Determine the next ADR number**

Run: `ls docs/wiki/decisions/ | grep -oE '^[0-9]{4}' | sort -n | tail -1`
Use that number + 1 (zero-padded) as `NNNN` below.

- [ ] **Step 2: Write the ADR**

Create `docs/wiki/decisions/NNNN-nurbs-tessellation-first-party.md` capturing: NURBS curve+patch tessellation is first-party (I/O-free + spec-prescribed ⇒ no genericity payoff, so NOT a seam); `externalGeometryResolver` stays the unrecognized-geometry fallback; convex-hull bounds; clamped-uniform default knots; full periodic `closed` handling; deferrals (trimmed/swept/swung, interpolators, authored texCoord, 2D geometry). Cite the spec `docs/superpowers/specs/2026-06-27-nurbs-curve-patch-design.md`. Match the heading/section style of a recent ADR (e.g. `ls -t docs/wiki/decisions/*.md | head -3`).

- [ ] **Step 3: Add the ADR coverage row**

Add the new ADR to the Decisions table in `docs/wiki/coverage.md` (contiguous numbering — the `coverage-gate` enforces this).

- [ ] **Step 4: Update conformance findings**

In `docs/conformance/findings.yaml`, change NRB-1 from DEFERRED to resolved-for-curve+patch, noting the deferred NURBS nodes remain open. Then regenerate the view:

Run: `mise run conformance`
Expected: regenerates `docs/conformance/` with no schema errors. Do NOT hand-edit the generated `.md`.

- [ ] **Step 5: Update the subsystem + capability docs**

In `docs/wiki/subsystems/extract.md`: add `runtime/extract/NurbsEval.hpp` to the key-files table and a short "NURBS" paragraph (curve→Lines, patch→Triangles with analytic normals, convex-hull bounds, resolver-fallback for the deferred nodes). In `docs/sdk/v1-capabilities.md`: add the NURBS curve + patch capability claim.

- [ ] **Step 6: Run the drift suggester + docs gate**

Run: `mise run docs-drift working`
Run: `mise run docs-build`
Expected: docs-build passes strict (no dead links / nav orphans); review drift output and address any flagged page.

- [ ] **Step 7: Commit**

```bash
git add docs/
git commit -m "docs: NURBS curve+patch — ADR, conformance NRB-1, extract subsystem, capabilities"
```

---

## Task 10: Full CI gate

**Files:** none (verification only).

- [ ] **Step 1: Refresh the code RAG store (symbols moved/added)**

Run: `mise run code-ingest`
Expected: completes (keeps code-rag / docs-drift accurate per CLAUDE.md).

- [ ] **Step 2: Run the full pipeline**

Run: `mise run ci`
Expected: PASS — tests + golden + conformance-gate + coverage-gate + ci-preset build/ctest (per-header isolation ON) + cli-gate-regression all green. The per-header check compiles `NurbsEval.hpp` in isolation, catching any missing include.

- [ ] **Step 3: Commit any gate-required regenerations**

If `mise run ci` regenerated committed artifacts (e.g. conformance view), stage and commit them:

```bash
git add -A
git commit -m "chore: regenerate gate artifacts for NURBS curve+patch"
```

---

## Self-Review

**Spec coverage:** NurbsCurve→Lines (T5/Task 5) ✓; NurbsPatchSurface→Triangles+analytic normals (Task 6) ✓; Cox–de Boor + rational eval (Task 1) ✓; closed/periodic curve (Task 2) + surface (Task 4) ✓; valid-domain sampling (Task 1) ✓; tessellation field semantics via `tessellationToSegments` (Task 5) ✓; clamped-uniform default knots + repeated-knot 0/0 guard (Task 1) ✓; convex-hull bounds (Task 7) ✓; unrecognized-fallback specimen swap (Task 8) ✓; docs incl. ADR + NRB-1 (Task 9) ✓; full CI (Task 10) ✓. Deferred items (trimmed/swept/swung, interpolators, authored texCoord, 2D) are explicitly out of scope and not tasked.

**Type consistency:** `CurveDef`/`SurfaceDef`/`SurfaceSample` field names match across Tasks 1/3/5/6; `tessellateCurve`/`tessellateSurface`/`tessellationToSegments`/`evalSurface`/`prepareSurface`/`dersBasisFuns1` used consistently; grid order (v-major, u-fastest, `idx=b*(uSeg+1)+a`) consistent between Task 3 producer and Task 6 consumer; control-net order (`cp[i + j*uDim]`, u-fastest) consistent between `evalSurface`, `prepareSurface`, and the patch arm.

**Placeholder scan:** none — every code/test/command step is concrete; `NNNN` in Task 9 is resolved by its own Step 1.

**Known runtime check:** field reflected types (`weight`/`knot`/`uKnot`/`vKnot` as `std::vector<double>`; `order`/`*Dimension`/`*Tessellation` as `SFInt32`; `closed`/`*Closed` as `SFBool`) are assumed; `getField` asserts in debug on a type mismatch, so Task 5/6 builds fail loudly if any generated field differs — fix by matching the reflected type if that occurs.
