#include "doctest/doctest.h"
#include "DynamicField.hpp"
#include "InlineExpand.hpp"
#include "InlineRuntimeSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DProtoClone.hpp"
#include "X3DProtoExpand.hpp"
#include "XmlReader.hpp"
#include "ext/ExtResolver.hpp"
#include <any>
#include <memory>
#include <new>
using namespace x3d::runtime;
using namespace x3d::core;

namespace {
X3DDocument scriptOwnerDocument(float value) {
  return x3d::codec::XmlReader{}.readDocument(
      "<X3D><Scene><Script DEF='S'><field name='amount' type='SFFloat' "
      "accessType='inputOutput' value='" + std::to_string(value) +
      "'/></Script></Scene></X3D>");
}
}

TEST_CASE("author owners: fresh scenes are independent and shallow copies alias") {
  auto a = scriptOwnerDocument(1.f), b = scriptOwnerDocument(2.f);
  Scene alias = a.scene;
  auto sa = a.scene.resolve("S"), sb = b.scene.resolve("S");
  REQUIRE(sa); REQUIRE(sb); REQUIRE(sa != sb);
  CHECK(a.scene.authorFields != b.scene.authorFields);
  CHECK(alias.authorFields == a.scene.authorFields);
  a.scene.authorFields->setValue(*sa, "amount", 5.f);
  CHECK(std::any_cast<float>(alias.authorFields->getValue(*sa, "amount")) == 5.f);
  CHECK(std::any_cast<float>(b.scene.authorFields->getValue(*sb, "amount")) == 2.f);
  X3DExecutionContext ctx(a.scene.authorFields);
  ctx.buildSceneGraph(a.scene);
  ctx.detachNodes({sa.get()});
  CHECK(b.scene.authorFields->hasAuthorFields(*sb));
  a.scene.authorFields->clear();
  CHECK(b.scene.authorFields->hasAuthorFields(*sb));
}

TEST_CASE("author owners: Inline detach drops parent view and preserves retained child") {
  auto childDoc = scriptOwnerDocument(3.f);
  auto child = std::make_shared<Scene>(std::move(childDoc.scene));
  const auto script = child->resolve("S"); REQUIRE(script);
  auto parent = x3d::codec::XmlReader{}.readDocument(
      "<X3D><Scene><Inline DEF='I' url='\"child.x3d\"'/></Scene></X3D>");
  std::vector<InlineWarning> warnings;
  expandInlines(parent.scene, [child](const auto &, const auto &) { return child; }, "", warnings);
  REQUIRE(warnings.empty());
  REQUIRE(parent.scene.expandedInlineScenes.size() == 1);
  parent.scene.authorFields->setValue(*script, "amount", 4.f);
  CHECK(std::any_cast<float>(child->authorFields->getValue(*script, "amount")) == 4.f);
  auto field = parent.scene.authorFields->authorFields(*script).front();
  X3DExecutionContext ctx(parent.scene.authorFields);
  ctx.buildSceneGraph(parent.scene);
  ctx.detachNodes({script.get()});
  CHECK_FALSE(parent.scene.authorFields->hasAuthorFields(*script));
  REQUIRE(child->authorFields->hasAuthorFields(*script));
  CHECK(std::any_cast<float>(field.get(*script)) == 4.f);
  parent.scene.authorFields->importFrom(*child->authorFields);
  ctx.attachNewSubtree(script.get());
  CHECK(std::any_cast<float>(ctx.authorFields().getValue(*script, "amount")) == 4.f);
}

TEST_CASE("author owners: Script fields survive independent PROTO instantiation") {
  auto doc = x3d::codec::XmlReader{}.readDocument(R"(<X3D><Scene>
    <ProtoDeclare name='Counter'><ProtoBody>
      <Script DEF='S'><field name='amount' type='SFFloat' accessType='inputOutput' value='7'/></Script>
    </ProtoBody></ProtoDeclare>
    <ProtoInstance name='Counter' DEF='A'/><ProtoInstance name='Counter' DEF='B'/>
  </Scene></X3D>)");
  const auto decl = doc.scene.findProto("Counter"); REQUIRE(decl);
  REQUIRE(decl->body.nodes.size() == 1);
  REQUIRE(decl->authorFields->hasAuthorFields(*decl->body.nodes.front()));
  expandScene(doc.scene, x3d::codec::noopProtoResolver, "", doc.protoWarnings);
  REQUIRE(doc.protoWarnings.empty());
  auto a = doc.scene.resolve("A"), b = doc.scene.resolve("B");
  REQUIRE(a); REQUIRE(b); CHECK(a != b);
  REQUIRE(doc.scene.authorFields->hasAuthorFields(*a));
  REQUIRE(doc.scene.authorFields->hasAuthorFields(*b));
  doc.scene.authorFields->setValue(*a, "amount", 8.f);
  CHECK(std::any_cast<float>(doc.scene.authorFields->getValue(*b, "amount")) == 7.f);
  CHECK(std::any_cast<float>(decl->authorFields->getValue(*decl->body.nodes.front(), "amount")) == 7.f);
}

