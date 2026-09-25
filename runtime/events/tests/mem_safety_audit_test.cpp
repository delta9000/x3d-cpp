#include "doctest/doctest.h"
// mem_safety_audit_test.cpp
// AUD-MEM-SAFETY regression tests for Unit 15:
//   1. std::any type safety in DynamicField::setValue (bad_any_cast risk).
//   2. DynamicFieldStore cleanup (entries expire with their node; no stale
//      keys on address reuse).
//   3. Stale-entry sweep (the global table stays bounded).
//   4. FieldInfo thunks outlive erase()/clear() safely and type-check writes.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "x3d/nodes/Script.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <any>
#include <new>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
  // (2) DynamicFieldStore cleanup: a lifetime-tracked entry expires with its
  //     node, so a NEW node constructed at the SAME address does not inherit the
  //     old node's author fields. Address reuse is forced deterministically by
  //     constructing both Scripts in one buffer.
  // ---------------------------------------------------------------------------
  {
    dynamicFieldStore().clear();
    alignas(Script) unsigned char buf[sizeof(Script)];

    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});

    {
      std::shared_ptr<Script> first(new (buf) Script(),
                                    [](Script *s) { s->~Script(); });
      dynamicFieldStore().addAuthorField(first, d);
      check(dynamicFieldStore().hasAuthorFields(*first),
            "MEM-2a: node has author fields before destruction");
    } // first destroyed; its storage stays

    std::shared_ptr<Script> second(new (buf) Script(),
                                   [](Script *s) { s->~Script(); });
    check(static_cast<void *>(second.get()) == static_cast<void *>(buf),
          "MEM-2b: second node really reuses the first node's address");
    check(!dynamicFieldStore().hasAuthorFields(*second),
          "MEM-2c: a node reallocated at a destroyed node's address does not "
          "inherit its author fields");
    check(dynamicFieldStore().authorFields(*second).empty() &&
              !dynamicFieldStore().getValue(*second, "value").has_value(),
          "MEM-2d: stale entry is invisible to authorFields/getValue");
  }

  // ---------------------------------------------------------------------------
  // (3) Stale entries are swept: parsing many short-lived documents must not
  //     grow the process-global table without bound.
  // ---------------------------------------------------------------------------
  {
    dynamicFieldStore().clear();
    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    std::size_t peak = 0;
    for (int i = 0; i < 10000; ++i) {
      auto s = std::make_shared<Script>();
      dynamicFieldStore().addAuthorField(s, d);
      peak = std::max(peak, dynamicFieldStore().entryCount());
    }
    check(peak <= 128,
          "MEM-3: destroyed nodes' entries are swept (peak table size " +
              std::to_string(peak) + " for 10000 short-lived nodes)");
  }

  // ---------------------------------------------------------------------------
  // (4) A FieldInfo copied out of authorFields() outlives erase()/clear()
  //     safely (its thunks hold a weak_ptr), and its set thunk enforces the
  //     same AUD-MEM-1 type check as setValue().
  // ---------------------------------------------------------------------------
  {
    dynamicFieldStore().clear();
    auto script = std::make_shared<Script>();
    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    dynamicFieldStore().addAuthorField(script, d);

    std::vector<FieldInfo> infos = dynamicFieldStore().authorFields(*script);
    check(infos.size() == 1 && infos[0].get && infos[0].set,
          "MEM-4a: inputOutput author field has get and set thunks");

    infos[0].set(*script, std::any(42)); // int into an SFFloat field
    check(dynamicFieldStore().typeMismatchDrops() == 1 &&
              std::any_cast<SFFloat>(infos[0].get(*script)) == 1.0f,
          "MEM-4b: set thunk rejects and counts a mismatched type");

    infos[0].set(*script, std::any(SFFloat{2.5f}));
    check(std::any_cast<SFFloat>(
              dynamicFieldStore().getValue(*script, "value")) == 2.5f,
          "MEM-4c: set thunk writes the live store");

    dynamicFieldStore().erase(*script);
    check(!infos[0].get(*script).has_value(),
          "MEM-4d: get thunk on an erased entry is inert (no use-after-free)");
    infos[0].set(*script, std::any(SFFloat{3.0f}));
    check(!dynamicFieldStore().hasAuthorFields(*script),
          "MEM-4e: set thunk on an erased entry is a no-op");
  }

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all mem-safety audit checks passed\n";
  return;
}
