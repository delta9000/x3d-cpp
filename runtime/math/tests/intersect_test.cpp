#include "Intersect.hpp"
#include "doctest/doctest.h"
#include <cmath>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

TEST_CASE("intersect_test") {
  Aabb box = Aabb::fromCenterSize({0,0,0}, {2,2,2}); // [-1,1]^3
  // ray from +z toward -z hits front face z=1 at t=9 (origin z=10)
  auto a = rayAabb(Ray{{0,0,10},{0,0,-1}}, box);
  CHECK((a && feq(*a, 9)));
  // miss (parallel, outside)
  CHECK((!rayAabb(Ray{{5,5,10},{0,0,-1}}, box)));
  // origin inside -> entry t is the exit (>0)
  auto inside = rayAabb(Ray{{0,0,0},{0,0,-1}}, box);
  CHECK((inside && feq(*inside, 1)));

  // unit sphere at origin, ray from +z
  auto s = raySphere(Ray{{0,0,10},{0,0,-1}}, 1.0f);
  CHECK((s && feq(*s, 9)));
  CHECK((!raySphere(Ray{{5,0,10},{0,0,-1}}, 1.0f))); // miss
  CHECK((!raySphere(Ray{{0,0,10},{0,0,-1}}, 0.0f))); // radius 0 -> none

  // triangle in z=0 plane; ray from +z through (0.25,0.25)
  SFVec3f v0{0,0,0}, v1{1,0,0}, v2{0,1,0};
  auto t = rayTriangle(Ray{{0.25f,0.25f,5},{0,0,-1}}, v0,v1,v2);
  CHECK((t && feq(*t, 5)));
  // outside the triangle
  CHECK((!rayTriangle(Ray{{1,1,5},{0,0,-1}}, v0,v1,v2)));

  // rayCone (X3D §13.3.2)
  // At y=0 the default cone radius is 0.5, so the front hit from z=10 has t=9.5.
  {
    auto hc = rayCone(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f);
    // lateral surface at y=0 has r=0.5 -> hit z=0.5, t = 10-0.5 = 9.5
    CHECK((hc && feq(*hc, 9.5f)));

    // At y=0.9 the cone radius is 0.05; x=0.9 stays outside it while crossing the AABB.
    auto cone_corner_miss = rayCone(Ray{{0.9f,0.9f,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((!cone_corner_miss)); // tighter than AABB

    // Bottom cap hit: ray straight down from above toward y=-1.
    // The ray (0.5,10,0) going (0,-1,0) first hits the lateral surface at y=0
    // (t=10) where cone radius at y=0 is 0.5, x=0.5 is on the surface. Then
    // hits the base cap at y=-1 (t=11). So best is the lateral hit t=10.
    auto hcap = rayCone(Ray{{0.5f,10,0},{0,-1,0}}, 1.0f, 2.0f);
    CHECK((hcap && feq(*hcap, 10.0f))); // lateral hit at t=10 beats cap at t=11

    // Ray parallel to axis from outside but inside AABB x-range — hits only cap
    // if x is within base radius (side=false, bottom=true).
    // Same ray, but only the bottom cap: t = (−1 − 10)/(−1) = 11.
    auto hcap2 = rayCone(Ray{{0.5f,10,0},{0,-1,0}}, 1.0f, 2.0f, /*side=*/false, /*bottom=*/true);
    CHECK((hcap2 && feq(*hcap2, 11.0f)));
    // side=true, bottom=false: ray through y=0 laterally from above
    auto hlat = rayCone(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f, /*side=*/true, /*bottom=*/false);
    CHECK((hlat && feq(*hlat, 9.5f)));
    // both false -> nullopt
    CHECK((!rayCone(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f, false, false)));
    // degenerate
    CHECK((!rayCone(Ray{{0,0,10},{0,0,-1}}, 0.0f, 2.0f)));
    CHECK((!rayCone(Ray{{0,0,10},{0,0,-1}}, 1.0f, 0.0f)));
  }

  // rayCylinder (X3D §13.3.3)
  // Default cylinder: radius=1, height=2 -> spans y in [-1,1].
  // Ray from +z at (0,0,10) going -z hits lateral surface at z=1, t=9.
  {
    auto hcyl = rayCylinder(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((hcyl && feq(*hcyl, 9.0f)));

    // y stays at 1.1, outside the cylinder height [-1,1]; dy=0 also misses the caps.
    auto cyl_height_miss = rayCylinder(Ray{{0.9f,1.1f,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((!cyl_height_miss)); // outside height range

    // A ray parallel to the caps and above the cylinder cannot hit it.
    auto cyl_top_miss = rayCylinder(Ray{{0,2,0},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((!cyl_top_miss)); // ray at y=2, cylinder ends at y=1, going -z misses

    // Caps:
    auto htopcap = rayCylinder(Ray{{0.5f,10,0},{0,-1,0}}, 1.0f, 2.0f);
    CHECK((htopcap && feq(*htopcap, 9.0f))); // top cap at y=1, t=10-1=9

    auto hbtmcap = rayCylinder(Ray{{0.5f,-10,0},{0,1,0}}, 1.0f, 2.0f);
    CHECK((hbtmcap && feq(*hbtmcap, 9.0f))); // bottom cap at y=-1, t=10-1=9

    // Flags: side=false, top/bottom=false -> no hit
    CHECK((!rayCylinder(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f, false, false, false)));
    // side only
    auto side_only = rayCylinder(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f, true, false, false);
    CHECK((side_only && feq(*side_only, 9.0f)));
    // degenerate
    CHECK((!rayCylinder(Ray{{0,0,10},{0,0,-1}}, 0.0f, 2.0f)));
    CHECK((!rayCylinder(Ray{{0,0,10},{0,0,-1}}, 1.0f, 0.0f)));
  }

  return;
}
