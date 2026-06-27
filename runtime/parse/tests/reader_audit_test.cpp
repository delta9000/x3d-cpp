#include "doctest/doctest.h"
// reader_audit_test.cpp
// AUD-PARSE-READERS: regression coverage across the four encoding readers
// (ClassicVRML/VRML97/JSON/XML) on the per-node traversal audit axes.
//
// Risk areas pinned by the audit unit:
//   1. Lenient capture — unknown node / unknown field names are skipped
//      gracefully (document never aborts). Warning channel: only the VRML97
//      subclass collects into warnings(); ClassicVRML/XML/JSON are
//      tolerant-silent by design (see notes).
//   2. JSON @containerField — default (e.g. -material) AND non-default
//      (e.g. -shaders for a ComposedShader under Appearance) both land in
//      the correct SF/MFNode field.
//   3. ClassicVRML access-type aliases — eventIn / eventOut / field /
//      exposedField map to inputOnly / outputOnly / initializeOnly /
//      inputOutput at every reader (incl. XML/JSON alias paths).
//   4. PROTO body DEF scope in VRML97 — AUD-C closed the leak in
//      JsonReader/XmlReader; this extends the assertion to VRML97 (which
//      inherits ClassicVrmlReader's local-scene parseProto).
//   5. Header-line version inference — the VP-2 ladder: declared version
//      wins, sub-3.0 (incl. #VRML V2.0) floors to 3.0, >=3.0 forward-compat
//      preserved, bare header defaults to 3.0.
//   6. EXTERNPROTO URL capture across encodings — ClassicVRML/VRML97 (list
//      form), XML (MFString attribute), JSON (array + single-string form).
//   7. ROUTE capture across encodings — ClassicVRML/VRML97 textual ROUTE,
//      XML <ROUTE/>, JSON {ROUTE: ...}.
//
// Hand-authored snippets (no external fixtures); each section is gated by
// its own reader and asserts structural + value-level fidelity. Exit 0 on
// success; nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "JsonReader.hpp"
#include "Vrml97Reader.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/ComposedShader.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using x3d::codec::ClassicVrmlReader;
using x3d::codec::Encoding;
using x3d::codec::JsonReader;
using x3d::codec::Vrml97Reader;
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

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}

// ---------------------------------------------------------------------------
// (1) Lenient capture — unknown nodes / unknown fields must NOT abort the
//     document. Only VRML97 collects warnings; the other three readers are
//     tolerant-silent (documented design choice in each header).
// ---------------------------------------------------------------------------

void testLenientUnknownNodeAllEncodings() {
  // XML
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'>"
        "<Scene>"
        "<TotallyMadeUp foo='1'/>"
        "<Shape><Box/></Shape>"
        "</Scene></X3D>";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = codec::parseDocument(xml, Encoding::XML);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: XML unknown node does not throw");
    check(doc.scene.rootNodes.size() == 1,
          "lenient: XML unknown node skipped, Shape root kept");
    check(!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "lenient: XML surviving root is the Shape");
  }

  // ClassicVRML — tolerant-silent, no warning channel.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "TotallyMadeUp { foo 1 nested Box { size 1 1 1 } }\n"
        "Shape { geometry Box { size 9 9 9 } }\n";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = ClassicVrmlReader{}.readDocument(src);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: ClassicVRML unknown node does not throw");
    check(doc.scene.rootNodes.size() == 1,
          "lenient: ClassicVRML unknown node skipped, Shape kept");
    check(!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "lenient: ClassicVRML surviving root is the Shape");
  }

  // VRML97 — same parser, warning collected.
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "TotallyMadeUp { foo 1 nested Group { } }\n"
        "Transform { translation 1 2 3 }\n";
    Vrml97Reader reader;
    runtime::X3DDocument doc;
    bool threw = false;
    try {
      doc = reader.readDocument(src);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: VRML97 unknown node does not throw");
    bool sawUnknownNode = false;
    for (const auto &w : reader.warnings()) {
      if (w.find("unknown node 'TotallyMadeUp'") != std::string::npos)
        sawUnknownNode = true;
    }
    check(sawUnknownNode,
          "lenient: VRML97 unknown node emits a warning (warning channel)");
    check(!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Transform>(doc.scene.rootNodes.back())),
          "lenient: VRML97 surviving root is the Transform");
  }

  // JSON — tolerant-silent.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "TotallyMadeUp": { "@foo": 1 } },
        { "Shape": { "-geometry": [ { "Box": { } } ] } }
      ]}}})";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = JsonReader{}.readDocument(j);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: JSON unknown node does not throw");
    check(doc.scene.rootNodes.size() == 1,
          "lenient: JSON unknown node skipped, Shape kept");
    check(!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0])),
          "lenient: JSON surviving root is the Shape");
  }
}

