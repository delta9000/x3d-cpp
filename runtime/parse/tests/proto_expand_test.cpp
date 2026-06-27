// runtime/parse/tests/proto_expand_test.cpp
#include "X3DDocument.hpp"
#include "X3DProto.hpp"
#include "X3DProtoExpand.hpp"
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
  CHECK((primary));

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

  // (2) Unresolved (resolver returns null) -> UnresolvedExtern warning, null.
  Scene s2; ExpandGuard g2; std::vector<ProtoWarning> w2;
  auto none = expandInstance(*inst, s2, x3d::codec::noopProtoResolver, "", g2, w2);
  CHECK((!none && w2.size() == 1 &&
         w2[0].kind == ProtoWarning::Kind::UnresolvedExtern));

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
  fv.nodeValue.push_back(createX3DNode("Box"));
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
  CHECK((warningsDefault.empty()));
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
