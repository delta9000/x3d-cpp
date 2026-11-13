// `x3d validate` structural checks that need a parsed document: ROUTE endpoint
// resolution and the runtime bridge's field, access and type checks.
#include "doctest/doctest.h"
#include "validate.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<x3d::cli_detail::Diag> routeDiags(const char *xml) {
  auto doc = x3d::sdk::parseDocument(xml, x3d::sdk::Encoding::XML);
  std::vector<x3d::cli_detail::Diag> diags;
  x3d::cli_detail::checkRoutes(doc, diags);
  return diags;
}

bool has(const std::vector<x3d::cli_detail::Diag> &diags, const std::string &needle) {
  for (const auto &d : diags)
    if (d.category == "route" && d.severity == "error" &&
        d.message.find(needle) != std::string::npos)
      return true;
  return false;
}

} // namespace

TEST_CASE("valid routes produce no diagnostics") {
  auto d = routeDiags(
      R"(<X3D version="4.0"><Scene><Transform DEF="T"/>)"
      R"(<TimeSensor DEF="C"/><PositionInterpolator DEF="P"/>)"
      R"(<ROUTE fromNode="C" fromField="fraction_changed" toNode="P" toField="set_fraction"/>)"
      R"(<ROUTE fromNode="P" fromField="value_changed" toNode="T" toField="set_translation"/>)"
      R"(</Scene></X3D>)");
  CHECK_MESSAGE((d.empty()), "valid routes produce no diagnostics");
}

TEST_CASE("an unknown DEF is reported") {
  auto d = routeDiags(
      R"(<X3D version="4.0"><Scene><Transform DEF="T"/><TimeSensor DEF="C"/>)"
      R"(<ROUTE fromNode="C" fromField="fraction_changed" toNode="X" toField="set_translation"/>)"
      R"(</Scene></X3D>)");
  CHECK_MESSAGE((has(d, "ROUTE C.fraction_changed TO X.set_translation: no node is DEF'd as 'X'")),
                "unknown DEF is reported");
}

TEST_CASE("a type mismatch is reported") {
  auto d = routeDiags(
      R"(<X3D version="4.0"><Scene><Transform DEF="T"/><TimeSensor DEF="C"/>)"
      R"(<ROUTE fromNode="C" fromField="fraction_changed" toNode="T" toField="set_translation"/>)"
      R"(</Scene></X3D>)");
  CHECK_MESSAGE((has(d, "ROUTE C.fraction_changed TO T.set_translation: type mismatch")),
                "type mismatch is reported");
}

TEST_CASE("an unknown field is reported") {
  auto d = routeDiags(
      R"(<X3D version="4.0"><Scene><Transform DEF="T"/><TimeSensor DEF="C"/>)"
      R"(<ROUTE fromNode="C" fromField="nope" toNode="T" toField="set_translation"/>)"
      R"(</Scene></X3D>)");
  CHECK_MESSAGE((has(d, "ROUTE C.nope TO T.set_translation")), "unknown field is reported");
}

TEST_CASE("IMPORTed names are not flagged") {
  // An IMPORTed name stays unresolved until its Inline loads; not an error.
  auto d = routeDiags(
      R"(<X3D version="4.0"><Scene><Transform DEF="T"/>)"
      R"(<Inline DEF="I" load="false" url='"missing.x3d"'/>)"
      R"(<IMPORT inlineDEF="I" importedDEF="Mover" AS="M"/>)"
      R"(<ROUTE fromNode="M" fromField="value_changed" toNode="T" toField="set_translation"/>)"
      R"(</Scene></X3D>)");
  CHECK_MESSAGE((!has(d, "no node is DEF'd as 'M'")), "IMPORTed names are not flagged");
}
