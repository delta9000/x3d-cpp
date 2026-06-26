// key_state_test.cpp — TDD test for KeyState + X3DExecutionContext keyboard seam
// (M2D PDS-4, unit U3).
//
// Asserts:
//   1. KeyState starts at revision 0 with an empty held-set.
//   2. setKey(code, true) inserts the code and bumps revision.
//   3. setKey(code, false) removes the code and bumps revision.
//   4. A double-press bumps revision a second time (unconditional bump).
//   5. Multiple different keys accumulate independently.
//   6. X3DExecutionContext::setKey delegates to keyState() and bumps revision.
//   7. keyState() const accessor reflects updates made via ctx.setKey.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "KeyState.hpp"
#include "X3DExecutionContext.hpp"

#include "doctest/doctest.h"
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

// -----------------------------------------------------------------------
// Part 1: KeyState struct in isolation
// -----------------------------------------------------------------------

void test_initial_state() {
    KeyState ks;
    check(ks.revision == 0, "KeyState: initial revision is 0");
    check(ks.held.empty(), "KeyState: initial held-set is empty");
}

void test_press_inserts_and_bumps() {
    KeyState ks;
    ks.setKey(65, true); // 'A'
    check(ks.revision == 1, "KeyState: press bumps revision to 1");
    check(ks.isHeld(65),    "KeyState: pressed key is held");
    check(!ks.isHeld(66),   "KeyState: unrelated key is not held");
}

void test_release_removes_and_bumps() {
    KeyState ks;
    ks.setKey(65, true);
    unsigned long r1 = ks.revision;
    ks.setKey(65, false);
    check(ks.revision == r1 + 1, "KeyState: release bumps revision");
    check(!ks.isHeld(65), "KeyState: released key is no longer held");
}

void test_double_press_bumps_again() {
    KeyState ks;
    ks.setKey(65, true);
    unsigned long r1 = ks.revision;
    ks.setKey(65, true); // already held — held-set unchanged, revision still bumps
    check(ks.revision == r1 + 1, "KeyState: double-press still bumps revision");
    check(ks.isHeld(65), "KeyState: key still held after double-press");
}

void test_release_not_held_bumps() {
    KeyState ks;
    ks.setKey(99, false); // release key that was never pressed
    check(ks.revision == 1, "KeyState: release of not-held key bumps revision");
    check(!ks.isHeld(99), "KeyState: not-held key stays not-held");
}

void test_multiple_keys() {
    KeyState ks;
    ks.setKey(1, true);
    ks.setKey(2, true);
    ks.setKey(3, true);
    check(ks.revision == 3, "KeyState: three presses accumulate revision to 3");
    check(ks.isHeld(1) && ks.isHeld(2) && ks.isHeld(3), "KeyState: all three keys held");
    ks.setKey(2, false);
    check(ks.revision == 4, "KeyState: release of middle key bumps revision to 4");
    check(ks.isHeld(1) && !ks.isHeld(2) && ks.isHeld(3),
          "KeyState: only middle key released");
}

// -----------------------------------------------------------------------
// Part 2: X3DExecutionContext setKey + keyState() accessor
// -----------------------------------------------------------------------

void test_context_initial_key_state() {
    X3DExecutionContext ctx;
    const KeyState &ks = ctx.keyState();
    check(ks.revision == 0, "context: initial key revision is 0");
    check(ks.held.empty(), "context: initial key held-set is empty");
}

void test_context_setKey_press() {
    X3DExecutionContext ctx;
    const KeyState &ks = ctx.keyState();

    ctx.setKey(87, true); // 'W'
    check(ks.revision == 1, "context: setKey(press) bumps revision to 1");
    check(ks.isHeld(87), "context: setKey(press) marks key held");
}

void test_context_setKey_release() {
    X3DExecutionContext ctx;
    ctx.setKey(87, true);
    ctx.setKey(87, false);
    const KeyState &ks = ctx.keyState();
    check(ks.revision == 2, "context: setKey(release) bumps revision to 2");
    check(!ks.isHeld(87), "context: setKey(release) clears held flag");
}

void test_context_multiple_keys() {
    X3DExecutionContext ctx;
    ctx.setKey(87, true);  // W
    ctx.setKey(65, true);  // A
    ctx.setKey(83, true);  // S
    const KeyState &ks = ctx.keyState();
    check(ks.revision == 3, "context: three presses accumulate revision to 3");
    check(ks.isHeld(87) && ks.isHeld(65) && ks.isHeld(83),
          "context: all three keys held");
}

void test_pointer_and_key_revisions_are_independent() {
    // PointerState and KeyState each have their own revision counters.
    X3DExecutionContext ctx;
    ctx.setPointerButton(true);
    ctx.setKey(87, true);
    // pointer revision == 1, key revision == 1 — they don't share state.
    check(ctx.pointerState().revision == 1,
          "context: pointer revision independent of key revision");
    check(ctx.keyState().revision == 1,
          "context: key revision independent of pointer revision");
}

} // namespace

TEST_CASE("key_state_test") {
    // KeyState standalone
    test_initial_state();
    test_press_inserts_and_bumps();
    test_release_removes_and_bumps();
    test_double_press_bumps_again();
    test_release_not_held_bumps();
    test_multiple_keys();

    // X3DExecutionContext keyboard seam
    test_context_initial_key_state();
    test_context_setKey_press();
    test_context_setKey_release();
    test_context_multiple_keys();
    test_pointer_and_key_revisions_are_independent();

    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        CHECK(false); return;
    }
    std::cout << "all key_state tests passed\n";
    return;
}
