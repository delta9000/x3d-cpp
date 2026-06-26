// xml_script_field_test.cpp
// TASK A — XML Script capture + round-trip (design §3.4, encoding=XML).
//
// Proves the file-authored Script un-tabling for the XML codec:
//   1. An <Script> with author <field name accessType type value> children plus
//      a <![CDATA[...]]> source body parses so the author fields land in the
//      DynamicFieldStore (visible via effectiveFields) and the CDATA body lands
//      in Script.sourceCode.
//   2. An inline ecmascript: scheme in `url` is still accepted as a source.
//   3. read -> write -> reparse round-trips: the <field> children (with their
//      accessType/type/value) and the CDATA body survive.
//   4. An author-field ROUTE resolves (an endpoint on a Script author field
//      wires through X3DSceneBridge, which resolves via effectiveFields).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"
#include "X3DSceneBridge.hpp"

#include "Script.hpp"
#include "TimeSensor.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d;

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
const FieldInfo *authorField(const X3DNode &node, const std::string &name,
                             FieldTable &tableOut) {
  tableOut = runtime::effectiveFields(node);
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
  check(script != nullptr, phase + ": Script node parsed");
  if (!script)
    return;

  FieldTable table;
  const FieldInfo *frac = authorField(*script, "set_fraction", table);
  check(frac != nullptr, phase + ": author field set_fraction visible");
  if (frac) {
    check(frac->access == AccessType::InputOnly,
          phase + ": set_fraction is inputOnly");
    check(frac->type == X3DFieldType::SFFloat,
          phase + ": set_fraction is SFFloat");
    // inputOnly: no get thunk (write-only event sink).
    check(!frac->isReadable(), phase + ": set_fraction has no getter");
    check(frac->isWritable(), phase + ": set_fraction has a setter");
  }

  FieldTable table2;
  const FieldInfo *color = authorField(*script, "color", table2);
  check(color != nullptr, phase + ": author field color visible");
  if (color) {
    check(color->access == AccessType::InputOutput,
          phase + ": color is inputOutput");
    check(color->type == X3DFieldType::SFColor, phase + ": color is SFColor");
    check(color->isReadable() && color->isWritable(),
          phase + ": color is read+write");
    // The boxed default 1 0 0 seeded into the store.
    std::any v = color->get(*script);
    check(v.has_value(), phase + ": color has a seeded value");
    if (v.has_value()) {
      auto c = std::any_cast<SFColor>(v);
      check(c.r == 1.0f && c.g == 0.0f && c.b == 0.0f,
            phase + ": color default is (1 0 0)");
    }
  }

  check(script->getSourceCode().find("function set_fraction") !=
            std::string::npos,
        phase + ": CDATA body captured into sourceCode");
}

} // namespace

int main() {
  // Test isolation: the DynamicFieldStore is process-global.
  runtime::dynamicFieldStore().clear();

  codec::XmlReader reader;
  codec::XmlWriter writer;

  // ---- Phase 1: parse captures decls + source ----
  runtime::X3DDocument doc = reader.readDocument(sampleXml());
  checkDecls(doc.scene, "parse");

  // ---- Author-field ROUTE resolves ----
  {
    runtime::X3DExecutionContext ctx;
    runtime::BridgeResult br = runtime::buildRoutes(doc.scene, ctx);
    check(br.rejected.empty(),
          "author-field ROUTE not rejected (resolves via effectiveFields)");
    check(br.routesAdded == 1, "author-field ROUTE added to context");
  }

  // ---- Phase 2: round-trip read -> write -> reparse ----
  std::string out = writer.writeDocument(doc);
  check(out.find("set_fraction") != std::string::npos,
        "writer re-emits author field set_fraction");
  check(out.find("accessType=\"inputOnly\"") != std::string::npos,
        "writer re-emits inputOnly accessType on author field");
  check(out.find("accessType=\"inputOutput\"") != std::string::npos,
        "writer re-emits inputOutput accessType on author field");
  check(out.find("function set_fraction") != std::string::npos,
        "writer re-emits CDATA source body");

  runtime::dynamicFieldStore().clear();
  runtime::X3DDocument doc2 = reader.readDocument(out);
  checkDecls(doc2.scene, "reparse");

  // The round-tripped ROUTE still resolves on reparse.
  {
    runtime::X3DExecutionContext ctx;
    runtime::BridgeResult br = runtime::buildRoutes(doc2.scene, ctx);
    check(br.rejected.empty(), "reparse: author-field ROUTE still resolves");
    check(br.routesAdded == 1, "reparse: author-field ROUTE added");
  }

  // ---- Inline ecmascript: scheme in url is still accepted as a source ----
  {
    runtime::dynamicFieldStore().clear();
    const std::string urlXml =
        "<X3D profile=\"Interchange\" version=\"4.0\"><Scene>"
        "<Script DEF=\"U\" url='\"ecmascript: function f(){}\"'>"
        "<field name=\"on\" accessType=\"inputOutput\" type=\"SFBool\""
        " value=\"true\"/>"
        "</Script></Scene></X3D>";
    runtime::X3DDocument d = reader.readDocument(urlXml);
    auto s = findScript(d.scene);
    check(s != nullptr, "url-source Script parsed");
    if (s) {
      check(s->getUrl().size() == 1 &&
                s->getUrl()[0].find("ecmascript:") != std::string::npos,
            "inline ecmascript: scheme retained in url");
      FieldTable t;
      check(authorField(*s, "on", t) != nullptr,
            "author field present alongside url source");
    }
  }

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "All XML Script author-field checks passed.\n";
  return 0;
}
