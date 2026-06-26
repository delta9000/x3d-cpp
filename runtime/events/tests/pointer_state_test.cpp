// pointer_state_test.cpp — TDD test for PointerState + X3DExecutionContext
// input setters (M2.5 input seam, spec §4.1/§6).
//
// Asserts:
//   1. PointerState starts at revision 0 with neutral defaults.
//   2. Each setter individually bumps the revision.
//   3. Multiple setters accumulate revision bumps (additive, not idempotent).
//   4. X3DExecutionContext::setPointer / setPointerButton / setPointerPresent
//      update the state visible via pointerState() and bump the revision.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "PointerState.hpp"
#include "X3DExecutionContext.hpp"

#include "doctest/doctest.h"
#include <cmath>
#include <iostream>
#include <string>

using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
    if (!cond) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    } else {
        std::cout << "ok: " << what << "\n";
    }
}

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-6f; }

// -----------------------------------------------------------------------
// Part 1: PointerState struct in isolation
// -----------------------------------------------------------------------

void test_initial_state() {
    PointerState ps;
    check(ps.revision == 0, "PointerState: initial revision is 0");
    check(!ps.buttonDown, "PointerState: initial buttonDown is false");
    check(!ps.present,    "PointerState: initial present is false");
    // Default Ray: origin {0,0,0}, direction {0,0,-1}.
    check(feq(ps.ray.origin.x, 0) && feq(ps.ray.origin.y, 0) && feq(ps.ray.origin.z, 0),
          "PointerState: initial ray origin is {0,0,0}");
    check(feq(ps.ray.direction.z, -1),
          "PointerState: initial ray direction.z is -1");
}

void test_setRay_bumps_revision() {
    PointerState ps;
    unsigned long r0 = ps.revision;
    ps.setRay(Ray{{1, 2, 3}, {0, 0, -1}});
    check(ps.revision == r0 + 1, "PointerState: setRay bumps revision by 1");
    check(feq(ps.ray.origin.x, 1) && feq(ps.ray.origin.y, 2) && feq(ps.ray.origin.z, 3),
          "PointerState: setRay stores the new origin");
}

void test_setButtonDown_bumps_revision() {
    PointerState ps;
    unsigned long r0 = ps.revision;
    ps.setButtonDown(true);
    check(ps.revision == r0 + 1, "PointerState: setButtonDown bumps revision by 1");
    check(ps.buttonDown, "PointerState: setButtonDown stores true");
    ps.setButtonDown(false);
    check(ps.revision == r0 + 2, "PointerState: setButtonDown again bumps revision again");
    check(!ps.buttonDown, "PointerState: setButtonDown stores false");
}

void test_setPresent_bumps_revision() {
    PointerState ps;
    unsigned long r0 = ps.revision;
    ps.setPresent(true);
    check(ps.revision == r0 + 1, "PointerState: setPresent bumps revision by 1");
    check(ps.present, "PointerState: setPresent stores true");
}

void test_multiple_setters_accumulate() {
    PointerState ps;
    ps.setRay(Ray{{0, 0, 5}, {0, 0, -1}});
    ps.setButtonDown(true);
    ps.setPresent(true);
    check(ps.revision == 3, "PointerState: three setters accumulate to revision 3");
}

// -----------------------------------------------------------------------
// Part 2: X3DExecutionContext setters + pointerState() accessor
// -----------------------------------------------------------------------

void test_context_setPointer() {
    X3DExecutionContext ctx;
    const PointerState &ps = ctx.pointerState();
    check(ps.revision == 0, "context: initial pointer revision is 0");

    Ray r{{0, 0, 10}, {0, 0, -1}};
    ctx.setPointer(r);

    check(ps.revision == 1, "context: setPointer bumps revision to 1");
    check(feq(ps.ray.origin.z, 10), "context: setPointer stores ray origin.z");
    check(feq(ps.ray.direction.z, -1), "context: setPointer stores ray direction.z");
}

void test_context_setPointerButton() {
    X3DExecutionContext ctx;
    const PointerState &ps = ctx.pointerState();

    ctx.setPointerButton(true);
    check(ps.revision == 1, "context: setPointerButton bumps revision");
    check(ps.buttonDown, "context: setPointerButton stores true");

    ctx.setPointerButton(false);
    check(ps.revision == 2, "context: setPointerButton false bumps revision again");
    check(!ps.buttonDown, "context: setPointerButton stores false");
}

void test_context_setPointerPresent() {
    X3DExecutionContext ctx;
    const PointerState &ps = ctx.pointerState();

    ctx.setPointerPresent(true);
    check(ps.revision == 1, "context: setPointerPresent bumps revision");
    check(ps.present, "context: setPointerPresent stores true");
}

void test_context_all_three_setters() {
    X3DExecutionContext ctx;
    const PointerState &ps = ctx.pointerState();

    ctx.setPointer(Ray{{1, 0, 0}, {-1, 0, 0}});
    ctx.setPointerButton(true);
    ctx.setPointerPresent(true);

    check(ps.revision == 3, "context: three setters accumulate to revision 3");
    check(ps.buttonDown && ps.present, "context: button and present reflect updates");
    check(feq(ps.ray.origin.x, 1), "context: ray origin.x stored correctly");
}

} // namespace

TEST_CASE("pointer_state_test") {
    // PointerState standalone
    test_initial_state();
    test_setRay_bumps_revision();
    test_setButtonDown_bumps_revision();
    test_setPresent_bumps_revision();
    test_multiple_setters_accumulate();

    // X3DExecutionContext setters
    test_context_setPointer();
    test_context_setPointerButton();
    test_context_setPointerPresent();
    test_context_all_three_setters();

    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        CHECK(false); return;
    }
    std::cout << "all pointer_state tests passed\n";
    return;
}
