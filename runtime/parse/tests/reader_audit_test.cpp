#include "doctest/doctest.h"
// Encoding readers preserve fields, local scope, routes, and header versions.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "JsonReader.hpp"
#include "NodeBuilder.hpp"
#include "Vrml97Reader.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/ComposedShader.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/PixelTexture.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TextureProjectorParallel.hpp"
#include "x3d/nodes/Transform.hpp"

#include <cmath>
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

bool approx(double a, double b, double eps = 1e-4) {
  return std::fabs(a - b) <= eps;
}

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}
// (1) Lenient capture — unknown nodes / unknown fields must not abort the
//     document. Only VRML97 collects warnings; the other three readers are
//     tolerant-silent (documented design choice in each header).

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
    CHECK_MESSAGE((!threw), "lenient: XML unknown node does not throw");
    // The XML reader keeps the unknown element as an opaque node.
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 2),
          "lenient: XML unknown node kept opaque, Shape root kept");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 2 &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[1]))),
          "lenient: XML second root is the Shape");
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
    CHECK_MESSAGE((!threw), "lenient: ClassicVRML unknown node does not throw");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1),
          "lenient: ClassicVRML unknown node skipped, Shape kept");
    CHECK_MESSAGE((!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0]))),
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
    CHECK_MESSAGE((!threw), "lenient: VRML97 unknown node does not throw");
    bool sawUnknownNode = false;
    for (const auto &w : reader.warnings()) {
      if (w.find("unknown node 'TotallyMadeUp'") != std::string::npos)
        sawUnknownNode = true;
    }
    CHECK_MESSAGE((sawUnknownNode),
          "lenient: VRML97 unknown node emits a warning (warning channel)");
    CHECK_MESSAGE((!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Transform>(doc.scene.rootNodes.back()))),
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
    CHECK_MESSAGE((!threw), "lenient: JSON unknown node does not throw");
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1),
          "lenient: JSON unknown node skipped, Shape kept");
    CHECK_MESSAGE((!doc.scene.rootNodes.empty() &&
              static_cast<bool>(as<Shape>(doc.scene.rootNodes[0]))),
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
    CHECK_MESSAGE((!threw), "lenient: XML unknown attribute does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(tr)),
          "lenient: XML Transform kept despite unknown attribute");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      CHECK_MESSAGE((approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3)),
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
    CHECK_MESSAGE((!threw), "lenient: ClassicVRML unknown field does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(tr)), "lenient: ClassicVRML Transform kept");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      CHECK_MESSAGE((approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3)),
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
    CHECK_MESSAGE((!threw), "lenient: VRML97 unknown field does not throw");
    bool sawUnknownField = false;
    for (const auto &w : reader.warnings()) {
      if (w.find("unknown field 'bogusField'") != std::string::npos)
        sawUnknownField = true;
    }
    CHECK_MESSAGE((sawUnknownField),
          "lenient: VRML97 unknown field emits a warning (warning channel)");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(tr)),
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
    CHECK_MESSAGE((!threw), "lenient: JSON unknown attribute does not throw");
    auto tr = as<Transform>(doc.scene.rootNodes.empty()
                                ? nullptr
                                : doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(tr)), "lenient: JSON Transform kept");
    if (tr) {
      SFVec3f t = tr->getTranslation();
      CHECK_MESSAGE((approx(t.x, 1) && approx(t.y, 2) && approx(t.z, 3)),
            "lenient: JSON known field parsed past unknown attribute");
    }
  }
}
// (2) JSON @containerField — default (-material) and non-default (-shaders)
//     both land in the correct SF/MFNode field. Confirms the slot routing
//     chain in build::attachChild.
void testJsonContainerField() {
  // (a) Default slot -material -> Appearance.material (SFNode).
  {
    const std::string j = R"({"X3D":{"@profile":"Immersive","@version":"4.0",
      "Scene":{"-children":[
        { "Appearance": { "-material": [ { "Material": {
            "@diffuseColor": [1,0,0] } } ] } }
      ]}}})";
    runtime::X3DDocument doc = JsonReader{}.readDocument(j);
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), "cf-default: one Appearance root");
    auto app = as<Appearance>(doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(app)), "cf-default: root is Appearance");
    if (app) {
      auto mat = as<Material>(app->getMaterial());
      CHECK_MESSAGE((static_cast<bool>(mat)),
            "cf-default: -material slot landed in Appearance.material");
      if (mat) {
        auto c = mat->getDiffuseColor();
        CHECK_MESSAGE((approx(c.r, 1) && approx(c.g, 0) && approx(c.b, 0)),
              "cf-default: Material.diffuseColor parsed (1,0,0)");
      }
      // The non-default slot must remain empty.
      CHECK_MESSAGE((app->getShaders().empty()),
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
    CHECK_MESSAGE((static_cast<bool>(app)), "cf-nondefault: root is Appearance");
    if (app) {
      auto shaders = app->getShaders();
      CHECK_MESSAGE((shaders.size() == 1),
            "cf-nondefault: -shaders slot -> Appearance.shaders (size==1)");
      CHECK_MESSAGE((!shaders.empty() &&
                static_cast<bool>(as<ComposedShader>(shaders[0]))),
            "cf-nondefault: the shaders child is a ComposedShader");
      // The default slot must stay empty.
      CHECK_MESSAGE((!app->getMaterial()),
            "cf-nondefault: -material slot untouched when absent");
    }
  }
}
// (3) Access-type aliases. Spec §4.4.4.2 defines 4 X3D names; VRML97 alias
//     forms are accepted by the spec's "equivalent" rule. ClassicVRML/VRML97
//     must map both spellings; XML/JSON take the X3D canonical form (verified
//     via the shared alias-aware accessTypeFromName).
//
// Also pins the case-sensitivity gap: an unknown-case access token (EventIn
// not eventIn) falls through to InputOutput via the default branch. This is
// the documented literal-case behavior of the readers (spec is literal-case).
// Recording it as a known gap, not a fix.
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
    CHECK_MESSAGE((doc.scene.protoDeclarations.size() == 1),
          "alias: ClassicVRML one PROTO");
    auto p = doc.scene.protoDeclarations[0];
    CHECK_MESSAGE((p->interface.size() == 4), "alias: 4 interface fields");
    if (p->interface.size() == 4) {
      CHECK_MESSAGE((p->interface[0].access == AccessType::InputOnly),
            "alias: eventIn -> InputOnly (ClassicVRML)");
      CHECK_MESSAGE((p->interface[1].access == AccessType::OutputOnly),
            "alias: eventOut -> OutputOnly (ClassicVRML)");
      CHECK_MESSAGE((p->interface[2].access == AccessType::InitializeOnly),
            "alias: field -> InitializeOnly (ClassicVRML)");
      CHECK_MESSAGE((p->interface[3].access == AccessType::InputOutput),
            "alias: exposedField -> InputOutput (ClassicVRML)");
    }
  }

  // VRML97: same mapping (subclass shares accessTypeFromName).
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
    CHECK_MESSAGE((p->interface.size() == 4), "alias: VRML97 4 interface fields");
    if (p->interface.size() == 4) {
      CHECK_MESSAGE((p->interface[0].access == AccessType::InputOnly),
            "alias: eventIn -> InputOnly (VRML97)");
      CHECK_MESSAGE((p->interface[3].access == AccessType::InputOutput),
            "alias: exposedField -> InputOutput (VRML97)");
    }
  }

  // XML: X3D canonical spelling (inputOnly / initializeOnly / ...) reaches
  // the same AccessType via accessTypeFromName. Aliases work too (the
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
    CHECK_MESSAGE((p->interface.size() == 4), "alias: XML 4 interface fields");
    if (p->interface.size() == 4) {
      CHECK_MESSAGE((p->interface[0].access == AccessType::InitializeOnly),
            "alias: initializeOnly (XML canonical)");
      CHECK_MESSAGE((p->interface[1].access == AccessType::InputOnly),
            "alias: inputOnly (XML canonical)");
      CHECK_MESSAGE((p->interface[2].access == AccessType::OutputOnly),
            "alias: outputOnly (XML canonical)");
      CHECK_MESSAGE((p->interface[3].access == AccessType::InputOutput),
            "alias: inputOutput (XML canonical)");
    }
  }

  // JSON: same canonical spellings via accessTypeFromName.
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
    CHECK_MESSAGE((p->interface.size() == 4), "alias: JSON 4 interface fields");
    if (p->interface.size() == 4) {
      CHECK_MESSAGE((p->interface[0].access == AccessType::InitializeOnly),
            "alias: initializeOnly (JSON canonical)");
      CHECK_MESSAGE((p->interface[1].access == AccessType::InputOnly),
            "alias: inputOnly (JSON canonical)");
      CHECK_MESSAGE((p->interface[2].access == AccessType::OutputOnly),
            "alias: outputOnly (JSON canonical)");
      CHECK_MESSAGE((p->interface[3].access == AccessType::InputOutput),
            "alias: inputOutput (JSON canonical)");
    }
  }

  // Case-sensitivity gap (documented behavior, not fixed): an accessType token
  // with non-canonical casing ("EventIn") falls through the literal-case
  // switch in accessTypeFromName()
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
    CHECK_MESSAGE((p->interface.size() == 1), "alias-case: 1 interface field");
    if (p->interface.size() == 1) {
      // 'EventIn' is not in the case-sensitive switch; the default branch
      // returns InputOutput. Documented behavior (spec is literal-case).
      CHECK_MESSAGE((p->interface[0].access == AccessType::InputOutput),
            "alias-case: 'EventIn' falls through to InputOutput (literal-case "
            "default branch)");
    }
  }
}
// PROTO body DEFs remain local and body-internal USEs share identity.
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
  CHECK_MESSAGE((doc.scene.protoDeclarations.size() == 1),
        "body-def: one PROTO declared");
  if (!doc.scene.protoDeclarations.empty()) {
    const auto &body = doc.scene.protoDeclarations[0]->body;
    // Body has 2 nodes (DEF Shape + USE Shape).
    CHECK_MESSAGE((body.nodes.size() == 2),
          "body-def: body has 2 nodes (DEF Shape + USE Shape)");
    // Body-internal USE shares the DEF — same shared_ptr.
    CHECK_MESSAGE((body.nodes.size() == 2 && body.nodes[0] && body.nodes[1] &&
              body.nodes[0].get() == body.nodes[1].get()),
          "body-def: body-internal USE shares the DEF Shape identity");
    // The body's DEF 'Body' does not leak into the outer scene's DEF table.
    CHECK_MESSAGE((doc.scene.resolve("Body") == nullptr),
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
    CHECK_MESSAGE((doc2.scene.protoDeclarations.size() == 1),
          "body-def: ClassicVRML one PROTO declared");
    if (!doc2.scene.protoDeclarations.empty()) {
      const auto &body = doc2.scene.protoDeclarations[0]->body;
      CHECK_MESSAGE((body.nodes.size() == 2 && body.nodes[0] && body.nodes[1] &&
                body.nodes[0].get() == body.nodes[1].get()),
            "body-def: ClassicVRML body-internal USE shares DEF identity");
      CHECK_MESSAGE((doc2.scene.resolve("Body") == nullptr),
            "body-def: ClassicVRML body DEF 'Body' is body-scoped");
    }
  }
}
// (5) Header-line version inference.  ladder: declared wins, sub-3.0
//     (incl. #VRML V2.0) floors to 3.0, >=3.0 forward-compat preserved, bare
//     header defaults to 3.0. (The bare-floor case is driven through the
//     readers' own parser, not just the global Encoding sniffer.)
void testVersionFloorAllEncodings() {
  // VRML97: #VRML V2.0 -> 3.0.
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n",
                                    Encoding::VRML97);
    CHECK_MESSAGE((doc.version == "3.0"), "floor: VRML97 #VRML V2.0 -> 3.0");
  }
  // ClassicVRML: #X3D V2.0 -> 3.0.
  {
    auto doc = codec::parseDocument("#X3D V2.0 utf8\nGroup {}\n",
                                    Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "3.0"), "floor: ClassicVRML #X3D V2.0 -> 3.0");
  }
  // Forward-compat: #X3D V4.2 is preserved (>=3.0).
  {
    auto doc = codec::parseDocument("#X3D V4.2 utf8\nGroup {}\n",
                                    Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "4.2"),
          "floor: ClassicVRML #X3D V4.2 not clamped (forward-compat)");
  }
  // Bare header — no version line. ClassicVRML leaves doc.version at its
  // default ("3.0" per X3DRuntime.hpp), proving the bare-floor path.
  {
    auto doc = codec::parseDocument("PROFILE Interchange\nGroup {}\n",
                                    Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "3.0"),
          "floor: ClassicVRML bare header -> 3.0 (bare-floor default)");
  }
  // Bare XML — no version attribute on <X3D>. Defaults to 3.0.
  {
    auto doc = codec::parseDocument(
        "<X3D profile='Interchange'><Scene><Group/></Scene></X3D>",
        Encoding::XML);
    CHECK_MESSAGE((doc.version == "3.0"),
          "floor: XML bare header (no version attr) -> 3.0");
  }
  // XML with a declared 4.0 version is preserved.
  {
    auto doc = codec::parseDocument(
        "<X3D profile='Interchange' version='4.0'><Scene><Group/></Scene></X3D>",
        Encoding::XML);
    CHECK_MESSAGE((doc.version == "4.0"), "floor: XML declared version 4.0 preserved");
  }
  // JSON with a declared 4.0 version is preserved.
  {
    auto doc = codec::parseDocument(
        R"({"X3D":{"@profile":"Immersive","@version":"4.0",
        "Scene":{"-children":[{"Group":{}}]}}})",
        Encoding::JSON);
    CHECK_MESSAGE((doc.version == "4.0"), "floor: JSON declared version 4.0 preserved");
  }
  // JSON bare header (no @version) -> 3.0 (bare-floor rung).
  {
    auto doc = codec::parseDocument(
        R"({"X3D":{"@profile":"Immersive",
        "Scene":{"-children":[{"Group":{}}]}}})",
        Encoding::JSON);
    CHECK_MESSAGE((doc.version == "3.0"), "floor: JSON bare header -> 3.0");
  }
}
// (6) EXTERNPROTO URL capture across encodings. Local-file resolver only
//     (http/urn resolution is delegated to the caller). URL field captured into
//     ExternProtoDeclaration::url verbatim from each encoding.
void testExternProtoUrlAllEncodings() {
  // ClassicVRML — list form `[ "url" ]` and single-string form both captured.
  {
    const std::string src =
        "#X3D V4.0 utf8\n"
        "PROFILE Interchange\n"
        "EXTERNPROTO G [ initializeOnly SFFloat v ] [ \"g.x3d#G\" ]\n"
        "EXTERNPROTO H [ initializeOnly SFFloat v ] \"h.x3d#H\"\n";
    auto doc = ClassicVrmlReader{}.readDocument(src);
    CHECK_MESSAGE((doc.scene.externProtoDeclarations.size() == 2),
          "extern: two EXTERNPROTOs declared (ClassicVRML)");
    if (doc.scene.externProtoDeclarations.size() >= 1) {
      const auto &u0 = doc.scene.externProtoDeclarations[0]->url;
      CHECK_MESSAGE((u0.size() == 1 && u0[0] == "g.x3d#G"),
            "extern: ClassicVRML list-form url captured");
    }
    if (doc.scene.externProtoDeclarations.size() >= 2) {
      const auto &u1 = doc.scene.externProtoDeclarations[1]->url;
      CHECK_MESSAGE((u1.size() == 1 && u1[0] == "h.x3d#H"),
            "extern: ClassicVRML single-string url captured");
    }
  }

  // VRML97 — same (subclass uses base's parseExternProto).
  {
    const std::string src =
        "#VRML V2.0 utf8\n"
        "EXTERNPROTO G [ initializeOnly SFFloat v ] [ \"g.x3d#G\" ]\n";
    auto doc = Vrml97Reader{}.readDocument(src);
    CHECK_MESSAGE((doc.scene.externProtoDeclarations.size() == 1),
          "extern: one EXTERNPROTO (VRML97)");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      CHECK_MESSAGE((u.size() == 1 && u[0] == "g.x3d#G"),
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
    CHECK_MESSAGE((doc.scene.externProtoDeclarations.size() == 1),
          "extern: XML one EXTERNPROTO");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      CHECK_MESSAGE((u.size() == 1 && u[0] == "g.x3d#G"),
            "extern: XML url captured from MFString attribute");
    }
  }

  // JSON URL arrays and single-string forms retain candidates.
  {
    const std::string arr =
        R"({"X3D":{"@profile":"Immersive","@version":"4.0","Scene":{"-children":[
          { "ExternProtoDeclare": { "@name":"G", "@url": ["g.x3d#G"],
              "field": [
                { "@name":"v", "@type":"SFFloat",
                  "@accessType":"initializeOnly" } ] } }
        ]}}})";
    auto doc = JsonReader{}.readDocument(arr);
    CHECK_MESSAGE((doc.scene.externProtoDeclarations.size() == 1),
          "extern: JSON array-form one EXTERNPROTO");
    if (!doc.scene.externProtoDeclarations.empty()) {
      const auto &u = doc.scene.externProtoDeclarations[0]->url;
      CHECK_MESSAGE((u.size() == 1 && u[0] == "g.x3d#G"),
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
    CHECK_MESSAGE((doc2.scene.externProtoDeclarations.size() == 1),
          "extern: JSON single-string one EXTERNPROTO");
    if (!doc2.scene.externProtoDeclarations.empty()) {
      const auto &u = doc2.scene.externProtoDeclarations[0]->url;
      CHECK_MESSAGE((u.size() == 1 && u[0] == "h.x3d#H"),
            "extern: JSON single-string url captured");
    }
  }
}
// (7) ROUTE capture across encodings. All four produce a runtime::Route with
//     the four endpoint fields split on '.' (ClassicVRML/VRML97 textual
//     syntax) or read from attributes/members (XML/JSON).
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
    CHECK_MESSAGE((doc.scene.routes.size() == 1),
          "route: ClassicVRML one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      CHECK_MESSAGE((r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation"),
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
    CHECK_MESSAGE((doc.scene.routes.size() == 1), "route: VRML97 one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      CHECK_MESSAGE((r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation"),
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
    CHECK_MESSAGE((doc.scene.routes.size() == 1), "route: XML one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      CHECK_MESSAGE((r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation"),
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
    CHECK_MESSAGE((doc.scene.routes.size() == 1), "route: JSON one ROUTE captured");
    if (!doc.scene.routes.empty()) {
      const auto &r = doc.scene.routes[0];
      CHECK_MESSAGE((r.fromNode == "TS" && r.fromField == "fraction_changed" &&
                r.toNode == "XF" && r.toField == "set_translation"),
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
    CHECK_MESSAGE((doc.scene.routes.size() == 2),
          "route: JSON array-form two ROUTEs captured");
    if (doc.scene.routes.size() >= 2) {
      CHECK_MESSAGE((doc.scene.routes[0].fromNode == "TS" &&
                doc.scene.routes[1].fromNode == "XF"),
            "route: JSON array-form preserves order");
    }
  }
}

