#include "doctest/doctest.h"
// A ProtoBody has its own DEF scope (ISO/IEC 19775-1 §4.4.4).
// Body DEFs stay local and body-internal USEs retain shared identity.

#include "x3d/sdk.hpp"
#include <string>

namespace sdk = x3d::sdk;

// PROTO whose body DEFs "B" and USEs it (internal sharing), instantiated once.
static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <ProtoDeclare name="Twin">
    <ProtoBody>
      <Group>
        <Shape DEF="B"><Appearance><Material/></Appearance><Box size="2 2 2"/></Shape>
        <Transform translation="5 0 0"><Shape USE="B"/></Transform>
      </Group>
    </ProtoBody>
  </ProtoDeclare>
  <ProtoInstance name="Twin"/>
</Scene></X3D>)X3D";

static long items(const sdk::X3DDocument &doc) {
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(const_cast<sdk::Scene &>(doc.scene));
  ctx.buildFrom(const_cast<sdk::Scene &>(doc.scene));
  sdk::SceneExtractor ex(ctx, doc.scene);
  return static_cast<long>(ex.fullSnapshot().added.size());
}

static void assertScene(const sdk::X3DDocument &d, const std::string &label) {
  CHECK_MESSAGE((d.scene.defs.find("B") == d.scene.defs.end()),
        (label + ": body DEF 'B' does NOT leak into scene.defs"));
  CHECK_MESSAGE((items(d) == 2),
        (label + ": instance expands to 2 shapes (body-internal USE resolved)"));
}

TEST_CASE("proto_body_defscope_test") {
  sdk::X3DDocument orig = sdk::parseDocument(kScene, sdk::Encoding::XML);
  assertScene(orig, "parse");

  // Round-trip through every encoding: scope must stay correct on reparse too.
  assertScene(sdk::parseDocument(sdk::XmlWriter().writeDocument(orig),  sdk::Encoding::XML),         "xml-rt");
  assertScene(sdk::parseDocument(sdk::JsonWriter().writeDocument(orig), sdk::Encoding::JSON),        "json-rt");
  assertScene(sdk::parseDocument(sdk::VrmlWriter().writeDocument(orig), sdk::Encoding::ClassicVRML), "vrml-rt");
  return;
}
