// scene_extractor_clip_test.cpp — REQ-CLIP (§11.4.1): enabled ClipPlane nodes are
// collected into extraction and carried on the RenderItem as world-space planes.
//
// X3D §11.4.1 semantics exercised here:
//   * a ClipPlane affects the FOLLOWING siblings (and their subtrees) within its
//     parent grouping node;
//   * `plane` is authored in the ClipPlane's LOCAL frame, so the reported
//     descriptor is that plane mapped to WORLD space;
//   * `enabled=false` disables the plane (it is not carried).
//
// Scene under test:
//   Group (root)
//     ├─ Shape (before)                              [no plane yet]
//     ├─ ClipPlane(plane=0 1 0 0)                     [affects `after`]
//     └─ Shape (after)                                [carries the plane]
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p,
                     const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

static std::shared_ptr<X3DNode> makeTriShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  return shape;
}

// The RenderItem whose path contains `shape`.
static const extract::RenderItem *itemOf(extract::SceneExtractor &ex,
                                         const X3DNode *shape) {
  for (extract::RenderItemId id = 0; id < ex.itemCount(); ++id)
    for (const X3DNode *n : ex.item(id).path)
      if (n == shape) return &ex.item(id);
  return nullptr;
}

TEST_CASE("scene_extractor_clip_test") {
  // === 1) A plane affects the following sibling, not the preceding one ======
  {
    auto shapeBefore = makeTriShape();
    auto clip = createX3DNode("ClipPlane");
    setF(clip, "plane", std::any(SFVec4f{0, 1, 0, 0}));
    auto shapeAfter = makeTriShape();

    auto root = createX3DNode("Group");
    addChild(root, shapeBefore);
    addChild(root, clip);
    addChild(root, shapeAfter);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    (void)ex.fullSnapshot();

    const extract::RenderItem *before = itemOf(ex, shapeBefore.get());
    const extract::RenderItem *after = itemOf(ex, shapeAfter.get());
    REQUIRE(before);
    REQUIRE(after);
    CHECK((before->clipPlanes.empty())); // plane comes AFTER this sibling.
    REQUIRE((after->clipPlanes.size == 1));
    const SFVec4f p = after->clipPlanes.items[0].planeWorld;
    CHECK((feq(p.x, 0.0f) && feq(p.y, 1.0f) && feq(p.z, 0.0f) && feq(p.w, 0.0f)));
  }

  // === 2) enabled=false disables the plane ==================================
  {
    auto clip = createX3DNode("ClipPlane");
    setF(clip, "enabled", std::any(false));
    setF(clip, "plane", std::any(SFVec4f{0, 1, 0, 0}));
    auto shape = makeTriShape();
    auto root = createX3DNode("Group");
    addChild(root, clip);
    addChild(root, shape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    (void)ex.fullSnapshot();
    const extract::RenderItem *it = itemOf(ex, shape.get());
    REQUIRE(it);
    CHECK((it->clipPlanes.empty()));
  }

  // === 3) the plane is transformed into WORLD space (Transform translation) ==
  // A ClipPlane under a Transform translated +5 in Y: the authored plane
  // (0,1,0,0) (visible where y>=0) becomes (0,1,0,-5) in world (visible y>=5).
  {
    auto clip = createX3DNode("ClipPlane");
    setF(clip, "plane", std::any(SFVec4f{0, 1, 0, 0}));
    auto shape = makeTriShape();
    auto xform = createX3DNode("Transform");
    setF(xform, "translation", std::any(SFVec3f{0, 5, 0}));
    addChild(xform, clip);
    addChild(xform, shape);

    Scene scene;
    scene.addRootNode(xform);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    (void)ex.fullSnapshot();
    const extract::RenderItem *it = itemOf(ex, shape.get());
    REQUIRE(it);
    REQUIRE((it->clipPlanes.size == 1));
    const SFVec4f p = it->clipPlanes.items[0].planeWorld;
    CHECK((feq(p.x, 0.0f) && feq(p.y, 1.0f) && feq(p.z, 0.0f) && feq(p.w, -5.0f)));
  }

  // === 4) a plane scoped to a child group does not leak to its siblings =====
  // root > { inner > { ClipPlane, shapeIn }, shapeOut }: shapeIn carries the
  // plane, shapeOut (a following sibling of inner) does not.
  {
    auto clip = createX3DNode("ClipPlane");
    setF(clip, "plane", std::any(SFVec4f{0, 0, 1, 0}));
    auto shapeIn = makeTriShape();
    auto inner = createX3DNode("Group");
    addChild(inner, clip);
    addChild(inner, shapeIn);
    auto shapeOut = makeTriShape();
    auto root = createX3DNode("Group");
    addChild(root, inner);
    addChild(root, shapeOut);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    (void)ex.fullSnapshot();
    const extract::RenderItem *in = itemOf(ex, shapeIn.get());
    const extract::RenderItem *out = itemOf(ex, shapeOut.get());
    REQUIRE(in);
    REQUIRE(out);
    CHECK((in->clipPlanes.size == 1));
    CHECK((out->clipPlanes.empty()));
  }

  // === 5) a live plane edit is reflected by a fresh snapshot ================
  {
    auto clip = createX3DNode("ClipPlane");
    setF(clip, "plane", std::any(SFVec4f{0, 1, 0, 0}));
    auto shape = makeTriShape();
    auto root = createX3DNode("Group");
    addChild(root, clip);
    addChild(root, shape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    (void)ex.fullSnapshot();
    const extract::RenderItem *it = itemOf(ex, shape.get());
    REQUIRE(it);
    CHECK((feq(it->clipPlanes.items[0].planeWorld.w, 0.0f)));

    setF(clip, "plane", std::any(SFVec4f{0, 1, 0, -2}));
    (void)ex.fullSnapshot();
    it = itemOf(ex, shape.get());
    REQUIRE(it);
    REQUIRE((it->clipPlanes.size == 1));
    CHECK((feq(it->clipPlanes.items[0].planeWorld.w, -2.0f)));
  }
}
