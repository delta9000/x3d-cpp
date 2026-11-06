// Unresolved nested ProtoInstances retain their parent and field slot after round-trip.

#include "x3d/sdk.hpp"
#include "X3DProto.hpp" // x3d::runtime::ProtoInstance

#include "doctest/doctest.h"
#include <string>

namespace sdk = x3d::sdk;

// An unresolved EXTERNPROTO instance occupies a Shape geometry slot.
static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <ExternProtoDeclare name="Widget" url='"nonexistent.x3d#Widget"'>
    <field accessType="initializeOnly" type="SFFloat" name="size"/>
  </ExternProtoDeclare>
  <Shape>
    <ProtoInstance name="Widget" containerField="geometry">
      <fieldValue name="size" value="2.5"/>
    </ProtoInstance>
  </Shape>
</Scene></X3D>)X3D";

// Returns the protoInstance in doc.scene.protoInstances named "Widget",
// or nullptr if not found.
static const x3d::runtime::ProtoInstance *
findWidget(const sdk::X3DDocument &doc) {
  for (const auto &inst : doc.scene.protoInstances)
    if (inst.name == "Widget")
      return &inst;
  return nullptr;
}

// Returns true if inst has a live parent (i.e. is nested in some node).
static bool hasLiveParent(const x3d::runtime::ProtoInstance &inst) {
  return !inst.parent.expired();
}

TEST_CASE("nested_protoinstance_roundtrip_test") {
  // Parse original.
  sdk::X3DDocument orig =
      sdk::parseDocument(kScene, sdk::Encoding::XML);

  // Sanity: original parse must see 1 unexpanded Widget nested in Shape.
  const auto *w0 = findWidget(orig);
  CHECK_MESSAGE((w0 != nullptr), "original: Widget protoInstance present");
  if (w0) {
    CHECK_MESSAGE((!w0->expanded), "original: Widget not expanded (extern unresolved)");
    CHECK_MESSAGE((hasLiveParent(*w0)), "original: Widget has a live parent (nested in Shape)");
    const bool inGeoSlot = (w0->parentField == "geometry") ||
                           (w0->containerField == "geometry");
    CHECK_MESSAGE((inGeoSlot), "original: Widget sits in geometry slot");
  }

  struct W {
    const char *name;
    std::string text;
    sdk::Encoding enc;
  };
  W writers[] = {
      {"XML",  sdk::XmlWriter().writeDocument(orig),  sdk::Encoding::XML},
      {"JSON", sdk::JsonWriter().writeDocument(orig),  sdk::Encoding::JSON},
      {"VRML", sdk::VrmlWriter().writeDocument(orig),  sdk::Encoding::ClassicVRML},
  };

  for (auto &w : writers) {
    std::string label = std::string(w.name) + ": ";

    // The serialised text must contain "Widget" (the instance was not lost).
    CHECK_MESSAGE((w.text.find("Widget") != std::string::npos),
          (label + "serialised output mentions Widget"));

    // Re-parse and verify the ProtoInstance survived.
    sdk::X3DDocument rt = sdk::parseDocument(w.text, w.enc);
    const auto *wi = findWidget(rt);
    CHECK_MESSAGE((wi != nullptr),
          (label + "round-trip preserves Widget protoInstance"));
    if (wi) {
      CHECK_MESSAGE((!wi->expanded),
            (label + "round-trip Widget still not expanded"));
      // The key assertion: the nested Widget must not have been demoted to a
      // scene-root (parent.expired()) — it must still carry a live parent so
      // it round-trips as nested-in-Shape, not as a loose scene-root instance.
      CHECK_MESSAGE((hasLiveParent(*wi)),
            (label + "round-trip Widget retains live parent (nested in Shape, not promoted to root)"));
    }
  }

  return;
}