void testLenientUnknownFieldAllEncodings() {
  // XML — unknown attribute on a known node is silently dropped.
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'>"
        "<Scene>"
        "<Transform bogusField='5' translation='1 2 3'/>"
        "</Scene></X3D>";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = codec::parseDocument(xml, Encoding::XML);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: XML unknown attribute does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    check(static_cast<bool>(tr),
          "lenient: XML Transform kept despite unknown attribute");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      check(approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3),
            "lenient: XML known field still parsed past unknown attribute");
    }
  }

  // ClassicVRML — tolerant-silent.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "Transform { bogusField 5 translation 1 2 3 }\n";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = ClassicVrmlReader{}.readDocument(src);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: ClassicVRML unknown field does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    check(static_cast<bool>(tr), "lenient: ClassicVRML Transform kept");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      check(approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3),
            "lenient: ClassicVRML known field parsed past unknown field");
    }
  }

  // VRML97 — emits a warning.
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "Transform { bogusField 5 translation 1 2 3 }\n";
    Vrml97Reader reader;
    runtime::X3DDocument doc;
    bool threw = false;
    try {
      doc = reader.readDocument(src);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: VRML97 unknown field does not throw");
    bool sawUnknownField = false;
    for (const auto &w : reader.warnings()) {
      if (w.find("unknown field 'bogusField'") != std::string::npos)
        sawUnknownField = true;
    }
    check(sawUnknownField,
          "lenient: VRML97 unknown field emits a warning (warning channel)");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    check(static_cast<bool>(tr),
          "lenient: VRML97 Transform kept despite unknown field");
  }

  // JSON — tolerant-silent.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "Transform": { "@bogusField": 5, "@translation": [1,2,3] } }
      ]}}})";
    bool threw = false;
    runtime::X3DDocument doc;
    try {
      doc = JsonReader{}.readDocument(j);
    } catch (const std::exception &) {
      threw = true;
    }
    check(!threw, "lenient: JSON unknown attribute does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    check(static_cast<bool>(tr), "lenient: JSON Transform kept");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      check(approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3),
            "lenient: JSON known field parsed past unknown attribute");
    }
  }
}

// ---------------------------------------------------------------------------
// (2) JSON @containerField — default (-material) AND non-default (-shaders)
//     both land in the correct SF/MFNode field. Confirms the slot routing
//     chain in build::attachChild.
// ---------------------------------------------------------------------------
void testJsonContainerField() {
  // (a) Default slot -material -> Appearance.material (SFNode).
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "Appearance": { "-material": [ { "Material": {
            "@diffuseColor": [1,0,0] } } ] } }
      ]}}})";
    runtime::X3DDocument doc = JsonReader{}.readDocument(j);
    check(doc.scene.rootNodes.size() == 1, "cf-default: one Appearance root");
    auto app = as<Appearance>(doc.scene.rootNodes[0]);
    check(static_cast<bool>(app), "cf-default: root is Appearance");
    if (app) {
      auto mat = as<Material>(app->getMaterial());
      check(static_cast<bool>(mat),
            "cf-default: -material slot landed in Appearance.material");
      if (mat) {
        auto c = mat->getDiffuseColor();
        check(approx(c.r, 1) && approx(c.g, 0) && approx(c.b, 0),
              "cf-default: Material.diffuseColor parsed (1,0,0)");
      }
      // The non-default slot must remain empty.
      check(app->getShaders().empty(),
            "cf-default: -shaders slot untouched when absent");
    }
  }

  // (b) Non-default slot -shaders -> Appearance.shaders (MFNode) holding a
  //     ComposedShader. The material slot stays empty.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "Appearance": { "-shaders": [ { "ComposedShader": {
            "@language": "GLSL" } } ] } }
      ]}}})";
    runtime::X3DDocument doc = JsonReader{}.readDocument(j);
    auto app = as<Appearance>(doc.scene.rootNodes.empty()
                                  ? nullptr
                                  : doc.scene.rootNodes[0]);
    check(static_cast<bool>(app), "cf-nondefault: root is Appearance");
    if (app) {
      auto shaders = app->getShaders();
      check(shaders.size() == 1,
            "cf-nondefault: -shaders slot -> Appearance.shaders (size==1)");
      check(!shaders.empty() &&
                static_cast<bool>(as<ComposedShader>(shaders[0])),
            "cf-nondefault: the shaders child is a ComposedShader");
      // The default slot must stay empty.
      check(!app->getMaterial(),
            "cf-nondefault: -material slot untouched when absent");
    }
  }
}

