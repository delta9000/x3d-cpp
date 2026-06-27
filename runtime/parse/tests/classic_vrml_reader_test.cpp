// classic_vrml_reader_test.cpp
// Unit test for the ClassicVRML (.x3dv) reader
// (runtime/parse/ClassicVrmlReader).
//
// Two layers of coverage:
//   1. Corpus fixtures (the four .x3dv files named in the reader spec, copied
//      into runtime/parse/tests/data/x3dv/). Each is parsed into an X3DDocument
//      and asserted on: header/version, node types reached via reflection,
//      representative field values, DEF/USE shared identity, and ROUTEs.
//   2. Hand-authored snippets exercising specific grammar productions in
//      isolation: header statements (PROFILE/COMPONENT/UNIT/META), DEF/USE
//      identity, ROUTE endpoint split, MF re-join (comma+whitespace, bracketed,
//      MFString quotes), enum fields, PROTO/EXTERNPROTO/instance/IS capture,
//      and IMPORT/EXPORT.
//
// Driven entirely through reflection getters + the runtime model; no per-node
// codec code. Exit 0 on success, nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "X3DParse.hpp" // parseDocument front door (content-sniffs ClassicVRML)
#include "X3DRuntime.hpp"

// Concrete node types asserted on.
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/NavigationInfo.hpp"
#include "x3d/nodes/NurbsPatchSurface.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Sphere.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace x3d;
using x3d::codec::ClassicVrmlReader;
using x3d::codec::Encoding;
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

bool approx(double a, double b, double eps = 1e-4) {
  return std::fabs(a - b) <= eps;
}

// The fixtures dir is passed as argv[1] (CMake sets it); fall back to a path
// relative to this source file's build layout.
std::string g_dataDir;

std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::cerr << "FAIL: cannot open fixture: " << path << "\n";
    ++failures;
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}

// ---------------------------------------------------------------------------
// Corpus fixture 1: ship.x3dv — smallest sanity (header + Shape{ Box }).
// ---------------------------------------------------------------------------
void testShipFixture() {
  std::string text = readFile(g_dataDir + "/ship.x3dv");
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  check(doc.version == "3.0", "ship: version 3.0 from header line");
  check(doc.profile == runtime::Profile::Immersive, "ship: PROFILE Immersive");
  check(doc.scene.rootNodes.size() == 1, "ship: one root node");

  auto shape =
      as<Shape>(doc.scene.rootNodes.empty() ? nullptr : doc.scene.rootNodes[0]);
  check(static_cast<bool>(shape), "ship: root is a Shape");
  if (shape) {
    auto box = as<Box>(shape->getGeometry());
    check(static_cast<bool>(box),
          "ship: Shape.geometry is a Box (SFNode child)");
    // Box.size is an initializeOnly field. As of Task 2 of the modernization
    // plan, initializeOnly fields have a setXUnchecked() data-layer write path
    // and a reflection `set` thunk, so readers CAN and DO populate them.
    // ship.x3dv sets "size 20 5 5" — assert the reader ingested that value.
    if (box)
      check(approx(box->getSize().x, 20.f),
            "ship: Box.size loaded from initializeOnly field (20 5 5)");
  }
}

