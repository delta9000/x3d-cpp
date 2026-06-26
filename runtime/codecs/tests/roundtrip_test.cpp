#include "doctest/doctest.h"
// roundtrip_test.cpp
// Integration test for the X3D codecs (Stage C): build a sample scene
// programmatically, serialize to X3D-XML, parse it back, serialize again, and
// prove the XML round-trip is stable (modulo insignificant whitespace), that a
// DEF/USE-shared node resolves to a single shared_ptr, and that a ROUTE
// survives. Also exercises the JSON and ClassicVRML writers (writer-only).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

// The JSON and ClassicVRML *readers* (X3DCodecs.hpp exposes the XML reader and
// every writer, but the non-XML readers are pulled in explicitly) — needed to
// parse those encodings back and prove the FULL cross-encoding cycle, not just
// that the writers emit the right text.
#include "parse/ClassicVrmlReader.hpp"
#include "parse/JsonReader.hpp"

// Concrete node types used by the sample scene.
#include "Appearance.hpp"
#include "Box.hpp"
#include "Material.hpp"
#include "Shape.hpp"
#include "TimeSensor.hpp"
#include "Transform.hpp"

#include <cctype>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

// Collapse runs of whitespace to a single space and trim, for whitespace-
// insensitive comparison of two XML serializations.
std::string canon(const std::string &s) {
  std::string out;
  bool inSpace = false;
  for (char c : s) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      inSpace = true;
    } else {
      if (inSpace && !out.empty())
        out += ' ';
      inSpace = false;
      out += c;
    }
  }
  return out;
}

// Build the sample scene programmatically:
//   Transform DEF=Root translation=(0 1 0)
//     children [
//       Shape DEF=Box1 { appearance DEF=SharedApp Appearance{ material Material{ diffuseColor 1 0 0 } }
//                        geometry Box{ size 2 2 2 } }
//       Shape { appearance USE=SharedApp     // DEF/USE shared appearance
//               geometry Box{} }
//     ]
//   TimeSensor DEF=Clock { loop true }
//   ROUTE Clock.fraction_changed TO Root.set_translation  (illustrative)
runtime::X3DDocument buildSampleDocument() {
  runtime::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = runtime::Profile::Interchange;
  doc.head.meta.push_back({"title", "Stage C sample", "", "", "", ""});

  auto transform = std::make_shared<Transform>();
  transform->setDEF("Root");
  transform->setTranslation(SFVec3f{0, 1, 0});

  // Shared appearance (DEF/USE target).
  auto sharedApp = std::make_shared<Appearance>();
  sharedApp->setDEF("SharedApp");
  auto material = std::make_shared<Material>();
  material->setDiffuseColor(SFColor{1, 0, 0});
  sharedApp->setMaterial(material);

  // First shape: uses the shared appearance + a box geometry. (Box.size is
  // initializeOnly in the UOM — constructor-time only, no runtime setter — so
  // we leave it at its default to keep the round-trip a fair test of settable
  // fields; the material's diffuseColor above is the non-default value carried.)
  auto shape1 = std::make_shared<Shape>();
  shape1->setDEF("Box1");
  shape1->setAppearance(sharedApp);
  auto box1 = std::make_shared<Box>();
  shape1->setGeometry(box1);

  // Second shape: USES the SAME appearance shared_ptr (identity sharing).
  auto shape2 = std::make_shared<Shape>();
  shape2->setAppearance(sharedApp); // same shared_ptr -> emits USE
  auto box2 = std::make_shared<Box>();
  shape2->setGeometry(box2);

  transform->setChildren({shape1, shape2});

  auto clock = std::make_shared<TimeSensor>();
  clock->setDEF("Clock");
  clock->setLoop(true);

  doc.scene.addRootNode(transform);
  doc.scene.addRootNode(clock);

  doc.scene.routes.emplace_back("Clock", "fraction_changed", "Root",
                                "set_translation");
  return doc;
}

} // namespace

