// proto_deep_is_roundtrip_test.cpp
// PRF-2: an IS/connect block on a body node nested DEEPER than the direct body
// child (here a <Box> inside a <Transform>) must survive a full XML round-trip:
//   read -> write -> re-read, then the IsConnection must reappear bound to the
//   deep node's field.
//
// Before PRF-2 the writer only attached <IS> to the top body node and the
// reader only scanned the top body node's direct children, so a depth-2 IS was
// silently dropped on write (and would not have been re-read). This test fails
// without the recursive attach/collect on both sides.
#include "X3DParse.hpp"
#include "XmlWriter.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

// Count how many of `scene`'s first ProtoDeclaration's IsConnections map the
// proto interface field `protoField` to a node's `nodeField`.
static int countIs(const runtime::Scene &scene, const std::string &nodeField,
                   const std::string &protoField) {
  CHECK((!scene.protoDeclarations.empty()));
  const auto &body = scene.protoDeclarations.front()->body;
  int n = 0;
  for (const auto &ic : body.isConnections)
    if (ic.nodeField == nodeField && ic.protoField == protoField)
      ++n;
  return n;
}

TEST_CASE("proto_deep_is_roundtrip_test") {
  // Body: a Transform (direct body child) whose child Box (depth-2) carries an
  // IS connecting its `size` field to the interface field `boxSize`. The
  // Transform itself also IS-binds `translation` -> `pos` (depth-1) to prove the
  // top-level case still works alongside the deep one.
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Deep'>"
      "  <ProtoInterface>"
      "    <field name='pos' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>"
      "    <field name='boxSize' type='SFVec3f' accessType='inputOutput' value='2 2 2'/>"
      "  </ProtoInterface>"
      "  <ProtoBody>"
      "    <Transform>"
      "      <IS><connect nodeField='translation' protoField='pos'/></IS>"
      "      <Shape><Box><IS><connect nodeField='size' protoField='boxSize'/></IS></Box></Shape>"
      "    </Transform>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "</Scene></X3D>";

  // 1) Initial parse: both the depth-1 and the depth-2 IS must be captured.
  auto doc0 = codec::parseDocument(xml);
  CHECK((countIs(doc0.scene, "translation", "pos") == 1));
  CHECK((countIs(doc0.scene, "size", "boxSize") == 1)); // deep IS captured on read

  // 2) Write back to XML. The deep <IS> must be re-emitted (not just the top).
  std::string out = codec::XmlWriter().writeDocument(doc0);
  CHECK((out.find("protoField=\"pos\"") != std::string::npos));
  CHECK((out.find("protoField=\"boxSize\"") != std::string::npos)); // deep re-emit
  // Exactly two <connect ... > elements survive.
  {
    std::size_t n = 0, p = 0;
    while ((p = out.find("<connect", p)) != std::string::npos) { ++n; p += 8; }
    CHECK((n == 2));
  }

  // 3) Re-read the written XML: the deep IsConnection must survive the full
  //    read -> write -> re-read cycle.
  auto doc1 = codec::parseDocument(out);
  CHECK((countIs(doc1.scene, "translation", "pos") == 1));
  CHECK((countIs(doc1.scene, "size", "boxSize") == 1));

  return;
}
