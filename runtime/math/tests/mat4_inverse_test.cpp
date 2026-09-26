// mat4_inverse_test.cpp
#include "Mat4.hpp"
#include "Ray.hpp"
#include "doctest/doctest.h"
#include <cmath>
#include <numbers>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

TEST_CASE("mat4_inverse_test") {
  // M * inverse(M) ~= I for a non-trivial translate*rotate*scale.
  Mat4 M = Mat4::translation({3,-2,5})
         * Mat4::rotation(SFRotation{0,1,0, static_cast<float>(std::numbers::pi/3)})
         * Mat4::scale({2,4,0.5f});
  Mat4 I = M * M.inverse();
  for (int c=0;c<4;++c) for (int r=0;r<4;++r)
    CHECK((feq(I.m[c*4+r], c==r ? 1.0f : 0.0f)));

  // transformDirection ignores translation; transformPoint does not.
  Mat4 T = Mat4::translation({10,20,30});
  CHECK((feq(T.transformDirection({1,0,0}).x, 1) && feq(T.transformDirection({1,0,0}).y, 0)));
  CHECK((feq(T.transformPoint({0,0,0}).x, 10)));

  // singular -> identity (documented)
  Mat4 zero; // all-zero (empty Mat4)
  Mat4 zi = zero.inverse();
  CHECK((feq(zi.m[0],1) && feq(zi.m[5],1) && feq(zi.m[15],1)));
  CHECK((!zero.tryInverse().has_value())); // MEM-4: singular -> nullopt.

  // MEM-4: a uniform 1e-7 scale is NOT singular. M*inv ~ I (relative tolerance).
  {
    const float s = 1e-7f;
    Mat4 S = Mat4::scale({s, s, s});
    auto inv = S.tryInverse();
    CHECK((inv.has_value()));
    if (inv) {
      Mat4 P = S * *inv;
      for (int c=0;c<4;++c) for (int r=0;r<4;++r)
        CHECK((feq(P.m[c*4+r], c==r ? 1.0f : 0.0f)));
    }
  }

  // A truly singular (rank-deficient) matrix -> nullopt from tryInverse.
  {
    Mat4 sing = Mat4::scale({1, 1, 0}); // zero Z scale -> det 0.
    CHECK((!sing.tryInverse().has_value()));
  }

  // Anisotropic scales: the old max-magnitude test wrongly called these
  // singular; the column-norm ratio is scale-invariant per axis.
  const auto checkInverts = [](const Mat4 &M) {
    auto inv = M.tryInverse();
    REQUIRE(inv.has_value());
    Mat4 P = M * *inv;
    for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r)
      CHECK(std::fabs(P.m[c*4+r] - (c == r ? 1.0f : 0.0f)) < 1e-3f);
  };
  {
    Mat4 M = Mat4::translation({3,-2,5}) * Mat4::scale({1e4f, 1.0f, 1.0f}); // det 1e4
    checkInverts(M);
  }
  {
    Mat4 M = Mat4::scale({1e-3f, 1e3f, 1.0f}); // det 1
    checkInverts(M);
  }
  {
    Mat4 M = Mat4::rotation(SFRotation{0.3f, 0.5f, 0.8f, 1.1f})
           * Mat4::scale({1e-7f, 1e5f, 3.0f});
    checkInverts(M);
  }

  // Rank-2: two parallel columns -> nullopt.
  {
    Mat4 r2; // zero
    r2.m[0] = 1.0f; r2.m[4] = 2.0f; r2.m[10] = 1.0f; r2.m[15] = 1.0f;
    CHECK((!r2.tryInverse().has_value()));
  }
  // Zero-scale axis -> nullopt (zero column norm).
  {
    Mat4 z = Mat4::scale({1.0f, 0.0f, 1.0f});
    CHECK((!z.tryInverse().has_value()));
  }

  // Ray::pointAt
  Ray ray{{0,0,0},{0,0,-1}};
  CHECK((feq(ray.pointAt(5).z, -5)));
  return;
}
