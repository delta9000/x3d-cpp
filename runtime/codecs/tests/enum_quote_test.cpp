#include "doctest/doctest.h"
// enum_quote_test.cpp
// Regression for corpus-audit Family D (AUD-D): SFEnum/MFEnum values arrive on
// the wire with MFString-style quoting (NavigationInfo type='"FLY" "EXAMINE"'),
// but the generated setEnumString matched bare tokens, so a quoted "FLY" never
// matched and the WHOLE field was silently dropped (type=""). Fix strips the
// quotes before matching in all reader paths.
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool c, const std::string &what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else std::cout << "ok: " << what << "\n";
}

static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <NavigationInfo type='"FLY" "EXAMINE" "ANY"'/>
</Scene></X3D>)X3D";

static void expectKept(const sdk::X3DDocument &d, const std::string &label) {
  // Each writer must round the authored MFEnum back out (not drop it).
  std::string x = sdk::XmlWriter().writeDocument(d);
  std::string j = sdk::JsonWriter().writeDocument(d);
  std::string v = sdk::VrmlWriter().writeDocument(d);
  check(x.find("FLY") != std::string::npos && x.find("EXAMINE") != std::string::npos,
        label + ": XML retains MFEnum tokens");
  check(j.find("FLY") != std::string::npos && j.find("EXAMINE") != std::string::npos,
        label + ": JSON retains MFEnum tokens");
  check(v.find("FLY") != std::string::npos && v.find("EXAMINE") != std::string::npos,
        label + ": VRML retains MFEnum tokens");
}

TEST_CASE("enum_quote_test") {
  sdk::X3DDocument orig = sdk::parseDocument(kScene, sdk::Encoding::XML);
  expectKept(orig, "parse(XML)");
  // Reparse each encoding's output and confirm the tokens still survive — proves
  // the quoted form every writer emits is accepted by every reader.
  expectKept(sdk::parseDocument(sdk::XmlWriter().writeDocument(orig),  sdk::Encoding::XML),         "rt(XML)");
  expectKept(sdk::parseDocument(sdk::JsonWriter().writeDocument(orig), sdk::Encoding::JSON),        "rt(JSON)");
  expectKept(sdk::parseDocument(sdk::VrmlWriter().writeDocument(orig), sdk::Encoding::ClassicVRML), "rt(VRML)");
  CHECK(failures == 0);
  return;
}
