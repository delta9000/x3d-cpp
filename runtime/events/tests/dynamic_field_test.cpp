#include "doctest/doctest.h"
#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "X3DScene.hpp"

#include <any>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

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
  // (1) Store round-trip + reflection contract
  {
    Scene owner;
    auto &fields = *owner.authorFields;
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

    fields.addAuthorFields(script, {io, in, out, init});

    auto infos = fields.authorFields(script);
    CHECK_MESSAGE((infos.size() == 4), ("all four author fields registered"));

    const FieldInfo *ioI = find(infos, "value");
    const FieldInfo *inI = find(infos, "set_trigger");
    const FieldInfo *outI = find(infos, "result");
    const FieldInfo *initI = find(infos, "amplitude");
    CHECK_MESSAGE((ioI && inI && outI && initI), ("all author FieldInfos present"));

    // Reflection contract: get empty for inputOnly; set empty for read-only.
    CHECK_MESSAGE((ioI && ioI->isReadable() && ioI->isWritable()), ("inputOutput author field is read+write"));
    CHECK_MESSAGE((inI && !inI->isReadable() && inI->isWritable()),
          "inputOnly author field: no get, has set");
    CHECK_MESSAGE((outI && outI->isReadable() && !outI->isWritable()), ("outputOnly author field: has get, no set"));
    CHECK_MESSAGE((initI && initI->isReadable() && !initI->isWritable()),
          "initializeOnly author field: has get, no set");

    // initialValue seeded for inputOutput / initializeOnly.
    CHECK_MESSAGE((std::any_cast<SFFloat>(fields.getValue(script, "value")) ==
              0.25f), ("inputOutput initialValue seeded"));
    CHECK_MESSAGE((std::any_cast<SFFloat>(
              fields.getValue(script, "amplitude")) == 2.0f),
          "initializeOnly initialValue seeded");

    // get/set round-trip through the store.
    fields.setValue(script, "value", std::any(SFFloat{0.75f}));
    CHECK_MESSAGE((std::any_cast<SFFloat>(ioI->get(script)) == 0.75f), ("value get reflects store write"));

    ioI->set(const_cast<Script &>(script), std::any(SFFloat{0.9f}));
    CHECK_MESSAGE((std::any_cast<SFFloat>(
              fields.getValue(script, "value")) == 0.9f), ("value set thunk writes into store"));

    // Writing an unknown name is a no-op.
    fields.setValue(script, "nope", std::any(SFFloat{1.0f}));
    CHECK_MESSAGE((!fields.getValue(script, "nope").has_value()), ("set on undeclared name is ignored"));
  }

  // (2) effectiveFields concatenation
  {
    Scene owner;
    auto &fields = *owner.authorFields;
    Script script;

    // Before any author fields: effectiveFields == static fields().
    CHECK_MESSAGE((effectiveFields(script, fields).size() == script.fields().size()), ("effectiveFields equals static fields() with no author fields"));
    CHECK_MESSAGE((!hasField(effectiveFields(script, fields), "custom")),
          "no author field 'custom' yet");

    AuthorFieldDecl d;
    d.x3dName = "custom";
    d.type = X3DFieldType::SFInt32;
    d.access = AccessType::InputOutput;
    d.initialValue = std::any(SFInt32{7});
    fields.addAuthorField(script, d);

    FieldTable eff = effectiveFields(script, fields);
    CHECK_MESSAGE((eff.size() == script.fields().size() + 1), ("effectiveFields = static + 1 author field"));
    CHECK_MESSAGE((hasField(eff, "custom")), "effectiveFields includes author field");
    // Static fields (e.g. sourceCode) still present.
    CHECK_MESSAGE((hasField(eff, "sourceCode")), ("effectiveFields still includes static fields"));

    // A node with no author fields is unaffected.
    TimeSensor ts;
    CHECK_MESSAGE((effectiveFields(ts, fields).size() == ts.fields().size()), ("unrelated node's effectiveFields unchanged"));
  }

  // (3) author-field ROUTE wires + delivers end-to-end
  {
    Scene owner;
    auto &fields = *owner.authorFields;

    auto sensor = std::make_shared<TimeSensor>();
    auto script = std::make_shared<Script>();

    // Author inputOnly sink field on the Script: SFTime to match cycleTime.
    AuthorFieldDecl sink;
    sink.x3dName = "set_tick";
    sink.type = X3DFieldType::SFTime;
    sink.access = AccessType::InputOnly;
    fields.addAuthorField(*script, sink);

    Scene &scene = owner;
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

    X3DExecutionContext ctx(scene.authorFields);
    BridgeResult res = buildRoutes(scene, ctx);
    CHECK_MESSAGE((res.ok()), ("author-field ROUTE produced no rejections"));
    CHECK_MESSAGE((res.routesAdded == 1), ("author-field ROUTE wired (1 edge added)"));

    // Deliver a value exactly as the cascade's deliver() will (via the
    // synthesized sink set thunk) and read it back through the SAI surface.
    FieldTable eff = effectiveFields(*script, fields);
    const FieldInfo *sinkInfo = find(eff, "set_tick");
    CHECK_MESSAGE((sinkInfo && sinkInfo->set), ("author sink has a set thunk"));
    sinkInfo->set(*script, std::any(SFTime{42.0}));

    SaiContext sai(ctx, *script, "x3d-cpp-gen", "test");
    std::any got = sai.getField(script.get(), "set_tick");
    // inputOnly has no get thunk, so getField returns empty — but the value did
    // land in the store (proving the set path). Verify via the store directly.
    CHECK_MESSAGE((!got.has_value()), ("SAI getField on inputOnly author field returns empty (no get)"));
    CHECK_MESSAGE((std::any_cast<SFTime>(
              fields.getValue(*script, "set_tick")) == 42.0),
          "delivered value landed in the author-field store end-to-end");
  }

  return;
}