void testTextureProjectorParallelShadowIntensityAliasAllEncodings() {
  auto checkProjector = [](const runtime::X3DDocument &doc,
                           const std::string &label) {
    CHECK_MESSAGE((doc.scene.rootNodes.size() == 1), (label + ": one root node"));
    auto projector = doc.scene.rootNodes.empty()
                         ? nullptr
                         : as<TextureProjectorParallel>(doc.scene.rootNodes[0]);
    CHECK_MESSAGE((static_cast<bool>(projector)),
          (label + ": root is TextureProjectorParallel"));
    if (projector)
      CHECK_MESSAGE((approx(projector->getShadowIntensity(), 0.25)),
            (label + ": shadowsIntensity aliases shadowIntensity"));
  };

  {
    const std::string xml =
        "<X3D profile='Full' version='4.0'><Scene>"
        "<TextureProjectorParallel shadowsIntensity='0.25'/>"
        "</Scene></X3D>";
    checkProjector(codec::parseDocument(xml, Encoding::XML),
                   "field-alias: XML TextureProjectorParallel");
  }

  {
    const std::string vrml =
        "#X3D V4.0 utf8\n"
        "PROFILE Full\n"
        "TextureProjectorParallel { shadowsIntensity 0.25 }\n";
    checkProjector(ClassicVrmlReader{}.readDocument(vrml),
                   "field-alias: ClassicVRML TextureProjectorParallel");
  }

  {
    const std::string vrml =
        "#VRML V2.0 utf8\n"
        "TextureProjectorParallel { shadowsIntensity 0.25 }\n";
    checkProjector(Vrml97Reader{}.readDocument(vrml),
                   "field-alias: VRML97 TextureProjectorParallel");
  }

  {
    const std::string json = R"({
      "X3D": {
        "@profile": "Full",
        "@version": "4.0",
        "Scene": {
          "-children": [
            { "TextureProjectorParallel": { "@shadowsIntensity": 0.25 } }
          ]
        }
      }
    })";
    checkProjector(JsonReader{}.readDocument(json),
                   "field-alias: JSON TextureProjectorParallel");
  }
}

} // namespace