// ---------------------------------------------------------------------------
// Corpus fixture 2: HelloWorld.x3dv — the canonical happy path. Exercises
// META lists (incl. escaped quotes), MFString, nested SFNode/MFNode children,
// DEF/USE shared Material identity, and field values.
// ---------------------------------------------------------------------------
void testHelloWorldFixture() {
  std::string text = readFile(g_dataDir + "/HelloWorld.x3dv");
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  check(doc.version == "3.3", "hello: version 3.3");
  check(doc.profile == runtime::Profile::Immersive, "hello: PROFILE Immersive");
  // META list survived (the file has many; one carries an escaped quote).
  check(doc.head.meta.size() >= 3, "hello: META statements captured");
  bool titleFound = false;
  for (const auto &m : doc.head.meta)
    if (m.name == "title" && m.content == "HelloWorld.x3d")
      titleFound = true;
  check(titleFound, "hello: META title == HelloWorld.x3d");

  // Roots: NavigationInfo, WorldInfo, Group (in document order).
  check(doc.scene.rootNodes.size() == 3, "hello: three root nodes");
  if (doc.scene.rootNodes.size() == 3) {
    check(static_cast<bool>(as<NavigationInfo>(doc.scene.rootNodes[0])),
          "hello: root[0] NavigationInfo");
    auto wi = as<WorldInfo>(doc.scene.rootNodes[1]);
    check(static_cast<bool>(wi), "hello: root[1] WorldInfo");
    if (wi)
      check(wi->getTitle() == "Hello World!", "hello: WorldInfo.title");

    auto group = as<Group>(doc.scene.rootNodes[2]);
    check(static_cast<bool>(group), "hello: root[2] Group");
    if (group) {
      const auto &kids = group->getChildren();
      // Viewpoint + two Transforms.
      check(kids.size() == 3, "hello: Group has 3 children");
    }
  }

  // DEF/USE: MaterialOffWhite is DEF'd in the first Transform's Shape and USE'd
  // in the second Transform's Shape. Both must be the SAME shared_ptr.
  auto matDef = doc.scene.resolve("MaterialOffWhite");
  check(static_cast<bool>(matDef), "hello: DEF MaterialOffWhite registered");
  if (auto mat = as<Material>(matDef)) {
    SFColor c = mat->getDiffuseColor();
    check(approx(c.r, 0.980392) && approx(c.g, 0.976471) &&
              approx(c.b, 0.964706),
          "hello: MaterialOffWhite.diffuseColor parsed");
  }

  // Walk to the two Shapes and confirm the shared Material identity.
  auto group = as<Group>(doc.scene.rootNodes.back());
  std::shared_ptr<Material> m1, m2;
  if (group) {
    for (const auto &k : group->getChildren()) {
      auto tr = as<Transform>(k);
      if (!tr)
        continue;
      for (const auto &kk : tr->getChildren()) {
        auto sh = as<Shape>(kk);
        if (!sh)
          continue;
        if (auto app = as<Appearance>(sh->getAppearance())) {
          auto mm = as<Material>(app->getMaterial());
          if (mm && !m1)
            m1 = mm;
          else if (mm)
            m2 = mm;
        }
      }
    }
  }
  check(m1 && m2 && m1.get() == m2.get(),
        "hello: DEF/USE Material is one shared_ptr across two Shapes");

  // ImageTexture MFString url with multiple entries (DEF ImageCloudlessEarth).
  if (auto tex = as<ImageTexture>(doc.scene.resolve("ImageCloudlessEarth"))) {
    check(tex->getUrl().size() >= 2, "hello: ImageTexture.url MFString list");
    check(tex->getUrl()[0] == "earth-topo.png",
          "hello: ImageTexture.url[0] == earth-topo.png");
  }
}

// ---------------------------------------------------------------------------
// Corpus fixture 3: AddDynamicRoutes.x3dv — Script node carried as data with an
// inline interface declaration (`inputOnly SFTime touchTime`) consumed, then a
// real field (url). Exercises the access-type-led interface-decl path.
// ---------------------------------------------------------------------------
void testScriptFixture() {
  std::string text = readFile(g_dataDir + "/AddDynamicRoutes.x3dv");
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  check(doc.version == "3.0", "script: version 3.0");
  // PROFILE is commented out in this fixture (`#PROFILE Immersive`), so it
  // stays at the default.
  check(doc.scene.rootNodes.size() == 1,
        "script: one root node (DEF SC Script)");
  if (!doc.scene.rootNodes.empty()) {
    auto sc = doc.scene.rootNodes[0];
    check(static_cast<bool>(sc), "script: Script node created");
    if (sc) {
      check(sc->getDEF() == "SC", "script: DEF SC captured");
      check(sc->nodeTypeName() == "Script", "script: node type is Script");
      // The interface declaration `inputOnly SFTime touchTime` was consumed;
      // the real `url` MFString field still parsed.
      const FieldInfo *urlF = nullptr;
      for (const FieldInfo &f : sc->fields())
        if (f.x3dName == "url")
          urlF = &f;
      check(urlF != nullptr, "script: Script has a url field");
      if (urlF && urlF->isReadable()) {
        auto v = urlF->get(*sc);
        auto urls = std::any_cast<std::vector<std::string>>(v);
        check(urls.size() == 1 && urls[0] == "SAIExample4.class",
              "script: url == [\"SAIExample4.class\"] after skipping the "
              "interface decl");
      }
    }
  }
  check(doc.scene.resolve("SC") != nullptr,
        "script: DEF SC resolvable in scene table");
}

