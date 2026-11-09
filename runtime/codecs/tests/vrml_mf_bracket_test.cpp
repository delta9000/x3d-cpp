#include "doctest/doctest.h"
// Multi-value MF fields require brackets in ClassicVRML.
// ISO/IEC 19776-2 / VRML97 grammar: mfValue ::= '[' ']' | '[' sfValue+ ']' | sfValue.

#include "x3d/sdk.hpp"
#include <string>

namespace sdk = x3d::sdk;

// A quad uses multi-value coordIndex and point fields.
static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <Shape>
    <IndexedFaceSet coordIndex="0 1 2 -1 0 2 3 -1">
      <Coordinate point="0 0 0  1 0 0  1 1 0  0 1 0"/>
    </IndexedFaceSet>
  </Shape>
</Scene></X3D>)X3D";

static long extractVerts(const sdk::X3DDocument &doc, long &items) {
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(const_cast<sdk::Scene &>(doc.scene));
  ctx.buildFrom(const_cast<sdk::Scene &>(doc.scene));
  sdk::SceneExtractor ex(ctx, doc.scene);
  sdk::RenderDelta f = ex.fullSnapshot();
  items = static_cast<long>(f.added.size());
  long verts = 0;
  for (sdk::RenderItemId id : f.added) verts += (long)ex.item(id).mesh->positions.size();
  return verts;
}

TEST_CASE("vrml_mf_bracket_test") {
  sdk::X3DDocument orig = sdk::parseDocument(kScene, sdk::Encoding::XML);
  long origItems = 0, origVerts = extractVerts(orig, origItems);
  CHECK_MESSAGE((origItems == 1 && origVerts > 0), "original extracts a non-empty mesh");

  std::string vrml = sdk::VrmlWriter().writeDocument(orig);

  // The writer must bracket the MF values.
  CHECK_MESSAGE((vrml.find("coordIndex [") != std::string::npos),
        "MFInt32 coordIndex is bracketed in VRML output");
  CHECK_MESSAGE((vrml.find("point [") != std::string::npos),
        "MFVec3f point is bracketed in VRML output");

  // And the VRML must reparse to the same rendered geometry (no truncation).
  sdk::X3DDocument rt = sdk::parseDocument(vrml, sdk::Encoding::ClassicVRML);
  long rtItems = 0, rtVerts = extractVerts(rt, rtItems);
  CHECK_MESSAGE((rtItems == origItems), "round-trip preserves render-item count");
  CHECK_MESSAGE((rtVerts == origVerts), "round-trip preserves vertex count (no MF truncation)");

  return;
}
