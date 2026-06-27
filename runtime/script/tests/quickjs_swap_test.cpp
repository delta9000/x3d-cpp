// quickjs_swap_test.cpp
// UNIT 3 — THE GENERICITY PROOF for the ScriptEngine seam.
//
// A behavioral SWAP-TEST: the SAME authored Script fixtures are driven through
// the full runtime (Script node -> ScriptSystem -> X3DExecutionContext cascade)
// under BOTH ScriptEngine backends — the reference Duktape EcmaScriptBackend and
// the new QuickJsBackend — over an IDENTICAL fixed tick schedule, and the
// OBSERVABLE behavior is captured and compared tick-for-tick. If both backends
// produce an identical observable trace for every fixture, the seam is proven
// genuinely backend-agnostic: a backend is a constructor argument, nothing more.
//
// WHAT "OBSERVABLE" MEANS HERE (Risk 2 of the design):
//   The trace is the runtime's OWN field representation, never JS-internal state
//   and never a raw JS string. Two runtime channels are snapshotted after each
//   tick:
//     (1) the script's author outputOnly/inputOutput field VALUES, read out of
//         the DynamicFieldStore — these are exactly what the backend marshalled
//         back across the seam (post-toValue) and posted into the cascade; and
//     (2) the VALUES on ROUTE-target nodes (a real Transform's translation,
//         etc.), read via the generated typed accessors — i.e. what the cascade
//         actually delivered downstream of the script's writes.
//   Both are stringified with CANONICAL numeric formatting (a fixed-precision
//   round) so float-formatting differences between engines can never masquerade
//   as a behavioral divergence: we compare the marshalled field VALUE, with the
//   coercion kept inside each backend.
//
// FIXTURES (each runs through both backends):
//   F1  Author output drives a ROUTE — set_value(v) writes position_changed =
//       (v,v,v) which a ROUTE carries to a Transform.translation. (Mirrors the
//       script_author_runtime_test T5 proof; the simplest cross-seam write.)
//   F2  inputOutput round-trip — count seeded 10, set_bump(v) does count+=v over
//       several ticks: a STATEFUL accumulation, so the trace is a real running
//       sequence (10 -> 15 -> 23 -> ...), not a constant.
//   F3  NON-TRIVIAL COMPUTED fixture (anti-vacuous): set_fraction(f) computes a
//       linear interpolation  value_changed = lo + f*(hi-lo)  over 3 control
//       fractions, with lo/hi seeded as author initializeOnly fields. The trace
//       is a genuine computed numeric sequence; the test ASSERTS the expected
//       interpolated values appear, so a backend that silently produced zeros
//       (or never ran the handler) would fail the anti-vacuous guard, not pass.
//   F4  The authored CDATA corpus fixture (data/AuthorScriptExample.x3d) loaded
//       through the REAL XmlReader -> expand -> attach path (the file the corpus
//       e2e test proves runs): drives enabled(true) and snapshots the outputOnly
//       'status' + initializeOnly 'landed' author fields. A real file-authored
//       Script, end to end, under both backends.
//
// A divergence between backends on ANY (fixture, tick, field) is a genericity
// leak and is reported with the exact field + tick + both values.
//
// Built ONLY when X3D_CPP_BUILD_QUICKJS=ON (when ON, Duktape is also available
// since x3d_ecmascript_backend is always built). ctest name: x3d_quickjs_swap.
//
// Exit code 0 on success; nonzero on any failed assertion / divergence.

#include "EcmaScriptBackend.hpp"
#include "QuickJsBackend.hpp"
#include "ScriptSystem.hpp"

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DFieldAddress.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include "X3DScene.hpp"
#include "X3DProtoExpand.hpp"
#include "parse/X3DProtoResolver.hpp"
#include "codecs/XmlReader.hpp"

#include <any>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
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

// --------------------------------------------------------------------------
// Backend factory list. With X3D_CPP_BUILD_QUICKJS=ON, BOTH the Duktape backend
// (x3d_ecmascript_backend — always built) and the QuickJS backend are linked.
// Each factory returns a fresh engine so no state leaks between fixtures.
// --------------------------------------------------------------------------
struct BackendFactory {
  std::string name;
  std::function<std::shared_ptr<ScriptEngine>()> make;
};

