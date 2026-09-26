#include "doctest/doctest.h"
// version_header_test.cpp
// VP2-L1: the header version floor applies to ALL three primary writers, not
// just VrmlWriter. A hand-built X3DDocument may carry a sub-3.0 (or empty)
// version; the XML and JSON writers must floor it to "3.0" exactly as VRML
// does, so no writer can emit an invalid X3D header. A normal >=3.0 document
// (including a future version) is emitted verbatim.
#include "VersionHeader.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include <string>

namespace {
bool has(const std::string &hay, const std::string &needle) {
  return hay.find(needle) != std::string::npos;
}
} // namespace

TEST_CASE("version_header_test: helper floors and passes through") {
  using x3d::codec::headerVersion;
  CHECK(headerVersion("2.0") == "3.0");
  CHECK(headerVersion("1") == "3.0");
  CHECK(headerVersion("") == "3.0");       // no leading integer major
  CHECK(headerVersion("VRML") == "3.0");   // non-numeric token
  CHECK(headerVersion("3.0") == "3.0");
  CHECK(headerVersion("4.0") == "4.0");
  CHECK(headerVersion("4.2") == "4.2");    // future >= 3.0 untouched
}

TEST_CASE("version_header_test: all writers floor a hand-built sub-3.0 doc") {
  using namespace x3d;
  for (const std::string v : {"2.0", ""}) {
    runtime::X3DDocument doc;
    doc.version = v;

    std::string vrml = codec::VrmlWriter().writeDocument(doc);
    CHECK(has(vrml, "#X3D V3.0 utf8"));
    CHECK_FALSE(has(vrml, "#X3D V2.0"));

    std::string xml = codec::XmlWriter().writeDocument(doc);
    CHECK(has(xml, "version=\"3.0\""));

    std::string json = codec::JsonWriter().writeDocument(doc);
    CHECK(has(json, "\"@version\": \"3.0\""));
  }
}

TEST_CASE("version_header_test: a normal 4.0 doc is unchanged") {
  using namespace x3d;
  runtime::X3DDocument doc;
  doc.version = "4.0";

  CHECK(has(codec::VrmlWriter().writeDocument(doc), "#X3D V4.0 utf8"));
  CHECK(has(codec::XmlWriter().writeDocument(doc), "version=\"4.0\""));
  CHECK(has(codec::JsonWriter().writeDocument(doc), "\"@version\": \"4.0\""));
}
