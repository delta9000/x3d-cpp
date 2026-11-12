// vrml97_reader_test.cpp
// Unit test for the VRML97 (.wrl) reader (runtime/parse/Vrml97Reader), the
// one-way VRML97 -> X3D bridge layered on the shared ClassicVRML parser via the
// thin Vrml97Dialect rename table.
//
// Four layers of coverage:
//   1. Dialect-table tripwire: every X3D target name the Vrml97Dialect maps to
//      (mapFieldName) must resolve to a real factory type /
//      FieldInfo. This fails CI if the generated bindings drift away from a
//      name the hand-authored table assumes (per the reader spec).
//   2. Corpus fixtures (the five text .wrl files named in the reader spec /
//      blueprint, copied into runtime/parse/tests/data/wrl/). Each parses into
//      an X3DDocument; assertions on node types reached via reflection, sampled
//      field values, DEF/USE identity, ROUTEs, PROTO carry-through, and an
//      empty warnings() list (strict where the spec says so).
//   3. Hand-authored snippets exercising the VRML97-specific bits in isolation:
//      TRUE/FALSE booleans, eventIn/field access-keyword aliases, the
//      LOD.level / Switch.choice -> children field aliases, VRML 1.0 rejection,
//      and the unknown-node/unknown-field warning path.
//   4. Front-door dispatch: parseDocument content-sniffs `#VRML V2.0` as VRML97
//      and routes to this reader; the gzip path is detected and refused
//      cleanly.
//
// Driven entirely through reflection getters + the runtime model; no per-node
// codec code. Exit 0 on success, nonzero on any failed assertion.

#include "doctest/doctest.h"
#include "Vrml97Dialect.hpp"
#include "Vrml97Reader.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

// Concrete node types asserted on.
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Cylinder.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/LOD.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Switch.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::codec::Encoding;
using x3d::codec::Vrml97Reader;

namespace {

bool approx(double a, double b, double eps = 1e-4) {
  return std::fabs(a - b) <= eps;
}

std::string g_dataDir;

std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    FAIL_CHECK("cannot open fixture: " << path);
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}

