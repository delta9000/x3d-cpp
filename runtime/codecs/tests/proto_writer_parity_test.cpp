// proto_writer_parity_test.cpp
// JSON and VRML writers must re-emit captured PROTO declarations and instances,
// suppressing the expansion's spliced primary — parity with XmlWriter.
#include "X3DParse.hpp"
#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"

#include "doctest/doctest.h"
#include <cstddef>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;

// ---------------------------------------------------------------------------
// Test: mixed scalar + node fieldValues must produce a SINGLE "fieldValue"
// JSON array, not two duplicate "fieldValue" keys (invalid JSON).
// ---------------------------------------------------------------------------
static void jsonMixedFieldValueValidJsonTest() {
  using namespace x3d::runtime;

  // Build a declaration with a scalar field (size:SFVec3f) and a node field
  // (geom:SFNode).
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Mix";
  {
    ProtoField f;
    f.name = "size";
    f.type = X3DFieldType::SFVec3f;
    f.access = AccessType::InputOutput;
    decl->interface.push_back(f);
  }
  {
    ProtoField f;
    f.name = "geom";
    f.type = X3DFieldType::SFNode;
    f.access = AccessType::InputOutput;
    decl->interface.push_back(f);
  }

  Scene scene;
  scene.protoDeclarations.push_back(decl);

  // A primary node stands in for the expanded instance.
  auto primary = createX3DNode("Group");
  scene.rootNodes.push_back(primary);

  x3d::runtime::ProtoInstance src;
  src.name = "Mix";
  src.declaration = decl;
  {
    ProtoFieldValue v;
    v.name = "size";
    v.value = std::any(SFVec3f{1.f, 2.f, 3.f});
    src.fieldValues.push_back(v);
  }
  {
    ProtoFieldValue v;
    v.name = "geom";
    v.nodeValue.push_back(createX3DNode("Box"));
    src.fieldValues.push_back(v);
  }
  scene.expandedSources[primary.get()] = src;

  x3d::runtime::X3DDocument doc;
  doc.scene = std::move(scene);

  std::string js = codec::JsonWriter().writeDocument(doc);

  // Count occurrences of "fieldValue" in the output — must be exactly 1.
  std::size_t n = 0, p = 0;
  while ((p = js.find("\"fieldValue\"", p)) != std::string::npos) {
    ++n;
    p += 5;
  }
  CHECK((n == 1 && "duplicate \"fieldValue\" keys in JSON output"));

  // Re-parse to confirm the output is structurally valid JSON.
  auto doc2 = codec::parseDocument(js); // must not throw
  (void)doc2;
}

TEST_CASE("proto_writer_parity_test") {
  // --- existing parity test ---
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);

  std::string js = codec::JsonWriter().writeDocument(doc);
  CHECK((js.find("ProtoDeclare") != std::string::npos));
  CHECK((js.find("ProtoInstance") != std::string::npos));
  // The instance's expansion (a Box) is re-emitted as a ProtoInstance, not as a
  // second standalone Box at scene root. The declaration body's Box may appear.
  CHECK((js.find("\"@name\": \"P\"") != std::string::npos ||
         js.find("\"@name\":\"P\"") != std::string::npos));

  std::string vr = codec::VrmlWriter().writeDocument(doc);
  CHECK((vr.find("PROTO P") != std::string::npos));
  // The scene-level instance is re-emitted as a proto instance `P { ... }`.
  CHECK((vr.find("P {") != std::string::npos));

  // --- new: mixed fieldValue single-array test ---
  jsonMixedFieldValueValidJsonTest();

  return;
}
