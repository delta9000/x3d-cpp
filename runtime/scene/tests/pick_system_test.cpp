// pick_system_test.cpp
#include "PickSystem.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-2f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}
static std::shared_ptr<X3DNode> shapeWith(const std::shared_ptr<X3DNode>& geom) {
  auto s = createX3DNode("Shape");
  setF(s, "geometry", std::any(std::shared_ptr<X3DNode>(geom)));
  return s;
}
// Build a PickResult for a single geometry node at the world origin (no transform).
static PickResult pickGeom(const std::shared_ptr<X3DNode>& geom, const Ray& ray) {
  auto shape = shapeWith(geom);
  Scene sc; sc.addRootNode(shape);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);
  return ps.pickClosest(ray, bs);
}
// Build a Coordinate node.
static std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}
static void attachCoord(const std::shared_ptr<X3DNode>& geom, std::vector<SFVec3f> pts) {
  setF(geom, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(std::move(pts)))));
}

TEST_CASE("pick_system_test") {
  // Transform(+5x) > Shape > Box(size 2). Ray from (5,0,10) toward -z hits z=1.
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5,0,0}));
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  auto shape = shapeWith(box);
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  PickSystem ps; ps.build(scene);

  auto hit = ps.pickClosest(Ray{{5,0,10},{0,0,-1}}, bs);
  CHECK((hit.hit && hit.node == shape.get()));
  CHECK((feq(hit.point.x,5) && feq(hit.point.z,1)));
  CHECK((feq(hit.distance, 9)));

  // Miss: ray well outside.
  auto miss = ps.pickClosest(Ray{{100,0,10},{0,0,-1}}, bs);
  CHECK((!miss.hit));

  // Sphere narrow-phase under no transform.
  auto sph = createX3DNode("Sphere"); setF(sph, "radius", std::any(2.0f));
  auto sshape = shapeWith(sph);
  Scene s2; s2.addRootNode(sshape);
  TransformSystem ts2; ts2.buildIndex(s2);
  BoundsSystem bs2; bs2.buildBounds(s2, ts2);
  PickSystem ps2; ps2.build(s2);
  auto hs = ps2.pickClosest(Ray{{0,0,10},{0,0,-1}}, bs2);
  CHECK((hs.hit && feq(hs.point.z, 2))); // sphere front at z=2

  // Closest-of-two along the same ray: a near Shape (front face z~1) and a far one
  // (translated to z=-20); the nearer Shape is returned.
  auto nearBox = createX3DNode("Box"); setF(nearBox, "size", std::any(SFVec3f{2,2,2}));
  auto farBox  = createX3DNode("Box"); setF(farBox,  "size", std::any(SFVec3f{2,2,2}));
  auto nearShape = shapeWith(nearBox);
  auto farT = createX3DNode("Transform"); setF(farT, "translation", std::any(SFVec3f{0,0,-20}));
  addChild(farT, shapeWith(farBox));
  Scene s3; s3.addRootNode(nearShape); s3.addRootNode(farT);
  TransformSystem ts3; ts3.buildIndex(s3);
  BoundsSystem bs3; bs3.buildBounds(s3, ts3);
  PickSystem ps3; ps3.build(s3);
  auto h2 = ps3.pickClosest(Ray{{0,0,10},{0,0,-1}}, bs3);
  CHECK((h2.hit && h2.node == nearShape.get() && feq(h2.point.z, 1)));

  // ---- M2D-2 tests: analytic Cone/Cylinder narrow-phase -------------------

  // Cone (default: bottomRadius=1, height=2): axis=Y, apex at (0,+1,0), base
  // circle radius 1 at y=-1. Ray along -z through x=0,y=0 hits the lateral
  // surface at z=0.5 (radius at y=0 is 0.5), t=9.5.
  {
    auto cone = createX3DNode("Cone");
    setF(cone, "bottomRadius", std::any(SFFloat(1.0f)));
    setF(cone, "height", std::any(SFFloat(2.0f)));
    auto hr = pickGeom(cone, Ray{{0,0,10},{0,0,-1}});
    CHECK((hr.hit));
    CHECK((feq(hr.point.z, 0.5f))); // lateral surface at y=0, z=0.5

    // AABB-corner tighter-than-proxy test: ray at (0.9,0.9,10) going (0,0,-1).
    // AABB = [-1,1]^3. x=0.9 and y=0.9 are both inside the AABB. The closest
    // approach of the cone lateral surface at x=0.9,z=any is only reached when
    // x²+z² = k²*(1-y)² with k=0.5. For x=0.9,z=0.9: x²+z²=1.62,
    // (1-0.9)²*0.25 = 0.0025 (at y=0.9) far too small, and at y=-1:
    // k²*(1-(-1))² = 0.25*4 = 1 < 1.62 -> base cap x²+z²=1.62>1 -> no cap.
    // The AABB proxy would return a hit; the analytic test returns no hit.
    auto miss_cone = pickGeom(cone, Ray{{0.9f,0.9f,10},{0,0,-1}});
    CHECK((!miss_cone.hit)); // tighter-than-AABB: analytic cone rejects corner

    // Axial ray from above (0,10,0) going -y: the lateral quadric has a double
    // root at the apex (A=-0.25, B=2.25, C=-20.25, disc=0) → t=9, hy=+1=halfH
    // (boundary, accepted); the base cap is at t=11. Nearest t=9 wins, so the
    // hit point is the apex at y=+1.
    auto apex_hit = pickGeom(cone, Ray{{0,10,0},{0,-1,0}});
    CHECK((apex_hit.hit && feq(apex_hit.point.y, 1.0f)));
  }

  // Cylinder (default: radius=1, height=2): spans y in [-1,1]. Ray along -z
  // from (0,0,10) hits the lateral surface at z=1, t=9.
  {
    auto cyl = createX3DNode("Cylinder");
    setF(cyl, "radius", std::any(SFFloat(1.0f)));
    setF(cyl, "height", std::any(SFFloat(2.0f)));
    auto hr = pickGeom(cyl, Ray{{0,0,10},{0,0,-1}});
    CHECK((hr.hit));
    CHECK((feq(hr.point.z, 1.0f))); // lateral surface at z=1

    // Tighter-than-AABB: a diagonal ray that crosses the AABB [-1,1]^3 but
    // never comes within radius=1 of the Y axis. Origin (-1.5,0,0), dir (1,0,1):
    // closest approach to the axis is at t=0.75 → point (-0.75,0,0.75),
    // dist = 0.75·√2 ≈ 1.06 > 1, so the cylinder is missed. The ray still enters
    // the AABB (x,z both in [-1,1] for t∈[0.7,1.4]), so the proxy would over-hit.
    auto miss_cyl_corner = pickGeom(cyl, Ray{{-1.5f,0,0},{1,0,1}});
    CHECK((!miss_cyl_corner.hit)); // AABB entered, analytic cylinder missed

    // Top cap hit from above.
    auto hr_top = pickGeom(cyl, Ray{{0.5f,10,0},{0,-1,0}});
    CHECK((hr_top.hit && feq(hr_top.point.y, 1.0f)));
  }

  // ---- M2D-2 tests: strip/fan triangle extraction via buildLocalMesh --------

  // TriangleStripSet: strip of 4 points P0,P1,P2,P3 in z=0 plane.
  // Produces 2 triangles. A ray hitting tri1 (P1,P3,P2 after winding flip) but
  // not tri0 should still get a hit; a ray THROUGH the AABB that lands in the
  // gap beyond the strip gets no hit.
  {
    // Strip in z=0 plane, a thin L-shape. Use the flat square strip:
    // P0=(0,0,0), P1=(1,0,0), P2=(0,1,0), P3=(1,1,0).
    // tri0 = P0,P1,P2 (even), tri1 = P1,P3,P2 (odd, winding flip).
    auto strip = createX3DNode("TriangleStripSet");
    attachCoord(strip, {{0,0,0},{1,0,0},{0,1,0},{1,1,0}});
    setF(strip, "stripCount", std::any(std::vector<int>{4}));
    // AABB covers [0,1]x[0,1]x[0,0]. Ray from (0.8,0.8,5) going -z hits
    // the strip (point is in tri1 region). This is inside AABB.
    auto hr_strip = pickGeom(strip, Ray{{0.8f,0.8f,5},{0,0,-1}});
    CHECK((hr_strip.hit && feq(hr_strip.point.z, 0.0f)));
    // Ray through the AABB at (0.5, 0.5, 5) also hits (inside both triangles).
    auto hr_strip2 = pickGeom(strip, Ray{{0.5f,0.5f,5},{0,0,-1}});
    CHECK((hr_strip2.hit));
  }

  // TriangleFanSet: a simple fan with 4 points producing 2 triangles.
  // P0=(0,0,0), P1=(1,0,0), P2=(1,1,0), P3=(0,1,0).
  // tri0=P0,P1,P2; tri1=P0,P2,P3. Covers the unit square in z=0.
  {
    auto fan = createX3DNode("TriangleFanSet");
    attachCoord(fan, {{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
    setF(fan, "fanCount", std::any(std::vector<int>{4}));
    auto hr_fan = pickGeom(fan, Ray{{0.8f,0.2f,5},{0,0,-1}});
    CHECK((hr_fan.hit && feq(hr_fan.point.z, 0.0f)));
    // A ray far from the fan misses.
    auto miss_fan = pickGeom(fan, Ray{{5,5,5},{0,0,-1}});
    CHECK((!miss_fan.hit));
  }

  // IndexedTriangleStripSet: same geometry, using index field.
  {
    auto iststrip = createX3DNode("IndexedTriangleStripSet");
    attachCoord(iststrip, {{0,0,0},{1,0,0},{0,1,0},{1,1,0}});
    setF(iststrip, "index", std::any(std::vector<int>{0,1,2,3}));
    auto hr_is = pickGeom(iststrip, Ray{{0.8f,0.8f,5},{0,0,-1}});
    CHECK((hr_is.hit && feq(hr_is.point.z, 0.0f)));
  }

  // IndexedTriangleFanSet: same as fan above with explicit index.
  {
    auto itfan = createX3DNode("IndexedTriangleFanSet");
    attachCoord(itfan, {{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
    setF(itfan, "index", std::any(std::vector<int>{0,1,2,3}));
    auto hr_if = pickGeom(itfan, Ray{{0.8f,0.2f,5},{0,0,-1}});
    CHECK((hr_if.hit && feq(hr_if.point.z, 0.0f)));
  }

  // ---- M2.5 tests: PickResult carries path + surface normal + texcoord -----

  // (a) pickClosest returns the full root→hit path.
  // Transform > Group > Shape > Box. Path = [T, G, shape] (the geometry-bearing
  // node is the Shape; Box hangs off it as the geometry field, not a child).
  {
    auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{0,0,0}));
    auto G = createX3DNode("Group");
    auto bx = createX3DNode("Box"); setF(bx, "size", std::any(SFVec3f{2,2,2}));
    auto sh = shapeWith(bx);
    addChild(G, sh); addChild(T, G);
    Scene sc; sc.addRootNode(T);
    TransformSystem tsp; tsp.buildIndex(sc);
    BoundsSystem bsp; bsp.buildBounds(sc, tsp);
    PickSystem psp; psp.build(sc);
    auto hr = psp.pickClosest(Ray{{0,0,10},{0,0,-1}}, bsp);
    CHECK((hr.hit && hr.node == sh.get()));
    CHECK((hr.path.size() == 3));
    CHECK((hr.path[0] == T.get() && hr.path[1] == G.get() && hr.path[2] == sh.get()));
  }

  // (b) Known normal on a Box face: ray -z onto the +Z (FRONT) face → world
  // normal (0,0,1). And a Sphere +Z front face → normal (0,0,1).
  {
    auto bx = createX3DNode("Box"); setF(bx, "size", std::any(SFVec3f{2,2,2}));
    auto hr = pickGeom(bx, Ray{{0,0,10},{0,0,-1}});
    CHECK((hr.hit));
    CHECK((feq(hr.normal.x,0) && feq(hr.normal.y,0) && feq(hr.normal.z,1)));
    // Box FRONT face hit at center → texcoord (0.5, 0.5) (§13.3.1).
    CHECK((feq(hr.texCoord.x,0.5f) && feq(hr.texCoord.y,0.5f)));

    auto sph = createX3DNode("Sphere"); setF(sph, "radius", std::any(2.0f));
    auto hs = pickGeom(sph, Ray{{0,0,10},{0,0,-1}});
    CHECK((hs.hit));
    CHECK((feq(hs.normal.x,0) && feq(hs.normal.y,0) && feq(hs.normal.z,1)));
    // Sphere front (+Z, x=0, z=r): §13.3.7 → s=0.5, t=0.5 (equator front).
    CHECK((feq(hs.texCoord.x,0.5f) && feq(hs.texCoord.y,0.5f)));
  }

  // (b2) Box +X (RIGHT) face: ray -x at x=10 → world normal (1,0,0).
  {
    auto bx = createX3DNode("Box"); setF(bx, "size", std::any(SFVec3f{2,2,2}));
    auto hr = pickGeom(bx, Ray{{10,0,0},{-1,0,0}});
    CHECK((hr.hit));
    CHECK((feq(hr.normal.x,1) && feq(hr.normal.y,0) && feq(hr.normal.z,0)));
    // RIGHT face center → (0.5,0.5).
    CHECK((feq(hr.texCoord.x,0.5f) && feq(hr.texCoord.y,0.5f)));
  }

  // (b3) Normal under a rotation: Box rotated 90° about Y so its local +X face
  // points to world +Z. Ray from +Z hits that face → world normal (0,0,1).
  {
    auto T = createX3DNode("Transform");
    setF(T, "rotation", std::any(SFRotation{0,1,0, 1.5707963f})); // +90° about Y
    auto bx = createX3DNode("Box"); setF(bx, "size", std::any(SFVec3f{2,2,2}));
    addChild(T, shapeWith(bx));
    Scene sc; sc.addRootNode(T);
    TransformSystem tsp; tsp.buildIndex(sc);
    BoundsSystem bsp; bsp.buildBounds(sc, tsp);
    PickSystem psp; psp.build(sc);
    auto hr = psp.pickClosest(Ray{{0,0,10},{0,0,-1}}, bsp);
    CHECK((hr.hit));
    // The local +X face now faces world +Z; the rotated outward normal is +Z.
    CHECK((feq(hr.normal.x,0) && feq(hr.normal.y,0) && feq(hr.normal.z,1)));
  }

  // (c) Known texcoord on a mesh via barycentric interp. A single quad in z=0
  // with explicit texCoords (TextureCoordinate). Hit at the center (0.5,0.5,0)
  // → texcoord (0.5,0.5).
  {
    auto ifs = createX3DNode("IndexedFaceSet");
    attachCoord(ifs, {{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
    setF(ifs, "coordIndex", std::any(std::vector<int>{0,1,2,3,-1}));
    // TextureCoordinate matching the corners.
    auto tc = createX3DNode("TextureCoordinate");
    setF(tc, "point", std::any(std::vector<SFVec2f>{{0,0},{1,0},{1,1},{0,1}}));
    setF(ifs, "texCoord", std::any(std::static_pointer_cast<X3DNode>(tc)));
    setF(ifs, "texCoordIndex", std::any(std::vector<int>{0,1,2,3,-1}));
    auto hr = pickGeom(ifs, Ray{{0.5f,0.5f,5},{0,0,-1}});
    CHECK((hr.hit && feq(hr.point.z,0.0f)));
    CHECK((feq(hr.texCoord.x,0.5f) && feq(hr.texCoord.y,0.5f)));
    // Face normal is +Z (CCW quad in z=0 plane).
    CHECK((feq(std::fabs(hr.normal.z),1.0f)));
  }

  // (c2) Mesh texcoord at a non-center point: hit at (0.25,0.75,0) → tex (0.25,0.75).
  {
    auto ifs = createX3DNode("IndexedFaceSet");
    attachCoord(ifs, {{0,0,0},{1,0,0},{1,1,0},{0,1,0}});
    setF(ifs, "coordIndex", std::any(std::vector<int>{0,1,2,3,-1}));
    auto tc = createX3DNode("TextureCoordinate");
    setF(tc, "point", std::any(std::vector<SFVec2f>{{0,0},{1,0},{1,1},{0,1}}));
    setF(ifs, "texCoord", std::any(std::static_pointer_cast<X3DNode>(tc)));
    setF(ifs, "texCoordIndex", std::any(std::vector<int>{0,1,2,3,-1}));
    auto hr = pickGeom(ifs, Ray{{0.25f,0.75f,5},{0,0,-1}});
    CHECK((hr.hit));
    CHECK((feq(hr.texCoord.x,0.25f) && feq(hr.texCoord.y,0.75f)));
  }

  // (d) Primitive face texcoord per §13: Box +X (RIGHT) face hit at a known
  // off-center local point. Ray -x hitting at local (1, 0.5, -0.5):
  // RIGHT: s = (-z/hz+1)/2 = (0.5+1)/2 = 0.75; t = (y/hy+1)/2 = (0.5+1)/2 = 0.75.
  {
    auto bx = createX3DNode("Box"); setF(bx, "size", std::any(SFVec3f{2,2,2}));
    auto hr = pickGeom(bx, Ray{{10,0.5f,-0.5f},{-1,0,0}});
    CHECK((hr.hit));
    CHECK((feq(hr.texCoord.x,0.75f) && feq(hr.texCoord.y,0.75f)));
  }

  // (d2) Cylinder side seam check: ray -z hits the front (+Z) of the lateral
  // surface at local (0,0,1) → s=0.5 (front), t=0.5 (mid-height); normal (0,0,1).
  {
    auto cyl = createX3DNode("Cylinder");
    setF(cyl, "radius", std::any(SFFloat(1.0f)));
    setF(cyl, "height", std::any(SFFloat(2.0f)));
    auto hr = pickGeom(cyl, Ray{{0,0,10},{0,0,-1}});
    CHECK((hr.hit));
    CHECK((feq(hr.texCoord.x,0.5f) && feq(hr.texCoord.y,0.5f)));
    CHECK((feq(hr.normal.x,0) && feq(hr.normal.y,0) && feq(hr.normal.z,1)));
  }

  return;
}
