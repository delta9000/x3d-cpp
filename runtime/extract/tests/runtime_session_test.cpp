// runtime_session_test.cpp — RuntimeSession contract.
//
// The session exists to own the buildSceneGraph + buildFrom + SceneExtractor
// ceremony and the unwritten lifetime contract between the three objects. These
// cases pin that it is EQUIVALENT to the low-level path (it must not become a
// policy layer with its own behaviour), that it does the wiring a caller can
// silently forget, and that the pieces stay reachable.

#include "RuntimeSession.hpp"

#include "X3DParse.hpp"
#include "XmlWriter.hpp"
#include "../../physics/PhysicsSystem.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/TextureTransform.hpp"
#include "doctest/doctest.h"

#include <memory>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;

namespace {

// A Transform driven by a TimeSensor->PositionInterpolator ROUTE chain: renders
// from buildSceneGraph, but only ANIMATES if buildFrom resolved the ROUTEs.
const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Viewpoint DEF="VP" position="0 0 10"/>
    <Transform DEF="Mover">
      <Shape><Appearance><Material/></Appearance><Box size="1 1 1"/></Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
    <PositionInterpolator DEF="Path" key="0 1"
                          keyValue="0 0 0 10 0 0"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed"
           toNode="Path" toField="set_fraction"/>
    <ROUTE fromNode="Path" fromField="value_changed"
           toNode="Mover" toField="translation"/>
  </Scene>
</X3D>)X3D";

X3DDocument parse() {
  return x3d::codec::parseDocument(kScene, x3d::codec::Encoding::XML);
}

} // namespace

TEST_CASE("RuntimeSession: wires both builds, so ROUTEs actually fire") {
  auto s = RuntimeSession::create(parse());

  // buildFrom ran: both authored ROUTEs resolved.
  CHECK(s->routes().routesAdded == 2);
  CHECK(s->routes().rejected.empty());

  s->fullSnapshot();
  CHECK(s->extractor().itemCount() == 1);

  // buildSceneGraph ran: the Viewpoint bound, so viewMatrix is NOT identity.
  // (Skipping buildSceneGraph is silent -- it leaves the camera at identity.)
  const Mat4 view = s->context().viewMatrix();
  CHECK(view.m[14] != 0.0f);

  // The ROUTE chain drives the Transform: tick and the delta reports it.
  s->tick(0.0);
  s->delta();
  s->tick(1.0);
  RenderDelta d = s->delta();
  CHECK_FALSE(d.updatedTransform.empty());
}

TEST_CASE("RuntimeSession: equivalent to the low-level path, not a policy layer") {
  // Same document, both ways -- the session must add no behaviour of its own.
  auto s = RuntimeSession::create(parse());
  RenderDelta viaSession = s->fullSnapshot();

  X3DDocument doc = parse();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  const BridgeResult bridge = ctx.buildFrom(doc.scene);
  extract::SceneExtractor ex(ctx, doc.scene);
  RenderDelta viaLowLevel = ex.fullSnapshot();

  CHECK(s->routes().routesAdded == bridge.routesAdded);
  CHECK(viaSession.added.size() == viaLowLevel.added.size());
  CHECK(s->extractor().itemCount() == ex.itemCount());
  CHECK(viaSession.cameraChanged == viaLowLevel.cameraChanged);
}

TEST_CASE("RuntimeSession: build order does NOT matter (buildFrom first is fine)") {
  // Pins the fact the session's doc comment asserts: the two builds write
  // disjoint state and commute. If this ever stops being true, the session's
  // "you cannot get the order wrong" claim becomes load-bearing rather than
  // convenient -- and this test is where that shows up.
  X3DDocument doc = parse();
  X3DExecutionContext ctx;
  ctx.buildFrom(doc.scene);      // REVERSED order...
  ctx.buildSceneGraph(doc.scene);
  extract::SceneExtractor ex(ctx, doc.scene);
  RenderDelta f0 = ex.fullSnapshot();

  auto s = RuntimeSession::create(parse()); // ...vs the session's order.
  RenderDelta ref = s->fullSnapshot();

  CHECK(f0.added.size() == ref.added.size());
  CHECK(ex.itemCount() == s->extractor().itemCount());
}