// ---------------------------------------------------------------------------
// Corpus fixture 4: animated_patch.x3dv — large MF arrays (multi-line, no
// commas), DEF'd nodes, and two ROUTEs that resolve to DEF'd endpoints.
// ---------------------------------------------------------------------------
void testNurbsFixture() {
  std::string text = readFile(g_dataDir + "/animated_patch.x3dv");
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(text);

  check(doc.version == "3.1", "nurbs: version 3.1");

  // DEF'd nodes reachable by name.
  auto patch = as<NurbsPatchSurface>(doc.scene.resolve("NurbsPatchSurface"));
  check(static_cast<bool>(patch), "nurbs: DEF NurbsPatchSurface resolvable");
  if (patch) {
    // `weight` is an inputOutput MFDouble that spans multiple physical lines
    // with NO commas (16 values). Asserting its size proves the multi-line MF
    // token re-join. (uKnot/uDimension/uOrder are initializeOnly => read-only
    // in this model; controlPoint is an SFNode in X3D4 but written as a raw
    // coordinate list in this X3D3.1 fixture, so it is tolerated/skipped.)
    check(patch->getWeight().size() == 16,
          "nurbs: weight has 16 MFDouble elements (multi-line MF re-join)");
  }

  auto ts = as<TimeSensor>(doc.scene.resolve("TimeSource"));
  check(static_cast<bool>(ts), "nurbs: DEF TimeSource resolvable");
  if (ts) {
    check(approx(ts->getCycleInterval(), 2.0),
          "nurbs: TimeSource.cycleInterval == 2.0");
    check(ts->getLoop() == true, "nurbs: TimeSource.loop == TRUE");
  }

  // Two ROUTEs survive and resolve to the DEF'd nodes.
  check(doc.scene.routes.size() == 2, "nurbs: two ROUTEs captured");
  bool route1 = false, route2 = false;
  for (const auto &r : doc.scene.routes) {
    if (r.fromNode == "TimeSource" && r.fromField == "fraction_changed" &&
        r.toNode == "CI" && r.toField == "setTime")
      route1 = true;
    if (r.fromNode == "CI" && r.fromField == "value_changed" &&
        r.toNode == "NurbsPatchSurface" && r.toField == "set_controlPoint")
      route2 = true;
  }
  check(route1, "nurbs: ROUTE TimeSource.fraction_changed -> CI.setTime");
  check(route2,
        "nurbs: ROUTE CI.value_changed -> NurbsPatchSurface.set_controlPoint");
  // resolveRoutes() wired the from/to weak_ptrs for resolvable endpoints.
  for (const auto &r : doc.scene.routes) {
    if (r.fromNode == "TimeSource")
      check(r.from.lock().get() == ts.get(),
            "nurbs: ROUTE.from resolved to TimeSource node");
  }
}

// ---------------------------------------------------------------------------
// Hand-authored grammar snippets.
// ---------------------------------------------------------------------------
void testHeaderStatements() {
  const std::string src = "#X3D V4.0 utf8\n"
                          "PROFILE Interchange\n"
                          "COMPONENT NURBS:3\n"
                          "UNIT length kilometer 1000.0\n"
                          "META \"title\" \"snippet\"\n"
                          "Shape { geometry Box { size 1 2 3 } }\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);

  check(doc.version == "4.0", "hdr: version 4.0");
  check(doc.profile == runtime::Profile::Interchange,
        "hdr: PROFILE Interchange");
  check(doc.head.components.size() == 1 &&
            doc.head.components[0].name == "NURBS" &&
            doc.head.components[0].level == 3,
        "hdr: COMPONENT NURBS:3");
  check(doc.head.units.size() == 1 && doc.head.units[0].category == "length" &&
            doc.head.units[0].name == "kilometer" &&
            approx(doc.head.units[0].conversionFactor, 1000.0),
        "hdr: UNIT length kilometer 1000");
  check(doc.head.meta.size() == 1 && doc.head.meta[0].name == "title" &&
            doc.head.meta[0].content == "snippet",
        "hdr: META title snippet");
}

void testDefUseIdentity() {
  const std::string src = "#X3D V4.0 utf8\n"
                          "PROFILE Interchange\n"
                          "Group {\n"
                          "  children [\n"
                          "    DEF A Transform { translation 1 2 3 }\n"
                          "    USE A\n"
                          "  ]\n"
                          "}\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);
  auto group = as<Group>(doc.scene.rootNodes.at(0));
  check(static_cast<bool>(group), "defuse: root Group");
  if (group) {
    const auto &kids = group->getChildren();
    check(kids.size() == 2, "defuse: two children (DEF + USE)");
    if (kids.size() == 2) {
      check(kids[0].get() == kids[1].get(),
            "defuse: USE A shares the DEF A shared_ptr (identity)");
      auto tr = as<Transform>(kids[0]);
      if (tr) {
        SFVec3f t = tr->getTranslation();
        check(approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3),
              "defuse: DEF A translation 1 2 3");
      }
    }
  }
}

void testMfReJoin() {
  // Mixed comma+whitespace, bracketed, and MFString quote preservation.
  const std::string src = "#X3D V4.0 utf8\n"
                          "PROFILE Interchange\n"
                          "WorldInfo {\n"
                          "  info [ \"alpha beta\", \"gamma\" ]\n"
                          "}\n"
                          "NurbsPatchSurface {\n"
                          "  weight [ 1, 1 1,1 2 2 2 2 ]\n"
                          "}\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);
  auto wi = as<WorldInfo>(doc.scene.rootNodes.at(0));
  check(static_cast<bool>(wi), "mf: WorldInfo root");
  if (wi) {
    const auto &info = wi->getInfo();
    check(info.size() == 2 && info[0] == "alpha beta" && info[1] == "gamma",
          "mf: MFString preserves quotes across comma+whitespace");
  }
  auto patch = as<NurbsPatchSurface>(doc.scene.rootNodes.at(1));
  check(static_cast<bool>(patch), "mf: NurbsPatchSurface root");
  if (patch)
    // `weight` is inputOutput MFDouble (writable). Mixed comma+whitespace.
    check(patch->getWeight().size() == 8,
          "mf: MFDouble re-join across mixed comma+whitespace == 8 elements");
}

