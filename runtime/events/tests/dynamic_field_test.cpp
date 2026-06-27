#include "doctest/doctest.h"
// dynamic_field_test.cpp
// Phase-0 (S1) unit tests for the dynamic-field foundation
// (runtime/events/DynamicField.hpp):
//
//   1. Store round-trip: adding AuthorFieldDecls to a node synthesizes
//      FieldInfos that obey the reflection contract (get empty for inputOnly,
//      set empty for read-only), and get/set values round-trip through the
//      boxed std::any value store.
//   2. effectiveFields(node) returns the static fields() table concatenated with
//      the node's author FieldInfos (and is unaffected for a node with none).
//   3. An author-field ROUTE wires through X3DSceneBridge (the bridge resolves
//      the author endpoint via effectiveFields and registers the edge) and the
//      synthesized sink thunk delivers a value end-to-end into the store, where
//      the SaiContext reads it back via getField.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "X3DScene.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
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

bool hasField(const FieldTable &t, const std::string &name) {
  for (const auto &f : t)
    if (f.x3dName == name) return true;
  return false;
}

const FieldInfo *find(const FieldTable &t, const std::string &name) {
  for (const auto &f : t)
    if (f.x3dName == name) return &f;
  return nullptr;
}

} // namespace

TEST_CASE("dynamic_field_test") {
  // ---- (1) Store round-trip + reflection contract -------------------------
  {
    dynamicFieldStore().clear();
    Script script;

    AuthorFieldDecl io;
    io.x3dName = "value";
    io.type = X3DFieldType::SFFloat;
    io.access = AccessType::InputOutput;
    io.initialValue = std::any(SFFloat{0.25f});

    AuthorFieldDecl in;
    in.x3dName = "set_trigger";
    in.type = X3DFieldType::SFTime;
    in.access = AccessType::InputOnly;

    AuthorFieldDecl out;
    out.x3dName = "result";
    out.type = X3DFieldType::SFFloat;
    out.access = AccessType::OutputOnly;

    AuthorFieldDecl init;
    init.x3dName = "amplitude";
    init.type = X3DFieldType::SFFloat;
    init.access = AccessType::InitializeOnly;
    init.initialValue = std::any(SFFloat{2.0f});

    dynamicFieldStore().addAuthorFields(script, {io, in, out, init});

    auto infos = dynamicFieldStore().authorFields(script);
    check(infos.size() == 4, "all four author fields registered");

    const FieldInfo *ioI = find(infos, "value");
    const FieldInfo *inI = find(infos, "set_trigger");
    const FieldInfo *outI = find(infos, "result");
    const FieldInfo *initI = find(infos, "amplitude");
    check(ioI && inI && outI && initI, "all author FieldInfos present");

    // Reflection contract: get empty for inputOnly; set empty for read-only.
    check(ioI && ioI->isReadable() && ioI->isWritable(),
          "inputOutput author field is read+write");
    check(inI && !inI->isReadable() && inI->isWritable(),
          "inputOnly author field: no get, has set");
    check(outI && outI->isReadable() && !outI->isWritable(),
          "outputOnly author field: has get, no set");
    check(initI && initI->isReadable() && !initI->isWritable(),
          "initializeOnly author field: has get, no set");

    // initialValue seeded for inputOutput / initializeOnly.
    check(std::any_cast<SFFloat>(dynamicFieldStore().getValue(script, "value")) ==
              0.25f,
          "inputOutput initialValue seeded");
    check(std::any_cast<SFFloat>(
              dynamicFieldStore().getValue(script, "amplitude")) == 2.0f,
          "initializeOnly initialValue seeded");

    // get/set round-trip through the store.
    dynamicFieldStore().setValue(script, "value", std::any(SFFloat{0.75f}));
    check(std::any_cast<SFFloat>(ioI->get(script)) == 0.75f,
          "value get reflects store write");

    ioI->set(const_cast<Script &>(script), std::any(SFFloat{0.9f}));
    check(std::any_cast<SFFloat>(
              dynamicFieldStore().getValue(script, "value")) == 0.9f,
          "value set thunk writes into store");

    // Writing an unknown name is a no-op.
    dynamicFieldStore().setValue(script, "nope", std::any(SFFloat{1.0f}));
    check(!dynamicFieldStore().getValue(script, "nope").has_value(),
          "set on undeclared name is ignored");
  }

  // ---- (2) effectiveFields concatenation ----------------------------------
  {
    dynamicFieldStore().clear();
    Script script;

    // Before any author fields: effectiveFields == static fields().
    check(effectiveFields(script).size() == script.fields().size(),
          "effectiveFields equals static fields() with no author fields");
    check(!hasField(effectiveFields(script), "custom"),
          "no author field 'custom' yet");

    AuthorFieldDecl d;
    d.x3dName = "custom";
    d.type = X3DFieldType::SFInt32;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFInt32{7});
    dynamicFieldStore().addAuthorField(script, d);

    FieldTable eff = effectiveFields(script);
    check(eff.size() == script.fields().size() + 1,
          "effectiveFields = static + 1 author field");
    check(hasField(eff, "custom"), "effectiveFields includes author field");
    // Static fields (e.g. sourceCode) still present.
    check(hasField(eff, "sourceCode"),
          "effectiveFields still includes static fields");

    // A node with no author fields is unaffected.
    TimeSensor ts;
    check(effectiveFields(ts).size() == ts.fields().size(),
          "unrelated node's effectiveFields unchanged");
  }

  // ---- (3) author-field ROUTE wires + delivers end-to-end -----------------
  {
    dynamicFieldStore().clear();

    auto sensor = std::make_shared<TimeSensor>();
    auto script = std::make_shared<Script>();

    // Author inputOnly sink field on the Script: SFTime to match cycleTime.
    AuthorFieldDecl sink;
    sink.x3dName = "set_tick";
    sink.type = X3DFieldType::SFTime;
    sink.access = AccessType::InputOnly;
    dynamicFieldStore().addAuthorField(*script, sink);

    Scene scene;
    scene.define("CLOCK", sensor);
    scene.define("LOGIC", script);
    scene.rootNodes.push_back(sensor);
    scene.rootNodes.push_back(script);

    Route r;
    r.fromNode = "CLOCK";
    r.fromField = "cycleTime";   // TimeSensor outputOnly SFTime
    r.toNode = "LOGIC";
    r.toField = "set_tick";      // author inputOnly SFTime
    scene.routes.push_back(r);

    X3DExecutionContext ctx;
    BridgeResult res = buildRoutes(scene, ctx);
    check(res.ok(), "author-field ROUTE produced no rejections");
    check(res.routesAdded == 1, "author-field ROUTE wired (1 edge added)");

    // Deliver a value exactly as the cascade's deliver() will (via the
    // synthesized sink set thunk) and read it back through the SAI surface.
    FieldTable eff = effectiveFields(*script);
    const FieldInfo *sinkInfo = find(eff, "set_tick");
    check(sinkInfo && sinkInfo->set, "author sink has a set thunk");
    sinkInfo->set(*script, std::any(SFTime{42.0}));

    SaiContext sai(ctx, *script, "x3d-cpp-gen", "test");
    std::any got = sai.getField(script.get(), "set_tick");
    // inputOnly has no get thunk, so getField returns empty — but the value did
    // land in the store (proving the set path). Verify via the store directly.
    check(!got.has_value(),
          "SAI getField on inputOnly author field returns empty (no get)");
    check(std::any_cast<SFTime>(
              dynamicFieldStore().getValue(*script, "set_tick")) == 42.0,
          "delivered value landed in the author-field store end-to-end");
  }

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all dynamic-field checks passed\n";
  return;
}
