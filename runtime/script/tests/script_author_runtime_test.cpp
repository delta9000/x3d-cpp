// script_author_runtime_test.cpp
// Task D (Phase 1) END-TO-END behavioral proof — the reason the whole
// file-authored-Script un-tabling effort exists: a Script whose interface is
// declared as AUTHOR fields (the S1 DynamicFieldStore) and whose body lives in
// Script.sourceCode (the CDATA destination, not url) actually RUNS through the
// real EcmaScriptBackend, and an author outputOnly write the handler makes
// drives a ROUTE to an observed field.
//
// Proves (ISO/IEC 19775-1 §29.2 + §3.3/§3.5 of the design):
//   1. ScriptSystem sources from Script.getSourceCode() when non-empty (the
//      CDATA path), falling back to the url inline scheme only when empty;
//   2. initialize() seeds JS globals from each author field's boxed
//      initialValue (initializeOnly/inputOutput), so the script reads its
//      authored defaults;
//   3. an author inputOnly/inputOutput event dispatches to the handler resolved
//      by author-field name (deliverInputEvent -> engine.invoke);
//   4. after the handler runs, the backend reads back the script's outputOnly /
//      inputOutput author-field globals into the DynamicFieldStore AND emits
//      them as cascade events carrying the triggering timestamp — which fan out
//      along a ROUTE from the author field to a real target node.
//
// Author fields are created directly via the Phase-0 DynamicFieldStore API; this
// test does NOT depend on the Phase-1 A/B/C codec readers.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "EcmaScriptBackend.hpp"
#include "ScriptSystem.hpp"

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DFieldAddress.hpp"

#include "Script.hpp"
#include "Transform.hpp"

#include <any>
#include <cmath>
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

bool nearly(double a, double b) { return std::fabs(a - b) < 1e-5; }

bool veq(const SFVec3f &a, float x, float y, float z) {
  return nearly(a.x, x) && nearly(a.y, y) && nearly(a.z, z);
}

// Declare one author field on a node via the Phase-0 store (encoding-agnostic).
AuthorFieldDecl decl(const std::string &name, X3DFieldType type,
                     AccessType access, std::any initial = {}) {
  AuthorFieldDecl d;
  d.x3dName = name;
  d.type = type;
  d.access = access;
  d.initialValue = std::move(initial);
  return d;
}

// ---------------------------------------------------------------------------
// T1: source from sourceCode (the CDATA destination), NOT url. A Script with an
//     empty url but a non-empty sourceCode must still load + initialize.
// ---------------------------------------------------------------------------
void testSourceFromSourceCode() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;
  Script script;
  // No url at all — only sourceCode carries the body (the reader-CDATA path).
  script.setSourceCode(
      "function initialize() { Browser.print('init-from-sourceCode'); }");
  script.setLoad(true);

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  sys->attach(&script, ctx);

  SaiContext *sai = sys->saiFor(&script);
  check(sai != nullptr, "T1: SAI surface exists for the sourceCode Script");
  check(sys->handleFor(&script) != kInvalidScriptHandle,
        "T1: Script with sourceCode (empty url) loaded a handle");
  check(sai && sai->log() == "init-from-sourceCode",
        "T1: initialize() ran from Script.sourceCode (CDATA path)");
}

// ---------------------------------------------------------------------------
// T2: sourceCode takes precedence over url when both are present.
// ---------------------------------------------------------------------------
void testSourceCodePrecedence() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;
  Script script;
  script.setUrl(MFString{"ecmascript:function initialize(){Browser.print('URL');}"});
  script.setSourceCode("function initialize(){Browser.print('SRC');}");
  script.setLoad(true);

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  sys->attach(&script, ctx);

  SaiContext *sai = sys->saiFor(&script);
  check(sai && sai->log() == "SRC",
        "T2: sourceCode preferred over url when both present");
}

// ---------------------------------------------------------------------------
// T3: url fallback still works when sourceCode is empty (no regression).
// ---------------------------------------------------------------------------
void testUrlFallback() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;
  Script script;
  script.setUrl(MFString{"ecmascript:function initialize(){Browser.print('URL');}"});
  script.setLoad(true);  // sourceCode empty

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  sys->attach(&script, ctx);

  SaiContext *sai = sys->saiFor(&script);
  check(sai && sai->log() == "URL",
        "T3: url inline scheme still decodes when sourceCode empty");
}