void testRouteAndImportExport() {
  const std::string src = "#X3D V4.0 utf8\n"
                          "PROFILE Interchange\n"
                          "DEF TS TimeSensor { cycleInterval 5 loop TRUE }\n"
                          "DEF XF Transform { }\n"
                          "ROUTE TS.fraction_changed TO XF.set_translation\n"
                          "IMPORT InlineScene.Clock AS RemoteClock\n"
                          "EXPORT XF AS PublicXF\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);

  check(doc.scene.routes.size() == 1, "route: one ROUTE");
  if (!doc.scene.routes.empty()) {
    const auto &r = doc.scene.routes[0];
    check(r.fromNode == "TS" && r.fromField == "fraction_changed" &&
              r.toNode == "XF" && r.toField == "set_translation",
          "route: endpoints split on '.'");
    check(r.from.lock() == doc.scene.resolve("TS") &&
              r.to.lock() == doc.scene.resolve("XF"),
          "route: resolveRoutes() wired both endpoints");
  }
  check(doc.scene.imports.size() == 1 &&
            doc.scene.imports[0].inlineDEF == "InlineScene" &&
            doc.scene.imports[0].importedDEF == "Clock" &&
            doc.scene.imports[0].as == "RemoteClock",
        "route: IMPORT inlineDEF.importedDEF AS local");
  check(doc.scene.exports.size() == 1 &&
            doc.scene.exports[0].localDEF == "XF" &&
            doc.scene.exports[0].as == "PublicXF",
        "route: EXPORT localDEF AS name");
}

// IMPORT/EXPORT statement variants: the single-line dot form, the non-standard
// block form (keys in any order), and a malformed single-line IMPORT whose slot
// bleeds an inline `{ }` body (defensive recovery, no throw). Corpus evidence:
// stylesheets/java/examples/SmokeTestProgramOutput_CommandLine.{wrl,x3dv} use
// the block form for both IMPORT and EXPORT.
void testImportVariants() {
  // 1. Single-line dot form (the baseline already covered by
  //    testRouteAndImportExport, re-asserted here for the variant matrix).
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "IMPORT Inl.Imported AS Local\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    check(doc.scene.imports.size() == 1 &&
              doc.scene.imports[0].inlineDEF == "Inl" &&
              doc.scene.imports[0].importedDEF == "Imported" &&
              doc.scene.imports[0].as == "Local",
          "import: single-line dot form");
  }

  // 2. Block form with keys in any order (matches the corpus ordering:
  //    AS / importedDEF / inlineDEF). Produces the same runtime::Import.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "IMPORT {\n"
                            "  AS Local\n"
                            "  importedDEF Imported\n"
                            "  inlineDEF Inl\n"
                            "}\n"
                            "Group { }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    check(doc.scene.imports.size() == 1 &&
              doc.scene.imports[0].inlineDEF == "Inl" &&
              doc.scene.imports[0].importedDEF == "Imported" &&
              doc.scene.imports[0].as == "Local",
          "import: block form, keys reordered, same Import");
    check(doc.scene.rootNodes.size() == 1 &&
              static_cast<bool>(as<Group>(doc.scene.rootNodes[0])),
          "import: statement after block IMPORT still reached (Group)");
  }

  // 3. Block EXPORT form (sibling category present in the same corpus file).
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "EXPORT {\n"
                            "  AS Exported\n"
                            "  localDEF Local\n"
                            "}\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    check(doc.scene.exports.size() == 1 &&
              doc.scene.exports[0].localDEF == "Local" &&
              doc.scene.exports[0].as == "Exported",
          "export: block form, keys reordered");
  }

  // 4. Malformed single-line IMPORT followed by a stray inline `{ }` body:
  //    the parser must NOT throw; one structural import is recorded and the
  //    document's following statement is still reached.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "IMPORT Inl.Imported { foo Bar { } }\n"
                            "Group { }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    check(doc.scene.imports.size() == 1 &&
              doc.scene.imports[0].inlineDEF == "Inl" &&
              doc.scene.imports[0].importedDEF == "Imported",
          "import: single-line + stray '{' body recovered (no throw)");
    check(doc.scene.rootNodes.size() == 1 &&
              static_cast<bool>(as<Group>(doc.scene.rootNodes[0])),
          "import: statement after recovered IMPORT still reached (Group)");
  }

  // 5. Block form with an unknown key (forward-compat tolerance): the unknown
  //    key/value pair is ignored, recognized keys still populate the Import.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "IMPORT {\n"
                            "  inlineDEF Inl\n"
                            "  futureKey somethingNew\n"
                            "  importedDEF Imported\n"
                            "}\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src);
    check(doc.scene.imports.size() == 1 &&
              doc.scene.imports[0].inlineDEF == "Inl" &&
              doc.scene.imports[0].importedDEF == "Imported",
          "import: block form ignores unknown key");
  }
}

