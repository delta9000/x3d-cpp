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
#include <string>
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

  FieldWriteResult result = FieldWriteResult::Ok; // outcome of the one write.

  void attach(X3DNode*, X3DExecutionContext&) override {}
  void update(double /*now*/, X3DExecutionContext& ctx) override {
    if (!fired) {
      result = ctx.writeField(target, fieldName, value);
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

// ---------------------------------------------------------------------------
// writeField REPORTS every way a stringly-typed, std::any-valued write can be
// wrong. It used to return void: a null node, a typo'd field name and an
// outputOnly field were all the same silent nothing, so a caller's mistake was
// indistinguishable from a runtime bug. A wrong-typed std::any was worse than
// silent — it escaped as an uncaught std::bad_any_cast from inside the generated
// setter thunk, crashing the caller from a write the SDK invited.
// ---------------------------------------------------------------------------
TEST_CASE("writeField: every failure mode has its own answer") {
  X3DExecutionContext ctx;
  auto T = createX3DNode("Transform");

  SUBCASE("Ok — real field, right type") {
    CHECK(ctx.writeField(T.get(), "translation", std::any(SFVec3f{1, 2, 3})) ==
          FieldWriteResult::Ok);
    CHECK(feq(getField<SFVec3f>(*T, "translation").x, 1.0f));
  }

  SUBCASE("NullNode") {
    CHECK(ctx.writeField(nullptr, "translation", std::any(SFVec3f{1, 2, 3})) ==
          FieldWriteResult::NullNode);
  }

  SUBCASE("UnknownField — a typo is now distinguishable from a runtime bug") {
    CHECK(ctx.writeField(T.get(), "translatoin", std::any(SFVec3f{1, 2, 3})) ==
          FieldWriteResult::UnknownField);
    // The real field is untouched — a failed write changes nothing.
    CHECK(feq(getField<SFVec3f>(*T, "translation").x, 0.0f));
  }

  SUBCASE("outputOnly fields ARE writable — the thunk routes to the emitter") {
    // Pins a fact that is easy to assume backwards (this test caught the author
    // assuming it): `outputOnly` does NOT mean "no set thunk". The generator
    // gives every access type a set thunk; an outputOnly field's routes to the
    // field's EMITTER. So writing TouchSensor.isActive succeeds and FIRES the
    // event — it is not rejected as read-only. All 4914 generated FieldInfos
    // carry a set thunk, which is why FieldWriteResult::NotWritable is defensive
    // depth rather than a state any current node reaches.
    auto ts = createX3DNode("TouchSensor");
    CHECK(ctx.writeField(ts.get(), "isActive", std::any(true)) ==
          FieldWriteResult::Ok);
  }

  SUBCASE("a Script/PROTO author field is UnknownField, not a silent success") {
    // Author-declared (dynamic) fields live in the DynamicFieldStore side-table
    // and resolve via effectiveFields(); writeField consults only the static
    // fields() table. Writing one therefore reports UnknownField rather than
    // appearing to succeed. Documented narrowing, pinned so a change is a
    // deliberate one — see FieldWriteResult::UnknownField.
    auto script = createX3DNode("Script");
    CHECK(ctx.writeField(script.get(), "myAuthorField", std::any(1.0f)) ==
          FieldWriteResult::UnknownField);
  }

  SUBCASE("TypeMismatch — contained, not thrown") {
    // translation is an SFVec3f; hand it a float. Every generated setter does an
    // unchecked any_cast<T>, so this used to propagate std::bad_any_cast out of
    // writeField. It must come back as a value instead.
    CHECK_NOTHROW((void)ctx.writeField(T.get(), "translation", std::any(3.5f)));
    CHECK(ctx.writeField(T.get(), "translation", std::any(3.5f)) ==
          FieldWriteResult::TypeMismatch);
  }

  SUBCASE("a failed write leaves the dirty-tracker clean") {
    // The write is atomic w.r.t. failure: a node must never be flagged dirty for
    // a change that did not happen, or delta() reports phantom updates.
    (void)ctx.writeField(T.get(), "translatoin", std::any(SFVec3f{9, 9, 9}));
    (void)ctx.writeField(T.get(), "translation", std::any(3.5f));
    CHECK(ctx.dirtyTracker().flags(T.get()) == 0u);
  }
}

TEST_CASE("writeField: fieldWriteResultName round-trips every result for diagnostics") {
  CHECK(std::string(fieldWriteResultName(FieldWriteResult::Ok)) == "Ok");
  CHECK(std::string(fieldWriteResultName(FieldWriteResult::NullNode)) == "NullNode");
  CHECK(std::string(fieldWriteResultName(FieldWriteResult::UnknownField)) == "UnknownField");
  CHECK(std::string(fieldWriteResultName(FieldWriteResult::NotWritable)) == "NotWritable");
  CHECK(std::string(fieldWriteResultName(FieldWriteResult::TypeMismatch)) == "TypeMismatch");
}
