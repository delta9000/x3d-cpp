// Per-world ownership, not numerical geodesy conformance. The intentionally
// incompatible mock scales geocentric space; published58's mutable selector
// makes A's live reads disagree with its cached transforms after B is created.
// Numerical projection accuracy remains covered by geo_projection_test and the
// optional PROJ swap test. Each world below owns a distinct native node graph.
#include "RuntimeSession.hpp"
#include "GeoPositionInterpolatorSystem.hpp"
#include "HAnimSkin.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/GeoPositionInterpolator.hpp"
#include "doctest/doctest.h"

#include <atomic>
#include <barrier>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

namespace {
class OwnershipProjection final : public geo::BuiltinGeoProjection {
public:
  explicit OwnershipProjection(double scale) : scale_(scale) {}
  bool geodeticToGeocentric(const geo::Ellipsoid &e, double lat, double lon,
                           double h, SFVec3d &out) const override {
    if (!geo::BuiltinGeoProjection::geodeticToGeocentric(e, lat, lon, h, out))
      return false;
    out = {out.x * scale_, out.y * scale_, out.z * scale_};
    return true;
  }
  bool geocentricToGeodetic(const geo::Ellipsoid &e, const SFVec3d &v,
                           double &lat, double &lon, double &h) const override {
    return geo::BuiltinGeoProjection::geocentricToGeodetic(
        e, {v.x / scale_, v.y / scale_, v.z / scale_}, lat, lon, h);
  }
private:
  const double scale_;
};

void author(const SFNode &n, const char *name, std::any value) {
  for (const auto &f : n->fields())
    if (f.x3dName == name && f.set) { f.set(*n, std::move(value)); return; }
  throw std::runtime_error(std::string("missing test field: ") + name);
}
template<class T> T read(const SFNode &n, const char *name) {
  for (const auto &f : n->fields())
    if (f.x3dName == name && f.get) return std::any_cast<T>(f.get(*n));
  throw std::runtime_error(std::string("missing test field: ") + name);
}
SFNode shape(const SFNode &geometry) {
  auto s = createX3DNode("Shape"); author(s, "geometry", geometry); return s;
}
bool near(float a, float b, float eps = .003f) { return std::abs(a - b) < eps; }
bool same(const SFVec3f &a, const SFVec3f &b, float eps = .003f) {
  return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}
SFVec3f scaled(const SFVec3f &v, float s) { return {v.x*s, v.y*s, v.z*s}; }
SFVec3f minus(const SFVec3f &a, const SFVec3f &b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
SFVec3f plus(const SFVec3f &a, const SFVec3f &b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
SFVec3f normal(const SFVec3f &a, const SFVec3f &b, const SFVec3f &c) {
  const auto u = minus(b,a), v = minus(c,a);
  SFVec3f n{u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x};
  const float length = std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
  return scaled(n, 1/length);
}

struct GeoWorld {
  SFNode origin = createX3DNode("GeoOrigin");
  SFNode location = createX3DNode("GeoLocation");
  SFNode transform = createX3DNode("GeoTransform");
  SFNode viewpoint = createX3DNode("GeoViewpoint");
  SFNode coord = createX3DNode("GeoCoordinate");
  SFNode geometry = createX3DNode("IndexedFaceSet");
  SFNode grid = createX3DNode("GeoElevationGrid");
  SFNode sensor = createX3DNode("GeoProximitySensor");
  std::shared_ptr<GeoPositionInterpolator> interpolator = std::make_shared<GeoPositionInterpolator>();
  SFNode coordShape, gridShape;
  std::unique_ptr<RuntimeSession> session;

  explicit GeoWorld(std::shared_ptr<const geo::GeoProjection> owner = {}) {
    author(origin, "geoCoords", SFVec3d{0,0,0});
    author(origin, "rotateYUp", true);
    for (const auto &n : {location, transform, viewpoint, coord, grid, sensor,
                          std::static_pointer_cast<X3DNode>(interpolator)})
      author(n, "geoOrigin", origin);
    author(location, "geoCoords", SFVec3d{0,0,10});
    author(location, "children", MFNode{shape(createX3DNode("Box"))});
    author(transform, "geoCenter", SFVec3d{0,0,20});
    author(transform, "translation", SFVec3f{100,1,0});
    author(transform, "scale", SFVec3f{1,2,1});
    author(transform, "children", MFNode{shape(createX3DNode("Box"))});
    author(viewpoint, "position", SFVec3d{0,0,30});
    author(coord, "point", MFVec3d{{0,0,3}, {0,.0001,3}, {.0001,0,3}});
    author(geometry, "coord", coord);
    author(geometry, "coordIndex", MFInt32{0,1,2,-1});
    author(geometry, "solid", false);
    author(grid, "xDimension", SFInt32{2});
    author(grid, "zDimension", SFInt32{2});
    author(grid, "xSpacing", SFDouble{.0001});
    author(grid, "zSpacing", SFDouble{.0001});
    author(grid, "geoGridOrigin", SFVec3d{.001,.001,0});
    author(grid, "height", MFDouble{0,1,3,2});
    author(grid, "solid", false);
    author(sensor, "size", SFVec3f{1000,1000,1000});
    interpolator->setKey({0,1});
    interpolator->setKeyValue({{0,0,4},{0,0,8}});
    coordShape = shape(geometry); gridShape = shape(grid);
    X3DDocument doc;
    doc.scene.rootNodes = {viewpoint,location,transform,coordShape,gridShape,sensor,interpolator};
    // Both the local options object and the caller's pointer may die here.
    SessionOptions options; options.geoProjection = std::move(owner);
    session = RuntimeSession::create(std::move(doc), std::move(options));
  }
  X3DExecutionContext &ctx() { return session->context(); }
};

const RenderItem *itemFor(GeoWorld &w, const X3DNode *node) {
  for (RenderItemId i=0; i<w.session->extractor().itemCount(); ++i) {
    const auto &item = w.session->extractor().item(i);
    for (const auto *part : item.path) if (part == node) return &item;
  }
  return nullptr;
}

void checkMesh(GeoWorld &w, const SFNode &geometry, const SFNode &shapeNode, float scale) {
  // The independent numeric oracle is the built-in's fixed reference space,
  // scaled analytically. Cross products independently check emitted normals.
  const auto reference = buildLocalMesh(geometry.get(), geo::builtinProjection());
  const auto *item = itemFor(w, shapeNode.get());
  REQUIRE(item);
  const auto &mesh = *item->mesh;
  REQUIRE(mesh.positions.size() == reference.positions.size());
  REQUIRE(mesh.positions.size() >= 3);
  REQUIRE(mesh.normals.size() == mesh.positions.size());
  const auto bounds = w.ctx().localBounds(shapeNode.get());
  REQUIRE_FALSE(bounds.empty);
  for (std::size_t i=0; i<mesh.positions.size(); ++i) {
    CHECK(same(mesh.positions[i], scaled(reference.positions[i],scale)));
    CHECK(mesh.positions[i].x >= bounds.min.x-.003f);
    CHECK(mesh.positions[i].y >= bounds.min.y-.003f);
    CHECK(mesh.positions[i].z >= bounds.min.z-.003f);
    CHECK(mesh.positions[i].x <= bounds.max.x+.003f);
    CHECK(mesh.positions[i].y <= bounds.max.y+.003f);
    CHECK(mesh.positions[i].z <= bounds.max.z+.003f);
  }
  for (std::size_t i=0; i<mesh.positions.size(); i+=3) {
    const auto n = normal(mesh.positions[i],mesh.positions[i+1],mesh.positions[i+2]);
    CHECK(same(n,mesh.normals[i],.0001f));
  }
  // PickSystem currently supports coordinate-fed triangle meshes. Grid mesh
  // vertices/normals/bounds are covered above; this is not a grid-picking claim.
  if (geometry->nodeTypeName()=="GeoElevationGrid") return;
  const auto n = normal(mesh.positions[0],mesh.positions[1],mesh.positions[2]);
  const auto center = scaled(plus(plus(mesh.positions[0],mesh.positions[1]),mesh.positions[2]),1.0f/3);
  const Ray ray{plus(center,scaled(n,7)),scaled(n,-1)};
  const auto direct = PickSystem::intersectGeometry(geometry.get(),w.ctx().geoProjection(),ray);
  REQUIRE(direct.has_value());
  CHECK(*direct == doctest::Approx(7).epsilon(.001));
  const auto picked = w.ctx().pick(ray);
  REQUIRE(picked.hit);
  CHECK(picked.node == shapeNode.get());
  CHECK(same(picked.point,center,.01f));
}

void checkWorld(GeoWorld &w, float scale, float locationHeight=10, float cameraHeight=30) {
  CHECK(near(w.ctx().worldTransform(w.location.get()).m[13],scale*locationHeight));
  CHECK(near(w.ctx().worldTransformAny(w.location.get()).m[13],scale*locationHeight));
  CHECK(near(w.ctx().worldOf(w.location.get()).m[13],scale*locationHeight));
  // GeoTransform conjugates its scale around geoCenter: c + 1 - 2*c.
  CHECK(near(w.ctx().worldTransformAny(w.transform.get()).m[13],1-scale*20));
  CHECK(near(w.ctx().cameraWorldPosition().y,scale*cameraHeight));
  CHECK(near(w.session->extractor().camera().viewMatrix.inverse().m[13],scale*cameraHeight));
  checkMesh(w,w.geometry,w.coordShape,scale);
  checkMesh(w,w.grid,w.gridShape,scale);
}
}

TEST_CASE("geo ownership: interleaved worlds retain projection across full delta dirty pick and camera") {
  auto ownerA = std::make_shared<OwnershipProjection>(1);
  auto ownerB = std::make_shared<OwnershipProjection>(3);
  std::weak_ptr<const geo::GeoProjection> weakA=ownerA, weakB=ownerB;
  auto a=std::make_unique<GeoWorld>(ownerA);
  const auto *identityA=ownerA.get();
  ownerA.reset();
  CHECK_FALSE(weakA.expired());
  CHECK(&a->ctx().geoProjection()==identityA);
  CHECK(a->interpolator->getValue_changed().y==doctest::Approx(4));
  a->session->fullSnapshot();
  auto b=std::make_unique<GeoWorld>(ownerB);
  ownerB.reset();
  CHECK_FALSE(weakB.expired());
  CHECK(b->interpolator->getValue_changed().y==doctest::Approx(12));
  b->session->fullSnapshot();
  for (int pass=0;pass<3;++pass) {
    b->session->tick(pass); b->session->delta();
    a->session->tick(pass); a->session->delta();
    checkWorld(*a,1); checkWorld(*b,3);
    for (auto *w : {a.get(),b.get()}) {
      w->interpolator->onSet_fraction(.5f); w->ctx().process();
      CHECK(read<SFBool>(w->sensor,"isActive"));
      CHECK(read<SFVec3d>(w->sensor,"geoCoord_changed").z==doctest::Approx(30).epsilon(.001));
    }
    CHECK(a->interpolator->getValue_changed().y==doctest::Approx(6));
    CHECK(b->interpolator->getValue_changed().y==doctest::Approx(18));
  }
  // Dirty writes exercise recomposition, mesh cache replacement, camera reads,
  // and bounds refresh after both worlds have already been built and ticked.
  a->ctx().postEvent(a->location.get(),"geoCoords",SFVec3d{0,0,11});
  a->ctx().postEvent(a->coord.get(),"point",MFVec3d{{0,0,5},{0,.0001,5},{.0001,0,5}});
  a->ctx().postEvent(a->grid.get(),"height",MFDouble{4,5,7,6});
  a->ctx().postEvent(a->viewpoint.get(),"position",SFVec3d{0,0,31});
  a->session->tick(4); const auto changed=a->session->delta();
  CHECK_FALSE(changed.updatedTransform.empty());
  CHECK(changed.updatedGeometry.size()==2);
  CHECK(changed.cameraChanged);
  checkWorld(*a,1,11,31); checkWorld(*b,3);
  a->session->fullSnapshot(); b->session->fullSnapshot();
  checkWorld(*a,1,11,31); checkWorld(*b,3);
  b.reset(); CHECK(weakB.expired()); CHECK_FALSE(weakA.expired());
  a->session->tick(5); a->session->delta(); checkWorld(*a,1,11,31);
  a.reset(); CHECK(weakA.expired());
}

TEST_CASE("geo ownership: default and null owners use the stable builtin and GC bypasses custom backends") {
  GeoWorld implicit;
  GeoWorld explicitNull{std::shared_ptr<const geo::GeoProjection>{}};
  CHECK(&implicit.ctx().geoProjection()==&geo::builtinProjection());
  CHECK(&explicitNull.ctx().geoProjection()==&geo::builtinProjection());
  CHECK(geo::builtinProjectionOwner().get()==&geo::builtinProjection());
  implicit.session->fullSnapshot(); explicitNull.session->fullSnapshot();
  checkWorld(implicit,1); checkWorld(explicitNull,1);
  OwnershipProjection odd(9);
  SFVec3d a,b;
  const auto gc=geo::parseGeoSystem({"GC"});
  REQUIRE(geo::toGeocentric(gc,{12,34,56},a,odd));
  REQUIRE(geo::fromGeocentric(gc,a,b,odd));
  CHECK(a==SFVec3d{12,34,56}); CHECK(b==a);
  auto coord=createX3DNode("GeoCoordinate");
  author(coord,"geoSystem",MFString{"GC"});
  author(coord,"point",MFVec3d{{1,2,3},{4,5,6}});
  const auto points=geombounds::getPointsLenient(*coord,"point",odd);
  REQUIRE(points.size()==2); CHECK(points[1]==SFVec3f{4,5,6});
}

TEST_CASE("geo ownership: the context retains its backend until destruction") {
  auto projection=std::make_shared<OwnershipProjection>(3);
  std::weak_ptr<const geo::GeoProjection> weak=projection;
  auto node=std::make_shared<GeoPositionInterpolator>();
  node->setKey({0,1}); node->setKeyValue({{0,0,0},{0,0,10}});
  {
    auto system=std::make_shared<GeoPositionInterpolatorSystem>();
    X3DExecutionContext context(projection);
    projection.reset();
    system->attach(node.get(),context); context.addSystem(system);
    SFVec3f p;
    CHECK(geo::toWorld(*node,{0,0,5},p,context.geoProjection()));
    node->onSet_fraction(.5f); context.process();
    CHECK(node->getValue_changed().x==doctest::Approx(3*(6378137.0+5)));
    CHECK_FALSE(weak.expired());
  }
  CHECK(weak.expired());
}

TEST_CASE("geo ownership: independent owner threads interleave construction tick extraction and destruction") {
  std::barrier together(2);
  std::atomic<int> failures{0};
  auto exercise=[&](float scale) {
    auto owner=std::make_shared<OwnershipProjection>(scale);
    std::weak_ptr<const geo::GeoProjection> weak=owner;
    {
      GeoWorld world(owner); owner.reset();
      world.session->fullSnapshot();
      together.arrive_and_wait();
      for (int i=0;i<12;++i) {
        world.session->tick(i); world.session->delta();
        world.interpolator->onSet_fraction(.5f); world.ctx().process();
        if (weak.expired() || !near(world.ctx().worldTransformAny(world.location.get()).m[13],10*scale) ||
            !near(world.ctx().cameraWorldPosition().y,30*scale) ||
            !near(world.interpolator->getValue_changed().y,6*scale)) ++failures;
        world.session->fullSnapshot();
        const auto *item=itemFor(world,world.coordShape.get());
        if (!item || item->mesh->positions.empty() || !near(item->mesh->positions[0].y,3*scale)) ++failures;
        together.arrive_and_wait();
      }
    }
    if (!weak.expired()) ++failures;
  };
  std::thread first(exercise,1), second(exercise,7);
  first.join(); second.join();
  CHECK(failures.load()==0);
}

TEST_CASE("geo ownership: navigation obtains elevation from its own inverse projection") {
  GeoWorld a(std::make_shared<OwnershipProjection>(1));
  GeoWorld b(std::make_shared<OwnershipProjection>(3));
  for (auto *w : {&a,&b}) {
    auto navigation=std::make_shared<NavigationSystem>();
    navigation->setForcedMode(NavigationSystem::Mode::Fly);
    w->ctx().addSystem(navigation);
    w->session->tick(0);
  }
  const auto beforeA=a.ctx().cameraWorldPosition(), beforeB=b.ctx().cameraWorldPosition();
  a.ctx().setKey(NavigationSystem::kKeyForward,true);
  b.ctx().setKey(NavigationSystem::kKeyForward,true);
  b.session->tick(1); a.session->tick(1);
  // Both author height 30, so both travel 30/10 metres per second, despite
  // incompatible projected camera heights. This exercises inverse conversion.
  CHECK(same(minus(a.ctx().cameraWorldPosition(),beforeA),{0,0,-3},.02f));
  CHECK(same(minus(b.ctx().cameraWorldPosition(),beforeB),{0,0,-3},.02f));
}

TEST_CASE("geo ownership: delayed Inline and GeoLOD tile attachments inherit the owning context") {
  struct TileWorld {
    SFNode origin=createX3DNode("GeoOrigin"), viewpoint=createX3DNode("GeoViewpoint");
    SFNode lod=createX3DNode("GeoLOD"), inlined=createX3DNode("Inline");
    struct Loaded { SFNode location; std::shared_ptr<GeoPositionInterpolator> interpolator; };
    std::shared_ptr<std::vector<Loaded>> loaded=std::make_shared<std::vector<Loaded>>();
    std::unique_ptr<RuntimeSession> session;
    explicit TileWorld(float scale) {
      author(origin,"geoCoords",SFVec3d{0,0,0}); author(origin,"rotateYUp",true);
      author(viewpoint,"geoOrigin",origin); author(viewpoint,"position",SFVec3d{0,0,30});
      author(lod,"geoOrigin",origin); author(lod,"center",SFVec3d{0,0,10});
      author(lod,"range",SFFloat{30});
      author(lod,"rootNode",MFNode{shape(createX3DNode("Box"))});
      author(lod,"child1Url",MFString{"tile"});
      author(inlined,"load",false); author(inlined,"url",MFString{"inline"});
      SessionOptions options;
      options.geoProjection=std::make_shared<OwnershipProjection>(scale);
      options.inlineResolver=[records=loaded,origin=origin](const MFString &, const std::string &) {
        auto child=std::make_shared<Scene>();
        auto location=createX3DNode("GeoLocation");
        author(location,"geoOrigin",origin); author(location,"geoCoords",SFVec3d{0,0,8});
        author(location,"children",MFNode{shape(createX3DNode("Box"))});
        auto interpolator=std::make_shared<GeoPositionInterpolator>();
        interpolator->setGeoOriginUnchecked(origin);
        interpolator->setKey({0,1}); interpolator->setKeyValue({{0,0,2},{0,0,6}});
        child->rootNodes={location,interpolator};
        records->push_back({location,interpolator});
        return child;
      };
      X3DDocument doc; doc.scene.rootNodes={viewpoint,lod,inlined};
      session=RuntimeSession::create(std::move(doc),std::move(options));
    }
  };
  TileWorld a(1),b(3);
  a.session->context().postEvent(a.inlined.get(),"load",true);
  b.session->context().postEvent(b.inlined.get(),"load",true);
  for (int i=0;i<3;++i) { b.session->tick(i); a.session->tick(i); }
  CHECK(read<SFInt32>(a.lod,"level_changed")==1); // own center 10, own eye 30
  CHECK(read<SFInt32>(b.lod,"level_changed")==0); // own center 30, own eye 90
  REQUIRE(a.loaded->size()==2); // Inline + near GeoLOD child
  REQUIRE(b.loaded->size()==1); // Inline only
  for (auto pair : {std::pair{&a,1.0f},std::pair{&b,3.0f}}) {
    auto &ctx=pair.first->session->context();
    const auto snapshot=pair.first->session->fullSnapshot();
    CHECK(snapshot.added.size()==2);
    for (const auto &node : *pair.first->loaded) {
      CHECK(near(ctx.worldTransformAny(node.location.get()).m[13],8*pair.second));
      CHECK(near(node.interpolator->getValue_changed().y,2*pair.second));
      node.interpolator->onSet_fraction(.5f); ctx.process();
      CHECK(near(node.interpolator->getValue_changed().y,4*pair.second));
    }
  }
}

TEST_CASE("geo ownership: standalone transform and pick systems keep their backend owner") {
  auto owner=std::make_shared<OwnershipProjection>(4);
  std::weak_ptr<const geo::GeoProjection> weak=owner;
  auto transform=std::make_unique<TransformSystem>(owner);
  auto pick=std::make_unique<PickSystem>(owner);
  owner.reset(); CHECK_FALSE(weak.expired());
  transform.reset(); CHECK_FALSE(weak.expired());
  pick.reset(); CHECK(weak.expired());
}

TEST_CASE("geo ownership: HAnim transform traversal takes an explicit projection for bind and live pose") {
  // Synthetic native graph seam oracle: a geographic transform between joints
  // forces both HAnim traversals through their generic transform dispatch.
  // This is not a claim that this graph is an ISO HAnim content profile.
  auto fixture=[] {
    auto origin=createX3DNode("GeoOrigin");
    author(origin,"geoCoords",SFVec3d{0,0,0}); author(origin,"rotateYUp",true);
    auto humanoid=createX3DNode("HAnimHumanoid"), root=createX3DNode("HAnimJoint");
    auto location=createX3DNode("GeoLocation"), joint=createX3DNode("HAnimJoint");
    auto coordinate=createX3DNode("Coordinate");
    author(coordinate,"point",MFVec3f{{0,0,0}});
    author(location,"geoOrigin",origin); author(location,"geoCoords",SFVec3d{0,0,5});
    author(location,"children",MFNode{joint}); author(root,"children",MFNode{location});
    author(joint,"skinCoordIndex",MFInt32{0}); author(joint,"skinCoordWeight",MFFloat{1});
    author(humanoid,"skinCoord",coordinate); author(humanoid,"joints",MFNode{joint});
    author(humanoid,"skeleton",MFNode{root});
    author(humanoid,"jointBindingPositions",MFVec3f{{0,0,0}});
    return std::pair{humanoid,location};
  };
  auto a=fixture(),b=fixture();
  OwnershipProjection projectionA(1),projectionB(3);
  const auto bindingA=hanim::compileBinding(*a.first,projectionA);
  const auto bindingB=hanim::compileBinding(*b.first,projectionB);
  REQUIRE(bindingA.inverseBind.size()==1); REQUIRE(bindingB.inverseBind.size()==1);
  CHECK(near(bindingA.inverseBind[0].m[13],-5));
  CHECK(near(bindingB.inverseBind[0].m[13],-15));
  author(a.second,"geoCoords",SFVec3d{0,0,7}); author(b.second,"geoCoords",SFVec3d{0,0,7});
  std::vector<SFVec3f> positionsA,positionsB;
  hanim::deform(bindingB,hanim::evaluatePose(bindingB,projectionB),positionsB,nullptr);
  hanim::deform(bindingA,hanim::evaluatePose(bindingA,projectionA),positionsA,nullptr);
  REQUIRE(positionsA.size()==1); REQUIRE(positionsB.size()==1);
  CHECK(near(positionsA[0].y,2)); CHECK(near(positionsB[0].y,6));
}
