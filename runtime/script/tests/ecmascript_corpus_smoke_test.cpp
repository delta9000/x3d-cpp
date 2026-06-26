// ecmascript_corpus_smoke_test.cpp
// Track-B corpus smoke (M4 / U4): drive the whole pipeline
//   parse (XML) -> expand (PROTO) -> attach ScriptSystem(EcmaScriptBackend)
//   -> tick
// over (a) an authored "known scene" with a deterministic Browser.print output,
// and (b) a handful of real Script-bearing .x3d from the testdata corpus, just
// to prove no crash on real-world content.
//
// KNOWN-SCENE NOTE: the inline script body is carried in the Script `url`
// attribute (ecmascript:...), which the XmlReader parses into the url field.
// Real corpus scenes carry their script body in a <![CDATA[...]]> block AND
// declare author <field>s; the reader does not yet capture either (the S1
// dynamic-field foundation is unimplemented — see the limitation logged in the
// task report). Those scenes therefore parse to an inert Script (empty url ->
// no handle) and the smoke proves robustness (no crash), not script execution.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "EcmaScriptBackend.hpp"
#include "ScriptSystem.hpp"

#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "Script.hpp"

#include "X3DScene.hpp"
#include "X3DProtoExpand.hpp"
#include "parse/X3DProtoResolver.hpp"
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

// Parse XML text -> expand protos -> return the scene (kept alive by caller).
runtime::Scene parseAndExpand(const std::string &xml) {
  x3d::codec::XmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(xml);
  std::vector<ProtoWarning> warnings;
  expandScene(doc.scene, x3d::codec::noopProtoResolver, "", warnings);
  return std::move(doc.scene);
}

std::string slurp(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

} // namespace

int main() {
  // -------------------------------------------------------------------------
  // KNOWN SCENE: a Script whose initialize() prints a deterministic string,
  // and an enabled() handler that toggles a global. Body lives in url= so the
  // reader captures it. Drive it through the real ScriptSystem + cascade.
  // -------------------------------------------------------------------------
  {
    // The url is an MFString — its single inline entry must be double-quoted so
    // the whole script body is one element (unquoted whitespace would split it
    // into separate url entries). The XML attribute is delimited by " (\"), the
    // one MFString element by &quot;, and the JS string literals by &apos; so
    // nothing collides with the C++/XML/MFString quoting layers.
    const std::string xml =
        "<X3D profile='Interactive' version='3.3'>"
        "  <Scene>"
        "    <Group>"
        "      <Script DEF='S' url=\"&quot;"
        "ecmascript:"
        " function initialize() { Browser.print(&apos;init@&apos; + Browser.getName()); }"
        " function ping(value, ts) { Browser.print(&apos;ping=&apos; + value + &apos;@&apos; + ts); }"
        "&quot;\"/>"
        "    </Group>"
        "  </Scene>"
        "</X3D>";

    runtime::Scene scene = parseAndExpand(xml);

    std::vector<Script *> scripts;
    for (auto &root : scene.rootNodes) collectScripts(root.get(), scripts);
    check(scripts.size() == 1, "known scene: one Script parsed from the graph");

    X3DExecutionContext ctx;
    auto backend = std::make_shared<EcmaScriptBackend>();
    auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
    ctx.addScriptSystem(sys);
    for (Script *s : scripts) sys->attach(s, ctx);

    // initialize() ran at attach time; verify its print landed.
    SaiContext *sai = sys->saiFor(scripts[0]);
    check(sai != nullptr, "known scene: SAI surface exists for the Script");
    check(sai && sai->log() == "init@x3d-cpp-gen",
          "known scene: initialize() printed via Browser (end-to-end)");

    // A few ticks must not crash (prepareEvents/eventsProcessed phases).
    ctx.tick(1.0);
    ctx.tick(2.0);
    check(true, "known scene: ticks run without crash");

    // Deliver an inputOnly-style event to the 'ping' handler; it prints
    // value@timestamp, proving marshalling through the real ScriptSystem seam.
    // mustEvaluate defaults FALSE, so the event is deferred until the batch
    // flush — a tick drives runEventsProcessed() (post-cascade) which invokes it.
    sys->deliverInputEvent(scripts[0], "ping", std::any(SFFloat(0.5f)),
                           X3DFieldType::SFFloat, 3.0);
    ctx.tick(3.0);
    check(sai && sai->log() == "init@x3d-cpp-genping=0.5@3",
          "known scene: ping handler received marshalled (value, timestamp)");
  }

  // -------------------------------------------------------------------------
  // REAL CORPUS: parse -> expand -> attach -> tick a handful of Script-bearing
  // .x3d files; assert no crash and that the pipeline runs to completion.
  // -------------------------------------------------------------------------
  {
    // Corpus root comes from the X3D_CORPUS_DIR env var (the archive is not
    // bundled). Unset => base is empty => every slurp misses => graceful skip.
    const char *corpusEnv = std::getenv("X3D_CORPUS_DIR");
    const std::string base =
        corpusEnv ? std::string(corpusEnv) +
                        "/x3d-code/www.web3d.org/x3d/content/examples/Savage/"
                  : std::string();
    const std::vector<std::string> files = {
        "AircraftFixedWing/F18BlueAngelUnitedStates/CanopyExample.x3d",
        "Weapons/SmallArms/RifleM24Example.x3d",
        "AircraftFixedWing/F18BlueAngelUnitedStates/WheelsFrontExample.x3d",
    };

    int ran = 0;
    for (const std::string &rel : files) {
      std::string xml = slurp(base + rel);
      if (xml.empty()) {
        std::cout << "skip (missing corpus file): " << rel << "\n";
        continue;
      }
      runtime::Scene scene = parseAndExpand(xml);
      std::vector<Script *> scripts;
      for (auto &root : scene.rootNodes) collectScripts(root.get(), scripts);

      X3DExecutionContext ctx;
      auto backend = std::make_shared<EcmaScriptBackend>();
      auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
      ctx.addScriptSystem(sys);
      for (Script *s : scripts) sys->attach(s, ctx);
      ctx.tick(0.0);
      ctx.tick(1.0);
      ctx.tick(2.0);
      ++ran;
      std::cout << "corpus ok (" << scripts.size() << " Script(s), no crash): "
                << rel << "\n";
    }
    // The conformance corpus is an optional local asset — it cannot be
    // committed (licensing) and is absent in CI / fresh checkouts. When present
    // we prove no-crash on real Script-bearing scenes; when absent we skip
    // without failing. The known scene above already exercises the full
    // parse -> attach -> tick -> event path deterministically, so coverage is
    // preserved. Mirrors script_corpus_e2e + corpus_sweep (graceful skip).
    if (ran == 0)
      std::cout << "skip (no corpus files present): real-corpus robustness "
                   "smoke not exercised\n";
    else
      std::cout << "corpus smoke: " << ran
                << " real Script-bearing scene(s) ran without crash\n";
  }

  if (failures == 0) {
    std::cout << "All ecmascript corpus smoke tests passed.\n";
    return 0;
  }
  std::cerr << failures << " corpus smoke test(s) FAILED.\n";
  return 1;
}
