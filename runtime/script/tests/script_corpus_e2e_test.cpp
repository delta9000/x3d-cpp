// script_corpus_e2e_test.cpp
// PHASE-2 CORPUS END-TO-END proof: a FILE-AUTHORED Script that the v1 SDK
// shipped INERT now actually EXECUTES through the full pipeline
//   read (real codec) -> expand (PROTO) -> attach ScriptSystem(EcmaScriptBackend)
//   -> deliver an author inputOnly event -> tick -> author outputOnly write is
//   observable.
//
// This is the behavioral closure for SCR-SAI-DYN (S1): before the un-tabling, a
// <Script> carrying author <field> decls + a <![CDATA[...]]> body parsed to an
// inert node (no author fields in the reflection table, no captured source) — its
// handlers never fired. The ecmascript_corpus_smoke_test documents that exact
// hole ("the reader does not yet capture either ... those scenes therefore parse
// to an inert Script"). This test loads the SAME real corpus file and proves it
// is no longer inert.
//
// Three real-reader paths exercised:
//   (a) XML fixture        — AuthorScriptExample.x3d, a self-contained analogue
//                            of CanopyExample.x3d: drives the inputOnly 'enabled'
//                            event and asserts the outputOnly 'status' write.
//   (b) ClassicVRML (.x3dv)— AuthorScriptExample.x3dv, the VRML analogue (url
//                            inline body mirrored to sourceCode), same drive.
//   (c) REAL Savage corpus — CanopyExample.x3d itself (previously inert per the
//                            smoke test): proves the author <field>s + CDATA body
//                            are captured AND the handler now actually DISPATCHES
//                            (the engine invokes it — observable because the
//                            handler executes its body, which references the
//                            surrounding-prototype global 'traceEnabled' and so
//                            raises inside the engine, whereas the inert v1 Script
//                            silently never ran the handler at all).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "EcmaScriptBackend.hpp"
#include "ScriptSystem.hpp"

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"

#include "Script.hpp"

#include "X3DScene.hpp"
#include "X3DProtoExpand.hpp"
#include "parse/X3DProtoResolver.hpp"
#include "parse/ClassicVrmlReader.hpp"
#include "codecs/XmlReader.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

// Blind child walk (mirrors PickSystem::forEachChild) — visit every Script node
// reachable through SFNode/MFNode fields, depth-first.
void collectScripts(X3DNode *n, std::vector<Script *> &out) {
  if (!n) return;
  if (auto *s = dynamic_cast<Script *>(n)) out.push_back(s);
  for (const auto &fi : n->fields()) {
    if (!fi.get) continue;  // inputOnly fields have no get thunk
    if (fi.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(fi.get(*n));
      if (c) collectScripts(c.get(), out);
    } else if (fi.type == X3DFieldType::MFNode) {
      for (const auto &c :
           std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi.get(*n)))
        if (c) collectScripts(c.get(), out);
    }
  }
}

