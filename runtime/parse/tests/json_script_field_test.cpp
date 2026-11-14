// JSON Script author fields and inline source survive read/write round-trips.
#include "doctest/doctest.h"
#include "DynamicField.hpp"
#include "JsonWriter.hpp"
#include "X3DParse.hpp"

#include "x3d/nodes/Script.hpp"

#include <any>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;

namespace {
std::shared_ptr<X3DNode> firstScript(const X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes)
    if (n && n->nodeTypeName() == "Script")
      return n;
  return nullptr;
}

// The caller-owned FieldTable must outlive the returned pointer.
// effectiveFields() returns by value, so callers keep the table in a local.
const FieldInfo *byName(const FieldTable &table, const std::string &name) {
  for (const auto &f : table)
    if (f.x3dName == name)
      return &f;
  return nullptr;
}

// A JSON Script with two author field decls and an inline #sourceText body.
const char *kScriptJson = R"JSON({ "X3D": { "@profile": "Interchange", "@version": "4.0", "Scene": {
  "-children": [
    { "Script": { "@DEF": "S",
        "field": [
          { "@name": "fraction", "@type": "SFFloat", "@accessType": "inputOnly" },
          { "@name": "scale", "@type": "SFVec3f", "@accessType": "inputOutput", "@value": "2 2 2" }
        ],
        "#sourceText": [
          "ecmascript:",
          "function fraction(v) { scale = new SFVec3f(v, v, v); }"
        ] } }
  ] } } })JSON";

TEST_CASE("parses decls and source") {
  auto doc = codec::parseDocument(kScriptJson);
  auto script = firstScript(doc);
  CHECK_MESSAGE((script != nullptr), "JSON Script parses to a Script node");
  if (!script)
    return;

  // Author field decls land in the dynamic store / effectiveFields.
  CHECK_MESSAGE((doc.scene.authorFields->hasAuthorFields(*script)),
                "Script has author fields in the dynamic store");

  const FieldTable fields = effectiveFields(*script, *doc.scene.authorFields);
  const FieldInfo *fraction = byName(fields, "fraction");
  CHECK_MESSAGE((fraction != nullptr), "author field 'fraction' visible in effectiveFields");
  if (fraction) {
    CHECK_MESSAGE((fraction->type == X3DFieldType::SFFloat), "'fraction' type SFFloat");
    CHECK_MESSAGE((fraction->access == AccessType::InputOnly), "'fraction' inputOnly");
    // inputOnly carries no readable value: get thunk is empty.
    CHECK_MESSAGE((!fraction->get), "'fraction' (inputOnly) has no get thunk");
  }

  const FieldInfo *scale = byName(fields, "scale");
  CHECK_MESSAGE((scale != nullptr), "author field 'scale' visible in effectiveFields");
  if (scale) {
    CHECK_MESSAGE((scale->type == X3DFieldType::SFVec3f), "'scale' type SFVec3f");
    CHECK_MESSAGE((scale->access == AccessType::InputOutput), "'scale' inputOutput");
    CHECK_MESSAGE((static_cast<bool>(scale->get) && static_cast<bool>(scale->set)),
                  ("'scale' (inputOutput) has get+set thunks"));
    if (scale->get) {
      auto v = std::any_cast<SFVec3f>(scale->get(*script));
      CHECK_MESSAGE((v.x == 2.f && v.y == 2.f && v.z == 2.f),
                    "'scale' initial value 2 2 2 seeded from @value");
    }
  }

  // Inline source captured into Script.sourceCode.
  auto *s = dynamic_cast<x3d::nodes::Script *>(script.get());
  CHECK_MESSAGE((s != nullptr), "node downcasts to Script");
  if (s) {
    const std::string src = s->getSourceCode();
    CHECK_MESSAGE((src.find("function fraction") != std::string::npos),
                  "sourceCode contains the inline #sourceText body");
  }
}

TEST_CASE("round trips") {
  auto doc0 = codec::parseDocument(kScriptJson);
  std::string js = codec::JsonWriter().writeDocument(doc0);

  // Re-parse the writer's output; decls + source must survive.
  auto doc1 = codec::parseDocument(js);
  auto script = firstScript(doc1);
  CHECK_MESSAGE((script != nullptr), "round-trip: Script survives write->reparse");
  if (!script)
    return;

  const FieldTable fields = effectiveFields(*script, *doc1.scene.authorFields);
  const FieldInfo *fraction = byName(fields, "fraction");
  const FieldInfo *scale = byName(fields, "scale");
  CHECK_MESSAGE((fraction != nullptr && fraction->access == AccessType::InputOnly),
                "round-trip: 'fraction' inputOnly decl survives");
  CHECK_MESSAGE((scale != nullptr && scale->type == X3DFieldType::SFVec3f &&
            scale->access == AccessType::InputOutput), "round-trip: 'scale' SFVec3f inputOutput decl survives");
  if (scale && scale->get) {
    auto v = std::any_cast<SFVec3f>(scale->get(*script));
    CHECK_MESSAGE((v.x == 2.f && v.y == 2.f && v.z == 2.f),
                  "round-trip: 'scale' default 2 2 2 survives");
  }

  auto *s = dynamic_cast<x3d::nodes::Script *>(script.get());
  CHECK_MESSAGE((s != nullptr), "round-trip: node downcasts to Script");
  if (s) {
    const std::string src = s->getSourceCode();
    CHECK_MESSAGE((src.find("function fraction") != std::string::npos),
                  "round-trip: sourceCode survives write->reparse");
  }
}

} // namespace
