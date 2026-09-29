// runtime/parse/tests/proto_expand_test.cpp
#include "X3DDocument.hpp"
#include "X3DProto.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DParse.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/core/X3DReflection.hpp"
#include <any>
#include "doctest/doctest.h"
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::codec::noopProtoResolver;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

// Local PROTO "Param": interface size (SFVec3f, initializeOnly,
// default 2,2,2), body = [ Box ] with size IS size.
// Instance overrides size = 5,5,5.
//
// This verifies initializeOnly value-forwarding via the reflection setter
// (setSizeUnchecked). Box.size is initializeOnly and has a set thunk.
static void localValueForwardTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Param";
  ProtoField pf;
  pf.name = "size"; pf.type = X3DFieldType::SFVec3f;
  pf.access = AccessType::InitializeOnly;
  pf.value = std::any(SFVec3f{2.f, 2.f, 2.f});
  decl->interface.push_back(pf);

  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);
  decl->body.isConnections.push_back({box, "size", "size"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Param";
  inst.declaration = decl;
  x3d::runtime::ProtoFieldValue fv; fv.name = "size";
  fv.value = std::any(SFVec3f{5.f, 5.f, 5.f});
  inst.fieldValues.push_back(fv);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((primary && primary->nodeTypeName() == "Box"));
  auto sz = std::any_cast<SFVec3f>(fieldByName(*primary, "size")->get(*primary));
  CHECK((sz.x == 5.f && sz.y == 5.f && sz.z == 5.f));
  CHECK(scene.authoredScalarFields.contains(primary, "size"));
  CHECK((warnings.empty()));
}

// A mistyped override value (e.g. an EXTERN instance whose <fieldValue> the
// reader could only type as the SFString fallback) must NOT throw out of
// expansion: the typed setter's bad_any_cast is caught and turned into an
// InterfaceMismatch warning, and expansion still yields the primary node.
static void mistypedValueLenientTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Param";
  ProtoField pf;
  pf.name = "size"; pf.type = X3DFieldType::SFVec3f;
  pf.access = AccessType::InitializeOnly;
  pf.value = std::any(SFVec3f{2.f, 2.f, 2.f});
  decl->interface.push_back(pf);

  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);
  decl->body.isConnections.push_back({box, "size", "size"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Param";
  inst.declaration = decl;
  x3d::runtime::ProtoFieldValue fv; fv.name = "size";
  fv.value = std::any(std::string("5 5 5")); // wrong type: string, not SFVec3f
  inst.fieldValues.push_back(fv);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((primary && primary->nodeTypeName() == "Box")); // expansion still succeeds
  CHECK((!warnings.empty()));
  CHECK((warnings.back().kind == ProtoWarning::Kind::InterfaceMismatch));
  // The mistyped set was skipped, so the field keeps its constructed default.
  auto sz = std::any_cast<SFVec3f>(fieldByName(*primary, "size")->get(*primary));
  CHECK((sz.x == 2.f && sz.y == 2.f && sz.z == 2.f));
  CHECK(!scene.authoredScalarFields.contains(primary, "size"));
}

// An SFString interface field IS-mapped to an enum-typed body field (X3D has no
// enum field type; bounded SimpleTypes become C++ enums in the bindings) must
// coerce the string override through the enum-string setter — exactly as the
// normal reader does for `<EspduTransform networkMode='networkReader'/>` — so
// the value actually lands instead of throwing bad_any_cast.
static void enumValueForwardTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Net";
  ProtoField pf;
  pf.name = "mode"; pf.type = X3DFieldType::SFString; // interface: SFString
  pf.access = AccessType::InputOutput;
  decl->interface.push_back(pf);

  auto et = createX3DNode("EspduTransform"); // networkMode is NetworkModeChoices
  decl->body.nodes.push_back(et);
  decl->body.isConnections.push_back({et, "networkMode", "mode"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Net";
  inst.declaration = decl;
  x3d::runtime::ProtoFieldValue fv; fv.name = "mode";
  fv.value = std::any(std::string("networkReader")); // string override
  inst.fieldValues.push_back(fv);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((primary && primary->nodeTypeName() == "EspduTransform"));
  CHECK((warnings.empty())); // coerced, not an InterfaceMismatch
  const FieldInfo *f = fieldByName(*primary, "networkMode");
  CHECK((f && f->isEnum()));
  CHECK((f->getEnumString(*primary) == "networkReader")); // value applied
  CHECK(scene.authoredScalarFields.contains(primary, "networkMode"));
}

// Verify that the default value is used when the instance provides no override.
static void localDefaultForwardTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Defaulted";
  ProtoField pf;
  pf.name = "diffuseColor"; pf.type = X3DFieldType::SFColor;
  pf.access = AccessType::InputOutput;
  pf.value = std::any(SFColor{0.5f, 0.5f, 0.5f});
  decl->interface.push_back(pf);

  auto mat = createX3DNode("Material");
  decl->body.nodes.push_back(mat);
  decl->body.isConnections.push_back({mat, "diffuseColor", "diffuseColor"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Defaulted";
  inst.declaration = decl;
  // No fieldValues — use the proto-field default.

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((primary && primary->nodeTypeName() == "Material"));
  auto col = std::any_cast<SFColor>(fieldByName(*primary, "diffuseColor")->get(*primary));
  CHECK((col.r == 0.5f && col.g == 0.5f && col.b == 0.5f));  // proto default used
  CHECK(scene.authoredScalarFields.contains(primary, "diffuseColor"));
  CHECK((warnings.empty()));
}

// Verify that a missing declaration yields a MissingDeclaration warning + null.
static void missingDeclarationTest() {
  x3d::runtime::ProtoInstance inst;
  inst.name = "Ghost";
  // inst.declaration left null

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((!primary));
  CHECK((warnings.size() == 1));
  CHECK((warnings[0].kind == ProtoWarning::Kind::MissingDeclaration));
  CHECK((warnings[0].instanceName == "Ghost"));
}

static void routesAndRedirectsTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Anim";
  // interface: value_changed (SFVec3f, outputOnly)
  ProtoField out; out.name = "value_changed";
  out.type = X3DFieldType::SFVec3f; out.access = AccessType::OutputOnly;
  decl->interface.push_back(out);

  auto ts = createX3DNode("TimeSensor");           ts->setDEF("TS");
  auto pi = createX3DNode("PositionInterpolator"); pi->setDEF("PI");
  decl->body.nodes.push_back(ts);
  decl->body.nodes.push_back(pi);
  // internal ROUTE TS.fraction_changed -> PI.set_fraction
  decl->body.routes.push_back(Route{"TS", "fraction_changed", "PI", "set_fraction"});
  // PI.value_changed IS value_changed
  decl->body.isConnections.push_back({pi, "value_changed", "value_changed"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Anim"; inst.declaration = decl; inst.DEF = "A";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  CHECK((primary != nullptr));

  // body route pre-resolved to concrete cloned endpoints
  CHECK((scene.resolvedProtoRoutes.size() == 1));
  const auto &r = scene.resolvedProtoRoutes[0];
  CHECK((r.from && r.from->nodeTypeName() == "TimeSensor" && r.fromField == "fraction_changed"));
  CHECK((r.to && r.to->nodeTypeName() == "PositionInterpolator" && r.toField == "set_fraction"));

  // redirect: interface value_changed -> cloned PI.value_changed
  auto &byField = scene.protoRedirects[primary.get()];
  CHECK((byField.count("value_changed") == 1));
  CHECK((byField["value_changed"].size() == 1));
  CHECK((byField["value_changed"][0].targetField == "value_changed"));
  CHECK((byField["value_changed"][0].targetNode &&
         byField["value_changed"][0].targetNode->nodeTypeName() == "PositionInterpolator"));
}

static void externResolveTest() {
  auto inst = std::make_shared<x3d::runtime::ProtoInstance>();
  inst->name = "ExtBox";
  auto ext = std::make_shared<ExternProtoDeclaration>();
  ext->name = "ExtBox"; ext->url = {"shapes.x3d#ExtBox"};
  inst->externDeclaration = ext;

  auto resolver = [](const std::vector<std::string> &urls,
                     const std::string &) -> std::shared_ptr<ProtoDeclaration> {
    CHECK((!urls.empty()));
    auto d = std::make_shared<ProtoDeclaration>();
    d->name = "ExtBox";
    d->body.nodes.push_back(createX3DNode("Box"));
    return d;
  };

  // (1) EXTERN resolves via the stub resolver -> a Box.
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(*inst, scene, resolver, "", guard, warnings);
  CHECK((primary && primary->nodeTypeName() == "Box"));
  CHECK((warnings.empty()));
  CHECK((inst->declaration && inst->declaration->name == "ExtBox"));

  // (2) Unresolved (resolver returns null) -> UnresolvedExtern warning, null.
  Scene s2; ExpandGuard g2; std::vector<ProtoWarning> w2;
  auto none = expandInstance(*inst, s2, x3d::codec::noopProtoResolver, "", g2, w2);
  CHECK((!none && w2.size() == 1 &&
         w2[0].kind == ProtoWarning::Kind::UnresolvedExtern));
  CHECK((!inst->declaration));

  // (3) Depth cap -> RecursionLimit warning, null.
  Scene s3; ExpandGuard g3; g3.depth = g3.maxDepth; std::vector<ProtoWarning> w3;
  auto capped = expandInstance(*inst, s3, resolver, "", g3, w3);
  CHECK((!capped && w3.size() == 1 &&
         w3[0].kind == ProtoWarning::Kind::RecursionLimit));
}

static void expandSceneSpliceTest() {
  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "P"; decl->body.nodes.push_back(createX3DNode("Box"));
  scene.protoDeclarations.push_back(decl);

  // Root-level instance.
  x3d::runtime::ProtoInstance rootInst;
  rootInst.name = "P"; rootInst.declaration = decl; rootInst.DEF = "R";
  scene.protoInstances.push_back(rootInst);

  // Nested instance: child of a Shape in the `geometry` slot.
  auto shape = createX3DNode("Shape");
  scene.addRootNode(shape);
  x3d::runtime::ProtoInstance nested;
  nested.name = "P"; nested.declaration = decl;
  nested.parent = shape; nested.parentField = "geometry";
  scene.protoInstances.push_back(nested);

  std::vector<ProtoWarning> warnings;
  expandScene(scene, x3d::codec::noopProtoResolver, "", warnings);

  // Root instance -> a Box root node, DEF "R".
  bool foundRootBox = false;
  for (auto &n : scene.rootNodes)
    if (n && n->nodeTypeName() == "Box" && n->getDEF() == "R") foundRootBox = true;
  CHECK((foundRootBox));

  // Nested -> spliced into shape.geometry.
  const FieldInfo *geo = fieldByName(*shape, "geometry");
  auto g = std::any_cast<std::shared_ptr<X3DNode>>(geo->get(*shape));
  CHECK((g && g->nodeTypeName() == "Box"));

  // round-trip source recorded for both primaries
  CHECK((scene.expandedSources.size() == 2));
}

// A ProtoInstance nested INSIDE a proto body (e.g. inside a Transform that is a
// body node) must be expanded once per outer instantiation and spliced into the
// CLONE of its parent — not dropped, not attached to the template.
static void nestedInBodyExpandTest() {
  // Inner proto "Leaf": body = [ Box ].
  auto leaf = std::make_shared<ProtoDeclaration>();
  leaf->name = "Leaf";
  leaf->body.nodes.push_back(createX3DNode("Box"));

  // Outer proto "Wrap": body = [ Transform ]; a Leaf instance nested under that
  // Transform's children.
  auto wrap = std::make_shared<ProtoDeclaration>();
  wrap->name = "Wrap";
  auto xform = createX3DNode("Transform");
  wrap->body.nodes.push_back(xform);
  x3d::runtime::ProtoInstance nested;
  nested.name = "Leaf";
  nested.declaration = leaf;
  nested.parent = xform;          // ORIGINAL body node (a cloneMap key)
  nested.parentField = "children";
  wrap->body.nestedInstances.push_back(nested);

  Scene scene;
  scene.protoDeclarations.push_back(leaf);
  scene.protoDeclarations.push_back(wrap);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Wrap";
  inst.declaration = wrap;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  // primary is a CLONE of the Transform; its children must hold the expanded Box.
  CHECK((primary && primary->nodeTypeName() == "Transform"));
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary, "children")->get(*primary));
  CHECK((kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box"));
  CHECK((warnings.empty()));
  // The nested primary is recorded for round-trip on write.
  CHECK((scene.expandedSources.count(kids[0].get()) == 1));

  // Second instantiation must be independent: the by-value copy of the body's
  // nestedInstances means the template is not mutated, so this clone also gets
  // exactly one Box (not two, and not zero).
  x3d::runtime::ProtoInstance inst2;
  inst2.name = "Wrap";
  inst2.declaration = wrap;
  ExpandGuard guard2;
  std::vector<ProtoWarning> warnings2;
  auto primary2 = expandInstance(inst2, scene, noopProtoResolver, "", guard2, warnings2);
  CHECK((primary2 && primary2->nodeTypeName() == "Transform"));
  auto kids2 = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary2, "children")->get(*primary2));
  CHECK((kids2.size() == 1 && kids2[0] && kids2[0]->nodeTypeName() == "Box"));
  CHECK((warnings2.empty()));
}

// Nested ProtoInstance field wiring through `field IS protoField` on the nested
// instance itself must forward outer overrides/defaults into the inner instance.
static void nestedInstanceIsConnectForwardTest() {
  auto anyShape = std::make_shared<ProtoDeclaration>();
  anyShape->name = "anyShape";
  ProtoField anyPf;
  anyPf.name = "myShape";
  anyPf.type = X3DFieldType::MFNode;
  anyPf.access = AccessType::InputOutput;
  anyPf.nodeDefault.push_back(createX3DNode("Sphere"));
  anyShape->interface.push_back(anyPf);
  auto anyXf = createX3DNode("Transform");
  anyShape->body.nodes.push_back(anyXf);
  anyShape->body.isConnections.push_back({anyXf, "children", "myShape"});

  auto one = std::make_shared<ProtoDeclaration>();
  one->name = "one";
  ProtoField onePf;
  onePf.name = "myShape";
  onePf.type = X3DFieldType::MFNode;
  onePf.access = AccessType::InputOutput;
  onePf.nodeDefault.push_back(createX3DNode("Cylinder"));
  one->interface.push_back(onePf);
  auto oneXf = createX3DNode("Transform");
  one->body.nodes.push_back(oneXf);
  x3d::runtime::ProtoInstance nested;
  nested.name = "anyShape";
  nested.declaration = anyShape;
  nested.parent = oneXf;
  nested.parentField = "children";
  nested.isConnections.push_back({"myShape", "myShape"});
  one->body.nestedInstances.push_back(nested);

  Scene scene;
  scene.protoDeclarations.push_back(anyShape);
  scene.protoDeclarations.push_back(one);
  x3d::runtime::ProtoInstance inst;
  inst.name = "one";
  inst.declaration = one;
  ProtoFieldValue fv;
  fv.name = "myShape";
  auto overrideBox = createX3DNode("Box");
  fv.nodeValue.push_back(overrideBox);
  inst.fieldValues.push_back(fv);

  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);

  CHECK((primary && primary->nodeTypeName() == "Transform"));
  auto outerKids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary, "children")->get(*primary));
  CHECK((outerKids.size() == 1 && outerKids[0] &&
         outerKids[0]->nodeTypeName() == "Transform"));
  auto innerKids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*outerKids[0], "children")->get(*outerKids[0]));
  CHECK((innerKids.size() == 1 && innerKids[0] &&
         innerKids[0]->nodeTypeName() == "Box"));
  CHECK(innerKids[0] == overrideBox);
  CHECK((warnings.empty()));

  // No outer override => forwarding falls back to outer proto default (Cylinder),
  // not the inner proto's own default (Sphere).
  x3d::runtime::ProtoInstance instDefault;
  instDefault.name = "one";
  instDefault.declaration = one;
  ExpandGuard guardDefault;
  std::vector<ProtoWarning> warningsDefault;
  auto primaryDefault =
      expandInstance(instDefault, scene, noopProtoResolver, "", guardDefault,
                     warningsDefault);
  CHECK((primaryDefault && primaryDefault->nodeTypeName() == "Transform"));
  auto outerKidsDefault = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primaryDefault, "children")->get(*primaryDefault));
  CHECK((outerKidsDefault.size() == 1 && outerKidsDefault[0] &&
         outerKidsDefault[0]->nodeTypeName() == "Transform"));
  auto innerKidsDefault = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*outerKidsDefault[0], "children")->get(*outerKidsDefault[0]));
  CHECK((innerKidsDefault.size() == 1 && innerKidsDefault[0] &&
         innerKidsDefault[0]->nodeTypeName() == "Cylinder"));
  x3d::runtime::ProtoInstance instDefaultAgain = instDefault;
  auto primaryDefaultAgain =
      expandInstance(instDefaultAgain, scene, noopProtoResolver, "",
                     guardDefault, warningsDefault);
  REQUIRE(primaryDefaultAgain);
  auto outerKidsDefaultAgain = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primaryDefaultAgain, "children")->get(*primaryDefaultAgain));
  REQUIRE(outerKidsDefaultAgain.size() == 1);
  auto innerKidsDefaultAgain = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*outerKidsDefaultAgain[0], "children")->get(*outerKidsDefaultAgain[0]));
  REQUIRE(innerKidsDefaultAgain.size() == 1);
  CHECK(innerKidsDefault[0] != innerKidsDefaultAgain[0]);
  CHECK(innerKidsDefault[0] != onePf.nodeDefault[0]);
  CHECK(innerKidsDefaultAgain[0] != onePf.nodeDefault[0]);
  CHECK((warningsDefault.empty()));

  // A present empty override crosses the nested IS connection as [] and
  // suppresses both the outer Cylinder and inner Sphere defaults.
  inst.fieldValues.front().nodeValue.clear();
  auto primaryEmpty =
      expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(primaryEmpty);
  auto outerKidsEmpty = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primaryEmpty, "children")->get(*primaryEmpty));
  REQUIRE(outerKidsEmpty.size() == 1);
  auto innerKidsEmpty = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*outerKidsEmpty[0], "children")->get(*outerKidsEmpty[0]));
  CHECK(innerKidsEmpty.empty());
  CHECK(warnings.empty());
}