TEST_CASE("RuntimeSession: owns the document, so the scene outlives the caller's") {
  // The low-level path's real hazard: doc and ctx must outlive the extractor,
  // which holds references into both. The session takes ownership so a caller
  // cannot drop the document while the extractor still reads it.
  std::unique_ptr<RuntimeSession> s;
  {
    X3DDocument doc = parse();
    s = RuntimeSession::create(std::move(doc)); // caller's doc dies here.
  }
  RenderDelta f0 = s->fullSnapshot(); // still valid: the session owns it.
  CHECK(f0.added.size() == 1);
  CHECK(s->document().scene.rootNodes.size() == 4);
  CHECK((s->scene().resolve("Mover") != nullptr));
}

TEST_CASE("RuntimeSession: SessionOptions.assetResolver drives LoadSensor (§9)") {
  // A LoadSensor watching one ImageTexture. The session's default standard
  // runtime attaches the LoadSensorSystem; SessionOptions.assetResolver is the
  // byte oracle it resolves through. A Ready resolver → the NSN-9 success burst.
  const char *xml = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <LoadSensor DEF="LS">
      <ImageTexture containerField="children" url='"a.png"'/>
    </LoadSensor>
  </Scene>
</X3D>)X3D";

  SessionOptions opts;
  opts.assetResolver = [](const std::string &, AssetKind) {
    return AssetResult::makeReady({});
  };
  auto s = RuntimeSession::create(
      x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML), std::move(opts));
  s->tick(3.0);

  x3d::nodes::LoadSensor *ls = nullptr;
  for (auto &r : s->scene().rootNodes)
    if (auto *p = dynamic_cast<x3d::nodes::LoadSensor *>(r.get()))
      ls = p;
  REQUIRE((ls != nullptr));
  CHECK(ls->getIsLoaded());
  CHECK(ls->getLoadTime() == 3.0);
}

TEST_CASE("RuntimeSession: delta() totality is inherited, not re-implemented") {
  auto s = RuntimeSession::create(parse());

  // No fullSnapshot() first => delta() promotes to the baseline.
  RenderDelta first = s->delta();
  CHECK(first.added.size() == 1);

  // No tick() in between => empty, not a re-diff.
  RenderDelta second = s->delta();
  CHECK(second.added.empty());
  CHECK(second.updatedTransform.empty());
}

TEST_CASE("UNIT runtime: equivalent authored length and angle geometry") {
  auto scene = [](bool altered) {
    std::string head = altered ?
      "<head><unit category='length' name='cm' conversionFactor='0.01'/>"
      "<unit category='angle' name='degree' conversionFactor='0.017453292519943295'/></head>" : "";
    std::string fields = altered ? "translation='100 200 300' rotation='0 0 2 90'" :
                                   "translation='1 2 3' rotation='0 0 2 1.5707963267948966'";
    return RuntimeSession::create(x3d::codec::parseDocument(
      "<X3D profile='Interchange' version='4.0'>" + head +
      "<Scene><Transform DEF='T' " + fields + "><Shape><Box DEF='B' size='" +
      (altered ? "200 400 600" : "2 4 6") + "'/></Shape></Transform></Scene></X3D>",
      x3d::codec::Encoding::XML));
  };
  auto initial = scene(false), altered = scene(true);
  const auto a = initial->fullSnapshot(), b = altered->fullSnapshot();
  REQUIRE(a.added.size() == 1);
  REQUIRE(b.added.size() == 1);
  const auto &ia = initial->extractor().item(a.added.front());
  const auto &ib = altered->extractor().item(b.added.front());
  for (int i = 0; i < 16; ++i)
    CHECK(ia.worldTransform.m[i] == doctest::Approx(ib.worldTransform.m[i]));
  CHECK(ia.mesh->positions == ib.mesh->positions);
  auto t = altered->scene().resolve("T");
  CHECK(std::any_cast<SFRotation>(findField(*t, "rotation")->get(*t)).z == 2);
}

