// write_field_test.cpp — M2C-3: ctx.writeField(node, fieldName, value) must
// (a) update the field value and (b) mark dirty with the same flags that a
// cascade-delivered event would produce, so a subsequent re-resolve sees the
// new world transform.
//
// Realistic usage: a System calls writeField from inside update(), which runs
// after dirty_.clear() and before cascade_.process() + transforms_.propagate()
// inside the same tick().  The test wires a one-shot System to make that
// call, then checks the dirty state pulled after tick completes.
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "DirtyTracker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void addChild(const std::shared_ptr<X3DNode>& parent,
                     const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child);
      f.set(*parent, std::any(std::move(kids)));
      return;
    }
}

template <typename T>
static T getField(X3DNode& n, const char* name) {
  for (const auto& f : n.fields())
    if (f.x3dName == name && f.get)
      return std::any_cast<T>(f.get(n));
  return T{};
}

// One-shot System: on the first update, calls ctx.writeField with the given
// value.  Arms a flag so it only fires once.
struct WriteOnceSystem : System {
  X3DNode* target;
  std::string fieldName;
  std::any value;
  bool fired = false;

  WriteOnceSystem(X3DNode* t, const char* f, std::any v)
      : target(t), fieldName(f), value(std::move(v)) {}

  void attach(X3DNode*, X3DExecutionContext&) override {}
  void update(double /*now*/, X3DExecutionContext& ctx) override {
    if (!fired) {
      ctx.writeField(target, fieldName, value);
      fired = true;
    }
  }
};

TEST_CASE("write_field_test") {
  // ── test 1: writeField on a Transform's translation (from a System::update)
  //            marks DirtyLocalTransform + DirtyWorldTransform and updates the
  //            world transform — parity with the cascade path.
  {
    auto root = createX3DNode("Transform");
    auto T = createX3DNode("Transform");
    addChild(root, T);
    Scene scene; scene.addRootNode(root);

    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    const SFVec3f newTrans{3.0f, 0.0f, 0.0f};
    ctx.addSystem(std::make_shared<WriteOnceSystem>(T.get(), "translation",
                                                    std::any(newTrans)));
    ctx.tick(1.0);

    // (a) DirtyLocalTransform flagged (same as cascade would produce).
    CHECK((ctx.dirtyTracker().flags(T.get()) & DirtyLocalTransform));
    // (b) DirtyWorldTransform flagged (TransformSystem propagated it).
    CHECK((ctx.dirtyTracker().flags(T.get()) & DirtyWorldTransform));

    // (c) Field value actually updated.
    auto stored = getField<SFVec3f>(*T, "translation");
    CHECK((feq(stored.x, newTrans.x) && feq(stored.y, newTrans.y) && feq(stored.z, newTrans.z)));

    // (d) World transform reflects new translation.
    auto wp = ctx.worldTransform(T.get()).transformPoint({0, 0, 0});
    CHECK((feq(wp.x, 3.0f) && feq(wp.y, 0.0f) && feq(wp.z, 0.0f)));

    // Second tick: system no longer fires -> dirty set is empty.
    ctx.tick(2.0);
    CHECK((ctx.dirtyTracker().changedNodes().empty()));
  }

  // ── test 2: writeField on a non-transform field (Shape bboxSize) marks
  //            DirtyField | DirtyBounds (bboxSize is in classifyDirty's kBounds set).
  {
    auto shape = createX3DNode("Shape");
    Scene scene; scene.addRootNode(shape);

    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    ctx.addSystem(std::make_shared<WriteOnceSystem>(shape.get(), "bboxSize",
                                                    std::any(SFVec3f{1, 2, 3})));
    ctx.tick(1.0);
    CHECK((ctx.dirtyTracker().flags(shape.get()) & DirtyField));
    CHECK((ctx.dirtyTracker().flags(shape.get()) & DirtyBounds)); // bboxSize is in kBounds
  }

  // ── test 3: writeField on an unknown field is a no-op (no crash, no dirty).
  {
    auto sphere = createX3DNode("Sphere");
    Scene scene; scene.addRootNode(sphere);

    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    ctx.addSystem(std::make_shared<WriteOnceSystem>(sphere.get(), "__nonexistent__",
                                                    std::any(0)));
    ctx.tick(1.0);
    CHECK((ctx.dirtyTracker().flags(sphere.get()) == DirtyNone));
  }

  // ── test 4: parity — writeField + tick produces identical dirty flags to
  //            postEvent + tick for a Transform translation.
  {
    const SFVec3f val{5.0f, 0.0f, 0.0f};

    // cascade path
    auto T_cascade = createX3DNode("Transform");
    Scene s1; s1.addRootNode(T_cascade);
    X3DExecutionContext ctx1;
    ctx1.buildSceneGraph(s1);
    ctx1.postEvent(T_cascade.get(), "translation", std::any(val));
    ctx1.tick(0.0);
    unsigned cascade_flags = ctx1.dirtyTracker().flags(T_cascade.get());

    // writeField path (via System::update)
    auto T_write = createX3DNode("Transform");
    Scene s2; s2.addRootNode(T_write);
    X3DExecutionContext ctx2;
    ctx2.buildSceneGraph(s2);
    ctx2.addSystem(std::make_shared<WriteOnceSystem>(T_write.get(), "translation",
                                                     std::any(val)));
    ctx2.tick(0.0);
    unsigned write_flags = ctx2.dirtyTracker().flags(T_write.get());

    CHECK((cascade_flags == write_flags));
  }

  return;
}