TEST_CASE("roundtrip_test") {
  codec::XmlWriter xmlWriter;
  codec::XmlReader xmlReader;
  codec::JsonWriter jsonWriter;
  codec::VrmlWriter vrmlWriter;

  // ---- Build + first serialization ----
  runtime::X3DDocument doc = buildSampleDocument();
  std::string xml1 = xmlWriter.writeDocument(doc);

  std::cout << "===== ROUND-TRIP: XML #1 (model -> XML) =====\n";
  std::cout << xml1 << "\n";

  // ---- Parse back ----
  runtime::X3DDocument doc2 = xmlReader.readDocument(xml1);

  // ---- Re-serialize ----
  std::string xml2 = xmlWriter.writeDocument(doc2);
  std::cout << "===== ROUND-TRIP: XML #2 (XML -> model -> XML) =====\n";
  std::cout << xml2 << "\n";

  // ---- Stability (modulo whitespace) ----
  check(canon(xml1) == canon(xml2),
        "XML round-trip is stable modulo whitespace");

  // ---- DEF/USE resolves to ONE shared node ----
  check(doc2.scene.rootNodes.size() == 2, "two root nodes parsed back");
  auto root = std::dynamic_pointer_cast<Transform>(doc2.scene.rootNodes[0]);
  check(static_cast<bool>(root), "first root is a Transform");
  std::shared_ptr<X3DNode> app1, app2;
  if (root) {
    const auto &kids = root->getChildren();
    check(kids.size() == 2, "transform has two children");
    if (kids.size() == 2) {
      auto s1 = std::dynamic_pointer_cast<Shape>(kids[0]);
      auto s2 = std::dynamic_pointer_cast<Shape>(kids[1]);
      check(s1 && s2, "both children are Shapes");
      if (s1 && s2) {
        app1 = s1->getAppearance();
        app2 = s2->getAppearance();
      }
    }
  }
  check(app1 && app2 && app1.get() == app2.get(),
        "DEF/USE-shared Appearance is ONE shared_ptr after round-trip");
  check(app1.use_count() >= 2, "shared Appearance use_count reflects sharing");

  // ---- ROUTE survives ----
  check(doc2.scene.routes.size() == 1, "one ROUTE parsed back");
  if (doc2.scene.routes.size() == 1) {
    const auto &r = doc2.scene.routes[0];
    check(r.fromNode == "Clock" && r.fromField == "fraction_changed" &&
              r.toNode == "Root" && r.toField == "set_translation",
          "ROUTE endpoints preserved");
    check(!r.from.expired() && !r.to.expired(),
          "ROUTE resolved to live node endpoints via DEF table");
  }

  // ---- meta survives ----
  check(doc2.head.meta.size() == 1 && doc2.head.meta[0].name == "title",
        "head <meta> survives round-trip");

  // ---- JSON writer ----
  std::string json = jsonWriter.writeDocument(doc);
  std::cout << "===== JSON (model -> X3D-JSON) =====\n" << json << "\n";
  check(json.find("\"Transform\"") != std::string::npos &&
            json.find("\"@USE\"") != std::string::npos &&
            json.find("\"-children\"") != std::string::npos,
        "JSON output contains node, USE reference, and child array");

  // ---- ClassicVRML writer ----
  std::string vrml = vrmlWriter.writeDocument(doc);
  std::cout << "===== ClassicVRML (model -> .x3dv) =====\n" << vrml << "\n";
  check(vrml.find("#X3D V4.0 utf8") != std::string::npos &&
            vrml.find("DEF Root Transform") != std::string::npos &&
            vrml.find("USE SharedApp") != std::string::npos &&
            vrml.find("ROUTE Clock.fraction_changed TO Root.set_translation") !=
                std::string::npos,
        "ClassicVRML output contains header, DEF, USE, and ROUTE");

  if (failures == 0) {
    std::cout << "\nALL ROUND-TRIP CHECKS PASSED\n";
    return;
  }
  std::cerr << "\n" << failures << " CHECK((S)) FAILED\n";
  CHECK(false);
  return;
}

// The case above proves the XML round-trip and exercises the JSON/ClassicVRML
// writers *writer-only* — it never parses those encodings back. A reader-side
// regression that dropped a USE reference or a ROUTE in JsonReader or
// ClassicVrmlReader would therefore pass unnoticed. This case closes that gap:
// it drives the sample document through the FULL cross-encoding cycle
//   model -> XML -> JSON -> ClassicVRML -> XML
// parsing back at every hop, and asserts the two invariants that a lossy hop
// would break: a DEF/USE node stays a single shared instance (USE is a shared
// reference, ISO/IEC 19775-1 §4.4.3 — not a copy), and a ROUTE survives intact.
TEST_CASE("roundtrip_test: USE + ROUTE survive XML->JSON->VRML->XML") {
  codec::XmlReader xmlReader;
  codec::XmlWriter xmlWriter;
  codec::JsonReader jsonReader;
  codec::JsonWriter jsonWriter;
  codec::ClassicVrmlReader vrmlReader;
  codec::VrmlWriter vrmlWriter;

  runtime::X3DDocument doc0 = buildSampleDocument();

  // Parse back at every hop, so each reader is on the hook, not just each writer.
  const std::string xml0 = xmlWriter.writeDocument(doc0);
  runtime::X3DDocument dx = xmlReader.readDocument(xml0);
  const std::string json = jsonWriter.writeDocument(dx);
  runtime::X3DDocument dj = jsonReader.readDocument(json);
  const std::string vrml = vrmlWriter.writeDocument(dj);
  runtime::X3DDocument dv = vrmlReader.readDocument(vrml);
  const std::string xmlN = xmlWriter.writeDocument(dv);
  runtime::X3DDocument dFinal = xmlReader.readDocument(xmlN);

  // The USE token must be carried by EVERY intermediate encoding's text — if any
  // reader silently expanded it into a copy, the next encoding would lose it.
  CHECK(json.find("\"@USE\"") != std::string::npos);
  CHECK(vrml.find("USE SharedApp") != std::string::npos);
  CHECK(xmlN.find("USE=\"SharedApp\"") != std::string::npos);

  // ...and after the whole cycle the DEF/USE-shared Appearance is still ONE node.
  auto root = std::dynamic_pointer_cast<Transform>(dFinal.scene.rootNodes.at(0));
  REQUIRE(root);
  const auto &kids = root->getChildren();
  REQUIRE(kids.size() == 2);
  auto s1 = std::dynamic_pointer_cast<Shape>(kids[0]);
  auto s2 = std::dynamic_pointer_cast<Shape>(kids[1]);
  REQUIRE(s1);
  REQUIRE(s2);
  auto a1 = s1->getAppearance();
  auto a2 = s2->getAppearance();
  REQUIRE(a1);
  REQUIRE(a2);
  CHECK(a1.get() == a2.get()); // shared identity survived XML->JSON->VRML->XML

  // The ROUTE must survive end-to-end with all four endpoints intact.
  REQUIRE(dFinal.scene.routes.size() == 1);
  const auto &r = dFinal.scene.routes[0];
  CHECK(r.fromNode == "Clock");
  CHECK(r.fromField == "fraction_changed");
  CHECK(r.toNode == "Root");
  CHECK(r.toField == "set_translation");

  // The XML at the end of the cycle is a stable fixed point (idempotent).
  const std::string xmlN2 = xmlWriter.writeDocument(xmlReader.readDocument(xmlN));
  CHECK(canon(xmlN) == canon(xmlN2));
}