TEST_CASE("proto_expand_test") {
  localValueForwardTest();
  mistypedValueLenientTest();
  enumValueForwardTest();
  localDefaultForwardTest();
  missingDeclarationTest();
  routesAndRedirectsTest();
  externResolveTest();
  expandSceneSpliceTest();
  nestedInBodyExpandTest();
  nestedInstanceIsConnectForwardTest();
  return;
}

TEST_CASE("PROTO node defaults belong to each instance") {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "WithDefault";
  ProtoField field;
  field.name = "parts";
  field.type = X3DFieldType::MFNode;
  field.access = AccessType::InputOutput;
  auto defaultBox = createX3DNode("Box");
  field.nodeDefault = {defaultBox, defaultBox};
  decl->interface.push_back(field);
  auto body = createX3DNode("Transform");
  decl->body.nodes.push_back(body);
  decl->body.isConnections.push_back({body, "children", "parts"});
  decl->authoredScalarFields.record(defaultBox, "size");

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance first;
  first.name = decl->name;
  first.declaration = decl;
  ProtoInstance second = first;
  auto a = expandInstance(first, scene, noopProtoResolver, "", guard, warnings);
  auto b = expandInstance(second, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(a);
  REQUIRE(b);
  auto children = [](const std::shared_ptr<X3DNode> &node) {
    return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
        fieldByName(*node, "children")->get(*node));
  };
  const auto aa = children(a);
  const auto bb = children(b);
  REQUIRE(aa.size() == 2);
  REQUIRE(bb.size() == 2);
  CHECK(aa[0] == aa[1]);
  CHECK(bb[0] == bb[1]);
  CHECK(aa[0] != bb[0]);
  CHECK(aa[0] != defaultBox);
  CHECK(bb[0] != defaultBox);
  CHECK(scene.authoredScalarFields.contains(aa[0], "size"));
  CHECK(scene.authoredScalarFields.contains(bb[0], "size"));
  fieldByName(*aa[0], "size")->set(*aa[0], std::any(SFVec3f{7, 8, 9}));
  const auto otherSize =
      std::any_cast<SFVec3f>(fieldByName(*bb[0], "size")->get(*bb[0]));
  const auto templateSize =
      std::any_cast<SFVec3f>(fieldByName(*defaultBox, "size")->get(*defaultBox));
  CHECK(otherSize.x == 2.f);
  CHECK(templateSize.x == 2.f);
  CHECK(warnings.empty());
}

