#include "doctest/doctest.h"
// version_floor_test.cpp
// VP-2 §8: a VRML97 (#VRML V2.0) source — and any sub-3.0 version token — must
// floor to a valid X3D version (3.0) at read time, so no writer ever emits the
// invalid `#X3D V2.0` header. A version >= 3.0 (incl. future) is left untouched.
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include <iostream>
#include <string>

static int failures = 0;
static void check(bool c, const std::string &what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else std::cout << "ok: " << what << "\n";
}

TEST_CASE("version_floor_test") {
  using namespace x3d;

  // VRML97 .wrl header -> floored to 3.0.
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n", codec::Encoding::VRML97);
    check(doc.version == "3.0", "VRML97 #VRML V2.0 floors doc.version to 3.0");
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    check(out.find("#X3D V2.0") == std::string::npos, "writer never emits #X3D V2.0");
    check(out.find("#X3D V3.0 utf8") != std::string::npos, "writer emits #X3D V3.0 utf8");
  }

  // ClassicVRML .x3dv carrying a stray sub-3.0 token -> clamped to 3.0.
  {
    auto doc = codec::parseDocument("#X3D V2.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    check(doc.version == "3.0", "ClassicVRML sub-3.0 version clamps to 3.0");
  }

  // A real X3D version is preserved (no over-clamping).
  {
    auto doc = codec::parseDocument("#X3D V4.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    check(doc.version == "4.0", "X3D V4.0 preserved");
  }

  // Forward-compat: a future >=3.0 version is NOT floored.
  {
    auto doc = codec::parseDocument("#X3D V4.2 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    check(doc.version == "4.2", "future X3D V4.2 not clamped");
  }

  // Defensive: a hand-built doc with a sub-3.0 version still writes a valid header.
  {
    runtime::X3DDocument doc;
    doc.version = "2.0";
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    check(out.find("#X3D V2.0") == std::string::npos, "writer clamps hand-built V2.0");
    check(out.find("#X3D V3.0 utf8") != std::string::npos, "writer floors hand-built doc to 3.0");
  }

  // Coverage: a ClassicVRML payload behind a `#VRML` magic floors at the reader
  // level too (pins parseHeaderLine's #VRML branch directly, not via Vrml97Reader).
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    check(doc.version == "3.0", "ClassicVRML #VRML magic floors to 3.0 in parseHeaderLine");
  }

  std::cout << (failures ? "FAILED\n" : "PASSED\n");
  CHECK(failures == 0);
  return;
}