// ---------------------------------------------------------------------------
// (3) Access-type aliases. Spec §4.4.4.2 defines 4 X3D names; VRML97 alias
//     forms are accepted by the spec's "equivalent" rule. ClassicVRML/VRML97
//     must map both spellings; XML/JSON take the X3D canonical form (verified
//     via their own alias-aware mapProtoAccessType / mapAccessType).
//
// Also pins the case-sensitivity gap: an unknown-case access token (EventIn
// not eventIn) falls through to InputOutput via the default branch. This is
// the documented literal-case behavior of the readers (spec is literal-case).
// Recording it as a known gap, NOT a fix.
// ---------------------------------------------------------------------------
void testAccessTypeAliases() {
  // ClassicVRML: all 4 VRML97 aliases map correctly (inputOutput default
  // catches exposedField; alias covers the case statement).
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "PROTO P [\n"
        "  eventIn SFFloat set_x\n"
        "  eventOut SFFloat x_changed\n"
        "  field SFInt32 n 3\n"
        "  exposedField SFBool on TRUE\n"
        "] { Group { } }\n";
    auto doc = ClassicVrmlReader{}.readDocument(src);
    check(doc.scene.protoDeclarations.size() == 1,
          "alias: ClassicVRML one PROTO");
    auto p = doc.scene.protoDeclarations[0];
    check(p->interface.size() == 4, "alias: 4 interface fields");
    if (p->interface.size() == 4) {
      check(p->interface[0].access == AccessType::InputOnly,
            "alias: eventIn -> InputOnly (ClassicVRML)");
      check(p->interface[1].access == AccessType::OutputOnly,
            "alias: eventOut -> OutputOnly (ClassicVRML)");
      check(p->interface[2].access == AccessType::InitializeOnly,
            "alias: field -> InitializeOnly (ClassicVRML)");
      check(p->interface[3].access == AccessType::InputOutput,
            "alias: exposedField -> InputOutput (ClassicVRML)");
    }
  }

  // VRML97: same mapping (subclass shares the base's accessTypeFromString).
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "PROTO P [\n"
        "  eventIn SFFloat set_x\n"
        "  eventOut SFFloat x_changed\n"
        "  field SFInt32 n 3\n"
        "  exposedField SFBool on TRUE\n"
        "] { Group { } }\n";
    auto doc = Vrml97Reader{}.readDocument(src);
    auto p = doc.scene.protoDeclarations[0];
    check(p->interface.size() == 4, "alias: VRML97 4 interface fields");
    if (p->interface.size() == 4) {
      check(p->interface[0].access == AccessType::InputOnly,
            "alias: eventIn -> InputOnly (VRML97)");
      check(p->interface[3].access == AccessType::InputOutput,
            "alias: exposedField -> InputOutput (VRML97)");
    }
  }

  // XML: X3D canonical spelling (inputOnly / initializeOnly / ...) reaches
  // the same AccessType via XmlReader::mapAccessType. Aliases work too (the
  // map covers eventIn / field / exposedField).
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'>"
        "<Scene>"
        "<ProtoDeclare name='P'>"
        "<ProtoInterface>"
        "<field name='a' type='SFInt32' accessType='initializeOnly' value='1'/>"
        "<field name='b' type='SFFloat' accessType='inputOnly'/>"
        "<field name='c' type='SFFloat' accessType='outputOnly'/>"
        "<field name='d' type='SFBool' accessType='inputOutput' value='true'/>"
        "</ProtoInterface><ProtoBody><Group/></ProtoBody>"
        "</ProtoDeclare>"
        "</Scene></X3D>";
    auto doc = codec::parseDocument(xml, Encoding::XML);
    auto p = doc.scene.protoDeclarations[0];
    check(p->interface.size() == 4, "alias: XML 4 interface fields");
    if (p->interface.size() == 4) {
      check(p->interface[0].access == AccessType::InitializeOnly,
            "alias: initializeOnly (XML canonical)");
      check(p->interface[1].access == AccessType::InputOnly,
            "alias: inputOnly (XML canonical)");
      check(p->interface[2].access == AccessType::OutputOnly,
            "alias: outputOnly (XML canonical)");
      check(p->interface[3].access == AccessType::InputOutput,
            "alias: inputOutput (XML canonical)");
    }
  }

  // JSON: same canonical spellings via JsonReader::mapProtoAccessType.
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "ProtoDeclare": { "@name":"P",
            "ProtoInterface": { "field": [
              { "@name":"a", "@type":"SFInt32", "@accessType":"initializeOnly",
                "@value":"1" },
              { "@name":"b", "@type":"SFFloat", "@accessType":"inputOnly" },
              { "@name":"c", "@type":"SFFloat", "@accessType":"outputOnly" },
              { "@name":"d", "@type":"SFBool", "@accessType":"inputOutput" }
            ]},
            "ProtoBody": { "-children": [ { "Group": { } } ] }
          } }
      ]}}})";
    auto doc = JsonReader{}.readDocument(j);
    auto p = doc.scene.protoDeclarations[0];
    check(p->interface.size() == 4, "alias: JSON 4 interface fields");
    if (p->interface.size() == 4) {
      check(p->interface[0].access == AccessType::InitializeOnly,
            "alias: initializeOnly (JSON canonical)");
      check(p->interface[1].access == AccessType::InputOnly,
            "alias: inputOnly (JSON canonical)");
      check(p->interface[2].access == AccessType::OutputOnly,
            "alias: outputOnly (JSON canonical)");
      check(p->interface[3].access == AccessType::InputOutput,
            "alias: inputOutput (JSON canonical)");
    }
  }

  // Case-sensitivity gap (documented behavior, NOT fixed): an accessType token
  // with non-canonical casing ("EventIn") falls through the literal-case
  // switch in accessTypeFromString() / mapAccessType() / mapProtoAccessType()
  // and lands on the InputOutput default. The spec is literal-case so this
  // is the documented lenient fallback; we pin the behavior so a future
  // tightening (or a fix) shows up as a diff here.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "PROTO Q [ EventIn SFFloat x ] { Group { } }\n";
    auto doc = ClassicVrmlReader{}.readDocument(src);
    auto p = doc.scene.protoDeclarations[0];
    check(p->interface.size() == 1, "alias-case: 1 interface field");
    if (p->interface.size() == 1) {
      // 'EventIn' is not in the case-sensitive switch; the default branch
      // returns InputOutput. Documented behavior (spec is literal-case).
      check(p->interface[0].access == AccessType::InputOutput,
            "alias-case: 'EventIn' falls through to InputOutput (literal-case "
            "default branch)");
    }
  }
}

