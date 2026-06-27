// initializeonly_read_test.cpp — initializeOnly fields must load from a document.
#include "X3DParse.hpp"
#include "XmlWriter.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Sphere.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "doctest/doctest.h"
#include <cstdio>
#include <functional>
#include <memory>
using namespace x3d::core;
using namespace x3d::nodes;
namespace runtime = x3d::runtime;
namespace codec = x3d::codec;

template <class T>
static std::shared_ptr<T> findFirst(const runtime::Scene &scene, const char *type) {
  std::shared_ptr<T> found;
  std::function<void(const std::shared_ptr<X3DNode> &)> walk =
      [&](const std::shared_ptr<X3DNode> &n) {
        if (!n || found) return;
        if (n->nodeTypeName() == type) { found = std::dynamic_pointer_cast<T>(n); return; }
        for (const auto &f : n->fields()) {
          if (!f.get) continue;
          if (f.type == X3DFieldType::SFNode) {
            walk(std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n)));
          } else if (f.type == X3DFieldType::MFNode) {
            for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
              walk(c);
          }
        }
      };
  for (const auto &r : scene.rootNodes) walk(r);
  return found;
}

TEST_CASE("initializeonly_read_test") {
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Sphere radius='9.5'/></Shape></Scene></X3D>");
    auto s = findFirst<Sphere>(doc.getScene(), "Sphere");
    CHECK((s && s->getRadius() == 9.5f));
  }
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Box size='3 4 5'/></Shape></Scene></X3D>");
    auto b = findFirst<Box>(doc.getScene(), "Box");
    CHECK((b && b->getSize().x == 3.f && b->getSize().y == 4.f && b->getSize().z == 5.f));
  }
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><IndexedFaceSet coordIndex='0 1 2 -1'/>"
        "</Shape></Scene></X3D>");
    auto ifs = findFirst<IndexedFaceSet>(doc.getScene(), "IndexedFaceSet");
    CHECK((ifs && ifs->getCoordIndex().size() == 4 && ifs->getCoordIndex()[2] == 2));
  }
  // Round-trip preserves an initializeOnly value (read -> write -> read).
  {
    auto doc = codec::parseDocument(
        "<X3D version='4.0'><Scene><Shape><Sphere radius='7.25'/></Shape></Scene></X3D>");
    codec::XmlWriter w;
    std::string xml = w.writeDocument(doc);
    auto doc2 = codec::parseDocument(xml);
    auto s = findFirst<Sphere>(doc2.getScene(), "Sphere");
    CHECK((s && s->getRadius() == 7.25f));
  }

  // initializeOnly is writable at the data layer but keeps initializeOnly access
  // (so it stays a NON-routable ROUTE sink — locked in by the bridge test).
  {
    auto sphere = createX3DNode("Sphere");
    const FieldInfo *radius = nullptr;
    for (const auto &f : sphere->fields())
      if (f.x3dName == "radius") radius = &f;
    CHECK((radius && radius->isWritable()));                 // data-layer writable now
    CHECK((radius->access == AccessType::InitializeOnly));    // access unchanged
  }

  std::puts("initializeonly_read_test OK");
  return;
}