// ---------------------------------------------------------------------------
// T4: initialize() seeds JS globals from author-field initialValue. An
//     initializeOnly author field's boxed default is visible to the script.
// ---------------------------------------------------------------------------
void testInitialValueSeeding() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;
  Script script;
  script.setSourceCode(
      "function initialize() { Browser.print('seed=' + amount); }");
  script.setLoad(true);

  // Author field 'amount' (initializeOnly SFFloat) with boxed default 0.5.
  dynamicFieldStore().addAuthorField(
      script, decl("amount", X3DFieldType::SFFloat, AccessType::InitializeOnly,
                   std::any(SFFloat(0.5f))));

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  sys->attach(&script, ctx);

  SaiContext *sai = sys->saiFor(&script);
  check(sai && sai->log() == "seed=0.5",
        "T4: initialize() saw the author field's boxed initialValue as a global");
}

// ---------------------------------------------------------------------------
// T5: THE BEHAVIORAL PROOF — an author inputOnly event drives the handler, the
//     handler writes an author outputOnly field, and that write fans out along
//     a ROUTE to a real Transform's translation.
//
//     This is what the whole effort exists for: a file-authored Script (here
//     constructed programmatically via the same seams a reader would use) whose
//     author field, written by its own handler, drives the cascade.
// ---------------------------------------------------------------------------
void testAuthorOutputDrivesRoute() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  Script script;
  // directOutput so the handler's own outputOnly write is unconditionally fine
  // (it writes its OWN field; directOutput not strictly required for self, but
  // set TRUE to mirror a real interacting script).
  script.setDirectOutputUnchecked(true);
  script.setMustEvaluateUnchecked(true);  // eager: invoke on delivery
  script.setLoad(true);
  // Handler: on set_value, write the author outputOnly field 'position_changed'
  // (an SFVec3f global) to (v, v, v). The backend reads it back post-handler.
  script.setSourceCode(
      "function set_value(v, ts) {"
      "  position_changed = { x: v, y: v, z: v };"
      "}");

  // Author interface: set_value (inputOnly SFFloat), position_changed
  // (outputOnly SFVec3f).
  dynamicFieldStore().addAuthorField(
      script, decl("set_value", X3DFieldType::SFFloat, AccessType::InputOnly));
  dynamicFieldStore().addAuthorField(
      script,
      decl("position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly));

  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);
  check(sys->handleFor(&script) != kInvalidScriptHandle,
        "T5: author-field Script loaded from sourceCode");

  // ROUTE: script.position_changed -> target.translation. (Programmatic so the
  // test does not depend on a codec reader resolving the author endpoint.)
  ctx.addRoute(FieldAddress{&script, "position_changed"},
               FieldAddress{&target, "translation"});

  // Deliver an inputOnly event to the script's author field 'set_value' — this
  // is what a ROUTE into the Script would do. The handler runs, writes
  // position_changed, and the backend emits it as a cascade event.
  sys->deliverInputEvent(&script, "set_value", std::any(SFFloat(4.0f)),
                         X3DFieldType::SFFloat, 1.0);
  ctx.process();  // drain the cascade the author output produced

  check(veq(target.getTranslation(), 4, 4, 4),
        "T5: author outputOnly write drove the ROUTE to target.translation");

  // The store also holds the written-back author-field value.
  std::any stored = dynamicFieldStore().getValue(script, "position_changed");
  check(stored.has_value(), "T5: author outputOnly value written back to store");
  if (stored.has_value()) {
    auto v = std::any_cast<SFVec3f>(stored);
    check(veq(v, 4, 4, 4), "T5: store holds the (4,4,4) author output value");
  }
}

// ---------------------------------------------------------------------------
// T6: an inputOutput author field round-trips — the handler reads its seeded
//     value, mutates it, and the mutation is read back into the store + emitted.
// ---------------------------------------------------------------------------
void testInputOutputAuthorField() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  Script script;
  script.setDirectOutputUnchecked(true);
  script.setMustEvaluateUnchecked(true);
  script.setLoad(true);
  // 'count' is inputOutput SFInt32 seeded to 10; set_bump increments it.
  script.setSourceCode("function set_bump(v, ts) { count = count + v; }");

  dynamicFieldStore().addAuthorField(
      script, decl("count", X3DFieldType::SFInt32, AccessType::InputOutput,
                   std::any(SFInt32(10))));
  dynamicFieldStore().addAuthorField(
      script, decl("set_bump", X3DFieldType::SFInt32, AccessType::InputOnly));

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);

  sys->deliverInputEvent(&script, "set_bump", std::any(SFInt32(5)),
                         X3DFieldType::SFInt32, 1.0);
  ctx.process();

  std::any stored = dynamicFieldStore().getValue(script, "count");
  check(stored.has_value() && std::any_cast<SFInt32>(stored) == 15,
        "T6: inputOutput author field seeded(10)+bump(5) -> 15 read back to store");
}

