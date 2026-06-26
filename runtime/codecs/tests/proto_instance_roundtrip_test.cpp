#include "doctest/doctest.h"
// proto_instance_roundtrip_test.cpp
// Regression for corpus-audit Family B (AUD-B): a ProtoInstance whose
// (EXTERN)PROTO does not expand (e.g. an unresolvable EXTERNPROTO in headless
// mode) was SILENTLY DROPPED on serialization across all 3 writers. expandScene
// splices successfully-expanded instances into rootNodes + records
// expandedSources (writers re-emit via that map); on !primary it `continue`d, so
// the instance survived in scene.protoInstances but reached no writer output.
// Per ISO/IEC 19775-1 §7.3.6 an X3DPrototypeInstance is a scene-graph node; the
// data model retains it, so the codec must round-trip it. Fix: mark expansion
// outcome and re-emit un-expanded scene-root instances in every writer.
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool c, const std::string &what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else std::cout << "ok: " << what << "\n";
}

// A scene-root ProtoInstance of an EXTERNPROTO whose url cannot resolve headless
// -> expansion fails -> pre-fix the instance vanishes on write.
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
  check(protoCount(orig) == 1, "original parses 1 (unexpanded) ProtoInstance");

  struct W { const char *name; std::string text; sdk::Encoding enc; };
  W writers[] = {
    {"XML",  sdk::XmlWriter().writeDocument(orig),  sdk::Encoding::XML},
    {"JSON", sdk::JsonWriter().writeDocument(orig), sdk::Encoding::JSON},
    {"VRML", sdk::VrmlWriter().writeDocument(orig), sdk::Encoding::ClassicVRML},
  };
  for (auto &w : writers) {
    check(w.text.find("Widget") != std::string::npos,
          std::string(w.name) + ": output mentions the ProtoInstance name");
    sdk::X3DDocument rt = sdk::parseDocument(w.text, w.enc);
    check(protoCount(rt) == 1,
          std::string(w.name) + ": round-trip preserves the ProtoInstance");
  }
  CHECK(failures == 0);
  return;
}