TEST_CASE("reader_audit_test") {

  testLenientUnknownNodeAllEncodings();
  testLenientUnknownFieldAllEncodings();

  testJsonContainerField();

  testAccessTypeAliases();

  testProtoBodyDefScopeVRML97();

  testVersionFloorAllEncodings();

  testExternProtoUrlAllEncodings();

  testRouteCaptureAllEncodings();

  testTextureProjectorParallelShadowIntensityAliasAllEncodings();

  return;
}

TEST_CASE("JSON typed author and prototype field values are retained") {
  const auto doc = JsonReader{}.readDocument(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
    {"ProtoDeclare":{"@name":"P","ProtoInterface":{"field":[
      {"@name":"size","@type":"SFVec3f","@accessType":"initializeOnly","@value":[3,4,5]}
    ]},"ProtoBody":{"-children":[{"Box":{"IS":{"connect":[{"@nodeField":"size","@protoField":"size"}]}}}]}}},
    {"ProtoInstance":{"@name":"P","fieldValue":[{"@name":"size","@value":[6,7,8]}]}},
    {"Script":{"@DEF":"S","field":[
      {"@name":"enabled","@type":"SFBool","@accessType":"initializeOnly","@value":true},
      {"@name":"gain","@type":"SFFloat","@accessType":"inputOutput","@value":2.5},
      {"@name":"samples","@type":"MFInt32","@accessType":"initializeOnly","@value":[2,4,6]},
      {"@name":"labels","@type":"MFString","@accessType":"initializeOnly","@value":["two words","quote\"slash\\"]},
      {"@name":"legacy","@type":"SFVec3f","@accessType":"initializeOnly","@value":"9 8 7"}
    ]}}
  ]}}})");
  REQUIRE(doc.scene.protoDeclarations.size() == 1);
  const auto &value = doc.scene.protoDeclarations[0]->interface[0].value;
  REQUIRE(std::any_cast<SFVec3f>(&value));
  CHECK(std::any_cast<SFVec3f>(value) == SFVec3f{3, 4, 5});
  REQUIRE(doc.scene.protoInstances.size() == 1);
  const auto &overrideValue = doc.scene.protoInstances[0].fieldValues[0].value;
  REQUIRE(std::any_cast<SFVec3f>(&overrideValue));
  CHECK(std::any_cast<SFVec3f>(overrideValue) == SFVec3f{6, 7, 8});
  auto script = doc.scene.resolve("S");
  REQUIRE(script);
  CHECK(std::any_cast<SFBool>(doc.scene.authorFields->getValue(*script, "enabled")));
  CHECK(std::any_cast<SFFloat>(doc.scene.authorFields->getValue(*script, "gain")) == 2.5f);
  CHECK(std::any_cast<MFInt32>(doc.scene.authorFields->getValue(*script, "samples")) == MFInt32{2, 4, 6});
  CHECK(std::any_cast<MFString>(doc.scene.authorFields->getValue(*script, "labels")) == MFString{"two words", "quote\"slash\\"});
  CHECK(std::any_cast<SFVec3f>(doc.scene.authorFields->getValue(*script, "legacy")) == SFVec3f{9, 8, 7});
}