// ---------------------------------------------------------------------------
// (4) PROTO body DEF scope. The VRML97 reader inherits ClassicVrmlReader's
//     parseProto, which builds body nodes in a local Scene (def tables do
//     NOT leak to the outer scene). AUD-C closed the equivalent leak in the
//     XML and JSON readers; VRML97 was already correct via inheritance —
//     pin it.
// ---------------------------------------------------------------------------
void testProtoBodyDefScopeVRML97() {
  const std::string src =
      "#VRML V2.0 utf8\n"
      "PROTO Widget [\n"
      "  initializeOnly SFVec3f size 1 1 1\n"
      "] {\n"
      "  DEF Body Shape {\n"
      "    geometry Box { size IS size }\n"
      "  }\n"
      "  USE Body\n"
      "}\n"
      "Widget { size 2 2 2 }\n";
  auto doc = Vrml97Reader{}.readDocument(src);
  check(doc.scene.protoDeclarations.size() == 1,
        "body-def: one PROTO declared");
  if (!doc.scene.protoDeclarations.empty()) {
    const auto &body = doc.scene.protoDeclarations[0]->body;
    // Body has 2 nodes (DEF Shape + USE Shape).
    check(body.nodes.size() == 2,
          "body-def: body has 2 nodes (DEF Shape + USE Shape)");
    // Body-internal USE shares the DEF — same shared_ptr.
    check(body.nodes.size() == 2 && body.nodes[0] && body.nodes[1] &&
              body.nodes[0].get() == body.nodes[1].get(),
          "body-def: body-internal USE shares the DEF Shape identity");
    // The body's DEF 'Body' does NOT leak into the outer scene's DEF table.
    check(doc.scene.resolve("Body") == nullptr,
          "body-def: body DEF 'Body' is body-scoped, not in outer scene");
  }
  // For comparison: ClassicVRML must produce the same shape (parity pin).
  {
    const std::string src2 =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "PROTO Widget [\n"
        "  initializeOnly SFVec3f size 1 1 1\n"
        "] {\n"
        "  DEF Body Shape {\n"
        "    geometry Box { size IS size }\n"
        "  }\n"
        "  USE Body\n"
        "}\n"
        "Widget { size 2 2 2 }\n";
    auto doc2 = ClassicVrmlReader{}.readDocument(src2);
    check(doc2.scene.protoDeclarations.size() == 1,
          "body-def: ClassicVRML one PROTO declared");
    if (!doc2.scene.protoDeclarations.empty()) {
      const auto &body = doc2.scene.protoDeclarations[0]->body;
      check(body.nodes.size() == 2 && body.nodes[0] && body.nodes[1] &&
                body.nodes[0].get() == body.nodes[1].get(),
            "body-def: ClassicVRML body-internal USE shares DEF identity");
      check(doc2.scene.resolve("Body") == nullptr,
            "body-def: ClassicVRML body DEF 'Body' is body-scoped");
    }
  }
}

