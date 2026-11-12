#include "FieldRead.hpp"
// Compare author fields and ROUTE targets after each tick under both engines.
// Numeric traces use fixed-precision rounding to compare marshalled field
// values without engine-specific formatting differences. See ADR-0022.
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

// Backend factory list. With X3D_CPP_BUILD_QUICKJS=ON, both the Duktape backend
// (x3d_ecmascript_backend — always built) and the QuickJS backend are linked.
// Each factory returns a fresh engine so no state leaks between fixtures.

struct BackendFactory {
  std::string name;
  std::function<std::shared_ptr<ScriptEngine>()> make;
};

const std::vector<BackendFactory> kBackends = {
    {"duktape", [] { return std::make_shared<EcmaScriptBackend>(); }},
    {"quickjs", [] { return std::make_shared<QuickJsBackend>(); }},
};

// canonical stringification of a boxed field value (Risk 2). Floats/doubles are
// rounded to a fixed precision and formatted identically regardless of engine,
// so engine-specific JS number->string coercion can never show up as a diff.
// We compare runtime field values, not JS strings.

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

// A watched field: read its value either from the DynamicFieldStore (an author
// field on a Script) or via the runtime reflection accessor (a ROUTE target).

struct Watch {
  std::string label;          // stable name in the trace
  X3DNode *node;              // the node whose field we read
  std::string field;         // x3d field name
  X3DFieldType type;         // for canonical formatting
  bool fromStore;            // true: read DynamicFieldStore; false: reflection
};

std::string readWatch(const Watch &w, const DynamicFieldStore &fields) {
  std::any v;
  if (w.fromStore) {
    v = fields.getValue(*w.node, w.field);
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

std::string snapshot(const std::vector<Watch> &watches,
                     const DynamicFieldStore &fields) {
  std::string line;
  for (std::size_t i = 0; i < watches.size(); ++i) {
    if (i) line += " | ";
    line += readWatch(watches[i], fields);
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

// F1: author output drives a ROUTE. set_value(v) -> position_changed=(v,v,v) ->
// ROUTE -> Transform.translation. Driven over a fixed schedule of input values.

Trace runF1(const BackendFactory &bf) {
  X3DExecutionContext ctx;

  auto script = std::make_unique<Script>();
  script->setDirectOutputUnchecked(true);
  script->setMustEvaluateUnchecked(true);
  script->setLoad(true);
  script->setSourceCode(
      "function set_value(v, ts) {"
      "  position_changed = { x: v, y: v, z: v };"
      "}");
  ctx.authorFields().addAuthorField(
      *script,
      AuthorFieldDecl{"set_value", X3DFieldType::SFFloat, AccessType::InputOnly,
                      {}});
  ctx.authorFields().addAuthorField(
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
    trace.push_back(snapshot(watches, ctx.authorFields()));
    now += 1.0;
  }
  return trace;
}

// F2: inputOutput stateful accumulation. count seeded 10; set_bump(v): count+=v.

Trace runF2(const BackendFactory &bf) {
  X3DExecutionContext ctx;

  auto script = std::make_unique<Script>();
  script->setDirectOutputUnchecked(true);
  script->setMustEvaluateUnchecked(true);
  script->setLoad(true);
  script->setSourceCode("function set_bump(v, ts) { count = count + v; }");
  ctx.authorFields().addAuthorField(
      *script, AuthorFieldDecl{"count", X3DFieldType::SFInt32,
                               AccessType::InputOutput, std::any(SFInt32(10))});
  ctx.authorFields().addAuthorField(
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
    trace.push_back(snapshot(watches, ctx.authorFields()));
    now += 1.0;
  }
  return trace;
}

// F3: non-trivial computed fixture — linear interpolation in the handler.
//   value_changed = lo + f * (hi - lo), with lo=2, hi=10 seeded as author
//   initializeOnly fields. Driven over fractions {0, 0.25, 0.5, 0.75, 1.0}
//   => expected outputs {2, 4, 6, 8, 10}. The handler also writes the SFVec3f
//   pos_changed=(value,value,value) to drive a ROUTE so the computed value is
// observed both as the script author field and on a downstream Transform.

Trace runF3(const BackendFactory &bf) {
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
  ctx.authorFields().addAuthorField(
      *script, AuthorFieldDecl{"lo", X3DFieldType::SFFloat,
                               AccessType::InitializeOnly, std::any(SFFloat(2.0f))});
  ctx.authorFields().addAuthorField(
      *script, AuthorFieldDecl{"hi", X3DFieldType::SFFloat,
                               AccessType::InitializeOnly, std::any(SFFloat(10.0f))});
  ctx.authorFields().addAuthorField(
      *script, AuthorFieldDecl{"set_fraction", X3DFieldType::SFFloat,
                               AccessType::InputOnly, {}});
  ctx.authorFields().addAuthorField(
      *script, AuthorFieldDecl{"value_changed", X3DFieldType::SFFloat,
                               AccessType::OutputOnly, {}});
  ctx.authorFields().addAuthorField(
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
    trace.push_back(snapshot(watches, ctx.authorFields()));
    now += 1.0;
  }
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
  x3d::runtime::forEachChildNode(*n, [&](const auto &, const auto &child) {
    collectScripts(child.get(), out);
  });
}

// F4: the authored CDATA corpus fixture, loaded through the real XmlReader ->
// expand -> attach path (the same file the corpus e2e test proves runs). We own
// the scene per-backend (a fresh parse) so the two runs are independent. Drives
// enabled(true) twice and snapshots the outputOnly 'status' + 'landed' fields.

Trace runF4(const BackendFactory &bf, const std::string &dataDir, bool &loaded) {
  loaded = false;

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

  X3DExecutionContext ctx(doc.scene.authorFields);
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
    trace.push_back(snapshot(watches, ctx.authorFields()));
    now += 1.0;
  }
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

  // F3 anti-vacuous content assertion: the duktape reference trace must contain
  // the expected interpolated endpoints (2 at f=0, 10 at f=1) — proves the
  // handler computed the value, not that both backends silently produced zeros.
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

  if (failures == 0) {
    std::cout << "ALL QUICKJS SWAP TESTS PASSED — ScriptEngine seam proven "
                 "generic (Duktape == QuickJS observable behavior)\n";
    return 0;
  }
  std::cerr << failures << " quickjs swap test(s) FAILED\n";
  return 1;
}
