#include "doctest/doctest.h"
// hanim_container_order_test.cpp
// Regression: a node instanced (DEF/USE) across two container fields of one
// parent must keep its AUTHORED DEF placement on round-trip, not fall back to
// static field-declaration order. HAnimHumanoid declares `joints` before
// `skeleton`, so the old writer expanded the DEF in `joints` and left a bare
// USE in `skeleton` (graph-equivalent, but an infidelity that also broke the
// HAnim convention that `skeleton` holds the hierarchy). The reader now records
// authored child-field order in the Scene and the round-trip writers replay it.

#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"
#include "parse/ClassicVrmlReader.hpp"
#include "parse/JsonReader.hpp"

#include <string>

using namespace x3d;

namespace {

// The opening tag of the <HAnimJoint> element carrying `marker` (DEF=/USE=).
std::string openingTag(const std::string &xml, const std::string &marker) {
  auto at = xml.find(marker);
  if (at == std::string::npos)
    return {};
  auto start = xml.rfind('<', at);
  auto end = xml.find('>', at);
  if (start == std::string::npos || end == std::string::npos)
    return {};
  return xml.substr(start, end - start + 1);
}

const char *kScene =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<X3D profile=\"Full\" version=\"4.0\"><Scene>\n"
    "  <HAnimHumanoid name=\"h\" DEF=\"Hum\">\n"
    "    <HAnimJoint name=\"humanoid_root\" containerField=\"skeleton\" DEF=\"root\">\n"
    "      <HAnimJoint name=\"sacroiliac\" containerField=\"children\"/>\n"
    "    </HAnimJoint>\n"
    "    <HAnimJoint USE=\"root\" containerField=\"joints\"/>\n"
    "  </HAnimHumanoid>\n"
    "</Scene></X3D>\n";

// The DEF joint (which carries the hierarchy) must round-trip into `skeleton`,
// and the USE reference into `joints` — regardless of declaration order.
void assertAuthoredPlacement(const std::string &xml) {
  const std::string defTag = openingTag(xml, "DEF=\"root\"");
  const std::string useTag = openingTag(xml, "USE=\"root\"");
  REQUIRE_FALSE(defTag.empty());
  REQUIRE_FALSE(useTag.empty());
  CHECK(defTag.find("containerField=\"skeleton\"") != std::string::npos);
  CHECK(useTag.find("containerField=\"joints\"") != std::string::npos);
}

} // namespace

TEST_CASE("hanim_container_order: DEF placement survives XML round-trip") {
  codec::XmlReader reader;
  codec::XmlWriter writer;
  runtime::X3DDocument doc = reader.readDocument(kScene);
  assertAuthoredPlacement(writer.writeDocument(doc));
}

TEST_CASE("hanim_container_order: survives XML -> JSON -> VRML -> XML") {
  codec::XmlReader xr;
  codec::JsonReader jr;
  codec::ClassicVrmlReader vr;
  codec::XmlWriter xw;
  codec::JsonWriter jw;
  codec::VrmlWriter vw;

  // XML -> JSON
  std::string json = jw.writeDocument(xr.readDocument(kScene));
  // JSON -> VRML
  std::string vrml = vw.writeDocument(jr.readDocument(json));
  // VRML -> XML
  std::string xml = xw.writeDocument(vr.readDocument(vrml));

  assertAuthoredPlacement(xml);
}
