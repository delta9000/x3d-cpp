// Public discovery must work through sdk.hpp without runtime helper names.
#include "x3d/sdk.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool cond, const char *what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

int main() {
  auto doc = sdk::parseDocument(R"(
    <X3D profile="Immersive" version="4.0"><Scene>
      <Transform DEF="Root">
        <Switch DEF="Inactive" whichChoice="-1">
          <Shape DEF="Shape"><Box DEF="Box"/></Shape>
        </Switch>
        <Viewpoint DEF="View" description="CAVE home"/>
      </Transform>
      <Viewpoint USE="View"/>
      <NavigationInfo DEF="Nav" headlight="false"/>
    </Scene></X3D>)", sdk::Encoding::XML);
  auto peer = sdk::parseDocument(R"(
    <X3D profile="Immersive" version="4.0"><Scene>
      <Viewpoint DEF="Peer"/>
    </Scene></X3D>)", sdk::Encoding::XML);
  doc.scene.protoPeerNodes.push_back(peer.scene.rootNodes.front());
  doc.scene.rootNodes.push_back(nullptr);

  // Add a back edge through reflection, then restore it to release ownership.
  auto &root = *doc.scene.rootNodes.front();
  auto children = sdk::readField<x3d::core::MFNode>(root, "children");
  check(children.has_value(), "MFNode field can be read");
  if (!children) return 1;
  auto cyclicChildren = *children;
  cyclicChildren.push_back(doc.scene.rootNodes.front());
  cyclicChildren.push_back(nullptr);
  for (const auto &field : root.fields())
    if (field.x3dName == "children") field.set(root, std::any(cyclicChildren));

  std::vector<std::string> visited;
  sdk::X3DNode *view = nullptr;
  sdk::X3DNode *nav = nullptr;
  sdk::forEachNode(doc.scene, [&](sdk::X3DNode *node) {
    visited.push_back(node->getDEF());
    if (node->getDEF() == "View") view = node;
    if (node->getDEF() == "Nav") nav = node;
  });
  for (const auto &field : root.fields())
    if (field.x3dName == "children") field.set(root, std::any(*children));
  check(visited == std::vector<std::string>{"Root", "Inactive", "Shape", "Box",
                                          "View", "Nav", "Peer"},
        "walk follows SFNode/MFNode, inactive branches and peers once despite USE/cycle/null");
  check(view && nav, "viewer nodes discovered");
  if (!view || !nav) return 1;

  auto description = sdk::readField<x3d::core::SFString>(*view, "description");
  check(description && *description == "CAVE home", "typed viewpoint label");
  auto headlight = sdk::readField<x3d::core::SFBool>(*nav, "headlight");
  check(headlight && !*headlight, "false is a present field value");
  check(!sdk::readField<x3d::core::SFString>(*view, "missing"), "absent field");
  check(!sdk::readField<x3d::core::SFBool>(*view, "description"),
        "wrong type returns nullopt without asserting");
  check(!sdk::readField<x3d::core::SFString>(*view, "set_description"),
        "event aliases are not field names");
  check(sdk::readField<x3d::core::SFString>(*view, "missing").value_or("fallback") ==
            "fallback", "host chooses an absent-field fallback");
  for (const auto &field : view->fields())
    if (field.x3dName == "description")
      field.set(*view, std::any(x3d::core::SFString{"Changed"}));
  check(description && *description == "CAVE home", "read owns its value");
  check(sdk::readField<x3d::core::SFString>(*view, "description").value_or("") ==
            "Changed", "later reads see field writes");

  sdk::Scene empty;
  sdk::forEachNode(empty, [](sdk::X3DNode *) {
    check(false, "empty scene has no visits");
  });
  if (failures == 0) std::cout << "sdk_node_inspection_test: OK\n";
  return failures == 0 ? 0 : 1;
}