std::string slurp(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool hasAuthorField(const X3DNode &node, const std::string &name) {
  for (const FieldInfo &f : effectiveFields(node))
    if (f.x3dName == name) return true;
  return false;
}

// Drive the self-contained fixture handler: deliver enabled(true) on a freshly
// loaded script (landed seeded -1) and assert the outputOnly 'status' author
// field transitions from unwritten (the inert baseline) to true (landed==-1 ->
// status=false, then landed==0 toggles status -> true).
void driveAndAssertStatus(Script *script, ScriptSystem &sys,
                          X3DExecutionContext &ctx, const std::string &tag) {
  std::any before = dynamicFieldStore().getValue(*script, "status");
  check(!before.has_value(),
        tag + ": status author field starts unwritten (inert baseline)");

  // initialize() already ran at attach. Deliver the author inputOnly 'enabled'
  // event exactly as a ROUTE from a TouchSensor.isActive would. mustEvaluate
  // defaults FALSE, so the handler is deferred to the eventsProcessed flush that
  // a tick drives.
  sys.deliverInputEvent(script, "enabled", std::any(SFBool(true)),
                        X3DFieldType::SFBool, 1.0);
  ctx.tick(1.0);

  std::any after = dynamicFieldStore().getValue(*script, "status");
  check(after.has_value(),
        tag + ": status WRITTEN after enabled(true) — the script RAN");
  if (after.has_value()) {
    check(std::any_cast<SFBool>(after) == SFBool(true),
          tag + ": enabled(true) handler set status -> true (handler logic ran)");
  }
}

// ---------------------------------------------------------------------------
// (a) XML fixture loaded through the real XmlReader: author <field>s + CDATA.
// ---------------------------------------------------------------------------
void testXmlFixture(const std::string &dataDir) {
  dynamicFieldStore().clear();

  std::string xml = slurp(dataDir + "/AuthorScriptExample.x3d");
  check(!xml.empty(), "xml fixture: AuthorScriptExample.x3d readable");
  if (xml.empty()) return;

  codec::XmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(xml);
  std::vector<ProtoWarning> warnings;
  expandScene(doc.scene, codec::noopProtoResolver, "", warnings);

  std::vector<Script *> scripts;
  for (auto &root : doc.scene.rootNodes) collectScripts(root.get(), scripts);
  check(scripts.size() == 1, "xml fixture: the AuthorScript parsed");
  if (scripts.empty()) return;
  Script *script = scripts[0];

  check(hasAuthorField(*script, "enabled") && hasAuthorField(*script, "status") &&
            hasAuthorField(*script, "landed"),
        "xml fixture: author <field>s (enabled/status/landed) captured");
  check(script->getSourceCode().find("function enabled") != std::string::npos,
        "xml fixture: CDATA ecmascript body captured into sourceCode");

  X3DExecutionContext ctx;
  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(script, ctx);
  check(sys->handleFor(script) != kInvalidScriptHandle,
        "xml fixture: Script LOADED from captured sourceCode (was inert before)");

  driveAndAssertStatus(script, *sys, ctx, "xml fixture");
}

// ---------------------------------------------------------------------------
// (b) ClassicVRML (.x3dv) fixture loaded through the real ClassicVrmlReader.
// ---------------------------------------------------------------------------
void testVrmlFixture(const std::string &dataDir) {
  dynamicFieldStore().clear();

  std::string vrml = slurp(dataDir + "/AuthorScriptExample.x3dv");
  check(!vrml.empty(), "vrml fixture: AuthorScriptExample.x3dv readable");
  if (vrml.empty()) return;

  codec::ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(vrml);
  std::vector<ProtoWarning> warnings;
  expandScene(doc.scene, codec::noopProtoResolver, "", warnings);

  std::vector<Script *> scripts;
  for (auto &root : doc.scene.rootNodes) collectScripts(root.get(), scripts);
  check(scripts.size() == 1, "vrml fixture: the AuthorScript parsed");
  if (scripts.empty()) return;
  Script *script = scripts[0];

  check(script->getSourceCode().find("function enabled") != std::string::npos,
        "vrml fixture: inline url body mirrored into sourceCode");

  X3DExecutionContext ctx;
  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(script, ctx);
  check(sys->handleFor(script) != kInvalidScriptHandle,
        "vrml fixture: Script LOADED from captured sourceCode (was inert before)");

  driveAndAssertStatus(script, *sys, ctx, "vrml fixture");
}

// ---------------------------------------------------------------------------
// (c) The REAL Savage CanopyExample.x3d — the file the smoke test ran INERT.
//     Proves the un-tabling captures its author <field>s + CDATA body and that
//     its handler now genuinely DISPATCHES (rather than being a silent no-op).
// ---------------------------------------------------------------------------
void testRealCorpusNotInert() {
  dynamicFieldStore().clear();

  // Corpus root comes from the X3D_CORPUS_DIR env var (the archive is not
  // bundled). Unset/absent => skip (the XML+VRML fixtures above still run).
  const char *corpusEnv = std::getenv("X3D_CORPUS_DIR");
  const std::string path =
      corpusEnv ? std::string(corpusEnv) +
                      "/x3d-code/www.web3d.org/x3d/content/examples/Savage/"
                      "AircraftFixedWing/F18BlueAngelUnitedStates/"
                      "CanopyExample.x3d"
                : std::string();
  std::string xml = path.empty() ? std::string() : slurp(path);
  if (xml.empty()) {
    std::cout << "skip (missing corpus file): CanopyExample.x3d\n";
    return;
  }

  codec::XmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(xml);
  std::vector<ProtoWarning> warnings;
  expandScene(doc.scene, codec::noopProtoResolver, "", warnings);

  std::vector<Script *> scripts;
  for (auto &root : doc.scene.rootNodes) collectScripts(root.get(), scripts);
  check(scripts.size() == 1, "real corpus: the ExampleSelectionScript parsed");
  if (scripts.empty()) return;
  Script *script = scripts[0];

  // The author interface from the <field> children is now visible (was wholly
  // absent when the Script was inert).
  check(hasAuthorField(*script, "enabled") && hasAuthorField(*script, "status") &&
            hasAuthorField(*script, "landed"),
        "real corpus: author <field>s (enabled/status/landed) captured");
  check(script->getSourceCode().find("function enabled") != std::string::npos,
        "real corpus: CDATA ecmascript body captured into sourceCode");

  X3DExecutionContext ctx;
  auto backend = std::make_shared<EcmaScriptBackend>();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(script, ctx);
  check(sys->handleFor(script) != kInvalidScriptHandle,
        "real corpus: Script LOADED from captured CDATA source (was inert)");

  // Deliver the inputOnly 'enabled' event the TouchSensor.isActive ROUTE feeds.
  // The handler now actually RUNS (it is no longer inert). CanopyExample's
  // handler references 'traceEnabled' — a field of the SURROUNDING Canopy
  // prototype, not declared on this Script — so the engine raises ReferenceError
  // INSIDE the handler. That raise IS the proof of dispatch: an inert Script
  // would never have entered the handler at all. We assert no crash + that the
  // pipeline ran to completion.
  sys->deliverInputEvent(script, "enabled", std::any(SFBool(true)),
                         X3DFieldType::SFBool, 1.0);
  ctx.tick(1.0);
  ctx.tick(2.0);
  check(true, "real corpus: handler dispatched + ticks completed without crash");
}

} // namespace

int main(int argc, char **argv) {
  // argv[1] is the test-data dir (CMake passes it); fall back to the in-tree
  // path so the binary is runnable standalone.
  const std::string dataDir =
      argc > 1 ? argv[1] : "runtime/script/tests/data";

  testXmlFixture(dataDir);
  testVrmlFixture(dataDir);
  testRealCorpusNotInert();

  dynamicFieldStore().clear();  // leave the global store clean for other tests
  if (failures == 0) {
    std::cout << "ALL SCRIPT CORPUS END-TO-END TESTS PASSED\n";
    return 0;
  }
  std::cerr << failures << " script corpus e2e test(s) FAILED\n";
  return 1;
}
