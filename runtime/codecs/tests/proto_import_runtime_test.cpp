#include "doctest/doctest.h"
#include "X3DParse.hpp"
#include "InlineRuntimeSystem.hpp"
#include "X3DSceneBridge.hpp"
#include "x3d/nodes/Transform.hpp"

using namespace x3d::runtime;
using namespace x3d::core;
namespace {
InlineResolver loader() {
  return [](const auto &, const auto &) {
    auto doc = x3d::codec::parseDocument(R"(<X3D><Scene>
      <Transform DEF='Exported'/><EXPORT localDEF='Exported' AS='Public'/>
    </Scene></X3D>)");
    return std::make_shared<Scene>(std::move(doc.scene));
  };
}
X3DDocument importedDocument(bool load = true) {
  return x3d::codec::parseDocument(std::string(R"(<X3D><Scene>
    <ProtoDeclare name='P'><ProtoBody><Group>
      <Inline DEF='I' url='"child.x3d"' load=')") + (load ? "true" : "false") + R"('/>
      <Transform DEF='Local'/></Group>
      <IMPORT inlineDEF='I' importedDEF='Public' AS='Imported'/>
      <ROUTE fromNode='Imported' fromField='translation_changed' toNode='Local' toField='set_translation'/>
      <ROUTE fromNode='Local' fromField='scale_changed' toNode='Imported' toField='set_scale'/>
    </ProtoBody></ProtoDeclare>
    <ProtoInstance name='P' DEF='A'/><ProtoInstance name='P' DEF='B'/>
    </Scene></X3D>)", x3d::codec::Encoding::Unknown, "",
    x3d::codec::localFileProtoResolver, loader());
}
}

TEST_CASE("ProtoBody IMPORT routes isolate two instances and preserve event direction") {
  auto doc = importedDocument();
  REQUIRE(doc.scene.deferredProtoRoutes.size() == 4);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 4);
  CHECK_FALSE(doc.scene.resolve("Imported"));
  CHECK_FALSE(doc.scene.resolve("I"));
  CHECK_FALSE(doc.scene.resolve("Local"));
  const auto first = doc.scene.resolvedInlineRoutes[0];
  const auto second = doc.scene.resolvedInlineRoutes[2];
  REQUIRE(first.from != second.from);
  REQUIRE(first.to != second.to);
  wireInlineImports(doc.scene);
  CHECK(doc.scene.resolvedInlineRoutes.size() == 4);
  X3DExecutionContext ctx(doc.scene.authorFields);
  ctx.buildSceneGraph(doc.scene);
  REQUIRE(ctx.buildFrom(doc.scene).routesAdded == 4);
  ctx.postEvent(first.from.get(), "translation", SFVec3f{3, 4, 5});
  ctx.tick(0);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*first.to).getTranslation().x == 3);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*second.to).getTranslation().x == 0);
  ctx.postEvent(first.to.get(), "scale", SFVec3f{2, 2, 2});
  ctx.tick(1);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*first.from).getScale().x == 2);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*second.from).getScale().x == 1);
}

TEST_CASE("ProtoBody IMPORT binds deferred loads and rebinds one instance on reload") {
  auto doc = importedDocument(false);
  REQUIRE(doc.scene.resolvedInlineRoutes.empty());
  auto firstInline = doc.scene.deferredProtoRoutes[0].source.inlineNode.lock();
  auto secondInline = doc.scene.deferredProtoRoutes[2].source.inlineNode.lock();
  REQUIRE(firstInline); REQUIRE(secondInline);
  X3DExecutionContext ctx(doc.scene.authorFields);
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  auto runtime = std::make_shared<InlineRuntimeSystem>(doc.scene, loader(), "");
  runtime->attach(firstInline.get(), ctx);
  runtime->attach(secondInline.get(), ctx);
  ctx.addSystem(runtime);
  ctx.postEvent(firstInline.get(), "load", true);
  ctx.postEvent(secondInline.get(), "load", true);
  ctx.tick(0);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 4);
  auto old = doc.scene.resolvedInlineRoutes[0];
  auto unaffected = doc.scene.resolvedInlineRoutes[2];
  ctx.postEvent(firstInline.get(), "load", false);
  ctx.tick(1);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 2);
  CHECK(doc.scene.resolvedInlineRoutes[0].from == unaffected.from);
  ctx.postEvent(firstInline.get(), "load", true);
  ctx.tick(2);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 4);
  const auto current = doc.scene.resolvedInlineRoutes[2];
  CHECK(current.from != old.from);
  CHECK(current.to == old.to);
  ctx.postEvent(current.from.get(), "translation", SFVec3f{9, 0, 0});
  ctx.tick(3);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*current.to).getTranslation().x == 9);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*unaffected.to).getTranslation().x == 0);
  // Old endpoints may be retained by clients, but their detached routes are gone.
  ctx.postEvent(old.from.get(), "translation", SFVec3f{1, 0, 0});
  ctx.tick(4);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*current.to).getTranslation().x == 9);
}

