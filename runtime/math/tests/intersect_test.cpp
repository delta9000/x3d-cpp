// intersect_test.cpp
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

  // --- rayCone (X3D §13.3.2) ---
  // Default cone: bottomRadius=1, height=2 -> apex at (0,1,0), base at y=-1.
  // The lateral surface at y=0 has radius 0.5.
  // Ray from +z through the lateral surface at y=0 should hit it.
  // At y=0 the radius is 0.5; a ray at x=0 from z=10 going -z hits z=0.5.
  // t should be 9.5.
  {
    auto hc = rayCone(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f);
    // lateral surface at y=0 has r=0.5 -> hit z=0.5, t = 10-0.5 = 9.5
    CHECK((hc && feq(*hc, 9.5f)));

    // AABB-corner tighter test: a ray that passes through the AABB but misses
    // the cone. Ray at x=0.9,z=10 going -z; at y=0 the cone radius is only
    // 0.5, and at y=-1 the base radius is 1. But x=0.9 only enters the
    // cone at base level — check: at y=-1 we need x²+z² <= 1. With a
    // vertical ray x=0.9 forever, the cone lateral surface requires
    // x²+z² = k²*(1-y)²; at y=? with x=0.9,z=0: 0.81 = (0.5*(1-y))²
    // -> 1-y = 1.8 -> y=-0.8. Check: is -0.8 in [-1,1]? Yes. So we DO
    // expect a hit at x=0.9, z=0 (ray along z-axis at that x doesn't work).
    // Actually use x=0.9, z=0 going -z: hits lateral at the computed y.
    // Use a ray that clearly misses: x=1.2 (outside the base circle).
    // AABB is [-1,1]x[-1,1]x[-1,1]. x=0.95 is inside AABB. Does it hit cone?
    // At y=-1 (base), radius=1; at y=+1 (apex), radius=0. For x=0.95, z=0:
    // need x²+z² = k²*(1-y)² -> 0.9025 = 0.25*(1-y)² -> (1-y)=sqrt(3.61)=1.9
    // -> y = -0.9 in [-1,1]. So it DOES hit. Use x=1.05 instead.
    // At y=-1 cone base radius=1. x=1.05 > 1 means OUTSIDE cone always.
    // But x=1.05 is also outside the AABB x-extent (box is [-1,1]) so the
    // broad-phase would catch it. Use a subtler approach: a corner ray.
    // Corner of AABB for this cone (at y=0): AABB = [-1,1]x[-1,1]x[-1,1].
    // At y=0 the cone radius is only 0.5. A ray at (0, 0, 10) going toward
    // (0.8, 0, 0) direction passes through AABB but hits a point where cone
    // radius < 0.8. Let's try x=0, z=10, direction toward z=0.6 on the surface:
    // Actually use a diagonal: ray at (0.9, 0, 10) going (0,0,-1):
    // Does it hit the lateral surface? x²+z² = 0.9²+z²; for this ray z varies.
    // At z=0: x=0.9. Cone eq: 0.81+0 = 0.25*(1-y)² -> y = 1 - sqrt(3.24)
    // = 1 - 1.8 = -0.8. y=-0.8 in [-1,1] -> YES hits. So this ray hits cone.
    // For a genuine miss, use the AABB corner diagonal at (0.9, 0.9, 10) -> (0,0,-1):
    // x=0.9, z=0.9 -> x²+z²=1.62. Cone eq: 1.62 = 0.25*(1-y)² -> (1-y)=2.546
    // -> y = -1.546 < -1 -> out of range. So this MISSES the lateral surface.
    // Check base cap at y=-1: x²+z² = 0.81+0.81 = 1.62 > 1 -> miss cap too.
    auto cone_corner_miss = rayCone(Ray{{0.9f,0.9f,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((!cone_corner_miss)); // tighter than AABB

    // Bottom cap hit: ray straight down from above toward y=-1.
    // The ray (0.5,10,0) going (0,-1,0) first hits the LATERAL surface at y=0
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

  // --- rayCylinder (X3D §13.3.3) ---
  // Default cylinder: radius=1, height=2 -> spans y in [-1,1].
  // Ray from +z at (0,0,10) going -z hits lateral surface at z=1, t=9.
  {
    auto hcyl = rayCylinder(Ray{{0,0,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((hcyl && feq(*hcyl, 9.0f)));

    // AABB-corner tighter test: same AABB [-1,1]^3. Ray at (0.9,0.9,10) -> -z:
    // At z=sqrt(1-0.81)=sqrt(0.19)~0.436, cylinder lateral at x=0.9 hits
    // z=sqrt(1-0.81)=0.436, y must be in [-1,1]. y=0.9 -> in range. So it hits.
    // Use (0.9,1.1,10): y=1.1 > 1 -> outside cylinder height. Lateral miss.
    // Cap test: y=1.1 is above the cylinder, so no cap hit either.
    // But check: the ray origin is at y=1.1, direction is (0,0,-1), so y stays
    // 1.1 throughout -> never in [-1,1] -> lateral miss, caps perpendicular (top
    // cap at y=1 only reached if dy != 0). No cap hit. So it misses.
    auto cyl_height_miss = rayCylinder(Ray{{0.9f,1.1f,10},{0,0,-1}}, 1.0f, 2.0f);
    CHECK((!cyl_height_miss)); // outside height range

    // An AABB corner ray that is inside the AABB x/z range but outside the
    // cylinder radius:
    // Ray at (0.8,0,10) going -z: x=0.8, z varies. Lateral: z=sqrt(1-0.64)=0.6.
    // y=0 in [-1,1] -> hits at t=9.4. So this hits the cylinder.
    // Use (0.8,0,10) going -z IS a hit. For a miss we need x²>r² always: x=1.05.
    // But x=1.05 is outside the AABB ([-1,1]), so the broad phase catches it.
    // Better: use a ray going diagonally across the AABB corner that misses the
    // cylinder. A ray at y=0 plane from (+1.5, 0, +1.5) toward (-1, 0, -1)
    // (normalised) passes through AABB corners but the cylinder has radius 1.
    // The closest approach to the y-axis for this diagonal ray at (1.5,0,1.5)
    // with dir (-1,0,-1)/sqrt(2): the closest point distance from y-axis equals
    // |origin cross dir| / |dir|. Cross (1.5,0,1.5) x (-1,0,-1)/sqrt(2):
    // = (0*(-1)-1.5*0, 1.5*(-1)-1.5*(-1), 1.5*0-0*(-1)) / sqrt(2)
    // = (0, 0, 0) / sqrt(2) = 0. Wait that means the ray PASSES THROUGH the
    // y-axis (at t=1.5/sqrt(2)*sqrt(2)=1.5). So it hits the cylinder (r=1,
    // origin on axis). Not useful.
    // Use a ray clearly passing between the cylinder and the AABB face:
    // cylinder radius 0.5, height=2. AABB x-extent would be [-0.5,0.5]. That
    // gives us a small cylinder to miss. A ray at (0.4, 0, 10) going -z hits
    // z=sqrt(0.25-0.16)=0.3, so it still hits.
    // Simplest: use height miss scenario with a ray going diagonally through the
    // top corner of the AABB: (0,0,10) with a sideways component ending up at
    // y>1. Let's test the height-boundary case:
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
