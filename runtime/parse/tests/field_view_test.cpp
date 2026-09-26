// field_view_test.cpp — zero-copy reflection reads (ADR-0049): FieldInfo::view,
// fieldPtr<T>, FieldRef<T> and forEachChildNode.
#include "DynamicField.hpp"
#include "FieldRead.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <memory>
#include <typeinfo>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using x3d::nodes::X3DNode;

namespace {
const FieldInfo &fieldNamed(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) return f;
  FAIL("no field " << name);
  return n.fields().front();
}
} // namespace

TEST_CASE("every readable generated field has a view typed like its get") {
  std::size_t checked = 0;
  for (const auto &[type, create] : x3d::nodes::X3DNodeFactory::registry()) {
    std::shared_ptr<X3DNode> n = create();
    REQUIRE(n);
    for (const auto &f : n->fields()) {
      INFO(type << "." << f.x3dName);
      CHECK(f.isViewable() == f.isReadable());
      if (!f.view) continue;
      const FieldView v = f.view(*n);
      REQUIRE(v.data);
      REQUIRE(v.type);
      CHECK(*v.type == f.get(*n).type());
      ++checked;
    }
  }
  CHECK(checked > 1000);
}

TEST_CASE("fieldPtr borrows the stored member") {
  x3d::nodes::Coordinate coord;
  coord.setPoint({{1, 2, 3}, {4, 5, 6}});
  const FieldInfo &point = fieldNamed(coord, "point");
  const auto *p = fieldPtr<MFVec3f>(coord, point);
  REQUIRE(p);
  CHECK(p == &coord.getPoint());
  CHECK(p->size() == 2);
  // A later write is visible through the same pointer (it aims at the member).
  coord.setPoint({{7, 8, 9}});
  CHECK(p->size() == 1);
  CHECK((*p)[0].x == 7);
}

TEST_CASE("FieldRef falls back to get() for synthesized author fields") {
  auto script = std::make_shared<x3d::nodes::Script>();
  dynamicFieldStore().addAuthorField(
      script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                              AccessType::InputOutput, std::any(0.25f)});
  bool found = false;
  for (const FieldInfo &f : dynamicFieldStore().authorFields(*script)) {
    if (f.x3dName != "level") continue;
    found = true;
    CHECK_FALSE(f.isViewable());
    CHECK(fieldPtr<float>(*script, f) == nullptr);
    FieldRef<float> level(*script, f);
    REQUIRE(level);
    CHECK(*level == 0.25f);
  }
  CHECK(found);
  dynamicFieldStore().erase(*script);
}

TEST_CASE("FieldRef is falsy for a write-only field") {
  x3d::nodes::Transform t;
  const FieldInfo &add = fieldNamed(t, "addChildren");
  CHECK_FALSE(add.isReadable());
  FieldRef<std::vector<std::shared_ptr<X3DNode>>> ref(t, add);
  CHECK_FALSE(ref);
}

TEST_CASE("forEachChildNode visits node children without copying them") {
  x3d::nodes::Transform t;
  auto a = x3d::nodes::X3DNodeFactory::create("Group");
  auto b = x3d::nodes::X3DNodeFactory::create("Shape");
  auto meta = x3d::nodes::X3DNodeFactory::create("MetadataString");
  t.setChildren(MFNode{a, nullptr, b});
  t.setMetadata(meta);

  std::vector<X3DNode *> seen;
  forEachChildNode(t, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
    // Borrowed: no copy of the vector or the pointer, so no refcount bump.
    CHECK(c.use_count() == 2);
    seen.push_back(c.get());
  });
  REQUIRE(seen.size() == 3);  // null child skipped
  CHECK(std::count(seen.begin(), seen.end(), a.get()) == 1);
  CHECK(std::count(seen.begin(), seen.end(), b.get()) == 1);
  CHECK(std::count(seen.begin(), seen.end(), meta.get()) == 1);
}