void testProto() {
  const std::string src =
      "#X3D V4.0 utf8\n"
      "PROFILE Interchange\n"
      "PROTO Widget [\n"
      "  inputOutput SFVec3f size 1 1 1\n"
      "  eventIn SFTime poke\n"         // VRML97 alias -> InputOnly
      "  field SFString label \"hi\"\n" // VRML97 alias -> InitializeOnly
      "] {\n"
      "  Transform {\n"
      "    translation IS size\n"
      "    children [ Shape { geometry Box { } } ]\n"
      "  }\n"
      "}\n"
      "EXTERNPROTO Gadget [ inputOutput SFFloat amount ] [ "
      "\"gadget.x3d#Gadget\" ]\n"
      "Widget { size 2 2 2 }\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);

  check(doc.scene.protoDeclarations.size() == 1, "proto: one PROTO declared");
  if (!doc.scene.protoDeclarations.empty()) {
    auto p = doc.scene.protoDeclarations[0];
    check(p->name == "Widget", "proto: name Widget");
    check(p->interface.size() == 3, "proto: 3 interface fields");
    if (p->interface.size() == 3) {
      check(p->interface[0].name == "size" &&
                p->interface[0].type == X3DFieldType::SFVec3f &&
                p->interface[0].access == AccessType::InputOutput,
            "proto: field[0] size SFVec3f inputOutput (with default)");
      check(p->interface[1].access == AccessType::InputOnly,
            "proto: field[1] eventIn -> InputOnly alias");
      check(p->interface[2].access == AccessType::InitializeOnly,
            "proto: field[2] field -> InitializeOnly alias");
    }
    check(p->body.nodes.size() == 1, "proto: body has one primary node");
    if (!p->body.nodes.empty())
      check(static_cast<bool>(as<Transform>(p->body.nodes[0])),
            "proto: body primary node is the Transform (IS captured+skipped)");
  }

  check(doc.scene.externProtoDeclarations.size() == 1,
        "proto: one EXTERNPROTO declared");
  if (!doc.scene.externProtoDeclarations.empty()) {
    auto e = doc.scene.externProtoDeclarations[0];
    check(e->name == "Gadget", "proto: EXTERNPROTO name Gadget");
    check(e->url.size() == 1 && e->url[0] == "gadget.x3d#Gadget",
          "proto: EXTERNPROTO url captured");
    check(e->interface.size() == 1,
          "proto: EXTERNPROTO interface (no default)");
  }

  // The `Widget { size 2 2 2 }` is a proto instance, carried as scene data and
  // not inserted as a node (factory has no Widget type).
  check(doc.scene.protoInstances.size() == 1,
        "proto: one ProtoInstance carried as data");
  if (!doc.scene.protoInstances.empty()) {
    const auto &inst = doc.scene.protoInstances[0];
    check(inst.name == "Widget", "proto: instance is a Widget");
    check(inst.declaration && inst.declaration->name == "Widget",
          "proto: instance resolved to its declaration");
    check(inst.fieldValues.size() == 1 && inst.fieldValues[0].name == "size",
          "proto: instance carries the size override");
  }
}

// PROTO body `IS` mapping capture + ProtoInstance parent linkage (Task 8 of the
// PROTO/EXTERNPROTO expansion plan). The PROTO body's `Box { size IS size }`
// must append one IsConnection to the declaration's ProtoBody, bound to the Box
// body node; and the nested `Param { ... }` instance must record its parent
// (the Transform) and the containerField slot it filled (`children`).
void protoIsCaptureTest() {
  const char *src =
      "#X3D V4.0 utf8\n"
      "PROTO Param [ initializeOnly SFVec3f size 2 2 2 ] {\n"
      "  Box { size IS size }\n"
      "}\n"
      "Transform { children [ Param { size 5 5 5 } ] }\n";
  ClassicVrmlReader reader;
  auto doc = reader.readDocument(src);
  auto &scene = doc.scene;
  check(scene.protoDeclarations.size() == 1,
        "proto-is: one ProtoDeclare captured");
  auto &decl = *scene.protoDeclarations[0];
  check(decl.body.isConnections.size() == 1,
        "proto-is: one IS connection captured");
  if (decl.body.isConnections.size() == 1) {
    check(decl.body.isConnections[0].nodeField == "size",
          "proto-is: IS nodeField=size");
    check(decl.body.isConnections[0].protoField == "size",
          "proto-is: IS protoField=size");
    check(decl.body.isConnections[0].node &&
              decl.body.isConnections[0].node->nodeTypeName() == "Box",
          "proto-is: IS bound to the Box body node");
  }
  check(scene.protoInstances.size() == 1,
        "proto-is: one ProtoInstance captured");
  if (scene.protoInstances.size() == 1) {
    auto &inst = scene.protoInstances[0];
    check(!inst.parent.expired(), "proto-is: instance has a parent");
    check(!inst.parent.expired() &&
              inst.parent.lock()->nodeTypeName() == "Transform",
          "proto-is: parent is Transform");
    check(inst.parentField == "children",
          "proto-is: instance parentField=children");
  }
}