TEST_CASE("JSON statement wrappers in children retain their execution scope") {
  const auto doc = JsonReader{}.readDocument(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
    {"TimeSensor":{"@DEF":"Clock"}},
    {"ScalarInterpolator":{"@DEF":"Interp"}},
    {"Inline":{"@DEF":"I","@load":false}},
    {"ROUTE":{"@fromNode":"Clock","@fromField":"fraction_changed","@toNode":"Interp","@toField":"set_fraction"}},
    {"IMPORT":[{"@inlineDEF":"I","@importedDEF":"Remote","@AS":"Local"}]},
    {"EXPORT":[{"@localDEF":"Interp","@AS":"Public"}]},
    {"Group":{"-children":[
      {"ROUTE":{"@fromNode":"Clock","@fromField":"fraction_changed","@toNode":"Interp","@toField":"set_fraction"}}
    ]}},
    {"ProtoDeclare":{"@name":"P","ProtoBody":{"-children":[
      {"Group":{"-children":[
        {"TimeSensor":{"@DEF":"BodyClock"}},
        {"ScalarInterpolator":{"@DEF":"BodyInterp"}},
        {"ROUTE":{"@fromNode":"BodyClock","@fromField":"fraction_changed","@toNode":"BodyInterp","@toField":"set_fraction"}}
      ]}},
      {"ROUTE":{"@fromNode":"BodyClock","@fromField":"fraction_changed","@toNode":"BodyInterp","@toField":"set_fraction"}}
    ]}}}
  ]}}})");
  CHECK(doc.scene.rootNodes.size() == 4);
  REQUIRE(doc.scene.routes.size() == 2);
  CHECK(doc.scene.routes[0].fromNode == "Clock");
  CHECK(doc.scene.routes[1].toNode == "Interp");
  REQUIRE(doc.scene.imports.size() == 1);
  CHECK(doc.scene.imports[0].inlineDEF == "I");
  CHECK(doc.scene.imports[0].importedDEF == "Remote");
  CHECK(doc.scene.imports[0].as == "Local");
  REQUIRE(doc.scene.exports.size() == 1);
  CHECK(doc.scene.exports[0].localDEF == "Interp");
  CHECK(doc.scene.exports[0].as == "Public");
  REQUIRE(doc.scene.protoDeclarations.size() == 1);
  const auto &body = doc.scene.protoDeclarations[0]->body;
  CHECK(body.nodes.size() == 1);
  REQUIRE(body.routes.size() == 2);
  CHECK(body.routes[0].fromNode == "BodyClock");
  CHECK(body.routes[1].toNode == "BodyInterp");
  CHECK(doc.readerWarnings.empty());
}