const std::vector<BackendFactory> kBackends = {
    {"duktape", [] { return std::make_shared<EcmaScriptBackend>(); }},
    {"quickjs", [] { return std::make_shared<QuickJsBackend>(); }},
};

// --------------------------------------------------------------------------
// CANONICAL stringification of a boxed field value (Risk 2). Floats/doubles are
// rounded to a fixed precision and formatted identically regardless of engine,
// so engine-specific JS number->string coercion can never show up as a diff.
// We compare runtime field VALUES, not JS strings.
// --------------------------------------------------------------------------
std::string num(double v) {
  // Round to 6 decimals, then trim trailing zeros so 4.0 and 4.000000 match.
  std::ostringstream ss;
  // Normalize -0 to 0.
  if (v == 0.0) v = 0.0;
  ss << std::fixed << std::setprecision(6) << v;
  std::string s = ss.str();
  auto dot = s.find('.');
  if (dot != std::string::npos) {
    std::size_t last = s.find_last_not_of('0');
    if (last == dot) last = dot - 1;  // strip the dot too if all-zero frac
    s.erase(last + 1);
  }
  if (s == "-0") s = "0";
  return s;
}

std::string canon(const std::any &v, X3DFieldType type) {
  if (!v.has_value()) return "<unset>";
  switch (type) {
  case X3DFieldType::SFBool:
    return std::any_cast<SFBool>(v) ? "true" : "false";
  case X3DFieldType::SFInt32:
    return std::to_string(std::any_cast<SFInt32>(v));
  case X3DFieldType::SFFloat:
    return num(std::any_cast<SFFloat>(v));
  case X3DFieldType::SFDouble:
    return num(std::any_cast<SFDouble>(v));
  case X3DFieldType::SFTime:
    return num(std::any_cast<SFTime>(v));
  case X3DFieldType::SFString:
    return "\"" + std::any_cast<SFString>(v) + "\"";
  case X3DFieldType::SFVec3f: {
    auto a = std::any_cast<SFVec3f>(v);
    return "(" + num(a.x) + "," + num(a.y) + "," + num(a.z) + ")";
  }
  case X3DFieldType::SFVec2f: {
    auto a = std::any_cast<SFVec2f>(v);
    return "(" + num(a.x) + "," + num(a.y) + ")";
  }
  case X3DFieldType::SFColor: {
    auto a = std::any_cast<SFColor>(v);
    return "(" + num(a.r) + "," + num(a.g) + "," + num(a.b) + ")";
  }
  default:
    return "<type " + std::to_string(static_cast<int>(type)) + ">";
  }
}

// --------------------------------------------------------------------------
// A watched field: read its value either from the DynamicFieldStore (an author
// field on a Script) or via the runtime reflection accessor (a ROUTE target).
// --------------------------------------------------------------------------
struct Watch {
  std::string label;          // stable name in the trace
  X3DNode *node;              // the node whose field we read
  std::string field;         // x3d field name
  X3DFieldType type;         // for canonical formatting
  bool fromStore;            // true: read DynamicFieldStore; false: reflection
};

std::string readWatch(const Watch &w) {
  std::any v;
  if (w.fromStore) {
    v = dynamicFieldStore().getValue(*w.node, w.field);
  } else {
    // Reflection read via the node's field table (the typed get thunk).
    for (const auto &fi : w.node->fields()) {
      if (fi.x3dName == w.field && fi.get) {
        v = fi.get(*w.node);
        break;
      }
    }
  }
  return w.label + "=" + canon(v, w.type);
}

// A trace is a vector of per-tick lines; each line is the watched fields joined.
using Trace = std::vector<std::string>;

std::string snapshot(const std::vector<Watch> &watches) {
  std::string line;
  for (std::size_t i = 0; i < watches.size(); ++i) {
    if (i) line += " | ";
    line += readWatch(watches[i]);
  }
  return line;
}

// Compare two traces tick-for-tick; report the exact diverging tick+content.
bool tracesEqual(const std::string &fixture, const Trace &a, const Trace &b) {
  bool eq = true;
  if (a.size() != b.size()) {
    std::cerr << "DIVERGE [" << fixture << "]: tick count duktape=" << a.size()
              << " quickjs=" << b.size() << "\n";
    eq = false;
  }
  std::size_t n = std::min(a.size(), b.size());
  for (std::size_t t = 0; t < n; ++t) {
    if (a[t] != b[t]) {
      std::cerr << "DIVERGE [" << fixture << "] tick " << t << ":\n"
                << "    duktape: " << a[t] << "\n"
                << "    quickjs: " << b[t] << "\n";
      eq = false;
    }
  }
  return eq;
}

