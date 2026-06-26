// proto_decl_body_roundtrip_test.cpp
// A <ProtoDeclare> whose <ProtoBody> nests a <ProtoInstance> must round-trip:
// the writer re-emits the nested <ProtoInstance> inside the declaration body,
// placed under its parent body node.
#include "X3DParse.hpp"
#include "XmlWriter.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

TEST_CASE("proto_decl_body_roundtrip_test") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Leaf'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody>"
      "<Transform><ProtoInstance name='Leaf' containerField='children'/></Transform>"
      "</ProtoBody></ProtoDeclare>"
      "</Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  std::string out = codec::XmlWriter().writeDocument(doc);

  // Both declarations re-emitted.
  CHECK((out.find("name=\"Leaf\"") != std::string::npos));
  CHECK((out.find("name=\"Wrap\"") != std::string::npos));
  // The Wrap body's nested <ProtoInstance name="Leaf"> is re-emitted (it must
  // appear as an element, not be dropped). The Leaf reference inside Wrap's body
  // is a <ProtoInstance>, so there must be a <ProtoInstance with name "Leaf".
  auto wrapPos = out.find("name=\"Wrap\"");
  auto pi = out.find("<ProtoInstance", wrapPos);
  CHECK((pi != std::string::npos));
  // It carries the Leaf name.
  CHECK((out.find("name=\"Leaf\"", pi) != std::string::npos));
  return;
}
