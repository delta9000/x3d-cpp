#include "emit.hpp"
#include "fixture_source.hpp"
#include "x3d/authoring.hpp"
#include "X3DParse.hpp"        // sdk parser, verification-only
#include "doctest/doctest.h"

using namespace x3d::asset_import;

TEST_CASE("emitted_cube_reparses_clean") {
  auto doc = emit(FixtureSource{}.load("cube"), {});
  doc.profile = x3d::runtime::Profile::Interchange;
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(doc);
  auto reparsed = x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML);
  CHECK(reparsed.rangeWarnings.empty());
  CHECK_FALSE(reparsed.scene.rootNodes.empty());
}