bool traceNonTrivial(const Trace &t) {
  for (const auto &line : t)
    if (line.find("<unset>") == std::string::npos &&
        line.find("=0") == std::string::npos)
      return true;
  // Allow lines that merely contain a non-zero somewhere.
  for (const auto &line : t)
    if (line.find_first_of("123456789") != std::string::npos &&
        line.find("<unset>") == std::string::npos)
      return true;
  return false;
}

// --------------------------------------------------------------------------
// F1: author output drives a ROUTE. set_value(v) -> position_changed=(v,v,v) ->
// ROUTE -> Transform.translation. Driven over a fixed schedule of input values.
// --------------------------------------------------------------------------
Trace runF1(const BackendFactory &bf) {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  auto script = std::make_unique<Script>();
  script->setDirectOutputUnchecked(true);
  script->setMustEvaluateUnchecked(true);
  script->setLoad(true);
  script->setSourceCode(
      "function set_value(v, ts) {"
      "  position_changed = { x: v, y: v, z: v };"
      "}");
  dynamicFieldStore().addAuthorField(
      *script,
      AuthorFieldDecl{"set_value", X3DFieldType::SFFloat, AccessType::InputOnly,
                      {}});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"position_changed", X3DFieldType::SFVec3f,
                               AccessType::OutputOnly, {}});

  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto backend = bf.make();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(script.get(), ctx);
  ctx.addRoute(FieldAddress{script.get(), "position_changed"},
               FieldAddress{&target, "translation"});

  std::vector<Watch> watches = {
      {"pos", script.get(), "position_changed", X3DFieldType::SFVec3f, true},
      {"target", &target, "translation", X3DFieldType::SFVec3f, false},
  };

  Trace trace;
  const double inputs[] = {4.0, -2.5, 0.0, 7.25};
  double now = 1.0;
  for (double in : inputs) {
    sys->deliverInputEvent(script.get(), "set_value", std::any(SFFloat(in)),
                           X3DFieldType::SFFloat, now);
    ctx.tick(now);
    trace.push_back(snapshot(watches));
    now += 1.0;
  }
  dynamicFieldStore().clear();
  return trace;
}

// --------------------------------------------------------------------------
// F2: inputOutput STATEFUL accumulation. count seeded 10; set_bump(v): count+=v.
// --------------------------------------------------------------------------
Trace runF2(const BackendFactory &bf) {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  auto script = std::make_unique<Script>();
  script->setDirectOutputUnchecked(true);
  script->setMustEvaluateUnchecked(true);
  script->setLoad(true);
  script->setSourceCode("function set_bump(v, ts) { count = count + v; }");
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"count", X3DFieldType::SFInt32,
                               AccessType::InputOutput, std::any(SFInt32(10))});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"set_bump", X3DFieldType::SFInt32,
                               AccessType::InputOnly, {}});

  auto backend = bf.make();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(script.get(), ctx);

  std::vector<Watch> watches = {
      {"count", script.get(), "count", X3DFieldType::SFInt32, true},
  };

  Trace trace;
  const int bumps[] = {5, 8, -3, 100};
  double now = 1.0;
  for (int b : bumps) {
    sys->deliverInputEvent(script.get(), "set_bump", std::any(SFInt32(b)),
                           X3DFieldType::SFInt32, now);
    ctx.tick(now);
    trace.push_back(snapshot(watches));
    now += 1.0;
  }
  dynamicFieldStore().clear();
  return trace;
}