void testFrontDoorSniff() {
  // parseDocument with no hint must content-sniff `#X3D V…` as ClassicVRML and
  // dispatch to the new reader.
  const std::string src =
      "#X3D V4.0 utf8\nPROFILE Interchange\nShape { geometry Box { } }\n";
  runtime::X3DDocument doc = codec::parseDocument(src);
  check(doc.scene.rootNodes.size() == 1,
        "frontdoor: parseDocument sniffs ClassicVRML and parses it");
  if (!doc.scene.rootNodes.empty())
    check(static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "frontdoor: produced a Shape via the front door");

  // makeReader(ClassicVRML) now returns a reader (no longer throws).
  auto reader = codec::makeReader(Encoding::ClassicVRML);
  check(reader && reader->encoding() == Encoding::ClassicVRML,
        "frontdoor: makeReader(ClassicVRML) yields a ClassicVrmlReader");
}

void testUnknownNodeSkipped() {
  const std::string src =
      "#X3D V4.0 utf8\n"
      "PROFILE Interchange\n"
      "TotallyMadeUpNode { foo 1 bar [ 1 2 3 ] nested Box { size 1 1 1 } }\n"
      "Shape { geometry Box { size 9 9 9 } }\n";
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(src);
  // The unknown node and its whole balanced body are skipped; the Shape after
  // it still parses.
  check(doc.scene.rootNodes.size() == 1,
        "skip: unknown node skipped, following Shape kept");
  if (!doc.scene.rootNodes.empty()) {
    auto sh = as<Shape>(doc.scene.rootNodes[0]);
    check(static_cast<bool>(sh), "skip: surviving node is the Shape");
    if (sh)
      check(static_cast<bool>(as<Box>(sh->getGeometry())),
            "skip: Shape after the unknown node still has its Box geometry");
  }
}

