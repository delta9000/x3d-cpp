#include "doctest/doctest.h"
// version_floor_test.cpp
// : a VRML97 (#VRML V2.0) source — and any sub-3.0 version token — must
// floor to a valid X3D version (3.0) at read time, so no writer ever emits the
// invalid `#X3D V2.0` header. A version >= 3.0 (incl. future) is left untouched.
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include <string>

TEST_CASE("version_floor_test") {
  using namespace x3d;

  // VRML97 .wrl header -> floored to 3.0.
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n", codec::Encoding::VRML97);
    CHECK_MESSAGE((doc.version == "3.0"), "VRML97 #VRML V2.0 floors doc.version to 3.0");
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    CHECK_MESSAGE((out.find("#X3D V2.0") == std::string::npos), "writer never emits #X3D V2.0");
    CHECK_MESSAGE((out.find("#X3D V3.0 utf8") != std::string::npos), "writer emits #X3D V3.0 utf8");
  }

  // ClassicVRML .x3dv carrying a stray sub-3.0 token -> clamped to 3.0.
  {
    auto doc = codec::parseDocument("#X3D V2.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "3.0"), "ClassicVRML sub-3.0 version clamps to 3.0");
  }

  // A real X3D version is preserved (no over-clamping).
  {
    auto doc = codec::parseDocument("#X3D V4.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "4.0"), "X3D V4.0 preserved");
  }

  // Forward-compat: a future >=3.0 version is not floored.
  {
    auto doc = codec::parseDocument("#X3D V4.2 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "4.2"), "future X3D V4.2 not clamped");
  }

  // Defensive: a hand-built doc with a sub-3.0 version still writes a valid header.
  {
    runtime::X3DDocument doc;
    doc.version = "2.0";
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    CHECK_MESSAGE((out.find("#X3D V2.0") == std::string::npos), "writer clamps hand-built V2.0");
    CHECK_MESSAGE((out.find("#X3D V3.0 utf8") != std::string::npos), "writer floors hand-built doc to 3.0");
  }

  // Coverage: a ClassicVRML payload behind a `#VRML` magic floors at the reader
  // level too (pins parseHeaderLine's #VRML branch directly, not via Vrml97Reader).
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n", codec::Encoding::ClassicVRML);
    CHECK_MESSAGE((doc.version == "3.0"), "ClassicVRML #VRML magic floors to 3.0 in parseHeaderLine");
  }

  // Unversioned XML/JSON documents floor to 3.0 explicitly in the readers —
  // the §1 bare-floor rung must not lean on the X3DDocument member default,
  // which is the authoring default (4.0) below.
  {
    auto doc = codec::parseDocument("<X3D profile='Interchange'><Scene/></X3D>",
                                    codec::Encoding::XML);
    CHECK_MESSAGE((doc.version == "3.0"), "unversioned <X3D> floors to 3.0");
  }
  {
    auto doc = codec::parseDocument("<Scene><Group/></Scene>", codec::Encoding::XML);
    CHECK_MESSAGE((doc.version == "3.0"), "bare <Scene> root floors to 3.0");
  }
  {
    auto doc = codec::parseDocument(R"({"X3D":{"Scene":{}}})", codec::Encoding::JSON);
    CHECK_MESSAGE((doc.version == "3.0"), "unversioned JSON floors to 3.0");
  }

  // Authoring default: a default-constructed document targets the UOM the
  // bindings are generated from (4.0), not the parse floor — a hand-built
  // scene must not serialize as a 3.0-labeled file by default.
  {
    runtime::X3DDocument doc;
    CHECK_MESSAGE((doc.version == "4.0"), "default-constructed X3DDocument is 4.0");
    codec::XmlWriter w;
    CHECK_MESSAGE((w.writeDocument(doc).find("version=\"4.0\"") != std::string::npos),
          "hand-built doc serializes as version 4.0");
  }

  return;
}