TEST_CASE("PROTO explicit node overrides retain caller identity and empty values") {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Override";
  ProtoField field;
  field.name = "parts";
  field.type = X3DFieldType::MFNode;
  field.access = AccessType::InputOutput;
  field.nodeDefault.push_back(createX3DNode("Box"));
  decl->interface.push_back(field);
  auto body = createX3DNode("Transform");
  decl->body.nodes.push_back(body);
  decl->body.isConnections.push_back({body, "children", "parts"});

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance inst;
  inst.name = decl->name;
  inst.declaration = decl;
  ProtoFieldValue value;
  value.name = "parts";
  auto callerBox = createX3DNode("Box");
  value.nodeValue.push_back(callerBox);
  inst.fieldValues.push_back(value);
  auto first = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  auto second = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(first);
  REQUIRE(second);
  auto children = [](const std::shared_ptr<X3DNode> &node) {
    return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
        fieldByName(*node, "children")->get(*node));
  };
  CHECK(children(first).at(0) == callerBox);
  CHECK(children(second).at(0) == callerBox);

  inst.fieldValues.front().nodeValue.clear(); // explicit []
  auto empty = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(empty);
  CHECK(children(empty).empty());

  auto single = std::make_shared<ProtoDeclaration>();
  single->name = "Single";
  ProtoField geometry;
  geometry.name = "geometry";
  geometry.type = X3DFieldType::SFNode;
  geometry.access = AccessType::InputOutput;
  geometry.nodeDefault.push_back(createX3DNode("Box"));
  single->interface.push_back(geometry);
  auto shape = createX3DNode("Shape");
  single->body.nodes.push_back(shape);
  single->body.isConnections.push_back({shape, "geometry", "geometry"});
  ProtoInstance nullInstance;
  nullInstance.name = single->name;
  nullInstance.declaration = single;
  ProtoFieldValue nullValue;
  nullValue.name = "geometry"; // explicit NULL
  nullInstance.fieldValues.push_back(nullValue);
  auto nullShape = expandInstance(nullInstance, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(nullShape);
  auto actual = std::any_cast<std::shared_ptr<X3DNode>>(
      fieldByName(*nullShape, "geometry")->get(*nullShape));
  CHECK_FALSE(actual);
  CHECK(warnings.empty());
}