TEST_CASE("ProtoBody IMPORT resolves exported interfaces and auxiliary Inline roots") {
  InlineResolver resolver = [](const auto &, const auto &) {
    auto child = x3d::codec::parseDocument(R"(<X3D><Scene>
      <ProtoDeclare name='Source'><ProtoInterface>
        <field name='position' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
      </ProtoInterface><ProtoBody><Transform><IS>
        <connect nodeField='translation' protoField='position'/>
      </IS></Transform></ProtoBody></ProtoDeclare>
      <ProtoInstance name='Source' DEF='Exported'/>
      <EXPORT localDEF='Exported' AS='Public'/>
      <Transform DEF='Private'/>
    </Scene></X3D>)");
    return std::make_shared<Scene>(std::move(child.scene));
  };
  auto doc = x3d::codec::parseDocument(R"(<X3D><Scene>
    <ProtoDeclare name='P'><ProtoBody><Transform DEF='Local'/>
      <Inline DEF='I' url='"child"'/>
      <IMPORT inlineDEF='I' importedDEF='Public' AS='Imported'/>
      <IMPORT inlineDEF='I' importedDEF='Private' AS='NotExported'/>
      <ROUTE fromNode='Imported' fromField='position_changed' toNode='Local' toField='set_translation'/>
      <ROUTE fromNode='Imported' fromField='position_changed' toNode='Local' toField='set_rotation'/>
      <ROUTE fromNode='NotExported' fromField='translation_changed' toNode='Local' toField='set_scale'/>
    </ProtoBody></ProtoDeclare><ProtoInstance name='P' DEF='Instance'/>
  </Scene></X3D>)", x3d::codec::Encoding::Unknown, "",
      x3d::codec::localFileProtoResolver, resolver);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 1);
  const auto route = doc.scene.resolvedInlineRoutes.front();
  CHECK(route.fromField == "translation");
  CHECK(route.toField == "translation");
  CHECK(route.to == doc.scene.resolve("Instance"));
  X3DExecutionContext ctx(doc.scene.authorFields);
  ctx.buildSceneGraph(doc.scene);
  REQUIRE(ctx.buildFrom(doc.scene).routesAdded == 1);
  ctx.postEvent(route.from.get(), "translation", SFVec3f{7, 0, 0});
  ctx.tick(0);
  CHECK(dynamic_cast<x3d::nodes::Transform &>(*route.to).getTranslation().x == 7);
  auto inl = doc.scene.deferredProtoRoutes.front().source.inlineNode.lock();
  REQUIRE(inl);
  auto runtime = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  runtime->attach(inl.get(), ctx);
  ctx.addSystem(runtime);
  ctx.postEvent(inl.get(), "load", false);
  ctx.tick(1);
  REQUIRE(doc.scene.protoPeerNodes.size() == 1);
  CHECK(doc.scene.protoPeerNodes.front() == inl);
  CHECK(doc.scene.resolvedInlineRoutes.empty());
  ctx.postEvent(inl.get(), "load", true);
  ctx.tick(2);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 1);
  CHECK(doc.scene.resolvedInlineRoutes.front().from != route.from);
}

