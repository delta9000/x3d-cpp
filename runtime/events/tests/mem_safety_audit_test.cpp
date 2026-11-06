#include "doctest/doctest.h"
#include "DynamicField.hpp"
#include "x3d/nodes/Script.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <any>
#include <new>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

} // namespace

TEST_CASE("mem_safety_audit_test") {
  // A rejected write preserves the correctly typed value.
  {
    DynamicFieldStore store;
    Script script;

    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    store.addAuthorField(script, d);

    store.setValue(script, "value", std::any(42));

    bool threw = false;
    try {
      std::any_cast<SFFloat>(store.getValue(script, "value"));
    } catch (const std::bad_any_cast &) {
      threw = true;
    }
    CHECK_MESSAGE((!threw), ("MEM-1: setValue with mismatched type does not explode on getValue "
          "(ideally rejected at set-time)"));
    // Rejected writes must remain observable through the drop counter.
    CHECK_MESSAGE((store.typeMismatchDrops() == 1), ("MEM-1: type-mismatch drop is counted (observable, not silent)"));
  }
  // (2) DynamicFieldStore cleanup: a lifetime-tracked entry expires with its
  //     node, so a new node constructed at the same address does not inherit the
  //     old node's author fields. Address reuse is forced deterministically by
  //     constructing both Scripts in one buffer.
  {
    DynamicFieldStore store;
    alignas(Script) unsigned char buf[sizeof(Script)];

    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});

    {
      std::shared_ptr<Script> first(new (buf) Script(),
                                    [](Script *s) { s->~Script(); });
      store.addAuthorField(first, d);
      CHECK_MESSAGE((store.hasAuthorFields(*first)), ("MEM-2a: node has author fields before destruction"));
    } // first destroyed; its storage stays

    std::shared_ptr<Script> second(new (buf) Script(),
                                   [](Script *s) { s->~Script(); });
    CHECK_MESSAGE((static_cast<void *>(second.get()) == static_cast<void *>(buf)), ("MEM-2b: second node really reuses the first node's address"));
    CHECK_MESSAGE((!store.hasAuthorFields(*second)),
          "MEM-2c: a node reallocated at a destroyed node's address does not "
          "inherit its author fields");
    CHECK_MESSAGE((store.authorFields(*second).empty() &&
              !store.getValue(*second, "value").has_value()), ("MEM-2d: stale entry is invisible to authorFields/getValue"));
  }
  // (3) Stale entries are swept: parsing many short-lived documents must not
  //     grow a long-lived owner table without bound.
  {
    DynamicFieldStore store;
    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    std::size_t peak = 0;
    for (int i = 0; i < 10000; ++i) {
      auto s = std::make_shared<Script>();
      store.addAuthorField(s, d);
      peak = std::max(peak, store.entryCount());
    }
    CHECK_MESSAGE((peak <= 128), ("MEM-3: destroyed nodes' entries are swept (peak table size " +
              std::to_string(peak) + " for 10000 short-lived nodes)"));
  }
  // (4) A FieldInfo copied out of authorFields() outlives erase()/clear()
  //     safely (its thunks hold a weak_ptr), and its set thunk enforces the
  //     same type check as setValue().
  {
    DynamicFieldStore store;
    auto script = std::make_shared<Script>();
    AuthorFieldDecl d;
    d.x3dName = "value";
    d.type = X3DFieldType::SFFloat;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFFloat{1.0f});
    store.addAuthorField(script, d);

    std::vector<FieldInfo> infos = store.authorFields(*script);
    CHECK_MESSAGE((infos.size() == 1 && infos[0].get && infos[0].set), ("MEM-4a: inputOutput author field has get and set thunks"));

    infos[0].set(*script, std::any(42)); // int into an SFFloat field
    CHECK_MESSAGE((store.typeMismatchDrops() == 1 &&
              std::any_cast<SFFloat>(infos[0].get(*script)) == 1.0f), ("MEM-4b: set thunk rejects and counts a mismatched type"));

    infos[0].set(*script, std::any(SFFloat{2.5f}));
    CHECK_MESSAGE((std::any_cast<SFFloat>(
              store.getValue(*script, "value")) == 2.5f), ("MEM-4c: set thunk writes the live store"));

    store.erase(*script);
    CHECK_MESSAGE((!infos[0].get(*script).has_value()), ("MEM-4d: get thunk on an erased entry is inert (no use-after-free)"));
    infos[0].set(*script, std::any(SFFloat{3.0f}));
    CHECK_MESSAGE((!store.hasAuthorFields(*script)), ("MEM-4e: set thunk on an erased entry is a no-op"));
  }

  return;
}