TEST_CASE("Classic Script declaration IS retains author field connections") {
  const std::string text = R"(#X3D V4.0 utf8
PROFILE Full
PROTO P [ initializeOnly SFFloat initial 3 inputOutput SFFloat value 4
          inputOnly SFFloat incoming outputOnly SFFloat outgoing ] {
  Script { initializeOnly SFFloat a IS initial inputOutput SFFloat b IS value
           inputOnly SFFloat c IS incoming outputOnly SFFloat d IS outgoing }
}
P {}
)";
  for (bool vrml97 : {false, true}) {
    CAPTURE(vrml97);
    auto doc = vrml97 ? Vrml97Reader{}.readDocument(text) : ClassicVrmlReader{}.readDocument(text);
    REQUIRE(doc.scene.protoDeclarations.size() == 1);
    const auto &decl = *doc.scene.protoDeclarations[0];
    REQUIRE(decl.body.isConnections.size() == 4);
    CHECK(decl.body.isConnections[0].nodeField == "a");
    CHECK(decl.body.isConnections[0].protoField == "initial");
    CHECK(decl.body.isConnections[1].nodeField == "b");
    CHECK(decl.body.isConnections[1].protoField == "value");
    CHECK(decl.body.isConnections[2].nodeField == "c");
    CHECK(decl.body.isConnections[2].protoField == "incoming");
    CHECK(decl.body.isConnections[3].nodeField == "d");
    CHECK(decl.body.isConnections[3].protoField == "outgoing");
    REQUIRE(decl.body.nodes.size() == 1);
    CHECK(decl.authorFields->hasAuthorFields(*decl.body.nodes[0]));
  }
  auto expanded = codec::parseDocument(text, Encoding::ClassicVRML);
  REQUIRE(expanded.scene.rootNodes.size() == 1);
  auto script = expanded.scene.rootNodes[0];
  CHECK(std::any_cast<SFFloat>(expanded.scene.authorFields->getValue(*script, "a")) == 3.f);
  CHECK(std::any_cast<SFFloat>(expanded.scene.authorFields->getValue(*script, "b")) == 4.f);
}