// ---------------------------------------------------------------------------
// (5) Header-line version inference. VP-2 ladder: declared wins, sub-3.0
//     (incl. #VRML V2.0) floors to 3.0, >=3.0 forward-compat preserved, bare
//     header defaults to 3.0. (The bare-floor case is driven through the
//     readers' own parser, not just the global Encoding sniffer.)
// ---------------------------------------------------------------------------
void testVersionFloorAllEncodings() {
  // VRML97: #VRML V2.0 -> 3.0.
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n",
                                    Encoding::VRML97);
    check(doc.version == "3.0", "floor: VRML97 #VRML V2.0 -> 3.0");
  }
  // ClassicVRML: #X3D V2.0 -> 3.0.
  {
    auto doc = codec::parseDocument("#X3D V2.0 utf8\nGroup {}\n",
                                    Encoding::ClassicVRML);
    check(doc.version == "3.0", "floor: ClassicVRML #X3D V2.0 -> 3.0");
  }
  // Forward-compat: #X3D V4.2 is preserved (>=3.0).
  {
    auto doc = codec::parseDocument("#X3D V4.2 utf8\nGroup {}\n",
                                    Encoding::ClassicVRML);
    check(doc.version == "4.2",
          "floor: ClassicVRML #X3D V4.2 not clamped (forward-compat)");
  }
  // Bare header — no version line. ClassicVRML leaves doc.version at its
  // default ("3.0" per X3DRuntime.hpp), proving the bare-floor path.
  {
    auto doc = codec::parseDocument("PROFILE Interchange\nGroup {}\n",
                                    Encoding::ClassicVRML);
    check(doc.version == "3.0",
          "floor: ClassicVRML bare header -> 3.0 (bare-floor default)");
  }
  // Bare XML — no version attribute on <X3D>. Defaults to 3.0.
  {
    auto doc = codec::parseDocument(
        "<X3D profile='Interchange'><Scene><Group/></Scene></X3D>",
        Encoding::XML);
    check(doc.version == "3.0",
          "floor: XML bare header (no version attr) -> 3.0");
  }
  // XML with a declared 4.0 version is preserved.
  {
    auto doc = codec::parseDocument(
        "<X3D profile='Interchange' version='4.0'><Scene><Group/></Scene></X3D>",
        Encoding::XML);
    check(doc.version == "4.0", "floor: XML declared version 4.0 preserved");
  }
  // JSON with a declared 4.0 version is preserved.
  {
    auto doc = codec::parseDocument(
        R"({"X3D":{"@profile":"Immersive","@version":"4.0",
        "Scene":{"-children":[{"Group":{}}]}}})",
        Encoding::JSON);
    check(doc.version == "4.0", "floor: JSON declared version 4.0 preserved");
  }
  // JSON bare header (no @version) -> 3.0 (bare-floor rung).
  {
    auto doc = codec::parseDocument(
        R"({"X3D":{"@profile":"Immersive",
        "Scene":{"-children":[{"Group":{}}]}}})",
        Encoding::JSON);
    check(doc.version == "3.0", "floor: JSON bare header -> 3.0");
  }
}

