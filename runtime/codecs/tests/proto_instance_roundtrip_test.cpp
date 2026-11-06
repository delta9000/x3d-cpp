#include "doctest/doctest.h"
// Unresolved ProtoInstances survive serialization in every encoding.
// ISO/IEC 19775-1 §7.3.6 defines an X3DPrototypeInstance as a scene-graph node.

#include "x3d/sdk.hpp"
#include <string>

namespace sdk = x3d::sdk;

// A scene-root ProtoInstance refers to an unresolved EXTERNPROTO.
static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <ExternProtoDeclare name="Widget" url='"urn:nonexistent#Widget" "does_not_exist.x3d#Widget"'>
    <field accessType="initializeOnly" type="SFFloat" name="size"/>
  </ExternProtoDeclare>
  <ProtoInstance name="Widget" DEF="W1">
    <fieldValue name="size" value="3.5"/>
  </ProtoInstance>
</Scene></X3D>)X3D";

static long protoCount(const sdk::X3DDocument &d) {
  return static_cast<long>(d.scene.protoInstances.size());
}

TEST_CASE("proto_instance_roundtrip_test") {
  sdk::X3DDocument orig = sdk::parseDocument(kScene, sdk::Encoding::XML);
  CHECK_MESSAGE((protoCount(orig) == 1), "original parses 1 (unexpanded) ProtoInstance");

  struct W { const char *name; std::string text; sdk::Encoding enc; };
  W writers[] = {
    {"XML",  sdk::XmlWriter().writeDocument(orig),  sdk::Encoding::XML},
    {"JSON", sdk::JsonWriter().writeDocument(orig), sdk::Encoding::JSON},
    {"VRML", sdk::VrmlWriter().writeDocument(orig), sdk::Encoding::ClassicVRML},
  };
  for (auto &w : writers) {
    CHECK_MESSAGE((w.text.find("Widget") != std::string::npos),
          (std::string(w.name) + ": output mentions the ProtoInstance name"));
    sdk::X3DDocument rt = sdk::parseDocument(w.text, w.enc);
    CHECK_MESSAGE((protoCount(rt) == 1),
          (std::string(w.name) + ": round-trip preserves the ProtoInstance"));
  }
  return;
}