TEST_CASE("Classic SFImage leading zero dimensions collect decimal pixel counts") {
  for (bool vrml97 : {false, true}) {
    CAPTURE(vrml97);
    const std::string text = "#X3D V4.0 utf8\nPROFILE Full\nPixelTexture { image 01 08 1 1 2 3 4 5 6 7 0xFF repeatS FALSE }";
    auto doc = vrml97 ? Vrml97Reader{}.readDocument(text) : ClassicVrmlReader{}.readDocument(text);
    REQUIRE(doc.scene.rootNodes.size() == 1);
    auto texture = as<PixelTexture>(doc.scene.rootNodes[0]);
    REQUIRE(texture);
    CHECK(texture->getImage().width == 1);
    CHECK(texture->getImage().height == 8);
    CHECK(texture->getImage().data == std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 255});
    CHECK_FALSE(texture->getRepeatS());
  }
}

TEST_CASE("Classic bare MFImage captures a complete single image") {
  for (bool vrml97 : {false, true}) {
    CAPTURE(vrml97);
    const std::string text = R"(#X3D V4.0 utf8
PROFILE Full
Script { initializeOnly MFImage images 1 1 3 0xFF8040
         initializeOnly MFImage emptyImage 0 0 0
         initializeOnly SFFloat after 2 }
)";
    auto doc = vrml97 ? Vrml97Reader{}.readDocument(text) : ClassicVrmlReader{}.readDocument(text);
    REQUIRE(doc.scene.rootNodes.size() == 1);
    auto script = doc.scene.rootNodes[0];
    const auto images = std::any_cast<MFImage>(doc.scene.authorFields->getValue(*script, "images"));
    REQUIRE(images.size() == 1);
    CHECK(images[0].width == 1);
    CHECK(images[0].height == 1);
    CHECK(images[0].numComponents == 3);
    CHECK(images[0].data == std::vector<std::uint8_t>{255, 128, 64});
    const auto empty = std::any_cast<MFImage>(doc.scene.authorFields->getValue(*script, "emptyImage"));
    REQUIRE(empty.size() == 1);
    CHECK(empty[0].data.empty());
    CHECK(std::any_cast<SFFloat>(doc.scene.authorFields->getValue(*script, "after")) == 2.f);
  }
}