// Recursively collect every node type name reachable from the roots, so a test
// can assert presence without hard-coding the tree shape.
void collectTypes(const std::shared_ptr<X3DNode> &n,
                  std::set<std::string> &out) {
  if (!n)
    return;
  out.insert(n->nodeTypeName());
  for (const FieldInfo &f : n->fields()) {
    if (!f.isReadable())
      continue;
    if (f.type == X3DFieldType::SFNode) {
      auto v = f.get(*n);
      if (v.has_value())
        if (auto c = std::any_cast<std::shared_ptr<X3DNode>>(v))
          collectTypes(c, out);
    } else if (f.type == X3DFieldType::MFNode) {
      auto v = f.get(*n);
      if (v.has_value())
        for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v))
          collectTypes(c, out);
    }
  }
}
// (1) Dialect-table tripwire: every mapped X3D target must really exist.
void testDialectTableResolves() {
  // VRML97 node names resolve directly through the X3D factory.
  for (const char *t :
       {"Shape", "Transform", "Group", "Switch", "LOD", "WorldInfo",
        "OrientationInterpolator", "TimeSensor", "Script", "Cylinder"}) {
    CHECK_MESSAGE((static_cast<bool>(X3DNodeFactory::create(t))), (std::string("dialect: node '") + t + "' resolves via factory"));
  }

  // Field mappings: the two real renames must land on a field that exists on
  // the mapped node's reflection table.
  auto hasField = [](const std::string &type, const std::string &x3dName) {
    auto n = X3DNodeFactory::create(type);
    if (!n)
      return false;
    for (const FieldInfo &f : n->fields())
      if (f.x3dName == x3dName)
        return true;
    return false;
  };
  CHECK_MESSAGE((codec::vrml97::mapFieldName("LOD", "level") == "children" &&
            hasField("LOD", "children")), "dialect: LOD.level -> children resolves to a real field");
  CHECK_MESSAGE((codec::vrml97::mapFieldName("Switch", "choice") == "children" &&
            hasField("Switch", "children")), "dialect: Switch.choice -> children resolves to a real field");
  // Deny-by-omission: an unmapped name passes through unchanged.
  CHECK_MESSAGE((codec::vrml97::mapFieldName("Transform", "translation") ==
            "translation"), "dialect: unmapped field passes through unchanged");
}
// (2) Corpus fixture: NewShape.wrl — minimal PROTO + instance + Switch/Group.
void testNewShapeFixture() {
  Vrml97Reader reader;
  reader.setStrict(true); // must parse cleanly
  runtime::X3DDocument doc =
      reader.readDocument(readFile(g_dataDir + "/NewShape.wrl"));

  CHECK_MESSAGE((doc.profile == runtime::Profile::Immersive), "newshape: VRML97 maps to Immersive profile");
  CHECK_MESSAGE((reader.warnings().empty()), "newshape: no warnings (clean parse)");

  // Roots in document order: WorldInfo, NavigationInfo, Switch (DEF Null1),
  // Group (DEF Null2). The PROTO 'NewShape' and its instance 'NewShape{...}'
  // are carried as scene data, not graph nodes.
  CHECK_MESSAGE((doc.scene.protoDeclarations.size() == 1), "newshape: one PROTO declared");
  if (!doc.scene.protoDeclarations.empty())
    CHECK_MESSAGE((doc.scene.protoDeclarations[0]->name == "NewShape"), "newshape: PROTO name NewShape");
  CHECK_MESSAGE((doc.scene.protoInstances.size() == 1), "newshape: one ProtoInstance (NewShape{...}) carried");

  auto sw = as<Switch>(doc.scene.resolve("Null1"));
  CHECK_MESSAGE((static_cast<bool>(sw)), "newshape: DEF Null1 is a Switch");
  auto grp = as<Group>(doc.scene.resolve("Null2"));
  CHECK_MESSAGE((static_cast<bool>(grp)), "newshape: DEF Null2 is a Group");

  // WorldInfo.info SFString/MFString survived.
  std::set<std::string> types;
  for (auto &n : doc.scene.rootNodes)
    collectTypes(n, types);
  CHECK_MESSAGE((types.count("WorldInfo") && types.count("NavigationInfo")), ("newshape: WorldInfo + NavigationInfo present"));
}
// (2) Corpus fixture: Rollers.wrl — DEF/USE + ROUTE + interpolators.
void testRollersFixture() {
  Vrml97Reader reader;
  reader.setStrict(true);
  runtime::X3DDocument doc =
      reader.readDocument(readFile(g_dataDir + "/Rollers.wrl"));
  CHECK_MESSAGE((reader.warnings().empty()), "rollers: no warnings (clean parse)");

  // DEF Forever TimeSensor { cycleInterval 6.0 loop TRUE startTime 1.0 }.
  auto ts = as<TimeSensor>(doc.scene.resolve("Forever"));
  CHECK_MESSAGE((static_cast<bool>(ts)), "rollers: DEF Forever TimeSensor resolvable");
  if (ts) {
    CHECK_MESSAGE((approx(ts->getCycleInterval(), 6.0)), "rollers: TimeSensor.cycleInterval == 6.0");
    CHECK_MESSAGE((ts->getLoop() == true), "rollers: TimeSensor.loop == TRUE -> true");
  }

  // DEF FullCircle OrientationInterpolator { key [0 0.5 1] keyValue [...] }.
  auto oi = as<OrientationInterpolator>(doc.scene.resolve("FullCircle"));
  CHECK_MESSAGE((static_cast<bool>(oi)), "rollers: DEF FullCircle OrientationInterpolator");
  if (oi) {
    CHECK_MESSAGE((oi->getKey().size() == 3), "rollers: interpolator key has 3 entries");
    CHECK_MESSAGE((oi->getKeyValue().size() == 3), "rollers: interpolator keyValue has 3 SFRotations");
  }

  // 22 ROUTEs (1 TimeSensor->interpolator + 21 interpolator->transform).
  CHECK_MESSAGE((doc.scene.routes.size() == 22), "rollers: 22 ROUTEs captured");
  bool drive = false;
  for (const auto &r : doc.scene.routes)
    if (r.fromNode == "Forever" && r.fromField == "fraction_changed" &&
        r.toNode == "FullCircle" && r.toField == "set_fraction")
      drive = true;
  CHECK_MESSAGE((drive), "rollers: ROUTE Forever.fraction_changed -> FullCircle.set_fraction");

  // DEF/USE shared identity: 'Column' is DEF'd once and USE'd many times.
  auto col = doc.scene.resolve("Column");
  CHECK_MESSAGE((static_cast<bool>(col)), "rollers: DEF Column resolvable");

  // resolveRoutes() wired the TimeSensor endpoint.
  for (const auto &r : doc.scene.routes)
    if (r.fromNode == "Forever")
      CHECK_MESSAGE((r.from.lock().get() == ts.get()), "rollers: ROUTE.from resolved to the TimeSensor node");
}
// (2) Corpus fixture: TranslationTestScene.wrl — PROTO + Billboard + instance.
void testTranslationFixture() {
  Vrml97Reader reader;
  runtime::X3DDocument doc =
      reader.readDocument(readFile(g_dataDir + "/TranslationTestScene.wrl"));
  CHECK_MESSAGE((reader.warnings().empty()), "translation: no warnings");
  CHECK_MESSAGE((doc.scene.protoDeclarations.size() == 1), "translation: one PROTO");
  CHECK_MESSAGE((doc.scene.protoInstances.size() == 1), "translation: one ProtoInstance (Object{...})");
  // DEF Null1 Billboard {} is a real graph node.
  CHECK_MESSAGE((static_cast<bool>(doc.scene.resolve("Null1"))), "translation: DEF Null1 (Billboard) resolvable");
}
// (2) Corpus fixtures: Chasers.wrl / Dampers.wrl — PROTO libraries with inline
// Script nodes carrying IS-bound interface declarations and multi-line
// vrmlscript: url strings. The point is they parse cleanly end-to-end.
void testProtoLibraryFixtures() {
  for (const char *name : {"Chasers.wrl", "Dampers.wrl"}) {
    Vrml97Reader reader;
    runtime::X3DDocument doc =
        reader.readDocument(readFile(g_dataDir + "/" + name));
    CHECK_MESSAGE((reader.warnings().empty()), (std::string(name) + ": parses with no warnings"));
    CHECK_MESSAGE((doc.scene.protoDeclarations.size() >= 4), (std::string(name) + ": PROTO library declarations carried"));
    // One real WorldInfo root; everything else is PROTO data.
    CHECK_MESSAGE((!doc.scene.rootNodes.empty()), (std::string(name) + ": at least one root node"));
  }
}
// (3) Hand-authored: TRUE/FALSE + access-keyword aliases (already in the shared
// base, asserted here through the VRML97 entry point).
void testBooleansAndAccessAliases() {
  const std::string src = "#VRML V2.0 utf8\n"
                          "DEF TS TimeSensor { loop TRUE enabled FALSE }\n"
                          "PROTO P [\n"
                          "  eventIn SFFloat set_x\n"       // -> inputOnly
                          "  eventOut SFFloat x_changed\n"  // -> outputOnly
                          "  field SFInt32 n 3\n"           // -> initializeOnly
                          "  exposedField SFBool on TRUE\n" // -> inputOutput
                          "] { Group {} }\n";
  Vrml97Reader reader;
  reader.setStrict(true);
  runtime::X3DDocument doc = reader.readDocument(src);

  auto ts = as<TimeSensor>(doc.scene.resolve("TS"));
  CHECK_MESSAGE((static_cast<bool>(ts)), "bool: DEF TS TimeSensor");
  if (ts) {
    CHECK_MESSAGE((ts->getLoop() == true), "bool: loop TRUE -> true");
    CHECK_MESSAGE((ts->getEnabled() == false), "bool: enabled FALSE -> false");
  }
  CHECK_MESSAGE((doc.scene.protoDeclarations.size() == 1), "alias: one PROTO");
  if (!doc.scene.protoDeclarations.empty()) {
    auto p = doc.scene.protoDeclarations[0];
    CHECK_MESSAGE((p->interface.size() == 4), "alias: 4 interface fields");
    if (p->interface.size() == 4) {
      CHECK_MESSAGE((p->interface[0].access == AccessType::InputOnly), "alias: eventIn -> InputOnly");
      CHECK_MESSAGE((p->interface[1].access == AccessType::OutputOnly), "alias: eventOut -> OutputOnly");
      CHECK_MESSAGE((p->interface[2].access == AccessType::InitializeOnly), "alias: field -> InitializeOnly");
      CHECK_MESSAGE((p->interface[3].access == AccessType::InputOutput), "alias: exposedField -> InputOutput");
    }
  }
}
// (3) Hand-authored: the LOD.level / Switch.choice -> children field aliases.
void testFieldAliases() {
  const std::string src = "#VRML V2.0 utf8\n"
                          "DEF L LOD {\n"
                          "  range [ 10 50 ]\n"
                          "  level [ Group {} Group {} Group {} ]\n"
                          "}\n"
                          "DEF SW Switch {\n"
                          "  whichChoice 1\n"
                          "  choice [ Group {} Group {} ]\n"
                          "}\n";
  Vrml97Reader reader;
  reader.setStrict(true);
  runtime::X3DDocument doc = reader.readDocument(src);

  auto lod = as<LOD>(doc.scene.resolve("L"));
  CHECK_MESSAGE((static_cast<bool>(lod)), "fieldalias: DEF L LOD");
  if (lod)
    CHECK_MESSAGE((lod->getChildren().size() == 3), "fieldalias: LOD.level -> children (3 nodes)");

  auto sw = as<Switch>(doc.scene.resolve("SW"));
  CHECK_MESSAGE((static_cast<bool>(sw)), "fieldalias: DEF SW Switch");
  if (sw) {
    CHECK_MESSAGE((sw->getChildren().size() == 2), "fieldalias: Switch.choice -> children (2 nodes)");
    CHECK_MESSAGE((sw->getWhichChoice() == 1), "fieldalias: Switch.whichChoice == 1");
  }
}
// (3) Hand-authored: VRML 1.0 is rejected; unknown node/field collected as a
// warning (and strict mode promotes it to a throw).
void testHeaderRejectionAndWarnings() {
  // VRML 1.0 header -> hard error.
  {
    Vrml97Reader reader;
    bool threw = false;
    try {
      reader.readDocument("#VRML V1.0 ascii\nSeparator { }\n");
    } catch (const std::exception &) {
      threw = true;
    }
    CHECK_MESSAGE((threw), "header: #VRML V1.0 is rejected with a throw");
  }

  // Unknown node + unknown field -> warnings (non-fatal), following node kept.
  {
    const std::string src = "#VRML V2.0 utf8\n"
                            "TotallyMadeUp { foo 1 nested Group {} }\n"
                            "Transform { bogusField 5 translation 1 2 3 }\n";
    Vrml97Reader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    CHECK_MESSAGE((reader.warnings().size() >= 2), ("warn: unknown node + unknown field both reported"));
    bool sawNode = false, sawField = false;
    for (const auto &w : reader.warnings()) {
      if (w.find("unknown node 'TotallyMadeUp'") != std::string::npos)
        sawNode = true;
      if (w.find("unknown field 'bogusField'") != std::string::npos)
        sawField = true;
    }
    CHECK_MESSAGE((sawNode), "warn: unknown node named in the warning");
    CHECK_MESSAGE((sawField), "warn: unknown field named in the warning");
    // The valid Transform after the unknown node still parses with its value.
    auto tr = as<Transform>(
        doc.scene.rootNodes.empty() ? nullptr : doc.scene.rootNodes.back());
    CHECK_MESSAGE((static_cast<bool>(tr)), "warn: Transform after unknown node kept");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      CHECK_MESSAGE((approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3)), "warn: Transform.translation 1 2 3 (unknown field skipped cleanly)");
    }
  }

  // Strict mode promotes the first warning to a throw.
  {
    Vrml97Reader reader;
    reader.setStrict(true);
    bool threw = false;
    try {
      reader.readDocument("#VRML V2.0 utf8\nMadeUpNode { }\n");
    } catch (const std::exception &) {
      threw = true;
    }
    CHECK_MESSAGE((threw), "strict: a warning is promoted to a throw in strict mode");
  }
}
// (4) Front door: parseDocument sniffs VRML97; makeReader yields a
// Vrml97Reader; gzip is detected and refused.
void testFrontDoor() {
  const std::string src = "#VRML V2.0 utf8\nShape { geometry Cylinder {} }\n";
  runtime::X3DDocument doc = codec::parseDocument(src);
  CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "frontdoor: parseDocument sniffs VRML97 and parses it");
  if (!doc.scene.rootNodes.empty())
    CHECK_MESSAGE((static_cast<bool>(as<Shape>(doc.scene.rootNodes[0]))), "frontdoor: produced a Shape via the front door");

  auto reader = codec::makeReader(Encoding::VRML97);
  CHECK_MESSAGE((reader && reader->encoding() == Encoding::VRML97), "frontdoor: makeReader(VRML97) yields a Vrml97Reader");

  // gzip: the front end detects but refuses (inflate is a gated/optional dep).
  const std::string gz = std::string("\x1f\x8b", 2) + "rest-is-binary";
  CHECK_MESSAGE((codec::isGzip(gz)), "frontdoor: gzip magic detected by sniffer");
}
// (3) Hand-authored: recoverable malformation reaches VRML97 through the shared
// ClassicVRML parse core (parser-gap Fix 2). A NURBS-style `geometry
// EXTERNPROTO X[...][url] X { controlPoint [ ... ] }` shape — an EXTERNPROTO
// declared inline as an SFNode field value, instantiated with a raw `[ ... ]`
// coordinate list assigned to an SFNode-typed proto field — must parse without
// throwing `expected node type name, got '['`. The EXTERNPROTO is declared, the
// instance carried, and the Shape still produced.
void testRecoverableMalformation() {
  const std::string src =
      "#VRML V2.0 utf8\n"
      "Shape {\n"
      "  geometry EXTERNPROTO NurbsSurface [\n"
      "    field SFNode controlPoint\n" // SFNode in the proto interface
      "  ] [ \"nurbs.wrl#NurbsSurface\" ]\n"
      "  NurbsSurface {\n"
      "    controlPoint [ 0 0 0  1 0 0  2 0 0 ]\n" // raw list for an SFNode
                                                   // field
      "  }\n"
      "}\n"
      "DEF AfterIt Transform { translation 7 8 9 }\n";
  Vrml97Reader reader;
  runtime::X3DDocument doc = reader.readDocument(src); // must not throw

  CHECK_MESSAGE((doc.scene.externProtoDeclarations.size() == 1), "recover: inline EXTERNPROTO NurbsSurface declared via subclass");
  CHECK_MESSAGE((doc.scene.protoInstances.size() == 1), "recover: NurbsSurface instance carried (raw [..] SFNode value skipped)");
  if (!doc.scene.protoInstances.empty())
    CHECK_MESSAGE((doc.scene.protoInstances[0].name == "NurbsSurface"), "recover: carried instance is a NurbsSurface");
  // The Shape (with its empty geometry) and the trailing Transform both
  // survive: the parser stayed aligned through the malformed field value.
  auto tr = as<Transform>(doc.scene.resolve("AfterIt"));
  CHECK_MESSAGE((static_cast<bool>(tr)), "recover: DEF AfterIt Transform after the proto instance still parsed");
  if (tr) {
    SFVec3f t = tr->getTranslation();
    CHECK_MESSAGE((approx(t.x, 7) && approx(t.y, 8) && approx(t.z, 9)), "recover: trailing Transform.translation 7 8 9 intact");
  }
}
// Stray-closer recovery reached via the VRML97 subclass: the
// SmokeTestProgramOutput_CommandLine.wrl shape — a child Shape inside a
// Group.children MFNode list omits its '}', so the list's ']' arrives at the
// node-body field position. The shared ClassicVRML recovery (node treated as
// closed, list consumes the ']') must apply through Vrml97Reader too, and must
// surface a warning() (this reader collects them). The trailing node still
// parses.
void testStrayCloserRecovery() {
  const std::string src = "#VRML V2.0 utf8\n"
                          "DEF G Group {\n"
                          "  children [\n"
                          "    Shape {\n" // '{' never closed before the ']'
                          "    # comment, then the array close on the next line\n"
                          "  ]\n"
                          "}\n"
                          "DEF AfterIt Transform { translation 7 8 9 }\n";
  Vrml97Reader reader;
  runtime::X3DDocument doc = reader.readDocument(src); // must not throw
  CHECK_MESSAGE((static_cast<bool>(as<Group>(doc.scene.resolve("G")))), "recover(wrl): DEF G Group recovered despite unclosed child Shape");
  auto tr = as<Transform>(doc.scene.resolve("AfterIt"));
  CHECK_MESSAGE((static_cast<bool>(tr)), "recover(wrl): trailing Transform after stray-']' resync still parsed");
  if (tr) {
    SFVec3f t = tr->getTranslation();
    CHECK_MESSAGE((approx(t.x, 7) && approx(t.y, 8) && approx(t.z, 9)), "recover(wrl): trailing Transform.translation 7 8 9 intact");
  }
  CHECK_MESSAGE((!reader.warnings().empty()), "recover(wrl): the recovery surfaced a warning() (not silent)");
}

} // namespace

TEST_CASE("vrml97_reader_test") {
  g_dataDir = X3D_TEST_DATA_DIR;

  std::cout << "===== VRML97: dialect table =====\n";
  testDialectTableResolves();
  std::cout << "===== VRML97: corpus fixtures =====\n";
  testNewShapeFixture();
  testRollersFixture();
  testTranslationFixture();
  testProtoLibraryFixtures();
  std::cout << "===== VRML97: grammar snippets =====\n";
  testBooleansAndAccessAliases();
  testFieldAliases();
  testHeaderRejectionAndWarnings();
  testRecoverableMalformation();
  testStrayCloserRecovery();
  testFrontDoor();

}