TEST_CASE("UNIT runtime: routed motion, defaults, and subsequent writes use initial units") {
  auto doc = x3d::codec::parseDocument(R"(
    <X3D profile='Interchange' version='4.0'>
    <head><unit category='length' name='cm' conversionFactor='0.01'/></head>
    <Scene><Transform DEF='Mover'><Shape><Box DEF='Default'/></Shape></Transform>
    <TimeSensor DEF='Clock' cycleInterval='4' loop='true'/>
    <PositionInterpolator DEF='Path' key='0 1' keyValue='0 0 0 1000 0 0'/>
    <ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Path' toField='set_fraction'/>
    <ROUTE fromNode='Path' fromField='value_changed' toNode='Mover' toField='translation'/>
    </Scene></X3D>)", x3d::codec::Encoding::XML);
  auto session = RuntimeSession::create(std::move(doc));
  auto box = session->scene().resolve("Default"), mover = session->scene().resolve("Mover");
  CHECK(std::any_cast<SFVec3f>(findField(*box, "size")->get(*box)).x == 2);
  session->tick(0); session->tick(1);
  CHECK(std::any_cast<SFVec3f>(findField(*mover, "translation")->get(*mover)).x == doctest::Approx(2.5));
  session->context().buildFrom(session->scene());
  session->context().buildSceneGraph(session->scene());
  auto path = session->scene().resolve("Path");
  CHECK(std::any_cast<std::vector<SFVec3f>>(findField(*path, "keyValue")->get(*path)).back().x == 10);
  CHECK(session->context().writeField(mover.get(), "translation", SFVec3f{7,0,0}) == FieldWriteResult::Ok);
  session->context().buildSceneGraph(session->scene());
  CHECK(std::any_cast<SFVec3f>(findField(*mover, "translation")->get(*mover)).x == 7);
}

TEST_CASE("UNIT runtime: Inline and EXTERNPROTO use their source units") {
  const std::string childText = R"(<X3D profile='Interchange' version='4.0'>
    <head><unit category='length' name='cm' conversionFactor='0.01'/></head><Scene>
    <ProtoDeclare name='Q'><ProtoInterface><field name='v' type='SFVec3f'
      accessType='initializeOnly'/></ProtoInterface><ProtoBody><Transform><IS>
      <connect nodeField='translation' protoField='v'/></IS></Transform></ProtoBody></ProtoDeclare>
    <ProtoDeclare name='P'><ProtoInterface><field name='custom' type='SFVec3f'
      accessType='initializeOnly' value='3 4 5'/></ProtoInterface><ProtoBody>
      <Transform translation='100 0 0'><Shape><Box size='200 400 600'/></Shape>
      <Transform><IS><connect nodeField='translation' protoField='custom'/></IS></Transform>
      <ProtoInstance name='Q'><fieldValue name='v' value='100 0 0'/></ProtoInstance>
      </Transform></ProtoBody></ProtoDeclare>
    <Shape><Box size='200 400 600'/></Shape></Scene></X3D>)";
  auto child = x3d::codec::parseDocument(childText);
  x3d::codec::ProtoDeclarationResolver proto =
    [&](const std::vector<std::string> &, const std::string &) { return child.scene.findProto("P"); };
  InlineResolver inl = [&](const std::vector<std::string> &, const std::string &) {
    return std::make_shared<Scene>(x3d::codec::parseDocument(childText).scene);
  };
  auto doc = x3d::codec::parseDocument(R"(<X3D profile='Interchange' version='4.0'>
    <head><unit category='length' name='km' conversionFactor='1000'/></head>
    <Scene><ExternProtoDeclare name='P' url='"memory#P"'>
    <field name='custom' type='SFVec3f' accessType='initializeOnly'/></ExternProtoDeclare>
    <Inline url='"memory"'/><ProtoInstance DEF='External' name='P'/></Scene></X3D>)",
    x3d::codec::Encoding::XML, "", proto, inl);
  // Codec output before entering runtime retains the author's numbers and UNIT.
  x3d::codec::XmlWriter writer;
  auto authored = x3d::codec::parseDocument(writer.writeDocument(doc),
    x3d::codec::Encoding::XML, "", proto, inl);
  CHECK(authored.head.units[0].conversionFactor == 1000);
  REQUIRE(authored.scene.expandedInlineScenes.size() == 1);
  const auto &authoredChild = *authored.scene.expandedInlineScenes.begin()->second;
  REQUIRE(authoredChild.rootNodes.size() == 1);
  auto authoredBox = std::any_cast<std::shared_ptr<X3DNode>>(
    findField(*authoredChild.rootNodes.front(), "geometry")->get(*authoredChild.rootNodes.front()));
  REQUIRE(authoredBox);
  CHECK(std::any_cast<SFVec3f>(findField(*authoredBox, "size")->get(*authoredBox)) == SFVec3f{200,400,600});
  auto session = RuntimeSession::create(std::move(doc));
  auto snapshot = session->fullSnapshot();
  REQUIRE(snapshot.added.size() == 2);
  for (auto id : snapshot.added) {
    const auto &mesh = session->extractor().item(id).mesh;
    REQUIRE_FALSE(mesh->positions.empty());
    float maxX = 0;
    for (const auto &position : mesh->positions) maxX = std::max(maxX, std::abs(position.x));
    CHECK(maxX == doctest::Approx(1));
  }
  auto primary = session->scene().resolve("External");
  REQUIRE(primary);
  CHECK(std::any_cast<SFVec3f>(findField(*primary, "translation")->get(*primary)).x == 1);
  std::vector<std::shared_ptr<X3DNode>> children =
    std::any_cast<MFNode>(findField(*primary, "children")->get(*primary));
  REQUIRE(children.size() == 3);
  CHECK(std::any_cast<SFVec3f>(findField(*children[1], "translation")->get(*children[1])).x == doctest::Approx(0.03));
  CHECK(std::any_cast<SFVec3f>(findField(*children[2], "translation")->get(*children[2])).x == 1);

  auto lazyDoc = x3d::codec::parseDocument(R"(<X3D profile='Interchange' version='4.0'>
    <head><unit category='length' name='km' conversionFactor='1000'/></head>
    <Scene><Inline DEF='Lazy' load='false' url='"memory"'/></Scene></X3D>)",
    x3d::codec::Encoding::XML, "", proto, inl);
  SessionOptions options; options.inlineResolver = inl;
  auto lazy = RuntimeSession::create(std::move(lazyDoc), options);
  CHECK(lazy->fullSnapshot().added.empty());
  CHECK(lazy->context().writeField(lazy->scene().resolve("Lazy").get(), "load", true) == FieldWriteResult::Ok);
  lazy->tick(0);
  auto loaded = lazy->fullSnapshot();
  REQUIRE(loaded.added.size() == 1);
  float maxX = 0;
  for (const auto &p : lazy->extractor().item(loaded.added.front()).mesh->positions)
    maxX = std::max(maxX, std::abs(p.x));
  CHECK(maxX == doctest::Approx(1));
}

