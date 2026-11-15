// pointer_state_test.cpp — TDD test for PointerState + X3DExecutionContext
// input setters (input seam, spec §4.1/§6).
// Asserts:
//   1. PointerState starts at revision 0 with neutral defaults.
//   2. Each setter individually bumps the revision.
//   3. Multiple setters accumulate revision bumps (additive, not idempotent).
//   4. X3DExecutionContext::setPointer / setPointerButton / setPointerPresent
//      update the state visible via pointerState() and bump the revision.

#include "PointerState.hpp"
#include "X3DExecutionContext.hpp"

#include "doctest/doctest.h"
#include <cmath>
#include <string>

using namespace x3d::runtime;

namespace {
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-6f; }
// Part 1: PointerState struct in isolation
TEST_CASE("pointer state: initial state") {
  PointerState ps;
  CHECK_MESSAGE((ps.revision == 0), ("PointerState: initial revision is 0"));
  CHECK_MESSAGE((!ps.buttonDown), ("PointerState: initial buttonDown is false"));
  CHECK_MESSAGE((!ps.present), ("PointerState: initial present is false"));
  // Default Ray: origin {0,0,0}, direction {0,0,-1}.
  CHECK_MESSAGE((feq(ps.ray.origin.x, 0) && feq(ps.ray.origin.y, 0) && feq(ps.ray.origin.z, 0)), ("PointerState: initial ray origin is {0,0,0}"));
  CHECK_MESSAGE((feq(ps.ray.direction.z, -1)),
        "PointerState: initial ray direction.z is -1");
}

TEST_CASE("pointer state: set ray bumps revision") {
  PointerState ps;
  unsigned long r0 = ps.revision;
  ps.setRay(Ray{{1, 2, 3}, {0, 0, -1}});
  CHECK_MESSAGE((ps.revision == r0 + 1), ("PointerState: setRay bumps revision by 1"));
  CHECK_MESSAGE((feq(ps.ray.origin.x, 1) && feq(ps.ray.origin.y, 2) && feq(ps.ray.origin.z, 3)), ("PointerState: setRay stores the new origin"));
}

TEST_CASE("pointer state: set button down bumps revision") {
  PointerState ps;
  unsigned long r0 = ps.revision;
  ps.setButtonDown(true);
  CHECK_MESSAGE((ps.revision == r0 + 1), ("PointerState: setButtonDown bumps revision by 1"));
  CHECK_MESSAGE((ps.buttonDown), ("PointerState: setButtonDown stores true"));
  ps.setButtonDown(false);
  CHECK_MESSAGE((ps.revision == r0 + 2), ("PointerState: setButtonDown again bumps revision again"));
  CHECK_MESSAGE((!ps.buttonDown), ("PointerState: setButtonDown stores false"));
}

TEST_CASE("pointer state: set present bumps revision") {
  PointerState ps;
  unsigned long r0 = ps.revision;
  ps.setPresent(true);
  CHECK_MESSAGE((ps.revision == r0 + 1), ("PointerState: setPresent bumps revision by 1"));
  CHECK_MESSAGE((ps.present), ("PointerState: setPresent stores true"));
}

TEST_CASE("pointer state: multiple setters accumulate") {
  PointerState ps;
  ps.setRay(Ray{{0, 0, 5}, {0, 0, -1}});
  ps.setButtonDown(true);
  ps.setPresent(true);
  CHECK_MESSAGE((ps.revision == 3), ("PointerState: three setters accumulate to revision 3"));
}
// Part 2: X3DExecutionContext setters + pointerState() accessor
TEST_CASE("pointer state: context set pointer") {
  X3DExecutionContext ctx;
  const PointerState &ps = ctx.pointerState();
  CHECK_MESSAGE((ps.revision == 0), ("context: initial pointer revision is 0"));

  Ray r{{0, 0, 10}, {0, 0, -1}};
  ctx.setPointer(r);

  CHECK_MESSAGE((ps.revision == 1), ("context: setPointer bumps revision to 1"));
  CHECK_MESSAGE((feq(ps.ray.origin.z, 10)), ("context: setPointer stores ray origin.z"));
  CHECK_MESSAGE((feq(ps.ray.direction.z, -1)), ("context: setPointer stores ray direction.z"));
}

TEST_CASE("pointer state: context set pointer button") {
  X3DExecutionContext ctx;
  const PointerState &ps = ctx.pointerState();

  ctx.setPointerButton(true);
  CHECK_MESSAGE((ps.revision == 1), ("context: setPointerButton bumps revision"));
  CHECK_MESSAGE((ps.buttonDown), ("context: setPointerButton stores true"));

  ctx.setPointerButton(false);
  CHECK_MESSAGE((ps.revision == 2), ("context: setPointerButton false bumps revision again"));
  CHECK_MESSAGE((!ps.buttonDown), ("context: setPointerButton stores false"));
}

TEST_CASE("pointer state: context set pointer present") {
  X3DExecutionContext ctx;
  const PointerState &ps = ctx.pointerState();

  ctx.setPointerPresent(true);
  CHECK_MESSAGE((ps.revision == 1), ("context: setPointerPresent bumps revision"));
  CHECK_MESSAGE((ps.present), ("context: setPointerPresent stores true"));
}

TEST_CASE("pointer state: context all three setters") {
  X3DExecutionContext ctx;
  const PointerState &ps = ctx.pointerState();

  ctx.setPointer(Ray{{1, 0, 0}, {-1, 0, 0}});
  ctx.setPointerButton(true);
  ctx.setPointerPresent(true);

  CHECK_MESSAGE((ps.revision == 3), ("context: three setters accumulate to revision 3"));
  CHECK_MESSAGE((ps.buttonDown && ps.present), ("context: button and present reflect updates"));
  CHECK_MESSAGE((feq(ps.ray.origin.x, 1)), ("context: ray origin.x stored correctly"));
}

} // namespace

