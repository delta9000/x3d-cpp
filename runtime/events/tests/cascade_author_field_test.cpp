#include "doctest/doctest.h"
// Routed and seeded author-field inputs reach their handlers.

#include "DynamicField.hpp"
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"

#include "x3d/nodes/Transform.hpp"

#include <any>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

bool veq(const std::any &v, float x, float y, float z) {
  if (!v.has_value()) return false;
  const auto p = std::any_cast<SFVec3f>(v);
  return p.x == x && p.y == y && p.z == z;
}

// Register one inputOnly author field on a node (the readers' parse-time path).
void addAuthorInput(const X3DNode &node, const std::string &name,
                    DynamicFieldStore &store) {
  AuthorFieldDecl d;
  d.x3dName = name;
  d.type = X3DFieldType::SFVec3f;
  d.access = AccessType::InputOnly;
  store.addAuthorFields(node, {d});
}

// A direct postEvent (seed) to an author field is delivered into the store.
void test_seed_delivers_to_author_field() {
  auto store = std::make_shared<DynamicFieldStore>();
  auto host = std::make_shared<Transform>();
  addAuthorInput(*host, "set_value", *store);

  EventGraph graph;
  EventCascade cascade(graph, store);
  cascade.postEvent(host.get(), "set_value", std::any(SFVec3f{1, 2, 3}));
  cascade.process();

  CHECK_MESSAGE((veq(store->getValue(*host, "set_value"), 1, 2, 3)), ("seed event reaches the dynamic author field"));
}

// A ROUTE whose sink is an author field fans the source value out to the store.
void test_route_delivers_to_author_field() {
  auto store = std::make_shared<DynamicFieldStore>();
  auto src  = std::make_shared<Transform>(); // translation: built-in inputOutput source
  auto host = std::make_shared<Transform>();
  addAuthorInput(*host, "set_value", *store);

  EventGraph graph;
  graph.addRoute({src.get(), "translation"}, {host.get(), "set_value"});

  EventCascade cascade(graph, store);
  cascade.postEvent(src.get(), "translation", std::any(SFVec3f{4, 5, 6}));
  cascade.process();

  CHECK_MESSAGE((veq(store->getValue(*host, "set_value"), 4, 5, 6)), ("routed event reaches the dynamic author field"));
}

} // namespace

TEST_CASE("cascade_author_field_test") {
  test_seed_delivers_to_author_field();
  test_route_delivers_to_author_field();

}