namespace {
class UnitRecordingBackend : public PhysicsBackend {
public:
  MassProperties mass;
  SFVec3f force{}, torque{};
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &m,
      bool, const SFVec3f &, const SFRotation &, const SFVec3f &, const SFVec3f &) override {
    mass = m; return 1;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override { return 1; }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &f, const SFVec3f &t) override {
    force = f; torque = t;
  }
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &l, SFVec3f &a) const override {
    l = {}; a = {};
  }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &p, SFRotation &o) const override {
    p = {}; o = {0,0,1,0};
  }
};
}

TEST_CASE("UNIT runtime: PhysicsSystem receives kilograms and newtons") {
  auto doc = x3d::codec::parseDocument(R"(<X3D profile='Full' version='4.0'>
    <head><unit category='mass' name='g' conversionFactor='0.001'/>
    <unit category='force' name='scaledNewton' conversionFactor='2'/>
    <unit category='length' name='cm' conversionFactor='0.01'/></head>
    <Scene><RigidBodyCollection DEF='World' gravity='0 0 0'>
    <RigidBody containerField='bodies' mass='2000' forces='0 5 0' torques='0 0 100'>
    <CollidableShape containerField='geometry'><Shape containerField='shape'><Box size='100 100 100'/></Shape></CollidableShape>
    </RigidBody></RigidBodyCollection></Scene></X3D>)");
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  auto backend = std::make_shared<UnitRecordingBackend>();
  PhysicsSystem physics(backend);
  physics.attach(doc.scene.resolve("World").get(), ctx);
  physics.update(0, ctx); physics.update(0.02, ctx);
  CHECK(backend->mass.mass == doctest::Approx(2));
  CHECK(backend->force.y == doctest::Approx(10));
  CHECK(backend->torque.z == doctest::Approx(2));
}

