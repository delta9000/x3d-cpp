// runtime/parse/tests/proto_clone_test.cpp
#include "X3DDocument.hpp"
#include "X3DProto.hpp"
#include "X3DProtoClone.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/core/X3DReflection.hpp"
#include <any>
#include "doctest/doctest.h"
using namespace x3d::core;
using namespace x3d::runtime;
using namespace x3d::nodes;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

static void cloneTest() {
  auto shape = createX3DNode("Shape");
  shape->setDEF("S");
  auto group = createX3DNode("Group");
  const FieldInfo *kids = fieldByName(*group, "children");
  CHECK((kids && kids->set));
  kids->set(*group, std::any(std::vector<std::shared_ptr<X3DNode>>{shape, shape}));

  auto clone = deepClone(group);
  CHECK((clone && clone.get() != group.get()));
  const FieldInfo *cKids = fieldByName(*clone, "children");
  auto out = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(cKids->get(*clone));
  CHECK((out.size() == 2));
  CHECK((out[0].get() == out[1].get()));      // USE sharing preserved
  CHECK((out[0].get() != shape.get()));        // genuinely a clone
  CHECK((out[0]->getDEF() == "S"));            // DEF copied
}

TEST_CASE("proto_clone_test") {
  ProtoBody body;
  body.isConnections.push_back({nullptr, "size", "size"});
  CHECK((body.isConnections.size() == 1));

  x3d::runtime::ProtoInstance inst;
  inst.parentField = "geometry";
  CHECK((inst.parent.expired()));            // weak_ptr, empty by default

  Scene scene;
  CHECK((scene.resolvedProtoRoutes.empty()));
  CHECK((scene.protoRedirects.empty()));
  CHECK((scene.expandedSources.empty()));

  X3DDocument doc;
  CHECK((doc.protoWarnings.empty()));

  ProtoWarning w{ProtoWarning::Kind::UnresolvedExtern, "Foo", "no url resolved"};
  CHECK((w.kind == ProtoWarning::Kind::UnresolvedExtern));

  cloneTest();
  return;
}
