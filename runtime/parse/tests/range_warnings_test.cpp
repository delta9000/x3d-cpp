#include "doctest/doctest.h"
// Range-warning collection: validateRanges() per node + collectRangeWarnings()
// walk over the node graph. Surfaces out-of-range values the lenient read path
// keeps (the owed "+ warnings" half of the lenient-read policy).
#include "X3DRangeValidate.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "X3DParse.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;

TEST_CASE("range_warnings_test") {
  // (a) a Material with an out-of-range specularColor component reports it.
  auto mat = std::make_shared<Material>();
  SFColor bad;
  bad.r = 1.5f;
  bad.g = 0.0f;
  bad.b = 0.0f;
  mat->setSpecularColorUnchecked(bad); // lenient write keeps it
  std::vector<RangeDiagnostic> own;
  mat->validateRanges(own);
  CHECK((own.size() == 1));
  CHECK((!own.empty() && own[0].nodeType == "Material"));
  CHECK((!own.empty() && own[0].fieldName == "specularColor"));
  CHECK((!own.empty() &&
        own[0].detail.find("above maximum of 1") != std::string::npos));

  // (b) nested: Shape -> appearance -> material; collectRangeWarnings finds it.
  auto shape = std::make_shared<Shape>();
  auto app = std::make_shared<Appearance>();
  app->setMaterial(mat);
  shape->setAppearance(app);
  auto all = collectRangeWarnings(*shape);
  CHECK((all.size() == 1));
  CHECK((!all.empty() && all[0].fieldName == "specularColor"));

  // (c) an in-range node yields nothing.
  auto clean = std::make_shared<Material>();
  std::vector<RangeDiagnostic> none;
  clean->validateRanges(none);
  CHECK((none.empty()));

  // (d) front-door: a document with an out-of-range value populates
  //     X3DDocument.rangeWarnings.
  const std::string xml =
      "<X3D profile='Interchange' version='4.0'><Scene>"
      "<Shape><Appearance><Material specularColor='1.5 0 0'/></Appearance>"
      "</Shape></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  CHECK((!doc.rangeWarnings.empty()));
  CHECK((!doc.rangeWarnings.empty() &&
        doc.rangeWarnings[0].fieldName == "specularColor"));

  // (e) an in-range document yields an empty channel.
  const std::string ok =
      "<X3D profile='Interchange' version='4.0'><Scene>"
      "<Shape><Appearance><Material specularColor='0.5 0 0'/></Appearance>"
      "</Shape></Scene></X3D>";
  CHECK((x3d::codec::parseDocument(ok).rangeWarnings.empty()));

  // (f) a cyclic node graph (a Group reachable from itself, as a USE of an
  //     ancestor produces) must not recurse forever. Regression: the walk had
  //     no cycle guard and stack-overflowed on such corpus files.
  {
    auto g = std::make_shared<Group>();
    FieldInfo *childrenSet = nullptr;
    for (const FieldInfo &f : g->fields()) {
      if (f.x3dName == "children" && f.set) {
        std::vector<std::shared_ptr<X3DNode>> kids{g}; // self-reference -> cycle
        f.set(*g, std::any(kids));
        childrenSet = const_cast<FieldInfo *>(&f);
        break;
      }
    }
    std::vector<RangeDiagnostic> cyc;
    collectRangeWarnings(*g, cyc); // must terminate, not stack-overflow
    CHECK((cyc.empty()));
    // Break the self-cycle before g goes out of scope (caught by ASan/LSan
    // under the san preset).
    if (childrenSet)
      childrenSet->set(*g, std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  }

  std::cout << "range_warnings_test OK\n";
  return;
}