TEST_CASE("UNIT runtime: custom PROTO values convert independently at dimensional IS targets") {
  auto doc = x3d::codec::parseDocument(R"(<X3D profile='Interchange' version='4.0'>
    <head><unit category='length' name='cm' conversionFactor='0.01'/></head>
    <Scene><ProtoDeclare name='P'><ProtoInterface>
    <field name='v' type='SFVec3f' accessType='initializeOnly' value='50 60 70'/>
    </ProtoInterface><ProtoBody><Transform><IS>
    <connect nodeField='translation' protoField='v'/><connect nodeField='scale' protoField='v'/>
    </IS><Shape><Box/></Shape></Transform></ProtoBody></ProtoDeclare>
    <ProtoInstance DEF='Override' name='P'><fieldValue name='v' value='100 200 300'/></ProtoInstance>
    <ProtoInstance DEF='Default' name='P'/></Scene></X3D>)");
  auto session = RuntimeSession::create(std::move(doc));
  auto override = session->scene().resolve("Override"), defaults = session->scene().resolve("Default");
  REQUIRE(override); REQUIRE(defaults);
  CHECK(std::any_cast<SFVec3f>(findField(*override, "translation")->get(*override)).x == 1);
  CHECK(std::any_cast<SFVec3f>(findField(*override, "scale")->get(*override)).x == 100);
  CHECK(std::any_cast<SFVec3f>(findField(*defaults, "translation")->get(*defaults)).x == doctest::Approx(0.5));
  CHECK(std::any_cast<SFVec3f>(findField(*defaults, "scale")->get(*defaults)).x == 50);
}

TEST_CASE("UNIT runtime: HAnim bind pose and displacement match initial units") {
  auto make = [](bool altered) {
    const std::string head = altered ?
      "<head><unit category='length' name='cm' conversionFactor='0.01'/>"
      "<unit category='angle' name='degree' conversionFactor='0.017453292519943295'/></head>" : "";
    const std::string position = altered ? "100 0 0" : "1 0 0";
    const std::string rotation = altered ? "0 0 1 90" : "0 0 1 1.5707963267948966";
    const std::string displacement = altered ? "0 0 100" : "0 0 1";
    const std::string points = altered ? "0 0 0 100 0 0 0 100 0" : "0 0 0 1 0 0 0 1 0";
    return RuntimeSession::create(x3d::codec::parseDocument(
      "<X3D profile='Full' version='4.0'>" + head + "<Scene>"
      "<HAnimHumanoid jointBindingPositions='" + position + "' jointBindingRotations='" + rotation + "'>"
      "<HAnimJoint DEF='J' containerField='skeleton' translation='" + position + "' rotation='" + rotation + "' "
      "skinCoordIndex='0 1 2' skinCoordWeight='1 1 1'>"
      "<HAnimDisplacer containerField='displacers' coordIndex='0' weight='1' displacements='" + displacement + "'/>"
      "</HAnimJoint><HAnimJoint USE='J' containerField='joints'/>"
      "<Coordinate DEF='C' containerField='skinCoord' point='" + points + "'/>"
      "<Shape containerField='skin'><IndexedFaceSet coordIndex='0 1 2 -1'>"
      "<Coordinate USE='C'/></IndexedFaceSet></Shape></HAnimHumanoid></Scene></X3D>"));
  };
  auto initial = make(false), altered = make(true);
  const auto a = initial->fullSnapshot(), b = altered->fullSnapshot();
  REQUIRE(a.added.size() == 1); REQUIRE(b.added.size() == 1);
  REQUIRE(initial->extractor().item(a.added.front()).skin.has_value());
  REQUIRE(altered->extractor().item(b.added.front()).skin.has_value());
  const auto ia = initial->extractor().deformedMesh(a.added.front());
  const auto ib = altered->extractor().deformedMesh(b.added.front());
  REQUIRE(ia.positions.size() == 3); REQUIRE(ib.positions.size() == 3);
  CHECK(ia.positions.front().z == doctest::Approx(1));
  for (std::size_t i = 0; i < ia.positions.size(); ++i) {
    CHECK(ia.positions[i].x == doctest::Approx(ib.positions[i].x));
    CHECK(ia.positions[i].y == doctest::Approx(ib.positions[i].y));
    CHECK(ia.positions[i].z == doctest::Approx(ib.positions[i].z));
  }
}

