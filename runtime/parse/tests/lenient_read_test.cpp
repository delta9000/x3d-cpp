#include "doctest/doctest.h"
// lenient_read_test.cpp
// The reader/data path is lenient: an out-of-range value is kept (not rejected),
// so a whole document never fails to load over one bad field. The typed setX
// API stays strict (throws std::out_of_range) for programmatic callers.
//

#include "X3DParse.hpp"

#include "x3d/nodes/Material.hpp"

#include <any>
#include <stdexcept>
#include <string>

using namespace x3d::core;
using namespace x3d::nodes;

namespace {

// The reflection set thunk (used by every reader) is non-validating: it keeps an
// out-of-range value instead of throwing.
void test_reflection_set_is_lenient() {
  auto m = std::make_shared<Material>();
  const FieldInfo *f = nullptr;
  for (const auto &fi : m->fields()) {
    if (fi.x3dName == "diffuseColor") {
      f = &fi;
    }
  }
  CHECK_MESSAGE((f != nullptr && f->isWritable()), "diffuseColor field is writable");

  bool threw = false;
  try {
    f->set(*m, std::any(SFColor{2.0f, 0.0f, 0.0f})); // 2.0 is out of [0,1]
  } catch (const std::out_of_range &) {
    threw = true;
  }
  CHECK_MESSAGE((!threw), "reflection set thunk does NOT validate (lenient read)");
  CHECK_MESSAGE((m->getDiffuseColor().r == 2.0f),
        "lenient reflection set keeps the out-of-range value");
}

// The typed public setter still enforces the range (binding contract unchanged).
void test_typed_setter_still_strict() {
  Material m;
  bool threw = false;
  try {
    m.setDiffuseColor(SFColor{2.0f, 0.0f, 0.0f});
  } catch (const std::out_of_range &) {
    threw = true;
  }
  CHECK_MESSAGE((threw), "typed setDiffuseColor still throws on out-of-range");
}

// A whole document with an out-of-range field value parses (does not abort).
void test_document_with_out_of_range_loads() {
  const std::string xml =
      "<X3D profile='Immersive' version='4.0'><Scene>"
      "<Shape><Appearance><Material diffuseColor='2 0 0'/></Appearance></Shape>"
      "</Scene></X3D>";
  bool threw = false;
  try {
    auto doc = x3d::codec::parseDocument(xml);
    (void)doc;
  } catch (const std::exception &e) {
    threw = true;
    INFO(e.what());
  }
  CHECK_MESSAGE((!threw), "document with out-of-range value loads without aborting");
}

} // namespace

TEST_CASE("lenient_read_test") {
  test_reflection_set_is_lenient();
  test_typed_setter_still_strict();
  test_document_with_out_of_range_loads();

  return;
}
