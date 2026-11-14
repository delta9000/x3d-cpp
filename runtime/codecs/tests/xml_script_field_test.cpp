// XML Script author fields and inline source survive round-trips (design §3.4).

#include "doctest/doctest.h"
#include "DynamicField.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/TimeSensor.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d::core;
using namespace x3d::nodes;
namespace runtime = x3d::runtime;
namespace codec = x3d::codec;

namespace {
// Find a Script node in a scene's root nodes (the test scenes place the Script
// at the root).
std::shared_ptr<Script> findScript(const runtime::Scene &scene) {
  for (const auto &n : scene.rootNodes) {
    if (auto s = std::dynamic_pointer_cast<Script>(n))
      return s;
  }
  return nullptr;
}

// Locate an author FieldInfo on a node by name via effectiveFields. `tableOut`
// owns the temporary FieldTable so the returned pointer stays valid.
const FieldInfo *authorField(const x3d::nodes::X3DNode &node, const std::string &name,
                             const runtime::DynamicFieldStore &store,
                             FieldTable &tableOut) {
  tableOut = runtime::effectiveFields(node, store);
  for (const FieldInfo &f : tableOut) {
    if (f.x3dName == name)
      return &f;
  }
  return nullptr;
}

// A scene with a Script carrying two author fields (mixed access) + CDATA body,
// and a ROUTE from a TimeSensor.fraction_changed into the Script's inputOnly
// author field 'set_fraction'.
std::string sampleXml() {
  return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<X3D profile=\"Interchange\" version=\"4.0\">\n"
         "  <Scene>\n"
         "    <TimeSensor DEF=\"Clock\" loop=\"true\"/>\n"
         "    <Script DEF=\"S\">\n"
         "      <field name=\"set_fraction\" accessType=\"inputOnly\""
         " type=\"SFFloat\"/>\n"
         "      <field name=\"color\" accessType=\"inputOutput\""
         " type=\"SFColor\" value=\"1 0 0\"/>\n"
         "      <![CDATA[\n"
         "ecmascript:\n"
         "  function set_fraction(value) { color = value; }\n"
         "      ]]>\n"
         "    </Script>\n"
         "    <ROUTE fromNode=\"Clock\" fromField=\"fraction_changed\""
         " toNode=\"S\" toField=\"set_fraction\"/>\n"
         "  </Scene>\n"
         "</X3D>\n";
}

// Verify the two author fields + sourceCode are present on a parsed Script.
void checkDecls(const runtime::Scene &scene, const std::string &phase) {
  auto script = findScript(scene);
  CHECK_MESSAGE((script != nullptr), (phase + ": Script node parsed"));
  if (!script)
    return;

  FieldTable table;
  const FieldInfo *frac =
      authorField(*script, "set_fraction", *scene.authorFields, table);
  CHECK_MESSAGE((frac != nullptr), (phase + ": author field set_fraction visible"));
  if (frac) {
    CHECK_MESSAGE((frac->access == AccessType::InputOnly), (phase + ": set_fraction is inputOnly"));
    CHECK_MESSAGE((frac->type == X3DFieldType::SFFloat), (phase + ": set_fraction is SFFloat"));
    // inputOnly: no get thunk (write-only event sink).
    CHECK_MESSAGE((!frac->isReadable()), (phase + ": set_fraction has no getter"));
    CHECK_MESSAGE((frac->isWritable()), (phase + ": set_fraction has a setter"));
  }

  FieldTable table2;
  const FieldInfo *color = authorField(*script, "color", *scene.authorFields, table2);
  CHECK_MESSAGE((color != nullptr), (phase + ": author field color visible"));
  if (color) {
    CHECK_MESSAGE((color->access == AccessType::InputOutput), (phase + ": color is inputOutput"));
    CHECK_MESSAGE((color->type == X3DFieldType::SFColor), (phase + ": color is SFColor"));
    CHECK_MESSAGE((color->isReadable() && color->isWritable()), (phase + ": color is read+write"));
    // The boxed default 1 0 0 seeded into the store.
    std::any v = color->get(*script);
    CHECK_MESSAGE((v.has_value()), (phase + ": color has a seeded value"));
    if (v.has_value()) {
      auto c = std::any_cast<SFColor>(v);
      CHECK_MESSAGE((c.r == 1.0f && c.g == 0.0f && c.b == 0.0f),
                    (phase + ": color default is (1 0 0)"));
    }
  }

  CHECK_MESSAGE((script->getSourceCode().find("function set_fraction") !=
            std::string::npos), (phase + ": CDATA body captured into sourceCode"));
}

} // namespace

