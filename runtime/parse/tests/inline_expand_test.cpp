#include "InlineExpand.hpp"
#include "X3DDocument.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DParse.hpp"
#include "doctest/doctest.h"
#include <iostream>
#include <memory>

using namespace x3d::runtime;
using namespace x3d::core;

// Helper: build a Scene whose single root is a DEF'd Shape (stands in for a
// loaded child asset). Returns the scene; registers the Shape under DEF "Geo".
static std::shared_ptr<Scene> makeChildScene() {
  auto s = std::make_shared<Scene>();
  auto shape = X3DNodeFactory::create("Shape");
  shape->setDEF("Geo");
  s->addRootNode(shape); // addRootNode registers the DEF in the child scope
  return s;
}

// Find a node of the given type among a node's MFNode "children", one level deep.
static X3DNode* firstChildOfType(const X3DNode& n, const std::string& type) {
  const FieldInfo* fi = x3d::runtime::findField(n, "children");
  if (!fi || !fi->get) return nullptr;
  for (auto& c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi->get(n)))
    if (c && c->nodeTypeName() == type) return c.get();
  return nullptr;
}

TEST_CASE("inline_expand_test") {
  // Parent scene: a Transform (DEF "Root") containing one Inline (url "child").
  Scene parent;
  auto root = X3DNodeFactory::create("Transform");
  root->setDEF("Root");
  auto inl = X3DNodeFactory::create("Inline");
  // set url = ["child"] via reflection
  {
    const FieldInfo* u = findField(*inl, "url");
    CHECK((u && u->set));
    u->set(*inl, std::any(std::vector<std::string>{"child"}));
  }
  // attach Inline as a child of Root
  {
    const FieldInfo* ch = findField(*root, "children");
    CHECK((ch && ch->set));
    auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(ch->get(*root));
    kids.push_back(inl);
    ch->set(*root, std::any(std::move(kids)));
  }
  parent.addRootNode(root);

  // Injected resolver: any url -> a fresh child scene.
  InlineResolver resolver = [](const std::vector<std::string>&,
                               const std::string&) { return makeChildScene(); };

  std::vector<InlineWarning> warnings;
  expandInlines(parent, resolver, "", warnings);

  // (1) The Inline under Root was replaced by a Group...
  X3DNode* group = firstChildOfType(*root, "Group");
  CHECK((group && "Inline should be replaced by a synthetic Group"));
  // ...and the Group contains the child's Shape.
  CHECK((firstChildOfType(*group, "Shape") && "child geometry must hang under the Group"));
  // (2) The Inline node is no longer a direct child of Root.
  CHECK((!firstChildOfType(*root, "Inline") && "Inline node must be spliced out of the live slot"));
  // (3) DEF isolation: child DEF "Geo" did NOT leak into the parent scope.
  CHECK((parent.resolve("Geo") == nullptr && "child DEF must not leak to parent"));
  // (4) The preserved Inline is recorded for writer round-trip.
  CHECK((parent.expandedInlines.count(group) == 1 && "group must map to preserved Inline"));
  CHECK((parent.expandedInlines[group]->nodeTypeName() == "Inline"));
  CHECK((warnings.empty()));

  // ── parseDocument injection seam ──────────────────────────────────────────
  // Verify that the inlineResolver parameter threads through parseDocument
  // correctly: a custom resolver should be called and its returned Scene
  // spliced in, exactly as the lower-level expandInlines path above.
  {
    // A tiny valid X3D XML document with one load=TRUE Inline.
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN"
  "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D version="4.0" profile="Interchange">
  <Scene>
    <Inline url='"synthetic"' load="true"/>
  </Scene>
</X3D>)";

    // Custom inline resolver: returns a hand-built scene with a single Group.
    bool resolverCalled = false;
    x3d::runtime::InlineResolver customResolver =
        [&resolverCalled](const std::vector<std::string> &,
                          const std::string &) -> std::shared_ptr<x3d::runtime::Scene> {
      resolverCalled = true;
      auto s = std::make_shared<x3d::runtime::Scene>();
      s->addRootNode(X3DNodeFactory::create("Group"));
      return s;
    };

    x3d::runtime::X3DDocument doc = x3d::codec::parseDocument(
        xml, x3d::codec::Encoding::XML, "",
        x3d::codec::localFileProtoResolver, customResolver);

    CHECK((resolverCalled && "custom inlineResolver must be called by parseDocument"));
    // The Inline was expanded: expandedInlines should record at least one entry.
    CHECK((!doc.scene.expandedInlines.empty() &&
           "parseDocument must record expanded Inlines when a custom resolver fires"));
  }

  std::cout << "inline_expand_test OK\n";
  return;
}