TEST_CASE("PROTO nested literal node overrides clone with outer instance") {
  auto leaf = std::make_shared<ProtoDeclaration>();
  leaf->name = "Leaf";
  ProtoField field;
  field.name = "parts";
  field.type = X3DFieldType::MFNode;
  field.access = AccessType::InputOutput;
  leaf->interface.push_back(field);
  auto leafBody = createX3DNode("Transform");
  leaf->body.nodes.push_back(leafBody);
  leaf->body.isConnections.push_back({leafBody, "children", "parts"});

  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  auto outerBody = createX3DNode("Transform");
  outer->body.nodes.push_back(outerBody);
  ProtoInstance literal;
  literal.name = leaf->name;
  literal.declaration = leaf;
  literal.parent = outerBody;
  literal.parentField = "children";
  auto literalBox = createX3DNode("Box");
  ProtoFieldValue nestedValue;
  nestedValue.name = "parts";
  nestedValue.nodeValue = {literalBox, literalBox};
  literal.fieldValues.push_back(nestedValue);
  outer->body.nestedInstances.push_back(literal);
  outer->authoredScalarFields.record(literalBox, "size");

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance inst;
  inst.name = outer->name;
  inst.declaration = outer;
  auto a = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  auto b = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(a);
  REQUIRE(b);
  auto children = [](const std::shared_ptr<X3DNode> &node) {
    return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
        fieldByName(*node, "children")->get(*node));
  };
  auto first = children(children(a).at(0));
  auto second = children(children(b).at(0));
  REQUIRE(first.size() == 2);
  REQUIRE(second.size() == 2);
  CHECK(first[0] == first[1]);
  CHECK(second[0] == second[1]);
  CHECK(first[0] != second[0]);
  CHECK(first[0] != literalBox);
  CHECK(scene.authoredScalarFields.contains(first[0], "size"));
  CHECK(scene.authoredScalarFields.contains(second[0], "size"));
  CHECK(warnings.empty());
}