// ---------------------------------------------------------------------------
// Recoverable malformation (parser-gap Fix 2): a node whose type name is a
// declared PROTO/EXTERNPROTO or is otherwise unknown must parse its body
// STRUCTURALLY (balanced braces, field assignments captured / skipped) and be
// recorded — never errored. PROTO instances land in scene.protoInstances,
// carried (not expanded). These snippets reproduce the shapes that the
// X3D conformance corpus surfaced (NURBS .wrl / HAnim .x3dv files):
//   * an empty / scalar-bearing MFNode list,
//   * an unknown node used with no `{ }` body,
//   * embedded ROUTE / EXTERNPROTO statements inside an MFNode list,
//   * a node-valued proto-instance field whose declaration is unknown,
//   * a raw `[ ... ]` value assigned to an SFNode-typed proto field.
// ---------------------------------------------------------------------------
void testRecoverableMalformation() {
  // (a) Empty MFNode list + a stray scalar list where nodes are expected: both
  // degrade to "no children", and the sibling Shape after them is still parsed.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "Group {\n"
        "  children [ ]\n" // empty MFNode list
        "}\n"
        "Group {\n"
        "  children [ 0 0 0 ]\n" // stray scalars where nodes are expected
        "}\n"
        "Shape { geometry Box { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.rootNodes.size() == 3,
          "recover: empty/scalar MFNode lists parsed, 3 roots kept");
    auto g0 = as<Group>(doc.scene.rootNodes.at(0));
    auto g1 = as<Group>(doc.scene.rootNodes.at(1));
    check(g0 && g0->getChildren().empty(),
          "recover: empty MFNode list -> 0 children");
    check(g1 && g1->getChildren().empty(),
          "recover: scalar-in-MFNode list -> 0 children (value skipped)");
    check(static_cast<bool>(as<Shape>(doc.scene.rootNodes.at(2))),
          "recover: Shape after malformed lists still parsed");
  }

  // (b) An unknown node type used WITHOUT a `{ }` body inside a Group's
  // children (an EXTERNPROTO-style name with no body): skipped, the Group still
  // parses, and a sibling node after the unknown one is still reached.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "Group {\n"
                            "  children [\n"
                            "    UndeclaredThing\n" // unknown node, NO body
                            "    Shape { geometry Box { } }\n"
                            "  ]\n"
                            "}\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    auto g = as<Group>(doc.scene.rootNodes.at(0));
    check(static_cast<bool>(g), "recover: Group with bodiless unknown node");
    check(g && g->getChildren().size() == 1,
          "recover: bodiless unknown node skipped, sibling Shape kept");
    if (g && g->getChildren().size() == 1)
      check(static_cast<bool>(as<Shape>(g->getChildren()[0])),
            "recover: surviving child is the Shape");
  }

  // (c) Embedded ROUTE + EXTERNPROTO statements inside an MFNode list: handled
  // in place (the ROUTE is captured, the EXTERNPROTO declared), and the real
  // nodes around them are still attached.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "Group {\n"
        "  children [\n"
        "    DEF A Transform { }\n"
        "    ROUTE A.translation_changed TO A.set_translation\n"
        "    EXTERNPROTO Ext [ inputOutput SFFloat amt ] [ \"e.x3d#Ext\" ]\n"
        "    DEF B Transform { }\n"
        "  ]\n"
        "}\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    auto g = as<Group>(doc.scene.rootNodes.at(0));
    check(g && g->getChildren().size() == 2,
          "recover: embedded ROUTE/EXTERNPROTO skipped, both Transforms kept");
    check(doc.scene.routes.size() == 1,
          "recover: ROUTE inside MFNode list captured at scene level");
    check(doc.scene.externProtoDeclarations.size() == 1,
          "recover: EXTERNPROTO inside MFNode list declared");
  }

  // (d) A proto instance whose node-valued field's declaration is UNKNOWN: the
  // inline node body is captured as a structural node value (not mis-read as a
  // scalar token, not expanded). MyProto is declared with `field MFNode shape`,
  // and the instance assigns `shape DEF S Shape { }`.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "PROTO MyProto [ initializeOnly MFNode shape ] {\n"
                            "  Group { }\n"
                            "}\n"
                            "MyProto { shape DEF S Shape { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.protoDeclarations.size() == 1,
          "recover: PROTO MyProto declared");
    check(doc.scene.protoInstances.size() == 1,
          "recover: MyProto instance carried (not expanded)");
    if (!doc.scene.protoInstances.empty()) {
      const auto &inst = doc.scene.protoInstances[0];
      check(inst.name == "MyProto", "recover: instance is a MyProto");
      check(inst.declaration && inst.declaration->name == "MyProto",
            "recover: instance resolved to its declaration");
      check(inst.fieldValues.size() == 1 && inst.fieldValues[0].name == "shape",
            "recover: instance carries the 'shape' field value");
      if (!inst.fieldValues.empty()) {
        check(inst.fieldValues[0].nodeValue.size() == 1,
              "recover: 'shape' captured as one structural node value");
        if (inst.fieldValues[0].nodeValue.size() == 1) {
          auto child = inst.fieldValues[0].nodeValue[0];
          check(static_cast<bool>(as<Shape>(child)),
                "recover: captured node value is a Shape (structural)");
          check(child && child->getDEF() == "S",
                "recover: captured child carries DEF 'S' (not expanded)");
        }
      }
    }
  }

  // (e) A raw `[ ... ]` value assigned to an SFNode-typed proto field (the
  // X3D-3.x NurbsPatchSurface.controlPoint coordinate-list shape): the value is
  // skipped rather than mis-parsed as a node; the instance is still recorded
  // and a following statement is still reached.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "EXTERNPROTO Surf [ initializeOnly SFNode controlPoint ]"
        " [ \"s.x3d#Surf\" ]\n"
        "Surf { controlPoint [ 0 0 0 1 0 0 2 0 0 ] }\n"
        "Shape { geometry Box { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.protoInstances.size() == 1,
          "recover: Surf instance carried despite raw [..] SFNode value");
    check(doc.scene.rootNodes.size() == 1 &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "recover: Shape after raw-value proto instance still parsed");
  }
}