TEST_CASE("author owners: copied thunk rejects expired and wrong node identities") {
  DynamicFieldStore fields, imported;
  alignas(x3d::nodes::Script) unsigned char storage[sizeof(x3d::nodes::Script)];
  auto construct = [&]() {
    return std::shared_ptr<x3d::nodes::Script>(new (storage) x3d::nodes::Script,
        [](x3d::nodes::Script *p) { p->~Script(); });
  };
  auto old = construct();
  fields.addAuthorField(old, {"amount", X3DFieldType::SFFloat, AccessType::InputOutput, 9.f});
  imported.importFrom(fields);
  auto info = fields.authorFields(*old).front();
  x3d::nodes::Script unrelated;
  CHECK_FALSE(info.get(unrelated).has_value());
  info.set(unrelated, 1.f);
  CHECK(std::any_cast<float>(info.get(*old)) == 9.f);
  const auto address = old.get(); old.reset();
  auto replacement = construct(); REQUIRE(replacement.get() == address);
  CHECK_FALSE(info.get(*replacement).has_value());
  info.set(*replacement, 2.f);
  CHECK_FALSE(fields.hasAuthorFields(*replacement));
  CHECK_FALSE(imported.hasAuthorFields(*replacement));
  fields.addAuthorField(replacement, {"amount", X3DFieldType::SFFloat, AccessType::InputOutput, 3.f});
  imported.importFrom(fields);
  CHECK(std::any_cast<float>(imported.getValue(*replacement, "amount")) == 3.f);
  CHECK_FALSE(info.get(*replacement).has_value());
}

TEST_CASE("author owners: conflicting imports reject before changing destination") {
  auto script = std::make_shared<x3d::nodes::Script>();
  DynamicFieldStore a, b;
  a.addAuthorField(script, {"amount", X3DFieldType::SFFloat, AccessType::InputOutput, 1.f});
  b.addAuthorField(script, {"amount", X3DFieldType::SFFloat, AccessType::InputOutput, 2.f});
  CHECK_THROWS_AS(a.importFrom(b), std::logic_error);
  CHECK(std::any_cast<float>(a.getValue(*script, "amount")) == 1.f);
}

TEST_CASE("author owners: explicit clone keeps node-valued defaults inside clone graph") {
  auto script = std::make_shared<x3d::nodes::Script>();
  auto child = x3d::nodes::X3DNodeFactory::create("Group");
  DynamicFieldStore source, destination;
  source.addAuthorField(script, {"one", X3DFieldType::SFNode, AccessType::InitializeOnly, SFNode{child}});
  source.addAuthorField(script, {"many", X3DFieldType::MFNode, AccessType::InputOutput, MFNode{child, child}});
  auto copy = deepClone(script, CloneContext{&source, &destination, {}}); REQUIRE(copy);
  const auto one = std::any_cast<SFNode>(destination.getValue(*copy, "one"));
  const auto many = std::any_cast<MFNode>(destination.getValue(*copy, "many"));
  REQUIRE(one); REQUIRE(many.size() == 2);
  CHECK(one != child); CHECK(many[0] == one); CHECK(many[1] == one);
  source.clear();
  CHECK(destination.hasAuthorFields(*copy));
  CHECK_THROWS_AS(deepClone(script, CloneContext{nullptr, &destination, {}}), std::invalid_argument);
}

TEST_CASE("author owners: declaration retains fields after source document destruction") {
  std::shared_ptr<ProtoDeclaration> decl;
  {
    auto source = x3d::codec::XmlReader{}.readDocument(R"(<X3D><Scene>
      <ProtoDeclare name='Counter'><ProtoBody><Script><field name='amount'
        type='SFFloat' accessType='inputOutput' value='6'/></Script></ProtoBody></ProtoDeclare>
    </Scene></X3D>)");
    decl = source.scene.findProto("Counter");
  }
  REQUIRE(decl);
  Scene target; ProtoInstance instance; instance.name = "Counter"; instance.declaration = decl;
  ExpandGuard guard; std::vector<ProtoWarning> warnings;
  auto copy = expandInstance(instance, target, x3d::codec::noopProtoResolver, "", guard, warnings);
  REQUIRE(copy); REQUIRE(warnings.empty());
  CHECK(std::any_cast<float>(target.authorFields->getValue(*copy, "amount")) == 6.f);
}