TEST_CASE("empty node overrides do not resolve unused external defaults") {
  auto remote = std::make_shared<ExternProtoDeclaration>();
  remote->name = "Remote";
  remote->url = {"missing.x3d#Remote"};
  ProtoInstance direct;
  direct.name = "Remote";
  direct.externDeclaration = remote;
  auto wrapper = std::make_shared<ProtoInstanceTemplate>(direct);

  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  ProtoField single;
  single.name = "single";
  single.type = X3DFieldType::SFNode;
  single.access = AccessType::InputOutput;
  single.nodeDefault = {wrapper};
  outer->interface.push_back(single);
  ProtoField many;
  many.name = "many";
  many.type = X3DFieldType::MFNode;
  many.access = AccessType::InputOutput;
  auto defaultGroup = createX3DNode("Group");
  many.nodeDefault = {defaultGroup};
  outer->interface.push_back(many);
  ProtoInstance contained = direct;
  contained.parent = defaultGroup;
  contained.parentField = "children";
  outer->body.nestedInstances.push_back(contained);

  auto shape = createX3DNode("Shape");
  auto group = createX3DNode("Group");
  outer->body.nodes = {shape, group};
  outer->body.isConnections.push_back({shape, "geometry", "single"});
  outer->body.isConnections.push_back({group, "children", "many"});

  ProtoInstance inst;
  inst.name = "Outer";
  inst.declaration = outer;
  ProtoFieldValue singleNull;
  singleNull.name = "single";
  ProtoFieldValue manyEmpty;
  manyEmpty.name = "many";
  inst.fieldValues = {singleNull, manyEmpty};
  int resolves = 0;
  auto resolver = [&resolves](const std::vector<std::string> &,
                             const std::string &) -> std::shared_ptr<ProtoDeclaration> {
    ++resolves;
    return nullptr;
  };
  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, resolver, "", guard, warnings);
  REQUIRE(primary);
  CHECK(primary->nodeTypeName() == "Shape");
  auto geometry = std::any_cast<std::shared_ptr<X3DNode>>(
      fieldByName(*primary, "geometry")->get(*primary));
  CHECK_FALSE(geometry);
  REQUIRE(scene.protoPeerNodes.size() == 1);
  auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*scene.protoPeerNodes.front(), "children")
          ->get(*scene.protoPeerNodes.front()));
  CHECK(children.empty());
  CHECK(resolves == 0);
  CHECK(warnings.empty());

  // A body USE keeps the default graph live even when the interface value is
  // overridden, so its contained instance is still attempted once.
  outer->body.nodes.push_back(defaultGroup);
  Scene aliasedScene;
  warnings.clear();
  auto aliasedPrimary = expandInstance(inst, aliasedScene, resolver, "", guard, warnings);
  REQUIRE(aliasedPrimary);
  CHECK(resolves == 1);
  REQUIRE(warnings.size() == 1);
  CHECK(warnings[0].kind == ProtoWarning::Kind::UnresolvedExtern);
}

