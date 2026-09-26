// aabb_test.cpp
#include "Aabb.hpp"
#include "Mat4.hpp"
#include "doctest/doctest.h"
#include <cmath>
#include <numbers>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

TEST_CASE("aabb_test") {
  Aabb e;                       // empty is union identity
  CHECK((e.empty));
  Aabb b = Aabb::fromCenterSize({0,0,0}, {2,2,2}); // [-1,1]^3
  CHECK((feq(b.min.x,-1) && feq(b.max.z,1) && !b.empty));
  CHECK((feq(b.center().y,0) && feq(b.size().x,2)));

  e.unionWith(b);               // empty ∪ b == b
  CHECK((feq(e.min.x,-1) && feq(e.max.x,1)));

  Aabb c = Aabb::fromCenterSize({3,0,0}, {2,2,2}); // [2,4]x[-1,1]x[-1,1]
  b.unionWith(c);
  CHECK((feq(b.min.x,-1) && feq(b.max.x,4)));

  // transform [-0.5,0.5]^3 by 45deg about +Z: x/y half-extent -> 0.5*(cos45+sin45)=0.7071
  Aabb u = Aabb::fromCenterSize({0,0,0}, {1,1,1});
  Aabb r = u.transformed(Mat4::rotation(SFRotation{0,0,1, static_cast<float>(std::numbers::pi/4)}));
  CHECK((feq(r.max.x, 0.70710f) && feq(r.max.y, 0.70710f) && feq(r.max.z, 0.5f)));

  Aabb t = Aabb{}.transformed(Mat4::identity()); // empty stays empty
  CHECK((t.empty));

  // MEM-3: non-finite points are ignored, so bounds are order-independent.
  const float nan = std::nanf("");
  const SFVec3f P{1,2,3};
  Aabb f1, f2;
  f1.expand(SFVec3f{nan, nan, nan}); f1.expand(P);
  f2.expand(P); f2.expand(SFVec3f{nan, nan, nan});
  CHECK((!f1.empty && !f2.empty));
  CHECK((feq(f1.min.x, f2.min.x) && feq(f1.max.y, f2.max.y) &&
         feq(f1.max.z, f2.max.z)));
  CHECK((feq(f1.min.x, 1) && feq(f1.max.z, 3))); // only the finite point counts

  // A box of ONLY non-finite points stays empty/invalid.
  Aabb n; n.expand(SFVec3f{nan, 0, 0}); n.expand(SFVec3f{0, nan, 0});
  CHECK((n.empty));
  // And a partial-NaN point is ignored whole (not just its NaN component).
  Aabb p; p.expand(P); p.expand(SFVec3f{nan, 0, 0});
  CHECK((feq(p.min.x, 1) && feq(p.max.x, 1)));
  return;
}
