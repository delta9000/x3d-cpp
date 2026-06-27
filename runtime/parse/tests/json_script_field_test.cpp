// json_script_field_test.cpp
// TASK C (SCR-SAI-DYN S1, JSON encoding): the JSON reader must capture a
// Script node's author field declarations (the "field" member array on the
// Script object, mirroring ProtoInterface's "field") and its inline source
// (the canonical X3D-JSON "#sourceText" array, or an @url inline scheme) into
// the runtime model:
//
//   * each author <field> decl -> AuthorFieldDecl -> DynamicFieldStore, so it
//     shows up in effectiveFields(script) (the view ROUTE endpoint resolution
//     and the SAI use);
//   * the inline body -> Script.sourceCode.
//
// The JSON writer must re-emit both so a read -> write -> reparse round-trip
// preserves the decls + source (the PRF-style fidelity the corpus run depends
// on). Follows the JSON proto round-trip tests as a pattern.
//
// Exit code 0 on success; nonzero on any failed assertion.

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

std::shared_ptr<X3DNode> firstScript(const X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes)
    if (n && n->nodeTypeName() == "Script")
      return n;
  return nullptr;
}

// Search a caller-owned FieldTable. The table MUST outlive the returned pointer:
// effectiveFields() returns a fresh FieldTable by value, so callers keep it in a
// local and take pointers into it. (An earlier version searched a single shared
// `static` table reassigned on every call — holding two results across a second
// call then read freed memory; ASan flagged the use-after-free.)
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

void parsesDeclsAndSource() {
  dynamicFieldStore().clear();
  auto doc = codec::parseDocument(kScriptJson);
  auto script = firstScript(doc);
  check(script != nullptr, "JSON Script parses to a Script node");
  if (!script)
    return;

  // Author field decls land in the dynamic store / effectiveFields.
  check(dynamicFieldStore().hasAuthorFields(*script),
        "Script has author fields in the dynamic store");

  const FieldTable fields = effectiveFields(*script);
  const FieldInfo *fraction = byName(fields, "fraction");
  check(fraction != nullptr, "author field 'fraction' visible in effectiveFields");
  if (fraction) {
    check(fraction->type == X3DFieldType::SFFloat, "'fraction' type SFFloat");
    check(fraction->access == AccessType::InputOnly, "'fraction' inputOnly");
    // inputOnly carries no readable value: get thunk is empty.
    check(!fraction->get, "'fraction' (inputOnly) has no get thunk");
  }

  const FieldInfo *scale = byName(fields, "scale");
  check(scale != nullptr, "author field 'scale' visible in effectiveFields");
  if (scale) {
    check(scale->type == X3DFieldType::SFVec3f, "'scale' type SFVec3f");
    check(scale->access == AccessType::InputOutput, "'scale' inputOutput");
    check(static_cast<bool>(scale->get) && static_cast<bool>(scale->set),
          "'scale' (inputOutput) has get+set thunks");
    if (scale->get) {
      auto v = std::any_cast<SFVec3f>(scale->get(*script));
      check(v.x == 2.f && v.y == 2.f && v.z == 2.f,
            "'scale' initial value 2 2 2 seeded from @value");
    }
  }

  // Inline source captured into Script.sourceCode.
  auto *s = dynamic_cast<Script *>(script.get());
  check(s != nullptr, "node downcasts to Script");
  if (s) {
    const std::string src = s->getSourceCode();
    check(src.find("function fraction") != std::string::npos,
          "sourceCode contains the inline #sourceText body");
  }
}

void roundTrips() {
  dynamicFieldStore().clear();
  auto doc0 = codec::parseDocument(kScriptJson);
  std::string js = codec::JsonWriter().writeDocument(doc0);

  // Re-parse the writer's output; decls + source must survive.
  dynamicFieldStore().clear();
  auto doc1 = codec::parseDocument(js);
  auto script = firstScript(doc1);
  check(script != nullptr, "round-trip: Script survives write->reparse");
  if (!script)
    return;

  const FieldTable fields = effectiveFields(*script);
  const FieldInfo *fraction = byName(fields, "fraction");
  const FieldInfo *scale = byName(fields, "scale");
  check(fraction != nullptr && fraction->access == AccessType::InputOnly,
        "round-trip: 'fraction' inputOnly decl survives");
  check(scale != nullptr && scale->type == X3DFieldType::SFVec3f &&
            scale->access == AccessType::InputOutput,
        "round-trip: 'scale' SFVec3f inputOutput decl survives");
  if (scale && scale->get) {
    auto v = std::any_cast<SFVec3f>(scale->get(*script));
    check(v.x == 2.f && v.y == 2.f && v.z == 2.f,
          "round-trip: 'scale' default 2 2 2 survives");
  }

  auto *s = dynamic_cast<Script *>(script.get());
  check(s != nullptr, "round-trip: node downcasts to Script");
  if (s) {
    const std::string src = s->getSourceCode();
    check(src.find("function fraction") != std::string::npos,
          "round-trip: sourceCode survives write->reparse");
  }
}

} // namespace

int main() {
  parsesDeclsAndSource();
  roundTrips();
  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all json_script_field checks passed\n";
  return 0;
}