// ---------------------------------------------------------------------------
// Lenient-recovery (parser-gap follow-up): four RECOVERABLE shapes the corpus
// surfaced that previously THREW and aborted the whole document. Each must now
// warn + resync and keep parsing:
//   (a) stray prose word colliding with a known type name, not followed by '{'
//       (leaked from a misused `#/* ... */` comment block) -> ignored;
//   (b) an implicit-containerField child node (a node type appearing where a
//       field name is expected: `Parent { Child { } }`) -> body skipped as a
//       unit instead of desyncing by one brace;
//   (c) a stray ']' where a field name is expected (an un-terminated MF array
//       whose intended ']' was swallowed by an inline comment) -> node treated
//       as closed, enclosing container consumes the ']';
//   (d) a child node missing its '}' inside an MFNode list, the array's ']'
//       arriving at the node-body field position -> node closed, list ']'
//       consumed, document continues.
// ---------------------------------------------------------------------------
void testStrayCloserAndImplicitChildRecovery() {
  // (a) Stray prose: a real type name (`Box`) appears as a bare word NOT
  // followed by '{' (the comment-leak shape from AddDynamicRoutes.x3dv). It is
  // ignored as a stray token; the following words drain word-by-word; the real
  // Shape after the prose still parses.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "creates a Box with a touchSensor\n" // leaked prose
                            "Shape { geometry Sphere { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.rootNodes.size() == 1,
          "recover(a): stray 'Box' prose word ignored, trailing Shape kept");
    check(!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "recover(a): surviving root is the Shape");
  }

  // (a') A DEF before a missing-'{' type still THROWS (DEF is an unambiguous
  // real-node signal — must not be masked).
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "DEF B Box with\n";
    ClassicVrmlReader reader;
    bool threw = false;
    try {
      reader.readDocument(src);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "recover(a'): DEF + missing '{' still throws (not masked)");
  }

  // (b) Implicit-containerField child: a node type written directly where a
  // field name is expected (`Group { Shape { } }` — the XML default-container
  // idiom in text). The child body is skipped as a balanced unit; the parent
  // and a trailing sibling both parse (no one-brace desync).
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "Group {\n"
                            "  Shape { geometry Box { } }\n" // headerless child
                            "}\n"
                            "Shape { geometry Sphere { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.rootNodes.size() == 2,
          "recover(b): implicit-child Group + trailing Shape both parsed");
    check(doc.scene.rootNodes.size() == 2 &&
              static_cast<bool>(as<Group>(doc.scene.rootNodes[0])) &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[1])),
          "recover(b): node graph stays aligned (no brace desync)");
  }

  // (c) Stray ']' at field-name position: an MFNode list (`skin [ ... ]`) whose
  // single Shape item's closing ']'/'}' was swallowed by a comment, so the
  // list's ']' lands inside the node body. The node is treated as closed, the
  // list consumes the ']', and the trailing field still parses.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "Group {\n"
                            "  children [\n"
                            "    Shape { geometry Box { } #swallowed ] }\n"
                            "  ]\n"  // this ']' closes children, seen in body
                            "}\n"
                            "Shape { geometry Sphere { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.rootNodes.size() == 2,
          "recover(c): stray ']' resync — Group + trailing Shape both parsed");
    auto g = as<Group>(doc.scene.rootNodes.at(0));
    check(g && g->getChildren().size() == 1,
          "recover(c): the one Shape child still attached");
  }

  // (d) Child node missing its '}' inside an MFNode list (the
  // SmokeTest...CommandLine shape): the array's ']' arrives at the Shape body's
  // field position. Shape is closed, the list ']' is consumed, the document
  // continues to a sibling node.
  {
    const std::string src = "#X3D V4.0 utf8\n"
                            "PROFILE Interchange\n"
                            "DEF G Group {\n"
                            "  children [\n"
                            "    Shape {\n" // '{' never closed before the ']'
                            "  ]\n"
                            "}\n"
                            "Shape { geometry Sphere { } }\n";
    ClassicVrmlReader reader;
    runtime::X3DDocument doc = reader.readDocument(src); // must not throw
    check(doc.scene.rootNodes.size() == 2,
          "recover(d): missing-'}' child closed, both roots parsed");
    auto g = as<Group>(doc.scene.resolve("G"));
    check(static_cast<bool>(g),
          "recover(d): DEF G Group recovered despite unclosed child");
  }
}

} // namespace

int main(int argc, char **argv) {
  if (argc > 1)
    g_dataDir = argv[1];
  else
    g_dataDir = "runtime/parse/tests/data/x3dv";

  std::cout << "===== ClassicVRML: corpus fixtures =====\n";
  testShipFixture();
  testHelloWorldFixture();
  testScriptFixture();
  testNurbsFixture();
  std::cout << "===== ClassicVRML: grammar snippets =====\n";
  testHeaderStatements();
  testDefUseIdentity();
  testMfReJoin();
  testRouteAndImportExport();
  testImportVariants();
  testProto();
  protoIsCaptureTest();
  testFrontDoorSniff();
  testUnknownNodeSkipped();
  testRecoverableMalformation();
  testStrayCloserAndImplicitChildRecovery();

  if (failures == 0) {
    std::cout << "\nALL CLASSICVRML READER CHECKS PASSED\n";
    return 0;
  }
  std::cerr << "\n" << failures << " CHECK(S) FAILED\n";
  return 1;
}