TEST_CASE("unresolved external node default leaves a null slot and one warning") {
  auto remote = std::make_shared<ExternProtoDeclaration>();
  remote->name = "Remote";
  remote->url = {"missing.x3d#Remote"};
  ProtoInstance source;
  source.name = "Remote";
  source.externDeclaration = remote;
  auto wrapper = std::make_shared<ProtoInstanceTemplate>(source);
  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  ProtoField many;
  many.name = "many";
  many.type = X3DFieldType::MFNode;
  many.access = AccessType::InputOutput;
  many.nodeDefault = {createX3DNode("Shape"), wrapper, wrapper};
  outer->interface.push_back(many);
  auto body = createX3DNode("Group");
  outer->body.nodes.push_back(body);
  outer->body.isConnections.push_back({body, "children", "many"});
  ProtoInstance inst;
  inst.name = "Outer";
  inst.declaration = outer;
  int resolves = 0;
  auto resolver = [&resolves](const std::vector<std::string> &,
                             const std::string &) -> std::shared_ptr<ProtoDeclaration> {
    ++resolves;
    return nullptr;
  };
  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, resolver, "", guard, warnings);
  REQUIRE(primary);
  auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary, "children")->get(*primary));
  REQUIRE(children.size() == 3);
  REQUIRE(children[0]);
  CHECK(children[0]->nodeTypeName() == "Shape");
  CHECK_FALSE(children[1]);
  CHECK_FALSE(children[2]);
  CHECK(resolves == 1);
  REQUIRE(warnings.size() == 1);
  CHECK(warnings[0].kind == ProtoWarning::Kind::UnresolvedExtern);
  CHECK(wrapper->instance.externDeclaration == remote);
}

TEST_CASE("throwing caller resolver does not poison shared wrapper or later instances") {
  auto remote = std::make_shared<ExternProtoDeclaration>();
  remote->name = "Remote";
  remote->url = {"bad.x3d#Remote"};
  ProtoInstance remoteInstance;
  remoteInstance.name = "Remote";
  remoteInstance.externDeclaration = remote;
  auto shared = std::make_shared<ProtoInstanceTemplate>(remoteInstance);

  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  ProtoField items;
  items.name = "items";
  items.type = X3DFieldType::MFNode;
  items.access = AccessType::InitializeOnly;
  outer->interface.push_back(items);
  auto body = createX3DNode("Group");
  outer->body.nodes.push_back(body);
  outer->body.isConnections.push_back({body, "children", "items"});
  ProtoInstance first;
  first.name = "Outer";
  first.declaration = outer;
  ProtoFieldValue fieldValue;
  fieldValue.name = "items";
  fieldValue.nodeValue = {shared};
  first.fieldValues.push_back(fieldValue);
  ProtoInstance second = first;

  auto valid = std::make_shared<ProtoDeclaration>();
  valid->name = "Valid";
  valid->body.nodes.push_back(createX3DNode("Shape"));
  ProtoInstance third;
  third.name = "Valid";
  third.declaration = valid;
  Scene scene;
  scene.protoInstances = {first, second, third};
  int resolves = 0;
  auto throwing = [&resolves](const std::vector<std::string> &,
                              const std::string &) -> std::shared_ptr<ProtoDeclaration> {
    ++resolves;
    throw std::runtime_error("resolver failed");
  };
  std::vector<ProtoWarning> warnings;
  expandScene(scene, throwing, "", warnings);
  CHECK(resolves == 1);
  CHECK(scene.protoInstances[2].expanded);
  CHECK_FALSE(scene.rootNodes.empty());
  CHECK(scene.rootNodes.back()->nodeTypeName() == "Shape");
  CHECK(std::none_of(warnings.begin(), warnings.end(), [](const ProtoWarning &warning) {
    return warning.kind == ProtoWarning::Kind::RecursionLimit;
  }));
}

