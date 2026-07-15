#include "doctest/doctest.h"
// vrml_mf_bracket_test.cpp
// Regression for the corpus-audit finding (Family A): VrmlWriter emitted
// multiple-value MF* fields WITHOUT the mandatory ClassicVRML square brackets
//   coordIndex 0 1 2 -1        (WRONG: a bare run of sfValues is not an mfValue)
// per ISO/IEC 19776-2 / VRML97 grammar  mfValue ::= '[' ']' | '[' sfValue+ ']' | sfValue.
// Consequence: multi-element MF values TRUNCATE to one element (silent data loss)
// or fail to reparse entirely. Fix brackets every MF value (incl. empty -> '[ ]').
//
// This drives the full round-trip through the façade: parse XML -> write VRML ->
// reparse VRML -> extract, and asserts the rendered geometry survives intact.
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool c, const std::string &what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else std::cout << "ok: " << what << "\n";
}

// A quad built from an IndexedFaceSet: multi-value MFInt32 coordIndex (8 elems)
// + MFVec3f point (4 verts). Pre-fix, the VRML writer truncates coordIndex to
// its first element and the reparse yields a degenerate/empty mesh (or throws).
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
  check(origItems == 1 && origVerts > 0, "original extracts a non-empty mesh");

  std::string vrml = sdk::VrmlWriter().writeDocument(orig);

  // The writer must bracket the MF values.
  check(vrml.find("coordIndex [") != std::string::npos,
        "MFInt32 coordIndex is bracketed in VRML output");
  check(vrml.find("point [") != std::string::npos,
        "MFVec3f point is bracketed in VRML output");

  // And the VRML must reparse to the SAME rendered geometry (no truncation).
  sdk::X3DDocument rt = sdk::parseDocument(vrml, sdk::Encoding::ClassicVRML);
  long rtItems = 0, rtVerts = extractVerts(rt, rtItems);
  check(rtItems == origItems, "round-trip preserves render-item count");
  check(rtVerts == origVerts, "round-trip preserves vertex count (no MF truncation)");

  CHECK(failures == 0);
  return;
}