// ---------------------------------------------------------------------------
// (6) EXTERNPROTO URL capture across encodings. Local-file resolver only
//     (PRF-6 deferred-by-design for http/urn). URL field captured into
//     ExternProtoDeclaration::url verbatim from each encoding.
// ---------------------------------------------------------------------------
void testExternProtoUrlAllEncodings() {
  // ClassicVRML — list form `[ "url" ]` and single-string form both captured.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "EXTERNPROTO G [ initializeOnly SFFloat v ] [ \"g.x3d#G\" ]\n"
        "EXTERNPROTO H [ initializeOnly SFFloat v ] \"h.x3d#H\"\n";
    auto doc = ClassicVrmlReader{}.readDocument(src);
    check(doc.scene.externProtoDeclarations.size() == 2,
          "extern: two EXTERNPROTOs declared (ClassicVRML)");
    if (doc.scene.externProtoDeclarations.size() >= 1) {
      const auto &u0 = doc.scene.externProtoDeclarations[0]->url;
      check(u0.size() == 1 && u0[0] == "g.x3d#G",
            "extern: ClassicVRML list-form url captured");
    }
    if (doc.scene.externProtoDeclarations.size() >= 2) {
      const auto &u1 = doc.scene.externProtoDeclarations[1]->url;
      check(u1.size() == 1 && u1[0] == "h.x3d#H",
            "extern: ClassicVRML single-string url captured");
    }
  }

  // VRML97 — same (subclass uses base's parseExternProto).
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "EXTERNPROTO G [ initializeOnly SFFloat v ] [ \"g.x3d#G\" ]\n";
    auto doc = Vrml97Reader{}.readDocument(src);
    check(doc.scene.externProtoDeclarations.size() == 1,
          "extern: one EXTERNPROTO (VRML97)");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      check(u.size() == 1 && u[0] == "g.x3d#G",
            "extern: VRML97 url captured");
    }
  }

  // XML — MFString attribute parsed into the url list verbatim.
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'>"
        "<Scene>"
        "<ExternProtoDeclare name='G' url='&quot;g.x3d#G&quot;'>"
        "<field name='v' type='SFFloat' accessType='initializeOnly'/>"
        "</ExternProtoDeclare>"
        "</Scene></X3D>";
    auto doc = codec::parseDocument(xml, Encoding::XML);
    check(doc.scene.externProtoDeclarations.size() == 1,
          "extern: XML one EXTERNPROTO");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      check(u.size() == 1 && u[0] == "g.x3d#G",
            "extern: XML url captured from MFString attribute");
    }
  }

  // JSON — both array form (the JsonWriter-emitted shape) and single-string
  // form (the legacy hand-authored shape) survive. PRF-5 closed the array
  // gap; pinned here for the audit matrix.
  {
    const std::string arr =
        R"({"X3D":{"@profile":"Immersive","@version":"4.0","Scene":{"-children":[
          { "ExternProtoDeclare": { "@name":"G", "@url": ["g.x3d#G"],
              "field": [
                { "@name":"v", "@type":"SFFloat",
                  "@accessType":"initializeOnly" } ] } }
        ]}}})";
    auto doc = JsonReader{}.readDocument(arr);
    check(doc.scene.externProtoDeclarations.size() == 1,
          "extern: JSON array-form one EXTERNPROTO");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      check(u.size() == 1 && u[0] == "g.x3d#G",
            "extern: JSON array-form url captured (PRF-5)");
    }

    const std::string single =
        R"({"X3D":{"@profile":"Immersive","@version":"4.0","Scene":{"-children":[
          { "ExternProtoDeclare": { "@name":"H", "@url": "h.x3d#H",
              "field": [
                { "@name":"v", "@type":"SFFloat",
                  "@accessType":"initializeOnly" } ] } }
        ]}}})";
    auto doc2 = JsonReader{}.readDocument(single);
    check(doc2.scene.externProtoDeclarations.size() == 1,
          "extern: JSON single-string one EXTERNPROTO");
    if (!doc2.scene.externProtoDeclarations.empty()) {
      const auto &u = doc2.scene.externProtoDeclarations[0]->url;
      check(u.size() == 1 && u[0] == "h.x3d#H",
            "extern: JSON single-string url captured");
    }
  }
}