TEST_CASE("Classic and JSON capture programmable shader author fields") {
  for (const std::string type : {"ComposedShader", "PackagedShader", "ShaderProgram"}) {
    CAPTURE(type);
    const std::string classic = "#X3D V4.0 utf8\nPROFILE Full\nDEF S " + type + R"( {
      inputOutput SFColor tint 1 0.5 0.25
      inputOnly SFFloat incoming outputOnly SFFloat outgoing
    })";
    const std::string json = R"({"X3D":{"@version":"4.0","Scene":{"-children":[{")" + type + R"(":{
      "@DEF":"S","field":[
        {"@name":"tint","@type":"SFColor","@accessType":"inputOutput","@value":[1,0.5,0.25]},
        {"@name":"incoming","@type":"SFFloat","@accessType":"inputOnly"},
        {"@name":"outgoing","@type":"SFFloat","@accessType":"outputOnly"}
      ]}}]}}})";
    for (bool useJson : {false, true}) {
      CAPTURE(useJson);
      auto doc = useJson ? JsonReader{}.readDocument(json) : ClassicVrmlReader{}.readDocument(classic);
      auto shader = doc.scene.resolve("S");
      REQUIRE(shader);
      CHECK(doc.scene.authorFields->hasAuthorFields(*shader));
      if (!doc.scene.authorFields->hasAuthorFields(*shader))
        continue;
      const auto fields = runtime::effectiveFields(*shader, *doc.scene.authorFields);
      const auto *tint = codec::build::findField(fields, "tint");
      REQUIRE(tint);
      CHECK(tint->type == X3DFieldType::SFColor);
      CHECK(tint->access == AccessType::InputOutput);
      CHECK(std::any_cast<SFColor>(tint->get(*shader)) == SFColor{1, 0.5f, 0.25f});
      REQUIRE(codec::build::findField(fields, "incoming"));
      CHECK(codec::build::findField(fields, "incoming")->access == AccessType::InputOnly);
      REQUIRE(codec::build::findField(fields, "outgoing"));
      CHECK(codec::build::findField(fields, "outgoing")->access == AccessType::OutputOnly);
    }
  }
}