// ---------------------------------------------------------------------------
// T7 (SCR-004): a script's prepareEvents() that writes an author outputOnly
//     field must have that write read back and emitted into the cascade — the
//     §29.2.5 "as if a built-in sensor node" purpose. The Browser.print probe
//     confirms prepareEvents actually ran (so a red is the missing readback,
//     not a missing call).
// ---------------------------------------------------------------------------
void testPrepareEventsAuthorOutput() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  Script script;
  script.setDirectOutputUnchecked(true);
  script.setMustEvaluateUnchecked(true);
  script.setLoad(true);
  script.setSourceCode(
      "function prepareEvents(ts) {"
      "  Browser.print('pe');"
      "  position_changed = { x: 7, y: 7, z: 7 };"
      "}");

  dynamicFieldStore().addAuthorField(
      script,
      decl("position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly));

  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);
  ctx.addRoute(FieldAddress{&script, "position_changed"},
               FieldAddress{&target, "translation"});

  ctx.tick(1.0); // drives the prepareEvents phase + drains the cascade

  SaiContext *sai = sys->saiFor(&script);
  check(sai && sai->log() == "pe", "T7: prepareEvents() ran during tick");
  check(veq(target.getTranslation(), 7, 7, 7),
        "T7: prepareEvents() author output read back + drove ROUTE (SCR-004)");
}

// ---------------------------------------------------------------------------
// T8 (SCR-006): SFMatrix author fields marshal both directions. A script that
//     copies an inputOnly SFMatrix4f to an outputOnly SFMatrix4f must round-trip
//     all 16 components; previously matrices fell through to JS undefined.
// ---------------------------------------------------------------------------
void testMatrixFieldMarshalling() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  Script script;
  script.setDirectOutputUnchecked(true);
  script.setMustEvaluateUnchecked(true);
  script.setLoad(true);
  script.setSourceCode("function set_m(v, ts) { m_changed = v; }");

  dynamicFieldStore().addAuthorField(
      script, decl("set_m", X3DFieldType::SFMatrix4f, AccessType::InputOnly));
  dynamicFieldStore().addAuthorField(
      script,
      decl("m_changed", X3DFieldType::SFMatrix4f, AccessType::OutputOnly));

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);

  SFMatrix4f in{};
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      in.matrix[r][c] = static_cast<float>(r * 4 + c + 1); // 1..16

  sys->deliverInputEvent(&script, "set_m", std::any(in), X3DFieldType::SFMatrix4f,
                         1.0);
  ctx.process();

  std::any stored = dynamicFieldStore().getValue(script, "m_changed");
  check(stored.has_value(), "scr006: SFMatrix4f author output written back");
  bool ok = stored.has_value();
  if (ok) {
    auto m = std::any_cast<SFMatrix4f>(stored);
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        if (!nearly(m.matrix[r][c], in.matrix[r][c])) ok = false;
  }
  check(ok, "scr006: SFMatrix4f round-trips through JS (1..16 preserved)");
}

// ---------------------------------------------------------------------------
// T9 (SCR-003): a script's eventsProcessed() that writes an author outputOnly
//     field must have that write read back + emitted (§29.2.4). mustEvaluate
//     FALSE defers the input to the batch so eventsProcessed fires this tick.
// ---------------------------------------------------------------------------
void testEventsProcessedAuthorOutput() {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  Script script;
  script.setDirectOutputUnchecked(true);
  script.setMustEvaluateUnchecked(false); // defer -> batch -> eventsProcessed
  script.setLoad(true);
  script.setSourceCode(
      "function set_trigger(v, ts) {}"
      "function eventsProcessed() { out_changed = { x: 9, y: 9, z: 9 }; }");

  dynamicFieldStore().addAuthorField(
      script, decl("set_trigger", X3DFieldType::SFFloat, AccessType::InputOnly));
  dynamicFieldStore().addAuthorField(
      script, decl("out_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly));

  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);
  ctx.addRoute(FieldAddress{&script, "out_changed"},
               FieldAddress{&target, "translation"});

  sys->deliverInputEvent(&script, "set_trigger", std::any(SFFloat(1.0f)),
                         X3DFieldType::SFFloat, 1.0);
  ctx.tick(1.0); // flush deferred -> eventsProcessed -> readback -> ROUTE

  check(veq(target.getTranslation(), 9, 9, 9),
        "scr003: eventsProcessed() author output read back + drove ROUTE");
}

} // namespace

int main() {
  testSourceFromSourceCode();
  testSourceCodePrecedence();
  testUrlFallback();
  testInitialValueSeeding();
  testAuthorOutputDrivesRoute();
  testInputOutputAuthorField();
  testPrepareEventsAuthorOutput();
  testMatrixFieldMarshalling();
  testEventsProcessedAuthorOutput();

  dynamicFieldStore().clear();  // leave the global store clean for other tests
  if (failures == 0) {
    std::cout << "ALL SCRIPT AUTHOR RUNTIME TESTS PASSED\n";
    return 0;
  }
  std::cerr << failures << " script author runtime test(s) FAILED\n";
  return 1;
}