TEST_CASE("XML Script author fields") {
  codec::XmlReader reader;
  codec::XmlWriter writer;

  SUBCASE("author fields and their ROUTE survive a write and reparse") {
    runtime::X3DDocument doc = reader.readDocument(sampleXml());
    checkDecls(doc.scene, "parse");
    {
      runtime::X3DExecutionContext ctx(doc.scene.authorFields);
      runtime::BridgeResult br = runtime::buildRoutes(doc.scene, ctx);
      CHECK_MESSAGE((br.rejected.empty()),
                    "author-field ROUTE not rejected (resolves via effectiveFields)");
      CHECK_MESSAGE((br.routesAdded == 1), "author-field ROUTE added to context");
    }
    std::string out = writer.writeDocument(doc);
    CHECK_MESSAGE((out.find("set_fraction") != std::string::npos),
                  "writer re-emits author field set_fraction");
    CHECK_MESSAGE((out.find("accessType=\"inputOnly\"") != std::string::npos),
                  "writer re-emits inputOnly accessType on author field");
    CHECK_MESSAGE((out.find("accessType=\"inputOutput\"") != std::string::npos),
                  "writer re-emits inputOutput accessType on author field");
    CHECK_MESSAGE((out.find("function set_fraction") != std::string::npos),
                  "writer re-emits CDATA source body");

    runtime::X3DDocument doc2 = reader.readDocument(out);
    checkDecls(doc2.scene, "reparse");

    // The round-tripped ROUTE still resolves on reparse.
    {
      runtime::X3DExecutionContext ctx(doc2.scene.authorFields);
      runtime::BridgeResult br = runtime::buildRoutes(doc2.scene, ctx);
      CHECK_MESSAGE((br.rejected.empty()), "reparse: author-field ROUTE still resolves");
      CHECK_MESSAGE((br.routesAdded == 1), "reparse: author-field ROUTE added");
    }
  }

  SUBCASE("a url-source Script keeps its author fields") {
    const std::string urlXml =
        "<X3D profile=\"Interchange\" version=\"4.0\"><Scene>"
        "<Script DEF=\"U\" url='\"ecmascript: function f(){}\"'>"
        "<field name=\"on\" accessType=\"inputOutput\" type=\"SFBool\""
        " value=\"true\"/>"
        "</Script></Scene></X3D>";
    runtime::X3DDocument d = reader.readDocument(urlXml);
    auto s = findScript(d.scene);
    CHECK_MESSAGE((s != nullptr), "url-source Script parsed");
    if (s) {
      CHECK_MESSAGE((s->getUrl().size() == 1 &&
                s->getUrl()[0].find("ecmascript:") != std::string::npos), "inline ecmascript: scheme retained in url");
      FieldTable t;
      CHECK_MESSAGE((authorField(*s, "on", *d.scene.authorFields, t) != nullptr),
                    "author field present alongside url source");
    }
  }

  SUBCASE("SFNode and MFNode author fields round-trip") {
    const std::string nodeXml =
        "<X3D profile=\"Immersive\" version=\"4.0\"><Scene>"
        "<Transform DEF=\"T\"/>"
        "<Script DEF=\"N\">"
        "<field name=\"target\" accessType=\"initializeOnly\" type=\"SFNode\">"
        "<Transform USE=\"T\"/></field>"
        "<field name=\"list\" accessType=\"inputOutput\" type=\"MFNode\">"
        "<Group/><Transform USE=\"T\"/></field>"
        "<field name=\"none\" accessType=\"inputOutput\" type=\"SFNode\"/>"
        "</Script></Scene></X3D>";
    auto nodeChecks = [&](const runtime::X3DDocument &d, const std::string &phase) {
      auto s = findScript(d.scene);
      CHECK_MESSAGE((s != nullptr), (phase + ": node-field Script parsed"));
      if (!s) return;
      const auto &store = *d.scene.authorFields;
      auto target = store.getValue(*s, "target");
      auto *one = std::any_cast<SFNode>(&target);
      CHECK_MESSAGE((one && *one == d.scene.resolve("T")), (phase + ": SFNode field holds USE T"));
      auto list = store.getValue(*s, "list");
      auto *many = std::any_cast<MFNode>(&list);
      CHECK_MESSAGE((many && many->size() == 2 && (*many)[1] == d.scene.resolve("T")),
                    (phase + ": MFNode field holds Group and USE T"));
      auto none = store.getValue(*s, "none");
      auto *empty = std::any_cast<SFNode>(&none);
      CHECK_MESSAGE((empty && !*empty), (phase + ": empty SFNode field is NULL"));
    };
    runtime::X3DDocument d = reader.readDocument(nodeXml);
    nodeChecks(d, "parse");
    const std::string written = writer.writeDocument(d);
    CHECK_MESSAGE((written.find("value=\"\"") == std::string::npos),
                  "node fields are written as children, not empty values");
    nodeChecks(reader.readDocument(written), "reparse");
  }
}