TEST_CASE("Classic and JSON retain Script author node defaults and aliases") {
  const std::string classic = R"(#X3D V4.0 utf8
PROFILE Full
PROTO Item [] { Group {} }
DEF S Script {
  initializeOnly SFNode target DEF Target Group {}
  inputOutput MFNode items [ USE Target DEF Member Transform {} ]
  initializeOnly SFNode none NULL
  inputOutput MFNode empty []
  initializeOnly SFNode prototype DEF DefaultInstance Item {}
  inputOutput MFNode prototypes [ USE DefaultInstance Group { children [ Item {} ] } ]
}
)";
  const std::string json = R"({"X3D":{"@version":"4.0","Scene":{"-children":[
    {"ProtoDeclare":{"@name":"Item","ProtoBody":{"-children":[{"Group":{}}]}}},
    {"Script":{
    "@DEF":"S","field":[
      {"@name":"target","@type":"SFNode","@accessType":"initializeOnly","-children":[{"Group":{"@DEF":"Target"}}]},
      {"@name":"items","@type":"MFNode","@accessType":"inputOutput","-children":[{"Group":{"@USE":"Target"}},{"Transform":{"@DEF":"Member"}}]},
      {"@name":"none","@type":"SFNode","@accessType":"initializeOnly","-children":[]},
      {"@name":"empty","@type":"MFNode","@accessType":"inputOutput","-children":[]},
      {"@name":"prototype","@type":"SFNode","@accessType":"initializeOnly","-children":[{"ProtoInstance":{"@name":"Item","@DEF":"DefaultInstance"}}]},
      {"@name":"prototypes","@type":"MFNode","@accessType":"inputOutput","-children":[{"ProtoInstance":{"@USE":"DefaultInstance"}},{"Group":{"-children":[{"ProtoInstance":{"@name":"Item"}}]}}]}
    ]}}]}}})";
  for (int encoding : {0, 1, 2}) {
    CAPTURE(encoding);
    auto doc = encoding == 0 ? JsonReader{}.readDocument(json)
        : encoding == 1 ? ClassicVrmlReader{}.readDocument(classic)
                        : Vrml97Reader{}.readDocument(classic);
    auto script = doc.scene.resolve("S");
    REQUIRE(script);
    const auto targetValue = doc.scene.authorFields->getValue(*script, "target");
    const auto *targetPtr = std::any_cast<SFNode>(&targetValue);
    CHECK(targetPtr);
    if (!targetPtr)
      continue;
    const auto target = *targetPtr;
    CHECK(target);
    if (!target)
      continue;
    CHECK(target->nodeTypeName() == "Group");
    CHECK(doc.scene.resolve("Target") == target);
    const auto items = std::any_cast<MFNode>(doc.scene.authorFields->getValue(*script, "items"));
    REQUIRE(items.size() == 2);
    CHECK(items[0] == target);
    CHECK(items[1]->nodeTypeName() == "Transform");
    CHECK(doc.scene.resolve("Member") == items[1]);
    CHECK_FALSE(std::any_cast<SFNode>(doc.scene.authorFields->getValue(*script, "none")));
    CHECK(std::any_cast<MFNode>(doc.scene.authorFields->getValue(*script, "empty")).empty());
    const auto prototype = std::any_cast<SFNode>(doc.scene.authorFields->getValue(*script, "prototype"));
    REQUIRE(prototype);
    CHECK(prototype->nodeTypeName() == "ProtoInstance");
    CHECK(prototype == doc.scene.resolve("DefaultInstance"));
    const auto prototypes = std::any_cast<MFNode>(doc.scene.authorFields->getValue(*script, "prototypes"));
    REQUIRE(prototypes.size() == 2);
    CHECK(prototypes[0] == prototype);
    const auto group = as<Group>(prototypes[1]);
    REQUIRE(group);
    REQUIRE(group->getChildren().size() == 1);
    CHECK(group->getChildren()[0]->nodeTypeName() == "ProtoInstance");
    CHECK(doc.scene.protoInstances.empty());
    CHECK(doc.scene.rootNodes.size() == 1);
  }
}
