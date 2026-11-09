#include "doctest/doctest.h"
// Codec round-trips preserve shared identity, ROUTEs, and field values.

#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include "parse/ClassicVrmlReader.hpp"
#include "parse/JsonReader.hpp"

// Concrete node types used by the sample scene.
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"

#include <cctype>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

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

  // Second shape: uses the same appearance shared_ptr (identity sharing).
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
  runtime::X3DDocument doc = buildSampleDocument();
  std::string xml1 = xmlWriter.writeDocument(doc);
  runtime::X3DDocument doc2 = xmlReader.readDocument(xml1);
  std::string xml2 = xmlWriter.writeDocument(doc2);
  CHECK_MESSAGE((canon(xml1) == canon(xml2)),
        "XML round-trip is stable modulo whitespace");
  CHECK_MESSAGE((doc2.scene.rootNodes.size() == 2), "two root nodes parsed back");
  auto root = std::dynamic_pointer_cast<Transform>(doc2.scene.rootNodes[0]);
  CHECK_MESSAGE((static_cast<bool>(root)), "first root is a Transform");
  std::shared_ptr<X3DNode> app1, app2;
  if (root) {
    const auto &kids = root->getChildren();
    CHECK_MESSAGE((kids.size() == 2), "transform has two children");
    if (kids.size() == 2) {
      auto s1 = std::dynamic_pointer_cast<Shape>(kids[0]);
      auto s2 = std::dynamic_pointer_cast<Shape>(kids[1]);
      CHECK_MESSAGE((s1 && s2), "both children are Shapes");
      if (s1 && s2) {
        app1 = s1->getAppearance();
        app2 = s2->getAppearance();
      }
    }
  }
  CHECK_MESSAGE((app1 && app2 && app1.get() == app2.get()),
        "DEF/USE-shared Appearance is ONE shared_ptr after round-trip");
  CHECK_MESSAGE((app1.use_count() >= 2), "shared Appearance use_count reflects sharing");
  CHECK_MESSAGE((doc2.scene.routes.size() == 1), "one ROUTE parsed back");
  if (doc2.scene.routes.size() == 1) {
    const auto &r = doc2.scene.routes[0];
    CHECK_MESSAGE((r.fromNode == "Clock" && r.fromField == "fraction_changed" &&
              r.toNode == "Root" && r.toField == "set_translation"),
          "ROUTE endpoints preserved");
    CHECK_MESSAGE((!r.from.expired() && !r.to.expired()),
          "ROUTE resolved to live node endpoints via DEF table");
  }
  CHECK_MESSAGE((doc2.head.meta.size() == 1 && doc2.head.meta[0].name == "title"),
        "head <meta> survives round-trip");
  std::string json = jsonWriter.writeDocument(doc);

  CHECK_MESSAGE((json.find("\"Transform\"") != std::string::npos &&
            json.find("\"@USE\"") != std::string::npos &&
            json.find("\"-children\"") != std::string::npos),
        "JSON output contains node, USE reference, and child array");
  std::string vrml = vrmlWriter.writeDocument(doc);

  CHECK_MESSAGE((vrml.find("#X3D V4.0 utf8") != std::string::npos &&
            vrml.find("DEF Root Transform") != std::string::npos &&
            vrml.find("USE SharedApp") != std::string::npos &&
            vrml.find("ROUTE Clock.fraction_changed TO Root.set_translation") !=
                std::string::npos),
        "ClassicVRML output contains header, DEF, USE, and ROUTE");

  return;
}

// USE retains shared identity (ISO/IEC 19775-1 §4.4.3) and ROUTEs retain endpoints
// across the XML -> JSON -> ClassicVRML -> XML cycle.
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

  // The USE token must be carried by every intermediate encoding's text — if any
  // reader silently expanded it into a copy, the next encoding would lose it.
  CHECK(json.find("\"@USE\"") != std::string::npos);
  CHECK(vrml.find("USE SharedApp") != std::string::npos);
  CHECK(xmlN.find("USE=\"SharedApp\"") != std::string::npos);

  // ...and after the whole cycle the DEF/USE-shared Appearance is still one node.
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
