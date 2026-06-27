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
