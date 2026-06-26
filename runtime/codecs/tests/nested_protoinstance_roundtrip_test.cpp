// nested_protoinstance_roundtrip_test.cpp
// Regression for a scene-level nested ProtoInstance round-trip hole:
// a ProtoInstance that sits INSIDE a non-root node's field slot (e.g. a
// Shape's "geometry" containerField) and is NOT expanded (e.g. its
// EXTERNPROTO url did not resolve) was silently DROPPED by all three writers.
//
// The pre-fix behaviour: writeSceneInto (XML), writeDocument (VRML), and
// writeSceneChildren (JSON) re-emit un-expanded ProtoInstances only when
// parent.expired() (scene-root). Instances whose parent is a live node
// (nested) are not in the node graph (expansion failed) so they are
// invisible to the node-walk, and the parent node (Shape) round-trips with
// an empty geometry slot.
//
// The fix: when each writer visits a node N (writeNodeElement /
// writeNode), also emit any scene.protoInstances entry I where
// !I.expanded && I.parent.lock() == N, placing it in I.parentField.
//
// Oracle: parse -> write -> re-parse; the round-tripped scene must still
// carry exactly one ProtoInstance and that instance must sit inside the
// Shape node (confirmed by looking for "Widget" in the serialised text AND
// by the re-parsed scene having a protoInstance with a live parent).
#include "x3d/sdk.hpp"
#include "X3DProto.hpp" // x3d::runtime::ProtoInstance

#include "doctest/doctest.h"
#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool c, const std::string &msg) {
  if (!c) {
    std::cerr << "FAIL: " << msg << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << msg << "\n";
  }
}

// Inline X3D: an ExternProtoDeclare for "Widget" with a nonexistent URL
// (headless resolver will not find it → expansion fails → un-expanded),
// and a Shape that carries a ProtoInstance as its geometry child.
// Before the fix the Shape round-trips as <Shape/> with no geometry.
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
  check(w0 != nullptr, "original: Widget protoInstance present");
  if (w0) {
    check(!w0->expanded, "original: Widget not expanded (extern unresolved)");
    check(hasLiveParent(*w0), "original: Widget has a live parent (nested in Shape)");
    const bool inGeoSlot = (w0->parentField == "geometry") ||
                           (w0->containerField == "geometry");
    check(inGeoSlot, "original: Widget sits in geometry slot");
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
    check(w.text.find("Widget") != std::string::npos,
          label + "serialised output mentions Widget");

    // Re-parse and verify the ProtoInstance survived.
    sdk::X3DDocument rt = sdk::parseDocument(w.text, w.enc);
    const auto *wi = findWidget(rt);
    check(wi != nullptr,
          label + "round-trip preserves Widget protoInstance");
    if (wi) {
      check(!wi->expanded,
            label + "round-trip Widget still not expanded");
      // The key assertion: the nested Widget must NOT have been demoted to a
      // scene-root (parent.expired()) — it must still carry a live parent so
      // it round-trips as nested-in-Shape, not as a loose scene-root instance.
      check(hasLiveParent(*wi),
            label + "round-trip Widget retains live parent (nested in Shape, not promoted to root)");
    }
  }

  CHECK(failures == 0);
  return;
}
