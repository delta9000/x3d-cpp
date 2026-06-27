// json_proto_test.cpp
// The JSON reader must capture PROTO declarations + instances (parity with the
// XML/ClassicVRML readers) so a JSON document with a proto expands like the
// others.
#include "X3DParse.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <string>

using namespace x3d;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}
static std::shared_ptr<X3DNode> firstRoot(const runtime::X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes) if (n) return n;
  return nullptr;
}

// Scene-level: ProtoDeclare P { body Box, size IS size }, ProtoInstance P size=5.
static void jsonProtoExpandTest() {
  const char *json = R"JSON({ "X3D": { "@version": "4.0", "Scene": {
    "-children": [
      { "ProtoDeclare": { "@name": "P",
          "ProtoInterface": { "field": [
            { "@name": "size", "@type": "SFVec3f", "@accessType": "initializeOnly", "@value": "2 2 2" } ] },
          "ProtoBody": { "-children": [
            { "Box": { "IS": { "connect": [ { "@nodeField": "size", "@protoField": "size" } ] } } } ] } } },
      { "ProtoInstance": { "@name": "P", "@DEF": "A",
          "fieldValue": [ { "@name": "size", "@value": "5 5 5" } ] } }
    ] } } })JSON";
  auto doc = codec::parseDocument(json);
  auto root = firstRoot(doc);
  CHECK((root && root->nodeTypeName() == "Box" && root->getDEF() == "A"));
  auto sz = std::any_cast<SFVec3f>(fieldByName(*root, "size")->get(*root));
  CHECK((sz.x == 5.f && sz.y == 5.f && sz.z == 5.f));
}

TEST_CASE("json_proto_test") {
  jsonProtoExpandTest();
  return;
}
