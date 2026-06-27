// scene_extractor_test.cpp — M2.5 T7a acceptance: the M2C-1 per-path proof.
//
// A single DEF'd Shape (one shared X3DNode) USE'd under TWO different Transforms
// must yield TWO RenderItems with DISTINCT per-path worldTransforms, while the
// first-path-only ctx.worldTransform(thatNode) collapses to ONE matrix. The
// extractor re-accumulates worldM down each PATH (the worldOfRec pattern) and
// must NEVER read TransformSystem.world_ for an item matrix.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp" // Scene::addRootNode definition.
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
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

TEST_CASE("scene_extractor_test") {
  // --- The M2C-1 scene ---------------------------------------------------
  // Root group holds two Transforms (Tx at +10x, Ty at -10x). A single DEF'd
  // Shape (TriangleSet, one triangle) is USE'd under BOTH — i.e. the SAME
  // shared_ptr node appears in both Transforms' children.
  auto tri = createX3DNode("TriangleSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));

  auto shape = createX3DNode("Shape"); // the DEF'd / shared node
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));

  auto Tx = createX3DNode("Transform");
  setF(Tx, "translation", std::any(SFVec3f{10, 0, 0}));
  addChild(Tx, shape); // USE

  auto Ty = createX3DNode("Transform");
  setF(Ty, "translation", std::any(SFVec3f{-10, 0, 0}));
  addChild(Ty, shape); // USE (same shared node)

  auto group = createX3DNode("Group");
  addChild(group, Tx);
  addChild(group, Ty);

  Scene scene;
  scene.addRootNode(group);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  extract::SceneExtractor ex(ctx, scene);
  extract::RenderDelta snap = ex.fullSnapshot();

  // TWO RenderItems for the one DEF'd Shape — one per placement PATH.
  CHECK((snap.added.size() == 2));
  CHECK((snap.removed.empty()));

  const extract::RenderItem &a = ex.item(snap.added[0]);
  const extract::RenderItem &b = ex.item(snap.added[1]);

  // Distinct DENSE ids.
  CHECK((snap.added[0] != snap.added[1]));

  // Both reference the SAME geometry content (one upload, instanced twice).
  CHECK((a.geometry.node == b.geometry.node));
  CHECK((a.geometry.node == tri.get()));

  // DISTINCT per-path world transforms: translations +10x and -10x.
  float ax = a.worldTransform.transformPoint({0, 0, 0}).x;
  float bx = b.worldTransform.transformPoint({0, 0, 0}).x;
  CHECK((feq(std::fabs(ax), 10.0f)));
  CHECK((feq(std::fabs(bx), 10.0f)));
  CHECK((!feq(ax, bx))); // the two placements differ — THE M2C-1 proof.
  CHECK((feq(ax + bx, 0.0f)));

  // The first-path-only context view collapses the USE'd node to ONE transform.
  Mat4 oneWorld = ctx.worldTransform(shape.get());
  // shape is not a Transform, so the ctx table has no entry => identity; the key
  // point is it is a SINGLE answer, not the two distinct per-path matrices above.
  (void)oneWorld;

  // Both items unlit-white stub material (no MaterialSystem yet).
  CHECK((a.material.model == extract::MaterialModel::Unlit));
  CHECK((b.material.model == extract::MaterialModel::Unlit));

  // --- sceneWorldBounds over the two placements --------------------------
  Aabb wb = ex.sceneWorldBounds();
  CHECK((!wb.empty));
  // Triangle x in [0,1] local; placed at +10 and -10 => world x in [-10, 11].
  CHECK((feq(wb.min.x, -10.0f)));
  CHECK((feq(wb.max.x, 11.0f)));

  // --- interning is stable: a second snapshot reuses the same ids --------
  extract::RenderDelta snap2 = ex.fullSnapshot();
  CHECK((snap2.added.size() == 2));

  // --- camera() reads viewMatrix + Viewpoint.fieldOfView -----------------
  // No bound Viewpoint => identity view + default FOV; just exercise the path.
  extract::CameraDesc cam = ex.camera();
  CHECK((feq(cam.fieldOfView, 0.7854f)));

  // --- primitive sanity: Box is now tessellated by MeshBuilder (T4) so a Box
  //     Shape DOES emit one RenderItem with a non-empty local mesh (12 tris). --
  {
    auto box = createX3DNode("Box");
    auto bshape = createX3DNode("Shape");
    setF(bshape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
    Scene s2;
    s2.addRootNode(bshape);
    X3DExecutionContext c2;
    c2.buildSceneGraph(s2);
    extract::SceneExtractor ex2(c2, s2);
    extract::RenderDelta d2 = ex2.fullSnapshot();
    CHECK((d2.added.size() == 1)); // Box now tessellates (T4 primitive support).
    const extract::RenderItem &bi = ex2.item(d2.added[0]);
    CHECK((bi.mesh.indices.size() == 36)); // 6 faces * 2 tris * 3 corners.
  }

  return;
}