TEST_CASE("PROTO direct nested instance is the ordered primary with an active ordinary peer") {
  auto leaf = std::make_shared<ProtoDeclaration>();
  leaf->name = "Leaf";
  leaf->body.nodes.push_back(createX3DNode("Group"));

  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  auto peer = createX3DNode("Transform");
  outer->body.nodes.push_back(peer);
  outer->body.nestedInstances.push_back(ProtoInstance{});
  auto &nested = outer->body.nestedInstances.back();
  nested.name = "Leaf";
  nested.declaration = leaf;
  outer->body.recordInstance(0);
  outer->body.recordNode(peer);

  ProtoField iface;
  iface.name = "shift";
  iface.type = X3DFieldType::SFVec3f;
  iface.access = AccessType::InputOutput;
  outer->interface.push_back(iface);
  outer->body.isConnections.push_back({peer, "translation", "shift"});

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance inst;
  inst.name = "Outer";
  inst.DEF = "OUTER";
  inst.declaration = outer;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(primary);
  CHECK(primary->nodeTypeName() == "Group");
  CHECK(primary->getDEF() == "OUTER");
  REQUIRE(scene.protoPeerNodes.size() == 1);
  CHECK(scene.protoPeerNodes.front()->nodeTypeName() == "Transform");
  CHECK(scene.protoRedirects.contains(primary.get()));
  REQUIRE(scene.protoRedirects[primary.get()]["shift"].size() == 1);
  CHECK(scene.protoRedirects[primary.get()]["shift"].front().targetNode ==
        scene.protoPeerNodes.front());
  CHECK(warnings.empty());
}

TEST_CASE("PROTO ordinary primary and direct nested peer are separate") {
  auto leaf = std::make_shared<ProtoDeclaration>();
  leaf->name = "Leaf";
  leaf->body.nodes.push_back(createX3DNode("Group"));
  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  auto primaryBody = createX3DNode("Transform");
  outer->body.nodes.push_back(primaryBody);
  outer->body.recordNode(primaryBody);
  ProtoInstance nested;
  nested.name = "Leaf";
  nested.declaration = leaf;
  outer->body.nestedInstances.push_back(nested);
  outer->body.recordInstance(0);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance inst;
  inst.name = "Outer";
  inst.declaration = outer;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(primary);
  CHECK(primary->nodeTypeName() == "Transform");
  auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary, "children")->get(*primary));
  CHECK(children.empty());
  REQUIRE(scene.protoPeerNodes.size() == 1);
  CHECK(scene.protoPeerNodes.front()->nodeTypeName() == "Group");
  CHECK(warnings.empty());
}

TEST_CASE("PROTO nested-only body expands and unresolved first instance is not skipped") {
  auto leaf = std::make_shared<ProtoDeclaration>();
  leaf->name = "Leaf";
  leaf->body.nodes.push_back(createX3DNode("Group"));
  auto outer = std::make_shared<ProtoDeclaration>();
  outer->name = "Outer";
  ProtoInstance nested;
  nested.name = "Leaf";
  nested.declaration = leaf;
  outer->body.nestedInstances.push_back(nested);
  outer->body.recordInstance(0);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  ProtoInstance inst;
  inst.name = "Outer";
  inst.declaration = outer;
  auto only = expandInstance(inst, scene, noopProtoResolver, "", guard, warnings);
  REQUIRE(only);
  CHECK(only->nodeTypeName() == "Group");
  CHECK(warnings.empty());

  outer->body.nestedInstances[0].declaration.reset();
  outer->body.nestedInstances[0].name = "Missing";
  auto peer = createX3DNode("Transform");
  outer->body.nodes.push_back(peer);
  outer->body.recordNode(peer);
  Scene failedScene;
  warnings.clear();
  auto failed = expandInstance(inst, failedScene, noopProtoResolver, "", guard, warnings);
  CHECK_FALSE(failed);
  REQUIRE(warnings.size() == 1);
  CHECK(warnings.front().kind == ProtoWarning::Kind::MissingDeclaration);
}

