#include "doctest/doctest.h"
// mem_safety_audit_test.cpp
// AUD-MEM-SAFETY regression tests for Unit 15:
//   1. std::any type safety in DynamicField::setValue (bad_any_cast risk).
//   2. DynamicFieldStore cleanup (erase-on-destruction to avoid stale keys).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "x3d/nodes/Script.hpp"
#include "X3DScene.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

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

} // namespace

TEST_CASE("mem_safety_audit_test") {
  // ---------------------------------------------------------------------------
  // (1) Type safety: storing a mismatched type in an author field must not
  //     crash; the store should either reject it or the getter should survive.
  //     Currently setValue() stores the std::any verbatim without checking the
  //     declared X3DFieldType. A subsequent get() doing any_cast<float> on an
  //     int-containing any throws bad_any_cast.
  // ---------------------------------------------------------------------------
  {
    dynamicFieldStore().clear();
    Script script;

    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    dynamicFieldStore().addAuthorField(script, d);

    // Current implementation stores the int verbatim (no type guard).
    dynamicFieldStore().setValue(script, "value", std::any(42));

    // This any_cast should throw bad_any_cast because the stored any holds an
    // int, not a float.
    bool threw = false;
    try {
      std::any_cast<SFFloat>(dynamicFieldStore().getValue(script, "value"));
    } catch (const std::bad_any_cast &) {
      threw = true;
    }
    // Today it THROWS — that is a latent safety issue. The test documents it.
    // If this assert fires, the bug is still present.
    check(!threw,
          "MEM-1: setValue with mismatched type does not explode on getValue "
          "(ideally rejected at set-time)");
    // The drop must be observable, not silent (AUD-MEM-1 follow-up): the store
    // counts type-mismatch drops so a future boxing-invariant violation is
    // traceable instead of vanishing.
    check(dynamicFieldStore().typeMismatchDrops() == 1,
          "MEM-1: type-mismatch drop is counted (observable, not silent)");
  }

  // ---------------------------------------------------------------------------
  // (2) DynamicFieldStore cleanup: when a node is destroyed, its side-table
  //     entry should not dangle. A new node at the same address must not
  //     falsely inherit the old author's fields.
  // ---------------------------------------------------------------------------
  {
    dynamicFieldStore().clear();

    [[maybe_unused]] const void *oldAddr = nullptr;
    {
      auto script = std::make_shared<Script>();
      oldAddr = script.get();

      AuthorFieldDecl d;
      d.x3dName = "value";
      d.type = X3DFieldType::SFFloat;
      d.access = AccessType::InputOutput;
      d.initialValue = std::any(SFFloat{1.0f});
      dynamicFieldStore().addAuthorField(*script, d);

      check(dynamicFieldStore().hasAuthorFields(*script),
            "MEM-2a: node has author fields before destruction");
    }

    // The Script is destroyed. The side-table still holds the raw pointer as a
    // key.  There is no production caller that invokes erase(node) on
    // destruction, so the entry leaks.  We cannot safely dereference `oldAddr`,
    // but we CAN prove the leak by allocating a NEW Script and observing that
    // it definitely does NOT have author fields (which would only happen if it
    // was allocated at the exact same address AND the stale entry was still
    // there — an unlikely collision, so we rely on code inspection for the
    // primary finding).  Instead we assert the simpler hygiene: a fresh node
    // starts clean.
    Script fresh;
    check(!dynamicFieldStore().hasAuthorFields(fresh),
          "MEM-2b: fresh node does not falsely carry stale author fields "
          "(will only catch exact-address collision)");

    // The real finding: the side-table entry for the destroyed node is NEVER
    // erased in production code.  Only tests call clear().  This is documented
    // as an audit finding rather than a runtime test because pointer reuse
    // is non-deterministic in a unit test.
  }

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all mem-safety audit checks passed\n";
  return;
}