// --------------------------------------------------------------------------
// F3: NON-TRIVIAL COMPUTED fixture — linear interpolation in the handler.
//   value_changed = lo + f * (hi - lo), with lo=2, hi=10 seeded as author
//   initializeOnly fields. Driven over fractions {0, 0.25, 0.5, 0.75, 1.0}
//   => expected outputs {2, 4, 6, 8, 10}. The handler also writes the SFVec3f
//   pos_changed=(value,value,value) to drive a ROUTE so the computed value is
//   observed BOTH as the script author field AND on a downstream Transform.
// --------------------------------------------------------------------------
Trace runF3(const BackendFactory &bf) {
  dynamicFieldStore().clear();
  X3DExecutionContext ctx;

  auto script = std::make_unique<Script>();
  script->setDirectOutputUnchecked(true);
  script->setMustEvaluateUnchecked(true);
  script->setLoad(true);
  script->setSourceCode(
      "function set_fraction(f, ts) {"
      "  var v = lo + f * (hi - lo);"
      "  value_changed = v;"
      "  pos_changed = { x: v, y: v, z: v };"
      "}");
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"lo", X3DFieldType::SFFloat,
                               AccessType::InitializeOnly, std::any(SFFloat(2.0f))});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"hi", X3DFieldType::SFFloat,
                               AccessType::InitializeOnly, std::any(SFFloat(10.0f))});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"set_fraction", X3DFieldType::SFFloat,
                               AccessType::InputOnly, {}});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"value_changed", X3DFieldType::SFFloat,
                               AccessType::OutputOnly, {}});
  dynamicFieldStore().addAuthorField(
      *script, AuthorFieldDecl{"pos_changed", X3DFieldType::SFVec3f,
                               AccessType::OutputOnly, {}});

  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto backend = bf.make();
  auto sys = std::make_shared<ScriptSystem>(backend, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(script.get(), ctx);
  ctx.addRoute(FieldAddress{script.get(), "pos_changed"},
               FieldAddress{&target, "translation"});

  std::vector<Watch> watches = {
      {"val", script.get(), "value_changed", X3DFieldType::SFFloat, true},
      {"target", &target, "translation", X3DFieldType::SFVec3f, false},
  };

  Trace trace;
  const float fracs[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
  double now = 1.0;
  for (float f : fracs) {
    sys->deliverInputEvent(script.get(), "set_fraction", std::any(SFFloat(f)),
                           X3DFieldType::SFFloat, now);
    ctx.tick(now);
    trace.push_back(snapshot(watches));
    now += 1.0;
  }
  dynamicFieldStore().clear();
  return trace;
}

std::string slurp(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void collectScripts(X3DNode *n, std::vector<Script *> &out) {
  if (!n) return;
  if (auto *s = dynamic_cast<Script *>(n)) out.push_back(s);
  for (const auto &fi : n->fields()) {
    if (!fi.get) continue;
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

// --------------------------------------------------------------------------
// F4: the authored CDATA corpus fixture, loaded through the REAL XmlReader ->
// expand -> attach path (the same file the corpus e2e test proves runs). We own
// the scene per-backend (a fresh parse) so the two runs are independent. Drives
// enabled(true) twice and snapshots the outputOnly 'status' + 'landed' fields.
// --------------------------------------------------------------------------
Trace runF4(const BackendFactory &bf, const std::string &dataDir, bool &loaded) {
  loaded = false;
  dynamicFieldStore().clear();

  std::string xml = slurp(dataDir + "/AuthorScriptExample.x3d");
  if (xml.empty()) return {};

  codec::XmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(xml);
  std::vector<ProtoWarning> warnings;
  expandScene(doc.scene, codec::noopProtoResolver, "", warnings);

  std::vector<Script *> scripts;
  for (auto &root : doc.scene.rootNodes) collectScripts(root.get(), scripts);
  if (scripts.size() != 1) return {};
  Script *script = scripts[0];

  X3DExecutionContext ctx;
  auto backend = bf.make();
  auto sys = std::make_shared<ScriptSystem>(backend, "x3d-cpp-gen", "4.0");
  ctx.addScriptSystem(sys);
  sys->attach(script, ctx);
  if (sys->handleFor(script) == kInvalidScriptHandle) return {};
  loaded = true;

  std::vector<Watch> watches = {
      {"status", script, "status", X3DFieldType::SFBool, true},
      {"landed", script, "landed", X3DFieldType::SFInt32, true},
  };

  Trace trace;
  double now = 1.0;
  // enabled(true) twice: first toggles status false->true (landed -1->0),
  // second toggles status true->false. A two-state observable sequence.
  for (int i = 0; i < 2; ++i) {
    sys->deliverInputEvent(script, "enabled", std::any(SFBool(true)),
                           X3DFieldType::SFBool, now);
    ctx.tick(now);
    trace.push_back(snapshot(watches));
    now += 1.0;
  }
  dynamicFieldStore().clear();
  return trace;
}

// Run one fixture through every backend; assert all traces equal the first.
void swap(const std::string &fixture,
          const std::function<Trace(const BackendFactory &)> &run,
          bool requireNonTrivial = true) {
  std::vector<Trace> traces;
  for (const auto &bf : kBackends) traces.push_back(run(bf));

  // Anti-vacuous guard: the reference (duktape) trace must carry real values.
  check(!traces[0].empty(), fixture + ": duktape trace is non-empty");
  if (requireNonTrivial)
    check(traceNonTrivial(traces[0]),
          fixture + ": trace carries real (non-trivial) computed values");

  // Optional human-readable dump of the captured traces (X3D_SWAP_DUMP=1).
  if (std::getenv("X3D_SWAP_DUMP")) {
    for (std::size_t b = 0; b < kBackends.size(); ++b) {
      std::cout << "  [" << fixture << "/" << kBackends[b].name << "]\n";
      for (std::size_t t = 0; t < traces[b].size(); ++t)
        std::cout << "    tick " << t << ": " << traces[b][t] << "\n";
    }
  }

  // Compare every backend to the duktape reference.
  for (std::size_t i = 1; i < kBackends.size(); ++i) {
    bool eq = tracesEqual(fixture, traces[0], traces[i]);
    check(eq, fixture + ": " + kBackends[0].name + " trace == " +
                  kBackends[i].name + " trace (observable parity)");
  }
}

} // namespace

int main(int argc, char **argv) {
  const std::string dataDir =
      argc > 1 ? argv[1] : "runtime/script/tests/data";

  swap("F1 author-output-drives-route", runF1);
  swap("F2 inputOutput-accumulation", runF2);
  swap("F3 computed-interpolation", runF3);

  // F3 anti-vacuous CONTENT assertion: the duktape reference trace must contain
  // the expected interpolated endpoints (2 at f=0, 10 at f=1) — proves the
  // handler genuinely computed, not that both backends silently produced zeros.
  {
    Trace ref = runF3(kBackends[0]);
    bool sawLo = false, sawHi = false;
    for (const auto &line : ref) {
      if (line.find("val=2 ") != std::string::npos ||
          line.rfind("val=2", 0) == 0 || line.find("val=2 |") != std::string::npos)
        sawLo = true;
      if (line.find("val=10 ") != std::string::npos ||
          line.find("val=10 |") != std::string::npos)
        sawHi = true;
    }
    check(sawLo, "F3: computed value reached the lo endpoint (val=2) — real arithmetic");
    check(sawHi, "F3: computed value reached the hi endpoint (val=10) — real arithmetic");
  }

  // F4: the real authored CDATA fixture through the codec path, both backends.
  {
    bool loadedD = false, loadedQ = false;
    Trace tD = runF4(kBackends[0], dataDir, loadedD);
    Trace tQ = runF4(kBackends[1], dataDir, loadedQ);
    check(loadedD && loadedQ,
          "F4 corpus-cdata: fixture parsed + LOADED under both backends");
    if (loadedD && loadedQ) {
      check(!tD.empty(), "F4 corpus-cdata: duktape trace non-empty");
      bool eq = tracesEqual("F4 corpus-cdata", tD, tQ);
      check(eq, "F4 corpus-cdata: duktape trace == quickjs trace (observable parity)");
      // Content guard: status must actually toggle (true then false), proving
      // the authored handler ran identically and was not a vacuous all-unset.
      check(tD.size() == 2 && tD[0].find("status=true") != std::string::npos &&
                tD[1].find("status=false") != std::string::npos,
            "F4 corpus-cdata: status toggled true->false (authored handler ran)");
    }
  }

  dynamicFieldStore().clear();
  if (failures == 0) {
    std::cout << "ALL QUICKJS SWAP TESTS PASSED — ScriptEngine seam proven "
                 "generic (Duktape == QuickJS observable behavior)\n";
    return 0;
  }
  std::cerr << failures << " quickjs swap test(s) FAILED\n";
  return 1;
}