TEST_CASE("ProtoBody IMPORT bindings retire with an enclosing Inline scope") {
  InlineResolver resolver = [](const auto &, const auto &) {
    auto child = importedDocument();
    return std::make_shared<Scene>(std::move(child.scene));
  };
  auto doc = x3d::codec::parseDocument(
      "<X3D><Scene><Inline DEF='Outer' url='\"outer\"'/></Scene></X3D>",
      x3d::codec::Encoding::Unknown, "", x3d::codec::localFileProtoResolver, resolver);
  REQUIRE(doc.scene.deferredProtoRoutes.size() == 4);
  REQUIRE(doc.scene.resolvedInlineRoutes.size() == 4);
  auto outer = doc.scene.resolve("Outer");
  REQUIRE(outer);
  X3DExecutionContext ctx(doc.scene.authorFields);
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  auto runtime = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  runtime->attach(outer.get(), ctx);
  ctx.addSystem(runtime);
  ctx.postEvent(outer.get(), "load", false);
  ctx.tick(0);
  CHECK(doc.scene.deferredProtoRoutes.empty());
  CHECK(doc.scene.resolvedInlineRoutes.empty());
  ctx.postEvent(outer.get(), "load", true);
  ctx.tick(1);
  CHECK(doc.scene.deferredProtoRoutes.size() == 4);
  CHECK(doc.scene.resolvedInlineRoutes.size() == 4);
}

TEST_CASE("Enclosing Inline unload retires auxiliary ProtoBody Inline loads") {
  InlineResolver resolver = [](const auto &, const auto &) {
    auto child = x3d::codec::parseDocument(R"(<X3D><Scene>
      <ProtoDeclare name='P'><ProtoBody><Transform DEF='Local'/>
        <Inline DEF='I' url='"leaf"'/>
        <IMPORT inlineDEF='I' importedDEF='Public' AS='Imported'/>
        <ROUTE fromNode='Imported' fromField='translation_changed'
               toNode='Local' toField='set_translation'/>
      </ProtoBody></ProtoDeclare><ProtoInstance name='P'/>
    </Scene></X3D>)", x3d::codec::Encoding::Unknown, "",
      x3d::codec::localFileProtoResolver, loader());
    return std::make_shared<Scene>(std::move(child.scene));
  };
  auto doc = x3d::codec::parseDocument(
      "<X3D><Scene><Inline DEF='Outer' url='\"outer\"'/></Scene></X3D>",
      x3d::codec::Encoding::Unknown, "", x3d::codec::localFileProtoResolver, resolver);
  REQUIRE(doc.scene.expandedInlineScenes.size() == 2);
  REQUIRE(doc.scene.protoPeerNodes.size() == 1);
  std::weak_ptr<Scene> retiredChild;
  for (const auto &[node, child] : doc.scene.expandedInlineScenes)
    if (node != doc.scene.resolve("Outer").get()) retiredChild = child;
  auto outer = doc.scene.resolve("Outer");
  X3DExecutionContext ctx(doc.scene.authorFields);
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  auto runtime = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  runtime->attach(outer.get(), ctx);
  ctx.addSystem(runtime);
  for (int cycle = 0; cycle < 2; ++cycle) {
    ctx.postEvent(outer.get(), "load", false);
    ctx.tick(cycle * 3);
    CHECK(doc.scene.expandedInlineScenes.empty());
    CHECK(doc.scene.expandedInlines.empty());
    CHECK(doc.scene.protoPeerNodes.empty());
    CHECK(doc.scene.deferredProtoRoutes.empty());
    CHECK(doc.scene.resolvedInlineRoutes.empty());
    CHECK(retiredChild.expired());
    // A later tick must not revisit restored, now detached Inline originals.
    ctx.tick(cycle * 3 + 1);
    CHECK(doc.scene.expandedInlineScenes.empty());
    ctx.postEvent(outer.get(), "load", true);
    ctx.tick(cycle * 3 + 2);
    CHECK(doc.scene.expandedInlineScenes.size() == 2);
    CHECK(doc.scene.expandedInlines.size() == 2);
    CHECK(doc.scene.protoPeerNodes.size() == 1);
    CHECK(doc.scene.deferredProtoRoutes.size() == 1);
    CHECK(doc.scene.resolvedInlineRoutes.size() == 1);
  }
}
