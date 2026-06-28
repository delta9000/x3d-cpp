#include "doctest/doctest.h"
// cascade_author_field_test.cpp
// Regression: the event cascade must DELIVER routed/seeded events into a node's
// dynamic author <field> (Script / ComposedShader / any X3DProgrammableShaderObject).
//
// X3DEventCascade::deliver() resolved a sink field only through the node's static
// reflection table (X3DNode::fields()). Author fields live in the per-node
// DynamicFieldStore, not that table, so a ROUTE into an author inputOnly field
// validated through X3DSceneBridge (which uses effectiveFields) but then silently
// no-op'd at delivery — handlers never saw the value. deliver() now falls back to
// the DynamicFieldStore so author-field sinks actually receive events.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"

#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

bool veq(const std::any &v, float x, float y, float z) {
  if (!v.has_value()) return false;
  const auto p = std::any_cast<SFVec3f>(v);
  return p.x == x && p.y == y && p.z == z;
}

// Register one inputOnly author field on a node (the readers' parse-time path).
void addAuthorInput(const X3DNode &node, const std::string &name) {
  AuthorFieldDecl d;
  d.x3dName = name;
  d.type = X3DFieldType::SFVec3f;
  d.access = AccessType::InputOnly;
  dynamicFieldStore().addAuthorFields(node, {d});
}

// A direct postEvent (seed) to an author field is delivered into the store.
void test_seed_delivers_to_author_field() {
  dynamicFieldStore().clear();
  auto host = std::make_shared<Transform>();
  addAuthorInput(*host, "set_value");

  EventGraph graph;
  EventCascade cascade(graph);
  cascade.postEvent(host.get(), "set_value", std::any(SFVec3f{1, 2, 3}));
  cascade.process();

  check(veq(dynamicFieldStore().getValue(*host, "set_value"), 1, 2, 3),
        "seed event reaches the dynamic author field");
}

// A ROUTE whose sink is an author field fans the source value out to the store.
void test_route_delivers_to_author_field() {
  dynamicFieldStore().clear();
  auto src  = std::make_shared<Transform>(); // translation: built-in inputOutput source
  auto host = std::make_shared<Transform>();
  addAuthorInput(*host, "set_value");

  EventGraph graph;
  graph.addRoute({src.get(), "translation"}, {host.get(), "set_value"});

  EventCascade cascade(graph);
  cascade.postEvent(src.get(), "translation", std::any(SFVec3f{4, 5, 6}));
  cascade.process();

  check(veq(dynamicFieldStore().getValue(*host, "set_value"), 4, 5, 6),
        "routed event reaches the dynamic author field");
}

} // namespace

TEST_CASE("cascade_author_field_test") {
  test_seed_delivers_to_author_field();
  test_route_delivers_to_author_field();
  dynamicFieldStore().clear();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false);
    return;
  }
  std::cout << "all cascade author-field tests passed\n";
}
