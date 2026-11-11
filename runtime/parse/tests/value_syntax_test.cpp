#include "doctest/doctest.h"
// Value-syntax diagnostics: checkValueSyntax() and the ReaderWarning::Kind::Value
// records every reader leaves when a lenient read coerces a malformed value.
#include "FieldValueIO.hpp"
#include "X3DParse.hpp"

#include <algorithm>
#include <string>
#include <vector>

using x3d::codec::checkValueSyntax;
using x3d::core::X3DFieldType;
using Kind = x3d::runtime::ReaderWarning::Kind;

namespace {

std::vector<std::string> valueWarnings(const x3d::runtime::X3DDocument &doc) {
  std::vector<std::string> out;
  for (const auto &w : doc.readerWarnings)
    if (w.kind == Kind::Value) out.push_back(w.detail);
  return out;
}

bool anyContains(const std::vector<std::string> &v, const std::string &needle) {
  return std::any_of(v.begin(), v.end(), [&](const std::string &s) {
    return s.find(needle) != std::string::npos;
  });
}

} // namespace

TEST_CASE("checkValueSyntax accepts well-formed values") {
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFVec3f, "1 2 3"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFVec3f, " 1, -2.5e3 +3 "));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFRotation, "0 1 0 1.57"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFInt32, "0x1F"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFBool, "true"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFBool, "FALSE"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::MFVec3f, "1 2 3, 4 5 6"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::MFVec3f, ""));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::MFInt32, "0 1 2 -1"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::MFFloat, "1 2 3 4 5"));
  // Strings, nodes, images and enums are not checked.
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFString, "anything at all"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFImage, "garbage"));
  CHECK_FALSE(checkValueSyntax(X3DFieldType::SFEnum, "garbage"));
}

TEST_CASE("checkValueSyntax reports malformed values") {
  auto problem = [](X3DFieldType t, const char *s) {
    auto p = checkValueSyntax(t, s);
    return p ? *p : std::string();
  };
  CHECK(problem(X3DFieldType::SFVec3f, "1 2") == "expected 3 values, got 2");
  CHECK(problem(X3DFieldType::SFVec3f, "1 2 3 4") == "expected 3 values, got 4");
  CHECK(problem(X3DFieldType::SFVec3f, "1 abc 3") == "'abc' is not a number");
  CHECK(problem(X3DFieldType::SFFloat, "1.5x") == "'1.5x' is not a number");
  CHECK(problem(X3DFieldType::SFFloat, "+-1") == "'+-1' is not a number");
  CHECK(problem(X3DFieldType::SFFloat, "nan") == "'nan' is not a number");
  CHECK(problem(X3DFieldType::SFFloat, "") == "expected 1 value, got 0");
  CHECK(problem(X3DFieldType::SFInt32, "1.5") == "'1.5' is not an integer");
  CHECK(problem(X3DFieldType::SFBool, "yes") == "'yes' is not true or false");
  CHECK(problem(X3DFieldType::MFVec3f, "1 2 3 4") ==
        "4 values is not a multiple of 3; the trailing 1 value is dropped");
  CHECK(problem(X3DFieldType::MFRotation, "0 1 0 1 0 1") ==
        "6 values is not a multiple of 4; the trailing 2 values are dropped");
}

TEST_CASE("readers record coerced values as Value reader warnings") {
  SUBCASE("XML") {
    auto doc = x3d::codec::parseDocument(
        R"(<X3D version="4.0"><Scene><Transform translation="1 abc"/>)"
        R"(<Coordinate point="1 2 3 4"/></Scene></X3D>)",
        x3d::codec::Encoding::XML);
    const auto w = valueWarnings(doc);
    CHECK(anyContains(w, "Transform.translation=\"1 abc\": 'abc' is not a number"));
    CHECK(anyContains(w, "Coordinate.point=\"1 2 3 4\""));
  }
  SUBCASE("ClassicVRML") {
    auto doc = x3d::codec::parseDocument(
        "#X3D V4.0 utf8\nTransform { translation 1 2 }\n",
        x3d::codec::Encoding::ClassicVRML);
    CHECK(anyContains(valueWarnings(doc), "Transform.translation"));
  }
  SUBCASE("JSON") {
    auto doc = x3d::codec::parseDocument(
        R"({"X3D":{"@version":"4.0","Scene":{"-children":[)"
        R"({"Transform":{"@translation":[1,2]}}]}}})",
        x3d::codec::Encoding::JSON);
    CHECK(anyContains(valueWarnings(doc), "Transform.translation"));
  }
  SUBCASE("ProtoInstance fieldValue") {
    auto doc = x3d::codec::parseDocument(
        R"(<X3D version="4.0"><Scene><ProtoDeclare name="P"><ProtoInterface>)"
        R"(<field name="t" type="SFVec3f" accessType="inputOutput" value="0 0 0"/>)"
        R"(</ProtoInterface><ProtoBody><Transform><IS>)"
        R"(<connect nodeField="translation" protoField="t"/></IS></Transform>)"
        R"(</ProtoBody></ProtoDeclare><ProtoInstance name="P">)"
        R"(<fieldValue name="t" value="1 2"/></ProtoInstance></Scene></X3D>)",
        x3d::codec::Encoding::XML);
    CHECK(anyContains(valueWarnings(doc), "P.t=\"1 2\": expected 3 values, got 2"));
  }
  SUBCASE("well-formed document has none") {
    auto doc = x3d::codec::parseDocument(
        R"(<X3D version="4.0"><Scene><Transform translation="1 2 3" rotation="0 1 0 1">)"
        R"(<Shape><IndexedFaceSet coordIndex="0 1 2 -1"><Coordinate point="0 0 0 1 0 0 0 1 0"/>)"
        R"(</IndexedFaceSet></Shape></Transform></Scene></X3D>)",
        x3d::codec::Encoding::XML);
    CHECK(valueWarnings(doc).empty());
  }
}

TEST_CASE("value diagnostic scopes nest and restore") {
  std::vector<x3d::codec::ValueDiagnostic> outer, inner;
  {
    x3d::codec::ValueDiagnosticScope a(outer);
    {
      x3d::codec::ValueDiagnosticScope b(inner);
      x3d::codec::noteValueSyntax("N", "f", X3DFieldType::SFVec2f, "1");
    }
    x3d::codec::noteValueSyntax("N", "g", X3DFieldType::SFVec2f, "1");
  }
  x3d::codec::noteValueSyntax("N", "h", X3DFieldType::SFVec2f, "1"); // no scope
  REQUIRE(inner.size() == 1);
  CHECK(inner[0].field == "f");
  REQUIRE(outer.size() == 1);
  CHECK(outer[0].field == "g");
}