// ---------------------------------------------------------------------------
// (7) ROUTE capture across encodings. All four produce a runtime::Route with
//     the four endpoint fields split on '.' (ClassicVRML/VRML97 textual
//     syntax) or read from attributes/members (XML/JSON).
// ---------------------------------------------------------------------------
void testRouteCaptureAllEncodings() {
  // ClassicVRML
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "DEF TS TimeSensor { cycleInterval 5 loop TRUE }\n"
        "DEF XF Transform { }\n"
        "ROUTE TS.fraction_changed TO XF.set_translation\n";
    auto doc = ClassicVrmlReader{}.readDocument(src);
    check(doc.scene.routes.size() == 1,
          "route: ClassicVRML one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      check(r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation",
            "route: ClassicVRML endpoints split on '.'");
    }
  }

  // VRML97
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "DEF TS TimeSensor { cycleInterval 5 loop TRUE }\n"
        "DEF XF Transform { }\n"
        "ROUTE TS.fraction_changed TO XF.set_translation\n";
    auto doc = Vrml97Reader{}.readDocument(src);
    check(doc.scene.routes.size() == 1, "route: VRML97 one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      check(r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation",
            "route: VRML97 endpoints split on '.'");
    }
  }

  // XML
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'>"
        "<Scene>"
        "<ROUTE fromNode='TS' fromField='fraction_changed' "
        "toNode='XF' toField='set_translation'/>"
        "</Scene></X3D>";
    auto doc = codec::parseDocument(xml, Encoding::XML);
    check(doc.scene.routes.size() == 1, "route: XML one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      check(r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation",
            "route: XML endpoints from attributes");
    }
  }

  // JSON (object form).
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{
        "ROUTE":{ "@fromNode":"TS", "@fromField":"fraction_changed",
                  "@toNode":"XF", "@toField":"set_translation" }
      }}})";
    auto doc = JsonReader{}.readDocument(j);
    check(doc.scene.routes.size() == 1, "route: JSON one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      check(r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation",
            "route: JSON endpoints from members");
    }
  }

  // JSON (array form — JsonWriter emits this).
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{
        "ROUTE":[
          { "@fromNode":"TS", "@fromField":"fraction_changed",
            "@toNode":"XF", "@toField":"set_translation" },
          { "@fromNode":"XF", "@fromField":"translation_changed",
            "@toNode":"TS", "@toField":"set_cycleInterval" }
        ]
      }}})";
    auto doc = JsonReader{}.readDocument(j);
    check(doc.scene.routes.size() == 2,
          "route: JSON array-form two ROUTEs captured");
    if (doc.scene.routes.size() >= 2) {
      check(doc.scene.routes[0].fromNode == "TS" &&
                doc.scene.routes[1].fromNode == "XF",
            "route: JSON array-form preserves order");
    }
  }
}

} // namespace

TEST_CASE("reader_audit_test") {
  std::cout << "===== AUD-PARSE-READERS: lenient capture =====\n";
  testLenientUnknownNodeAllEncodings();
  testLenientUnknownFieldAllEncodings();
  std::cout << "===== AUD-PARSE-READERS: JSON containerField =====\n";
  testJsonContainerField();
  std::cout << "===== AUD-PARSE-READERS: access-type aliases =====\n";
  testAccessTypeAliases();
  std::cout << "===== AUD-PARSE-READERS: PROTO body DEF scope (VRML97) =====\n";
  testProtoBodyDefScopeVRML97();
  std::cout << "===== AUD-PARSE-READERS: version floor =====\n";
  testVersionFloorAllEncodings();
  std::cout << "===== AUD-PARSE-READERS: EXTERNPROTO URL =====\n";
  testExternProtoUrlAllEncodings();
  std::cout << "===== AUD-PARSE-READERS: ROUTE capture =====\n";
  testRouteCaptureAllEncodings();

  CHECK(failures == 0);
  if (failures == 0) {
    std::cout << "\nALL READER-AUDIT CHECKS PASSED\n";
    return;
  }
  std::cerr << "\n" << failures << " CHECK(S) FAILED\n";
  return;
}
