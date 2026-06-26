// proto_roundtrip_test.cpp
// Task 12: round-trip a <ProtoInstance>. parseDocument expands the instance to
// its primary node (a Box) and records the source instance in
// scene.expandedSources; the XML writer must re-emit the original
// <ProtoInstance ...> for that primary and suppress the expanded subtree,
// rather than serializing the spliced-in Box.
#include "X3DParse.hpp"
#include "XmlWriter.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

TEST_CASE("proto_roundtrip_test") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml); // expands to a Box root + expandedSources
  std::string out = codec::XmlWriter().writeDocument(doc);

  // The declaration is re-emitted (body included) AND the instance is re-emitted;
  // the EXPANSION's spliced Box is still suppressed via expandedSources.
  CHECK((out.find("<ProtoDeclare") != std::string::npos));
  CHECK((out.find("<ProtoBody") != std::string::npos));
  CHECK((out.find("<Box") != std::string::npos));     // the declaration body's Box
  CHECK((out.find("<ProtoInstance") != std::string::npos));
  CHECK((out.find("name=\"P\"") != std::string::npos));
  CHECK((out.find("DEF=\"A\"") != std::string::npos));
  // Exactly one Box element: the declaration body's, not a second from expansion.
  {
    std::size_t n = 0, p = 0;
    while ((p = out.find("<Box", p)) != std::string::npos) { ++n; p += 4; }
    CHECK((n == 1));
  }

  return;
}