TEST_CASE("author owners: context binding is explicit or one-time before first use") {
  Scene a, b; X3DExecutionContext ctx;
  ctx.buildSceneGraph(a);
  CHECK(&ctx.authorFields() == a.authorFields.get());
  CHECK_THROWS_AS(ctx.buildSceneGraph(b), std::logic_error);
  X3DExecutionContext scratch;
  (void)scratch.authorFields();
  CHECK_THROWS_AS(scratch.buildSceneGraph(a), std::logic_error);
  CHECK_THROWS_AS(X3DExecutionContext(std::shared_ptr<DynamicFieldStore>{}), std::invalid_argument);
}

TEST_CASE("author owners: extension resolver never configures unrelated cloning") {
  auto external = std::make_shared<x3d::runtime::ext::ExternalGeometry>();
  REQUIRE_FALSE(deepClone(external));
  auto first = x3d::runtime::ext::install(x3d::codec::noopProtoResolver);
  auto second = x3d::runtime::ext::install(x3d::codec::noopProtoResolver);
  const auto declaration = first({x3d::runtime::ext::kExternalGeometryUrn}, ""); REQUIRE(declaration);
  CHECK(declaration != second({x3d::runtime::ext::kExternalGeometryUrn}, ""));
  ProtoInstance instance; instance.name = "ExternalGeometry"; instance.declaration = declaration;
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> warnings;
  CHECK(expandInstance(instance, scene, x3d::codec::noopProtoResolver, "", guard, warnings));
  CHECK(warnings.empty());
  CHECK_FALSE(deepClone(external));
}

TEST_CASE("author owners: GeoLOD authored roots and retained tiles preserve fields on redisplay") {
  auto document = scriptOwnerDocument(11.f);
  auto rootScript = document.scene.resolve("S"); REQUIRE(rootScript);
  auto tileDoc = scriptOwnerDocument(22.f);
  auto tile = std::make_shared<Scene>(std::move(tileDoc.scene));
  auto tileScript = tile->resolve("S"); REQUIRE(tileScript);
  auto lod = std::make_shared<x3d::nodes::GeoLOD>();
  lod->setGeoSystemUnchecked({"GC"});
  lod->setCenterUnchecked({10, 0, 0});
  lod->setRangeUnchecked(2.f);
  lod->setRootNodeUnchecked({rootScript});
  lod->setChild1UrlUnchecked({"tile.x3d"});
  document.scene.rootNodes = {lod};
  X3DExecutionContext ctx(document.scene.authorFields);
  ctx.buildSceneGraph(document.scene);
  int loads = 0;
  auto system = std::make_shared<InlineRuntimeSystem>(document.scene,
      [&](const auto &, const auto &) { ++loads; return tile; }, "");
  system->attach(lod.get(), ctx); ctx.addSystem(system);
  ctx.tick(0.0);
  REQUIRE(lod->getLevel_changed() == 0);
  lod->setCenterUnchecked({0, 0, 0}); ctx.tick(1.0);
  REQUIRE(lod->getLevel_changed() == 1);
  ctx.authorFields().setValue(*tileScript, "amount", 23.f);
  lod->setCenterUnchecked({10, 0, 0}); ctx.tick(2.0);
  REQUIRE(lod->getLevel_changed() == 0);
  REQUIRE(ctx.authorFields().hasAuthorFields(*rootScript));
  CHECK(std::any_cast<float>(ctx.authorFields().getValue(*rootScript, "amount")) == 11.f);
  lod->setCenterUnchecked({0, 0, 0}); ctx.tick(3.0);
  REQUIRE(lod->getLevel_changed() == 1);
  CHECK(std::any_cast<float>(ctx.authorFields().getValue(*tileScript, "amount")) == 23.f);
  CHECK(loads == 2); // child tiles unload while far; the resolver retains its Scene

}

TEST_CASE("author owners: imported write diagnostics belong to the requesting owner") {
  auto script = std::make_shared<x3d::nodes::Script>();
  DynamicFieldStore source, destination;
  source.addAuthorField(script, {"amount", X3DFieldType::SFFloat,
                                 AccessType::InputOutput, 1.f});
  destination.importFrom(source);
  destination.authorFields(*script).front().set(*script, std::string("bad"));
  destination.setValue(*script, "amount", std::string("bad"));
  CHECK(destination.typeMismatchDrops() == 2);
  CHECK(source.typeMismatchDrops() == 0);
  source.authorFields(*script).front().set(*script, std::string("bad"));
  CHECK(source.typeMismatchDrops() == 1);
  CHECK(destination.typeMismatchDrops() == 2);
  CHECK(std::any_cast<float>(source.getValue(*script, "amount")) == 1.f);
}
