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
  // Verified in double precision to 2.2e-16; here points are stored as SFVec3f
  // (float), so the achievable radius error is ~1 float ulp (~6e-8). Tolerance
  // 1e-6 is the float-correct bound (the math is exact; only storage is float).
  const double s = std::sqrt(2.0)/2.0;
  nurbs::CurveDef c;
  c.cp = {{1,0,0},{1,1,0},{0,1,0},{-1,1,0},{-1,0,0},
          {-1,-1,0},{0,-1,0},{1,-1,0},{1,0,0}};
  c.w  = {1,s,1,s,1,s,1,s,1};
  c.knot = {0,0,0, 0.5,0.5, 1,1, 1.5,1.5, 2,2,2};
  c.order = 3;
  auto pts = nurbs::tessellateCurve(c, 64);
  CHECK(pts.size() == 65);
  for (auto& p : pts) CHECK(feq((double)p.x*p.x + (double)p.y*p.y, 1.0, 1e-6));
}

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
  // C1 across the seam: the seam vertex must be no more "kinked" than the
  // vertex diametrically opposite it. By the square's symmetry those two
  // chord-turning values are IDENTICAL (verified == to 1e-16) for a correct
  // periodic basis; a C0 kink at the seam would make the seam value far smaller.
  // (An absolute "tangent ~ 1" check is wrong here: adjacent chords straddle the
  // true tangent by the curvature, capping the dot at ~0.923 at every vertex.)
  auto turnDot = [&](int prev, int v, int next){
    SFVec3f a{pts[v].x-pts[prev].x, pts[v].y-pts[prev].y, 0};
    SFVec3f b{pts[next].x-pts[v].x, pts[next].y-pts[v].y, 0};
    double la=std::sqrt(a.x*a.x+a.y*a.y), lb=std::sqrt(b.x*b.x+b.y*b.y);
    return (a.x*b.x+a.y*b.y)/(la*lb);
  };
  double seam = turnDot(11, 12, 1); // incoming pts[11]->pts[12](==pts[0]), outgoing ->pts[1]
  double opp  = turnDot(5, 6, 7);   // diametrically opposite (top-mid) smooth vertex
  CHECK(feq(seam, opp, 1e-3));      // seam as smooth as the symmetric opposite vertex
}

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
  const float h=1e-4f; // float => (P().x-P().x)/(2*h) stays float (ci preset -Werror=narrowing)
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

TEST_CASE("nurbs_surface_already_closed_net_not_wrapped") {
  // Author supplies a net whose first u-column == last u-column (already closed)
  // AND sets uClosed. The coincident-boundary guard must SKIP the periodic wrap
  // (wrapping would duplicate the seam into zero-area sliver triangles). Proof:
  // the uClosed result is identical to the unwrapped (uClosed=false) result.
  nurbs::SurfaceDef base;
  base.uDim = 3; base.vDim = 2; base.uOrder = 3; base.vOrder = 2;
  base.cp = {{1,0,0},{0,1,0},{1,0,0},   // v=0: col0 == col2
             {1,1,0},{0,1,1},{1,1,0}};  // v=1: col0 == col2
  auto open   = base;  open.uClosed = false;
  auto closed = base;  closed.uClosed = true;
  auto go = nurbs::tessellateSurface(open, 5, 2);
  auto gc = nurbs::tessellateSurface(closed, 5, 2);
  REQUIRE(go.size() == gc.size());
  for (size_t k = 0; k < go.size(); ++k)
    CHECK((feq(go[k].p.x,gc[k].p.x) && feq(go[k].p.y,gc[k].p.y) && feq(go[k].p.z,gc[k].p.z)));
}

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