TEST_CASE("parsed PROTO body selects the first direct node or instance") {
  const char *prefix =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>";
  const char *suffix = "<ProtoInstance name='Outer'/></Scene></X3D>";
  const std::string bodyStart =
      "<ProtoDeclare name='Outer'><ProtoBody>";
  const std::string bodyEnd = "</ProtoBody></ProtoDeclare>";
  auto parse = [&](const std::string &body) {
    return x3d::codec::parseDocument(std::string(prefix) + bodyStart + body +
                                     bodyEnd + suffix);
  };

  auto only = parse("<ProtoInstance name='Leaf'/>");
  REQUIRE(only.scene.rootNodes.size() == 1);
  CHECK(only.scene.rootNodes.front()->nodeTypeName() == "Group");
  CHECK(only.protoWarnings.empty());

  auto nestedFirst = parse("<ProtoInstance name='Leaf'/><Transform/>");
  REQUIRE(nestedFirst.scene.rootNodes.size() == 1);
  CHECK(nestedFirst.scene.rootNodes.front()->nodeTypeName() == "Group");
  REQUIRE(nestedFirst.scene.protoPeerNodes.size() == 1);
  CHECK(nestedFirst.scene.protoPeerNodes.front()->nodeTypeName() == "Transform");
  CHECK(nestedFirst.protoWarnings.empty());

  auto ordinaryFirst = parse("<Transform/><ProtoInstance name='Leaf'/>");
  REQUIRE(ordinaryFirst.scene.rootNodes.size() == 1);
  CHECK(ordinaryFirst.scene.rootNodes.front()->nodeTypeName() == "Transform");
  auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*ordinaryFirst.scene.rootNodes.front(), "children")
          ->get(*ordinaryFirst.scene.rootNodes.front()));
  CHECK(children.empty());
  REQUIRE(ordinaryFirst.scene.protoPeerNodes.size() == 1);
  CHECK(ordinaryFirst.scene.protoPeerNodes.front()->nodeTypeName() == "Group");
  CHECK(ordinaryFirst.protoWarnings.empty());
}

TEST_CASE("nested PROTO children keep authored order and USE aliases in every instance") {
  const struct {
    x3d::codec::Encoding encoding;
    const char *text;
  } cases[] = {
      {x3d::codec::Encoding::XML,
       "<X3D version='4.0'><Scene>"
       "<ProtoDeclare name='A'><ProtoBody><Transform/></ProtoBody></ProtoDeclare>"
       "<ProtoDeclare name='B'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>"
       "<ProtoDeclare name='Outer'><ProtoBody><Group>"
       "<ProtoInstance name='A'/><Group DEF='Shared'/>"
       "<ProtoInstance name='B'/><Group USE='Shared'/>"
       "</Group></ProtoBody></ProtoDeclare>"
       "<ProtoInstance name='Outer'/><ProtoInstance name='Outer'/>"
       "</Scene></X3D>"},
      {x3d::codec::Encoding::JSON,
       R"({"X3D":{"@version":"4.0","Scene":{"-children":[{"ProtoDeclare":{"@name":"A","ProtoBody":{"-children":[{"Transform":{}}]}}},{"ProtoDeclare":{"@name":"B","ProtoBody":{"-children":[{"Shape":{}}]}}},{"ProtoDeclare":{"@name":"Outer","ProtoBody":{"-children":[{"Group":{"-children":[{"ProtoInstance":{"@name":"A"}},{"Group":{"@DEF":"Shared"}},{"ProtoInstance":{"@name":"B"}},{"Group":{"@USE":"Shared"}}]}}]}}},{"ProtoInstance":{"@name":"Outer"}},{"ProtoInstance":{"@name":"Outer"}}]}}})"},
      {x3d::codec::Encoding::ClassicVRML,
       "#X3D V4.0 utf8\nPROTO A [ ] { Transform { } }\n"
       "PROTO B [ ] { Shape { } }\n"
       "PROTO Outer [ ] { Group { children [ A { } DEF Shared Group { } "
       "B { } USE Shared ] } }\nOuter { }\nOuter { }\n"},
  };
  for (const auto &c : cases) {
    auto doc = x3d::codec::parseDocument(c.text, c.encoding);
    REQUIRE(doc.scene.rootNodes.size() == 2);
    REQUIRE(doc.protoWarnings.empty());
    auto children = [](const std::shared_ptr<X3DNode> &root) {
      return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
          fieldByName(*root, "children")->get(*root));
    };
    const auto first = children(doc.scene.rootNodes[0]);
    const auto second = children(doc.scene.rootNodes[1]);
    for (const auto &nodes : {first, second}) {
      REQUIRE(nodes.size() == 4);
      CHECK(nodes[0]->nodeTypeName() == "Transform");
      CHECK(nodes[1]->nodeTypeName() == "Group");
      CHECK(nodes[2]->nodeTypeName() == "Shape");
      CHECK(nodes[3] == nodes[1]);
    }
    CHECK(first[0] != second[0]);
    CHECK(first[1] != second[1]);
    CHECK(first[2] != second[2]);
  }
}

TEST_CASE("failed nested instance leaves surrounding MFNode children in order") {
  auto doc = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Outer'><ProtoBody><Group>"
      "<Transform/><ProtoInstance name='Missing'/><Shape/>"
      "</Group></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Outer'/></Scene></X3D>");
  REQUIRE(doc.scene.rootNodes.size() == 1);
  const auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*doc.scene.rootNodes.front(), "children")
          ->get(*doc.scene.rootNodes.front()));
  REQUIRE(children.size() == 2);
  CHECK(children[0]->nodeTypeName() == "Transform");
  CHECK(children[1]->nodeTypeName() == "Shape");
  REQUIRE(doc.protoWarnings.size() == 1);
  CHECK(doc.protoWarnings.front().kind == ProtoWarning::Kind::MissingDeclaration);
}