TEST_CASE("UNIT runtime: viewpoint far distance produces equivalent culling hints") {
  for (const std::string kind : {"Viewpoint", "OrthoViewpoint"}) {
    CAPTURE(kind);
    auto make = [&](bool altered) {
      const std::string head = altered ?
        "<head><unit category='length' name='cm' conversionFactor='0.01'/></head>" : "";
      return RuntimeSession::create(x3d::codec::parseDocument(
        "<X3D profile='Full' version='4.0'>" + head + "<Scene><" + kind +
        " DEF='V' position='0 0 0' nearDistance='" + (altered ? "100" : "1") +
        "' farDistance='" + (altered ? "500" : "5") + "'/>"
        "<Transform translation='0 0 " + (altered ? std::string("-400") : "-4") +
        "'><Shape><Box/></Shape></Transform><Transform translation='0 0 " +
        (altered ? std::string("-600") : "-6") + "'><Shape><Box/></Shape></Transform></Scene></X3D>"));
    };
    auto initial = make(false), altered = make(true);
    const auto a = initial->fullSnapshot(), b = altered->fullSnapshot();
    REQUIRE(a.added.size() == 2); REQUIRE(b.added.size() == 2);
    CHECK_FALSE(initial->extractor().item(a.added[0]).beyondVisibilityLimit);
    CHECK(initial->extractor().item(a.added[1]).beyondVisibilityLimit);
    for (std::size_t i = 0; i < a.added.size(); ++i)
      CHECK(initial->extractor().item(a.added[i]).beyondVisibilityLimit ==
            altered->extractor().item(b.added[i]).beyondVisibilityLimit);
    const auto viewpoint = altered->scene().resolve("V");
    CHECK(std::any_cast<float>(findField(*viewpoint, "nearDistance")->get(*viewpoint)) == 1);
    CHECK(std::any_cast<float>(findField(*viewpoint, "farDistance")->get(*viewpoint)) == 5);
  }
}

TEST_CASE("UNIT runtime: TextureTransform angle converts but UV dimensions do not") {
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><head>
<unit category='angle' name='degree' conversionFactor='0.017453292519943295'/>
<unit category='length' name='centimetre' conversionFactor='0.01'/>
</head><Scene><Shape><Appearance><TextureTransform DEF='UV' rotation='90'
center='0.25 0.5' translation='0.4 0.7' scale='2 3'/></Appearance><Box/></Shape>
</Scene></X3D>)");
  auto uv = std::dynamic_pointer_cast<x3d::nodes::TextureTransform>(doc.scene.resolve("UV"));
  REQUIRE(uv);
  CHECK(uv->getRotation() == doctest::Approx(90)); // authoring representation
  auto session = RuntimeSession::create(std::move(doc));
  CHECK(uv->getRotation() == doctest::Approx(1.5707963267948966));
  CHECK(uv->getCenter().x == doctest::Approx(0.25));
  CHECK(uv->getCenter().y == doctest::Approx(0.5));
  CHECK(uv->getTranslation().x == doctest::Approx(0.4));
  CHECK(uv->getTranslation().y == doctest::Approx(0.7));
  CHECK(uv->getScale().x == doctest::Approx(2));
  CHECK(uv->getScale().y == doctest::Approx(3));
  // A repeated runtime entry must not apply the authored factor a second time.
  normalizeRuntimeUnits(session->scene());
  CHECK(uv->getRotation() == doctest::Approx(1.5707963267948966));
  uv->setRotation(0.3f); // host writes already use radians
  normalizeRuntimeUnits(session->scene());
  CHECK(uv->getRotation() == doctest::Approx(0.3));
}

TEST_CASE("UNIT runtime: TextureTransform IS angle and built-in default") {
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><head>
<unit category='angle' name='degree' conversionFactor='0.017453292519943295'/>
</head><Scene><ProtoDeclare name='UVRotation'><ProtoInterface>
<field name='angle' type='SFFloat' accessType='inputOutput' value='45'/>
</ProtoInterface><ProtoBody><TextureTransform><IS>
<connect nodeField='rotation' protoField='angle'/>
</IS></TextureTransform></ProtoBody></ProtoDeclare>
<Shape><Appearance><ProtoInstance name='UVRotation' DEF='UV' containerField='textureTransform'>
<fieldValue name='angle' value='90'/></ProtoInstance></Appearance><Box/></Shape>
<Shape><Appearance><TextureTransform DEF='Default'/></Appearance><Box/></Shape>
</Scene></X3D>)");
  auto uv = std::dynamic_pointer_cast<x3d::nodes::TextureTransform>(doc.scene.resolve("UV"));
  auto defaults = std::dynamic_pointer_cast<x3d::nodes::TextureTransform>(doc.scene.resolve("Default"));
  REQUIRE(uv); REQUIRE(defaults);
  auto session = RuntimeSession::create(std::move(doc));
  CHECK(uv->getRotation() == doctest::Approx(1.5707963267948966));
  CHECK(defaults->getRotation() == doctest::Approx(0));
}
