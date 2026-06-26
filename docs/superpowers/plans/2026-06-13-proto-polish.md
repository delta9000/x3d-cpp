# PROTO Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make nested-in-body `<ProtoInstance>`s expand correctly, give the JSON reader PROTO capture (and verify the VRML97 reader's inherited capture), and bring all four codec writers to PROTO round-trip parity.

**Architecture:** Entirely hand-written runtime (`runtime/`, `runtime/parse/`, `runtime/codecs/`) plus the proto model header. **No codegen is touched — the golden header tree must stay byte-identical.** A `ProtoInstance` nested inside a `<ProtoBody>` becomes part of the body *template* (`ProtoBody::nestedInstances`); the expansion engine clones the body and recursively expands each nested instance against the *cloned* parent, once per outer instantiation. Readers route body-nested instances into the current body instead of the flat scene list. Writers re-emit `<ProtoDeclare>` bodies and `<ProtoInstance>`s in each encoding.

**Tech Stack:** C++20, header-only runtime; CMake + ctest (Ninja `dev` preset via `mise run build`); reflection-driven codecs.

**Build/test commands (from repo root):**
- Build + full test suite: `mise run build` (Ninja + ccache `dev` preset → pytest + ctest).
- Single ctest after a build: `ctest --preset dev -R <name> --output-on-failure`.
- Golden gate is part of pytest (`tests/test_golden_tree.py`) — must stay byte-identical since this push is runtime-only.

**Disambiguation gotcha (applies to every C++ TU below):** there are TWO `ProtoInstance` types — the generated node `generated_cpp_bindings/ProtoInstance.hpp` and the hand-written `x3d::runtime::ProtoInstance` struct. Always fully-qualify `x3d::runtime::ProtoInstance` in any TU that could see both.

---

## File Structure

| File | Responsibility | Change |
|------|----------------|--------|
| `runtime/X3DProto.hpp` | Proto data model | Add `ProtoBody::nestedInstances` |
| `runtime/X3DProtoExpand.hpp` | Expansion engine | Recursive nested expansion; move `attachToParent` above `expandInstance` |
| `runtime/parse/ClassicVrmlReader.hpp` | `.x3dv`/`.wrl` reader | Route body-nested instances into `currentProtoBody` |
| `runtime/codecs/XmlReader.hpp` | `.x3d` reader | Thread `currentProtoBody`; route body-nested instances |
| `runtime/parse/JsonReader.hpp` | `.json` reader | NEW PROTO capture (declare/extern/instance/IS) + body routing |
| `runtime/codecs/XmlWriter.hpp` | `.x3d` writer | Re-emit `<ProtoDeclare>`/`<ExternProtoDeclare>` bodies |
| `runtime/codecs/JsonWriter.hpp` | `.json` writer | Re-emit `ProtoInstance` + `ProtoDeclare` |
| `runtime/codecs/VrmlWriter.hpp` | `.x3dv` writer | Re-emit `ProtoInstance` + `ProtoDeclare` |
| `runtime/parse/tests/proto_expand_test.cpp` | Engine tests | Add nested-in-body test |
| `runtime/codecs/tests/proto_roundtrip_test.cpp` | XML round-trip | Flip body-suppression assertion; assert ProtoDeclare re-emit |
| `runtime/parse/tests/proto_nested_body_test.cpp` | NEW | XML + ClassicVRML nested-body end-to-end |
| `runtime/parse/tests/json_proto_test.cpp` | NEW | JSON PROTO capture + round-trip |
| `runtime/parse/tests/vrml97_proto_test.cpp` | NEW | `.wrl` PROTO read verification |
| `runtime/codecs/tests/proto_writer_parity_test.cpp` | NEW | JSON + VRML writer re-emit |
| `CMakeLists.txt` | Test wiring | Register the 4 new test executables |

---

## Task 1: Model — `ProtoBody::nestedInstances`

**Files:**
- Modify: `runtime/X3DProto.hpp` (the `ProtoBody` struct, ~lines 89-93)

- [ ] **Step 1: Add the field**

In `runtime/X3DProto.hpp`, change the `ProtoBody` struct to add a `nestedInstances` vector. The struct currently reads:

```cpp
struct ProtoBody {
  std::vector<std::shared_ptr<X3DNode>> nodes;
  std::vector<Route> routes;
  std::vector<IsConnection> isConnections;
};
```

Replace it with:

```cpp
struct ProtoBody {
  std::vector<std::shared_ptr<X3DNode>> nodes;
  std::vector<Route> routes;
  std::vector<IsConnection> isConnections;
  // ProtoInstances that appear INSIDE this body (template). Each carries its
  // placement (parent = an original body node, or empty for a direct body-root
  // child) so the expansion engine can splice the expanded primary into the
  // per-instantiation CLONE of that parent. Kept here, not in scene.protoInstances,
  // so a body-nested instance is expanded once per outer instantiation rather
  // than once globally / mis-attached to the un-cloned template.
  std::vector<ProtoInstance> nestedInstances;
};
```

`ProtoInstance` is declared later in the same header (line ~134). Forward-declare it above `ProtoBody` so the vector compiles. Immediately before `struct IsConnection` (line ~29) add:

```cpp
class ProtoInstance; // defined below; ProtoBody holds a vector of these
```

Note: `std::vector<ProtoInstance>` of an incomplete type is fine here because the vector is only *used* (constructed/resized) in TUs that include the full header, and `ProtoInstance` is fully defined later in this same header before any such use.

- [ ] **Step 2: Build to verify it still compiles**

Run: `mise run build`
Expected: PASS — pytest unchanged, ctest unchanged, **golden byte-identical** (model header is runtime-only; nothing references the new field yet).

- [ ] **Step 3: Commit**

```bash
git add runtime/X3DProto.hpp
git commit -m "Proto polish: add ProtoBody::nestedInstances to the model"
```

---

## Task 2: Expansion engine — recursive nested expansion

**Files:**
- Modify: `runtime/X3DProtoExpand.hpp`
- Test: `runtime/parse/tests/proto_expand_test.cpp`

The `attachToParent` helper currently lives in a `proto_detail` block *after* `expandInstance` (lines ~181-201). `expandInstance` now needs to call it, so move it *before* `expandInstance`.

- [ ] **Step 1: Write the failing test**

Append this test function to `runtime/parse/tests/proto_expand_test.cpp` (before `int main()`), and add a call to it inside `main()`:

```cpp
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
  assert(primary && primary->nodeTypeName() == "Transform");
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*primary, "children")->get(*primary));
  assert(kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box");
  assert(warnings.empty());
  // The nested primary is recorded for round-trip on write.
  assert(scene.expandedSources.count(kids[0].get()) == 1);
}
```

And in `main()` add `nestedInBodyExpandTest();` after `expandSceneSpliceTest();`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build` then `ctest --preset dev -R x3d_proto_expand --output-on-failure`
Expected: FAIL — `kids` is empty (nested instance is never expanded).

- [ ] **Step 3: Move `attachToParent` above `expandInstance`**

In `runtime/X3DProtoExpand.hpp`, **delete** the second `proto_detail` namespace block (lines ~181-201) that defines `attachToParent`, and **insert** its `attachToParent` definition into the *first* `proto_detail` block (the one starting at line ~24, right after `instanceValue`, before the block's closing `}`):

```cpp
/// Splice `primary` into `parent`'s `field` slot (SFNode set / MFNode append).
/// Empty field => the node's default containerField.
inline void attachToParent(const std::shared_ptr<X3DNode> &parent,
                           const std::string &field,
                           const std::shared_ptr<X3DNode> &primary) {
  std::string slot = field.empty() ? primary->defaultContainerField() : field;
  const FieldInfo *fi = findField(*parent, slot);
  if (!fi || !fi->set) return;
  if (fi->type == X3DFieldType::SFNode) {
    fi->set(*parent, std::any(primary));
  } else if (fi->type == X3DFieldType::MFNode) {
    auto kids =
        std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi->get(*parent));
    kids.push_back(primary);
    fi->set(*parent, std::any(std::move(kids)));
  }
}
```

- [ ] **Step 4: Add the nested-expansion block to `expandInstance`**

In `expandInstance`, immediately **before** the final `return primary;` (currently line ~178), insert:

```cpp
  // Expand ProtoInstances captured inside this body, once per outer
  // instantiation. Each was recorded against an ORIGINAL body node (a cloneMap
  // key) or with an empty parent (a direct body-root child). Recurse through the
  // shared guard — increment/decrement depth so deep/cyclic nesting terminates
  // with a RecursionLimit warning rather than overflowing.
  for (x3d::runtime::ProtoInstance nested : decl->body.nestedInstances) {
    if (!nested.declaration && !nested.externDeclaration) {
      if (auto d = scene.findProto(nested.name)) nested.declaration = d;
    }
    ++guard.depth;
    auto nestedPrimary =
        expandInstance(nested, scene, resolver, baseUrl, guard, warnings);
    --guard.depth;
    if (!nestedPrimary) continue;
    if (auto origParent = nested.parent.lock()) {
      // Case A: nested under a real body node — splice into that node's clone.
      auto pit = cloneMap.find(origParent.get());
      if (pit != cloneMap.end())
        proto_detail::attachToParent(pit->second, nested.parentField,
                                     nestedPrimary);
      else
        warnings.push_back({ProtoWarning::Kind::UnknownField, nested.name,
                            "nested parent not found in body clone"});
    } else {
      // Case B/C: direct body-root child — best-effort attach onto the outer
      // primary by the nested instance's containerField; lenient.
      const std::string slot =
          nested.parentField.empty() ? nested.containerField : nested.parentField;
      proto_detail::attachToParent(primary, slot, nestedPrimary);
    }
    scene.expandedSources[nestedPrimary.get()] = nested;
  }
```

- [ ] **Step 5: Run to verify it passes**

Run: `mise run build` then `ctest --preset dev -R x3d_proto_expand --output-on-failure`
Expected: PASS (all functions incl. `nestedInBodyExpandTest`).

- [ ] **Step 6: Commit**

```bash
git add runtime/X3DProtoExpand.hpp runtime/parse/tests/proto_expand_test.cpp
git commit -m "Proto polish: recursively expand nested-in-body ProtoInstances"
```

---

## Task 3: ClassicVrml reader — route body-nested instances

**Files:**
- Modify: `runtime/parse/ClassicVrmlReader.hpp` (the proto-detected branch in `parseNode`, ~lines 286-298)
- Test: covered by Task 5's `proto_nested_body_test.cpp` (ClassicVRML case)

`parseNode` already receives `runtime::ProtoBody *currentProtoBody` and already records `parent`/`parentField` on `scene.protoInstances.back()`. We add: when inside a body, move that just-recorded instance from `scene.protoInstances` into `currentProtoBody->nestedInstances`.

- [ ] **Step 1: Edit the proto-detected branch**

In `parseNode`, the current branch reads:

```cpp
    if (scene.findProto(rawTypeName) || findExternProto(scene, rawTypeName)) {
      parseProtoInstance(tok, scene, def, rawTypeName);
      if (!scene.protoInstances.empty()) {
        scene.protoInstances.back().parent = parentNode;
        scene.protoInstances.back().parentField = parentField;
      }
      return nullptr;
    }
```

Replace the inner `if` body with:

```cpp
    if (scene.findProto(rawTypeName) || findExternProto(scene, rawTypeName)) {
      parseProtoInstance(tok, scene, def, rawTypeName);
      if (!scene.protoInstances.empty()) {
        scene.protoInstances.back().parent = parentNode;
        scene.protoInstances.back().parentField = parentField;
        if (currentProtoBody) {
          // Inside a PROTO body: this instance belongs to the body template, not
          // the flat scene list — move it so it expands per outer instantiation.
          currentProtoBody->nestedInstances.push_back(
              std::move(scene.protoInstances.back()));
          scene.protoInstances.pop_back();
        }
      }
      return nullptr;
    }
```

- [ ] **Step 2: Verify `currentProtoBody` + `parentNode` thread into body sub-nodes**

Read `parseNodeBody` in the same file (the SF/MFNode value path, ~lines 475-520). Confirm the recursive `parseNode(...)` call for a field's node value passes BOTH `currentProtoBody` and the current node as `parentNode`. If `parentNode` is not already threaded there, pass the node being built. (Per the existing `expandSceneSpliceTest`, scene-scope parent linkage already works; this confirms it holds inside a body.) No code change if it already threads both.

- [ ] **Step 3: Build (full test run happens in Task 5)**

Run: `mise run build`
Expected: PASS, golden byte-identical. (Behavioral assertion deferred to Task 5's test; this step only guards against a compile break.)

- [ ] **Step 4: Commit**

```bash
git add runtime/parse/ClassicVrmlReader.hpp
git commit -m "Proto polish: ClassicVRML routes body-nested instances into the body"
```

---

## Task 4: XmlReader — thread `currentProtoBody`, route body-nested instances

**Files:**
- Modify: `runtime/codecs/XmlReader.hpp` (`readNode`, `readChildren` inline at ~180-210, `readProtoBody` ~286-318, `readProtoInstance` ~343-394)
- Test: covered by Task 5's `proto_nested_body_test.cpp` (XML case)

- [ ] **Step 1: Add a `currentProtoBody` param to `readNode`**

Change the `readNode` signature (line ~146) from:

```cpp
  std::shared_ptr<X3DNode> readNode(const xml::Element &el,
                                    runtime::Scene &scene) {
```

to:

```cpp
  std::shared_ptr<X3DNode> readNode(const xml::Element &el,
                                    runtime::Scene &scene,
                                    runtime::ProtoBody *currentProtoBody = nullptr) {
```

- [ ] **Step 2: Pass it at the nested-ProtoInstance site inside `readNode`'s child loop**

In `readNode`'s child loop, the current `ProtoInstance` branch (line ~185) reads:

```cpp
      if (childEl->name == "ProtoInstance") {
        const std::string slot =
            childEl->attrOr("containerField", "children");
        readProtoInstance(*childEl, scene, node, slot);
        continue;
      }
```

Replace with (route into the current body when inside one):

```cpp
      if (childEl->name == "ProtoInstance") {
        const std::string slot =
            childEl->attrOr("containerField", "children");
        readProtoInstance(*childEl, scene, node, slot, currentProtoBody);
        continue;
      }
```

- [ ] **Step 3: Add a `body` param to `readProtoInstance` and route the result**

Change `readProtoInstance` signature (line ~343) from:

```cpp
  void readProtoInstance(const xml::Element &el, runtime::Scene &scene,
                         const std::shared_ptr<X3DNode> &parent,
                         const std::string &parentSlot) {
```

to:

```cpp
  void readProtoInstance(const xml::Element &el, runtime::Scene &scene,
                         const std::shared_ptr<X3DNode> &parent,
                         const std::string &parentSlot,
                         runtime::ProtoBody *body = nullptr) {
```

and change its final line (currently `scene.protoInstances.push_back(std::move(inst));`, line ~393) to:

```cpp
    if (body)
      body->nestedInstances.push_back(std::move(inst));
    else
      scene.protoInstances.push_back(std::move(inst));
```

- [ ] **Step 4: Route through `readProtoBody`**

In `readProtoBody` (line ~286), the body-node read (line ~307) and the direct-ProtoInstance branch (line ~303-306) must pass `&out` as the current body. Change:

```cpp
      if (c->name == "ProtoInstance") {
        readProtoInstance(*c, scene, nullptr, "");
        continue;
      }
      auto node = readNode(*c, scene);
```

to:

```cpp
      if (c->name == "ProtoInstance") {
        // Direct body-root child: empty parent => Case B/C in the engine.
        readProtoInstance(*c, scene, nullptr, "", &out);
        continue;
      }
      auto node = readNode(*c, scene, &out);
```

This makes the body node's whole subtree carry `currentProtoBody = &out`, so a `<ProtoInstance>` nested any depth under a body node routes into `out.nestedInstances` with `parent` = its immediate parent node (Case A). Scene-root reads still call `readNode(*c, out)` with the default `nullptr`, so scene-scope instances are unchanged.

- [ ] **Step 5: Build to verify it compiles**

Run: `mise run build`
Expected: PASS, golden byte-identical.

- [ ] **Step 6: Commit**

```bash
git add runtime/codecs/XmlReader.hpp
git commit -m "Proto polish: XmlReader threads currentProtoBody for nested instances"
```

---

## Task 5: Nested-body end-to-end test (XML + ClassicVRML)

**Files:**
- Create: `runtime/parse/tests/proto_nested_body_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the test**

Create `runtime/parse/tests/proto_nested_body_test.cpp`:

```cpp
// proto_nested_body_test.cpp
// A <ProtoInstance> nested inside a <ProtoBody> must expand once per outer
// instantiation and land inside the outer primary's cloned subtree — verified
// end-to-end through the XML and ClassicVRML front doors.
#include "X3DParse.hpp"
#include "X3DReflection.hpp"

#include <any>
#include <cassert>
#include <memory>
#include <string>

using namespace x3d;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

// Outer proto "Wrap" body = Transform { children [ Leaf {} ] }, where Leaf is a
// proto whose body is a Box. Instantiating Wrap must yield Transform>Box.
static std::shared_ptr<X3DNode> firstRoot(const runtime::X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes) if (n) return n;
  return nullptr;
}

static void xmlNestedTest() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Leaf'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody>"
      "<Transform><ProtoInstance name='Leaf' containerField='children'/></Transform>"
      "</ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Wrap'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  auto root = firstRoot(doc);
  assert(root && root->nodeTypeName() == "Transform");
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*root, "children")->get(*root));
  assert(kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box");
}

static void classicVrmlNestedTest() {
  const char *vrml =
      "#X3D V4.0 utf8\n"
      "PROTO Leaf [] { Box {} }\n"
      "PROTO Wrap [] { Transform { children Leaf {} } }\n"
      "Wrap {}\n";
  auto doc = codec::parseDocument(vrml);
  auto root = firstRoot(doc);
  assert(root && root->nodeTypeName() == "Transform");
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*root, "children")->get(*root));
  assert(kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box");
}

int main() {
  xmlNestedTest();
  classicVrmlNestedTest();
  return 0;
}
```

- [ ] **Step 2: Register it in CMake**

In `CMakeLists.txt`, after the `x3d_proto_front_door` block (line ~414), add:

```cmake
    add_executable(x3d_proto_nested_body
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/proto_nested_body_test.cpp")
    target_link_libraries(x3d_proto_nested_body PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_proto_nested_body COMMAND x3d_proto_nested_body)
```

- [ ] **Step 3: Run to verify it passes**

Run: `mise run build` then `ctest --preset dev -R x3d_proto_nested_body --output-on-failure`
Expected: PASS. (If `classicVrmlNestedTest` fails on parent threading, fix Task 3 Step 2 — pass `parentNode`=current node into the nested `parseNode` in `parseNodeBody`.)

- [ ] **Step 4: Commit**

```bash
git add runtime/parse/tests/proto_nested_body_test.cpp CMakeLists.txt
git commit -m "Proto polish: end-to-end nested-body test (XML + ClassicVRML)"
```

---

## Task 6: JsonReader — PROTO capture

**Files:**
- Modify: `runtime/parse/JsonReader.hpp`
- Create: `runtime/parse/tests/json_proto_test.cpp`
- Modify: `CMakeLists.txt`

JSON encodes PROTO with the same element names as XML (`ProtoDeclare`, `ProtoInterface`, `ProtoBody`, `ProtoInstance`, `ExternProtoDeclare`, `field`, `fieldValue`, `IS`/`connect`), `@`-prefixed attributes, and `-`-prefixed child-node slots. Repeated elements may be a single object or an array — mirror the existing `readRoutes` leniency (handle both).

- [ ] **Step 1: Write the failing test**

Create `runtime/parse/tests/json_proto_test.cpp`:

```cpp
// json_proto_test.cpp
// The JSON reader must capture PROTO declarations + instances (parity with the
// XML/ClassicVRML readers) so a JSON document with a proto expands like the
// others, including a nested-in-body instance.
#include "X3DParse.hpp"
#include "X3DReflection.hpp"

#include <any>
#include <cassert>
#include <memory>
#include <string>

using namespace x3d;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}
static std::shared_ptr<X3DNode> firstRoot(const runtime::X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes) if (n) return n;
  return nullptr;
}

// Scene-level: ProtoDeclare P { body Box }, ProtoInstance P with size override.
static void jsonProtoExpandTest() {
  const char *json = R"JSON({ "X3D": { "@version": "4.0", "Scene": {
    "-children": [
      { "ProtoDeclare": { "@name": "P",
          "ProtoInterface": { "field": [
            { "@name": "size", "@type": "SFVec3f", "@accessType": "initializeOnly", "@value": "2 2 2" } ] },
          "ProtoBody": { "-children": [
            { "Box": { "IS": { "connect": [ { "@nodeField": "size", "@protoField": "size" } ] } } } ] } } },
      { "ProtoInstance": { "@name": "P", "@DEF": "A",
          "fieldValue": [ { "@name": "size", "@value": "5 5 5" } ] } }
    ] } } })JSON";
  auto doc = codec::parseDocument(json);
  auto root = firstRoot(doc);
  assert(root && root->nodeTypeName() == "Box" && root->getDEF() == "A");
  auto sz = std::any_cast<SFVec3f>(fieldByName(*root, "size")->get(*root));
  assert(sz.x == 5.f && sz.y == 5.f && sz.z == 5.f);
}

int main() {
  jsonProtoExpandTest();
  return 0;
}
```

- [ ] **Step 2: Register it + run to verify it FAILS**

Add to `CMakeLists.txt` after the `x3d_proto_nested_body` block:

```cmake
    add_executable(x3d_json_proto
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/json_proto_test.cpp")
    target_link_libraries(x3d_json_proto PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_json_proto COMMAND x3d_json_proto)
```

Run: `mise run build` then `ctest --preset dev -R x3d_json_proto --output-on-failure`
Expected: FAIL — the `ProtoDeclare` child is unknown to the factory, so it is skipped and `root` is null.

- [ ] **Step 3: Add the proto-statement interceptor + readers to JsonReader**

In `runtime/parse/JsonReader.hpp`, add helpers in the private section. First, include the field-type/access mapping. The reader already maps fields via `applyJsonField`; add small local mappers mirroring XmlReader's `mapFieldType`/`mapAccessType` (JsonReader may already have type mapping — reuse if present; otherwise add these statics):

```cpp
  // --- PROTO capture (parity with XmlReader) ----------------------------------

  // Returns true and captures if `wrapper` is a PROTO statement object
  // ({ "ProtoDeclare"|"ExternProtoDeclare"|"ProtoInstance": {...} }). `parent`
  // /`slot` give a nested instance's placement; `body` (non-null inside a
  // ProtoBody) routes a captured instance into that body's nestedInstances.
  bool tryReadProtoStatement(const json::Value &wrapper, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot,
                             runtime::ProtoBody *body) {
    if (wrapper.object.empty()) return false;
    const std::string &key = wrapper.object.front().first;
    const json::Value *obj = wrapper.object.front().second.get();
    if (!obj || !obj->isObject()) return false;
    if (key == "ProtoDeclare") { readJsonProtoDeclare(*obj, scene); return true; }
    if (key == "ExternProtoDeclare") { readJsonExternProtoDeclare(*obj, scene); return true; }
    if (key == "ProtoInstance") { readJsonProtoInstance(*obj, scene, parent, slot, body); return true; }
    return false;
  }

  void readJsonInterfaceFields(const json::Value &iface,
                               std::vector<runtime::ProtoField> &out,
                               runtime::Scene &scene) {
    const json::Value *fields = iface.member("field");
    if (!fields) return;
    auto one = [&](const json::Value &f) {
      if (!f.isObject()) return;
      runtime::ProtoField pf;
      pf.name = strMember(f, "@name");
      pf.type = mapProtoFieldType(strMember(f, "@type"));
      pf.access = mapProtoAccessType(strMemberOr(f, "@accessType", "inputOutput"));
      if (pf.type == X3DFieldType::SFNode || pf.type == X3DFieldType::MFNode) {
        if (const json::Value *kids = f.member("-children"); kids && kids->isArray())
          for (const auto &c : kids->array)
            if (c && c->isObject()) if (auto n = readNode(*c, scene)) pf.nodeDefault.push_back(n);
      } else if (const json::Value *v = f.member("@value")) {
        if (v->isString()) pf.value = parseValue(pf.type, v->str);
      }
      out.push_back(std::move(pf));
    };
    if (fields->isArray()) { for (const auto &f : fields->array) if (f) one(*f); }
    else one(*fields);
  }

  void readJsonProtoDeclare(const json::Value &obj, runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ProtoDeclaration>();
    decl->name = strMember(obj, "@name");
    if (const json::Value *iface = obj.member("ProtoInterface"); iface && iface->isObject())
      readJsonInterfaceFields(*iface, decl->interface, scene);
    if (const json::Value *bodyObj = obj.member("ProtoBody"); bodyObj && bodyObj->isObject())
      readJsonProtoBody(*bodyObj, scene, decl->body);
    scene.protoDeclarations.push_back(std::move(decl));
  }

  void readJsonExternProtoDeclare(const json::Value &obj, runtime::Scene &scene) {
    auto decl = std::make_shared<runtime::ExternProtoDeclaration>();
    decl->name = strMember(obj, "@name");
    if (const json::Value *u = obj.member("@url"); u && u->isString())
      decl->url = parseMFString(u->str);
    readJsonInterfaceFields(obj, decl->interface, scene);
    scene.externProtoDeclarations.push_back(std::move(decl));
  }

  void readJsonProtoBody(const json::Value &obj, runtime::Scene &scene,
                         runtime::ProtoBody &out) {
    if (const json::Value *kids = obj.member("-children"); kids && kids->isArray()) {
      for (const auto &c : kids->array) {
        if (!c || !c->isObject()) continue;
        // A body child may itself be a PROTO statement (nested instance) — route
        // into this body with no explicit parent (direct body-root child).
        if (tryReadProtoStatement(*c, scene, nullptr, "", &out)) continue;
        auto node = readNode(*c, scene, &out); // threads currentProtoBody (Step 4)
        if (!node) continue;
        out.nodes.push_back(node);
        collectJsonIsConnections(*c, node, out);
      }
    }
    readRoutesInto(obj.member("ROUTE"), out.routes);
  }

  // Record an "IS": { "connect": [ { "@nodeField", "@protoField" } ] } block
  // found on a body-node wrapper object.
  void collectJsonIsConnections(const json::Value &wrapper,
                                const std::shared_ptr<X3DNode> &node,
                                runtime::ProtoBody &out) {
    if (wrapper.object.empty()) return;
    const json::Value *bodyObj = wrapper.object.front().second.get();
    if (!bodyObj || !bodyObj->isObject()) return;
    const json::Value *is = bodyObj->member("IS");
    if (!is || !is->isObject()) return;
    const json::Value *conns = is->member("connect");
    if (!conns) return;
    auto one = [&](const json::Value &c) {
      if (!c.isObject()) return;
      out.isConnections.push_back(
          {node, strMember(c, "@nodeField"), strMember(c, "@protoField")});
    };
    if (conns->isArray()) { for (const auto &c : conns->array) if (c) one(*c); }
    else one(*conns);
  }

  void readJsonProtoInstance(const json::Value &obj, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot, runtime::ProtoBody *body) {
    x3d::runtime::ProtoInstance inst;
    inst.name = strMember(obj, "@name");
    inst.DEF = strMember(obj, "@DEF");
    inst.USE = strMember(obj, "@USE");
    inst.containerField = strMemberOr(obj, "@containerField", "children");
    if (parent) inst.parent = parent;
    inst.parentField = slot.empty() ? inst.containerField : slot;
    if (auto d = scene.findProto(inst.name)) inst.declaration = d;
    else for (const auto &e : scene.externProtoDeclarations)
      if (e && e->name == inst.name) { inst.externDeclaration = e; break; }

    if (const json::Value *fvs = obj.member("fieldValue")) {
      auto one = [&](const json::Value &fv) {
        if (!fv.isObject()) return;
        runtime::ProtoFieldValue pv;
        pv.name = strMember(fv, "@name");
        const runtime::ProtoField *decl = nullptr;
        if (inst.declaration)
          for (const auto &f : inst.declaration->interface) if (f.name == pv.name) decl = &f;
        if (!decl && inst.externDeclaration)
          for (const auto &f : inst.externDeclaration->interface) if (f.name == pv.name) decl = &f;
        bool nodeTyped = decl && (decl->type == X3DFieldType::SFNode ||
                                  decl->type == X3DFieldType::MFNode);
        if (const json::Value *kids = fv.member("-children");
            (nodeTyped || (!fv.member("@value"))) && kids && kids->isArray()) {
          for (const auto &c : kids->array)
            if (c && c->isObject()) if (auto n = readNode(*c, scene)) pv.nodeValue.push_back(n);
        }
        if (pv.nodeValue.empty()) {
          if (const json::Value *v = fv.member("@value"); v && v->isString())
            pv.value = decl ? parseValue(decl->type, v->str) : std::any(v->str);
        }
        inst.fieldValues.push_back(std::move(pv));
      };
      if (fvs->isArray()) { for (const auto &fv : fvs->array) if (fv) one(*fv); }
      else one(*fvs);
    }

    if (body) body->nestedInstances.push_back(std::move(inst));
    else scene.protoInstances.push_back(std::move(inst));
  }
```

Add a `readRoutesInto(const json::Value*, std::vector<runtime::Route>&)` helper if the existing `readRoutes` only targets a `Scene`; otherwise factor the body of `readRoutes` to fill a `std::vector<Route>&` and have both call it. Reuse the reader's existing `strMember`; add `strMemberOr(obj, key, dflt)` if not present (return the string member or `dflt`). For `mapProtoFieldType`/`mapProtoAccessType`, reuse JsonReader's existing type/access mapping if it exposes one; otherwise copy the small `kMap`/access tables from `XmlReader.hpp:422-485`.

- [ ] **Step 4: Thread `currentProtoBody` through JsonReader's `readNode`, and intercept proto statements**

Mirror Task 4 in JSON. Change `readNode`'s signature (line ~224) to add `runtime::ProtoBody *currentProtoBody = nullptr`. In its pass-2 child loop (the `-childField` members, line ~262+), before treating a child object as a node, call `tryReadProtoStatement(childWrapper, scene, node, slot, currentProtoBody)` and `continue` if it returns true. Pass `currentProtoBody` into the recursive `readNode(childWrapper, scene, currentProtoBody)` call for ordinary child nodes. In `readScene`'s `-children` loop (line ~146), before `readNode`, call `tryReadProtoStatement(*c, out, nullptr, "", nullptr)` and `continue` if handled (so scene-scope ProtoDeclare/Instance are captured at the root). Scene-scope instances keep `body == nullptr`, so they land in `scene.protoInstances` exactly like XML.

- [ ] **Step 5: Run to verify it passes**

Run: `mise run build` then `ctest --preset dev -R x3d_json_proto --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 6: Commit**

```bash
git add runtime/parse/JsonReader.hpp runtime/parse/tests/json_proto_test.cpp CMakeLists.txt
git commit -m "Proto polish: JSON reader captures PROTO (declare/extern/instance/IS)"
```

---

## Task 7: VRML97 `.wrl` PROTO read — verify

**Files:**
- Create: `runtime/parse/tests/vrml97_proto_test.cpp`
- Modify: `CMakeLists.txt`
- Possibly modify: `runtime/parse/Vrml97Reader.hpp` / `Vrml97Dialect.hpp` only if the test surfaces a dialect gap

`Vrml97Reader : public ClassicVrmlReader` inherits the PROTO machinery whole. This task proves a `.wrl`-dialect PROTO reads and expands; touch code only if it fails.

- [ ] **Step 1: Write the test**

Create `runtime/parse/tests/vrml97_proto_test.cpp`:

```cpp
// vrml97_proto_test.cpp
// VRML97 (.wrl) PROTO capture is inherited from ClassicVrmlReader. Verify a
// VRML97-dialect PROTO declaration + instance reads and expands, including the
// VRML eventType spellings (field/exposedField/eventIn/eventOut).
#include "Vrml97Reader.hpp"
#include "X3DReflection.hpp"

#include <any>
#include <cassert>
#include <memory>
#include <string>

using namespace x3d;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

int main() {
  const char *wrl =
      "#VRML V2.0 utf8\n"
      "PROTO Ball [ field SFFloat r 2.0 ] {\n"
      "  Sphere { radius IS r }\n"
      "}\n"
      "Ball { r 7.0 }\n";
  codec::Vrml97Reader reader;
  runtime::X3DDocument doc = reader.readDocument(wrl);
  // After the front door expands, the instance becomes a Sphere with radius 7.
  // (Vrml97Reader::readDocument returns a parsed-but-unexpanded doc; run the
  // expansion the same way the front door does.)
  std::vector<runtime::ProtoWarning> warnings;
  runtime::expandScene(doc.scene, codec::noopProtoResolver, "", warnings);

  std::shared_ptr<X3DNode> root;
  for (auto &n : doc.scene.rootNodes) if (n) { root = n; break; }
  assert(root && root->nodeTypeName() == "Sphere");
  auto radius = std::any_cast<float>(fieldByName(*root, "radius")->get(*root));
  assert(radius == 7.0f);
  return 0;
}
```

Note: confirm the exact expansion entry the front door uses for `.wrl`. If `codec::parseDocument` dispatches `.wrl` text through `Vrml97Reader` AND runs `expandScene` itself, prefer calling `codec::parseDocument(wrl)` and dropping the manual `expandScene`. Check `runtime/parse/X3DParse.hpp` for how encoding is sniffed from a string vs. a path; if string-sniffing can't tell `.wrl` from `.x3dv`, keep the explicit `Vrml97Reader` + `expandScene` form above. Include whichever headers that resolves (`X3DProtoExpand.hpp` for `expandScene`/`noopProtoResolver`).

- [ ] **Step 2: Register + run**

Add to `CMakeLists.txt` after the `x3d_json_proto` block:

```cmake
    add_executable(x3d_vrml97_proto
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/vrml97_proto_test.cpp")
    target_link_libraries(x3d_vrml97_proto PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_vrml97_proto COMMAND x3d_vrml97_proto)
```

Run: `mise run build` then `ctest --preset dev -R x3d_vrml97_proto --output-on-failure`
Expected: PASS. If it FAILS on the `field SFFloat r` interface spelling, the gap is in `accessTypeFromString`/`fieldTypeFromString` for the VRML dialect — fix there (smallest change), then re-run. Do not restructure the reader.

- [ ] **Step 3: Commit**

```bash
git add runtime/parse/tests/vrml97_proto_test.cpp CMakeLists.txt
git commit -m "Proto polish: verify VRML97 .wrl PROTO read/expand (inherited path)"
```

---

## Task 8: XmlWriter — re-emit `<ProtoDeclare>` / `<ExternProtoDeclare>` bodies

**Files:**
- Modify: `runtime/codecs/XmlWriter.hpp` (`writeSceneInto` ~114-141; add declaration emitters)
- Modify: `runtime/codecs/tests/proto_roundtrip_test.cpp` (flip the body-suppression assertion)

- [ ] **Step 1: Update the round-trip test to require the declaration body**

In `runtime/codecs/tests/proto_roundtrip_test.cpp`, the current test asserts `out.find("<Box") == std::string::npos`. The instance's expansion Box is still suppressed (re-emitted as `<ProtoInstance>`), but the **declaration body** `<ProtoDeclare><ProtoBody><Box/>...` is now re-emitted. Replace the body of `main()` assertions with:

```cpp
  // The declaration is re-emitted (body included) AND the instance is re-emitted;
  // the EXPANSION's spliced Box is still suppressed via expandedSources.
  assert(out.find("<ProtoDeclare") != std::string::npos);
  assert(out.find("<ProtoBody") != std::string::npos);
  assert(out.find("<Box") != std::string::npos);     // the declaration body's Box
  assert(out.find("<ProtoInstance") != std::string::npos);
  assert(out.find("name=\"P\"") != std::string::npos);
  assert(out.find("DEF=\"A\"") != std::string::npos);
  // Exactly one Box element: the declaration body's, not a second from expansion.
  {
    std::size_t n = 0, p = 0;
    while ((p = out.find("<Box", p)) != std::string::npos) { ++n; p += 4; }
    assert(n == 1);
  }
```

- [ ] **Step 2: Run to verify it FAILS**

Run: `mise run build` then `ctest --preset dev -R x3d_proto_roundtrip --output-on-failure`
Expected: FAIL — `<ProtoDeclare` / `<ProtoBody` / `<Box` not found (declarations are not yet emitted).

- [ ] **Step 3: Emit declarations in `writeSceneInto`**

In `runtime/codecs/XmlWriter.hpp`, at the **top** of `writeSceneInto` (right after `scene_ = &s;`, before the `rootNodes` loop), emit declarations first (X3D requires them before use):

```cpp
    for (const auto &d : s.protoDeclarations)
      if (d) scene->children.push_back(writeProtoDeclareElement(*d));
    for (const auto &e : s.externProtoDeclarations)
      if (e) scene->children.push_back(writeExternProtoDeclareElement(*e));
```

Then add these private methods (next to `writeProtoInstanceElement`):

```cpp
  /// Emit a <field name type accessType [value]> element; node-typed defaults
  /// nest their default child node(s).
  std::unique_ptr<xml::Element> writeProtoFieldElement(const runtime::ProtoField &f) {
    auto fe = std::make_unique<xml::Element>();
    fe->name = "field";
    fe->setAttr("name", f.name);
    fe->setAttr("type", fieldTypeName(f.type));
    fe->setAttr("accessType", accessTypeName(f.access));
    if (!f.nodeDefault.empty()) {
      for (const auto &n : f.nodeDefault) {
        auto ce = writeNodeElement(n, "");
        if (ce) fe->children.push_back(std::move(ce));
      }
    } else if (f.value.has_value() &&
               (f.access == AccessType::InitializeOnly ||
                f.access == AccessType::InputOutput)) {
      fe->setAttr("value", formatValue(f.type, f.value));
    }
    return fe;
  }

  std::unique_ptr<xml::Element>
  writeProtoDeclareElement(const runtime::ProtoDeclaration &d) {
    auto el = std::make_unique<xml::Element>();
    el->name = "ProtoDeclare";
    el->setAttr("name", d.name);
    auto iface = std::make_unique<xml::Element>();
    iface->name = "ProtoInterface";
    for (const auto &f : d.interface)
      iface->children.push_back(writeProtoFieldElement(f));
    if (!iface->children.empty()) el->children.push_back(std::move(iface));

    auto body = std::make_unique<xml::Element>();
    body->name = "ProtoBody";
    // Re-emit the body TEMPLATE nodes (not any expansion clones). Use a fresh
    // writer state for the body so its DEF/USE bookkeeping is independent of the
    // surrounding scene's `seen_` set, and so expandedSources does not redirect
    // the template's own nodes.
    XmlWriter bodyWriter;
    for (const auto &n : d.body.nodes) {
      auto ne = bodyWriter.writeNodeElement(n, "");
      if (!ne) continue;
      attachIsBlocks(*ne, n, d.body.isConnections);
      body->children.push_back(std::move(ne));
    }
    for (const auto &r : d.body.routes) {
      xml::Element *re = body->addChild("ROUTE");
      re->setAttr("fromNode", r.fromNode); re->setAttr("fromField", r.fromField);
      re->setAttr("toNode", r.toNode);     re->setAttr("toField", r.toField);
    }
    el->children.push_back(std::move(body));
    return el;
  }

  std::unique_ptr<xml::Element>
  writeExternProtoDeclareElement(const runtime::ExternProtoDeclaration &d) {
    auto el = std::make_unique<xml::Element>();
    el->name = "ExternProtoDeclare";
    el->setAttr("name", d.name);
    if (!d.url.empty()) el->setAttr("url", formatMFString(d.url));
    for (const auto &f : d.interface)
      el->children.push_back(writeProtoFieldElement(f)); // extern: fields directly
    return el;
  }

  /// Append <IS><connect nodeField protoField/></IS> for every IsConnection
  /// whose body node matches `node` (by identity) onto its emitted element.
  static void attachIsBlocks(xml::Element &nodeEl, const std::shared_ptr<X3DNode> &node,
                             const std::vector<runtime::IsConnection> &isc) {
    xml::Element *is = nullptr;
    for (const auto &c : isc) {
      if (c.node.get() != node.get()) continue;
      if (!is) is = nodeEl.addChild("IS");
      xml::Element *conn = is->addChild("connect");
      conn->setAttr("nodeField", c.nodeField);
      conn->setAttr("protoField", c.protoField);
    }
  }
```

Add the small name-mapping helpers (inverse of `XmlReader::mapFieldType`/`mapAccessType`) as private statics — `fieldTypeName(X3DFieldType)`, `accessTypeName(AccessType)` — and a `formatMFString(const std::vector<std::string>&)` if not already available in `FieldValueIO.hpp` (check there first; reuse if present). `fieldTypeName` returns e.g. `"SFVec3f"`; `accessTypeName` returns `"initializeOnly"`/`"inputOutput"`/`"inputOnly"`/`"outputOnly"`.

Note on `attachIsBlocks` and IS on a deep body node: it matches by node identity across the whole `isConnections` list, so an IS on a node nested inside a body node still attaches to *that* node's emitted element (because `writeNodeElement` recurses and we call `attachIsBlocks` only on the top body node — for full depth, instead call `attachIsBlocks` during the recursive descent). For this push, attach at the top body node level (the common case in the corpus); deeper IS re-emit is a documented follow-up if a corpus file needs it.

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build` then `ctest --preset dev -R 'x3d_proto_roundtrip|x3d_xml_proto_capture' --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/codecs/XmlWriter.hpp runtime/codecs/tests/proto_roundtrip_test.cpp
git commit -m "Proto polish: XmlWriter re-emits ProtoDeclare/ExternProtoDeclare bodies"
```

---

## Task 9: JSON + VRML writers — `ProtoInstance` + `ProtoDeclare` re-emit parity

**Files:**
- Modify: `runtime/codecs/JsonWriter.hpp`, `runtime/codecs/VrmlWriter.hpp`
- Create: `runtime/codecs/tests/proto_writer_parity_test.cpp`
- Modify: `CMakeLists.txt`

Both writers gain: (a) a `const runtime::Scene *scene_` set during scene emit (mirroring XmlWriter) so `writeNode` can detect an expanded primary via `scene_->expandedSources` and re-emit the instance instead of descending; (b) declaration emit before the root nodes.

- [ ] **Step 1: Write the failing test**

Create `runtime/codecs/tests/proto_writer_parity_test.cpp`:

```cpp
// proto_writer_parity_test.cpp
// JSON and VRML writers must re-emit captured PROTO declarations and instances,
// suppressing the expansion's spliced primary — parity with XmlWriter.
#include "X3DParse.hpp"
#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"

#include <cassert>
#include <string>

using namespace x3d;

int main() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);

  std::string js = codec::JsonWriter().writeDocument(doc);
  assert(js.find("ProtoDeclare") != std::string::npos);
  assert(js.find("ProtoInstance") != std::string::npos);
  assert(js.find("\"@name\": \"P\"") != std::string::npos ||
         js.find("\"@name\":\"P\"") != std::string::npos);

  std::string vr = codec::VrmlWriter().writeDocument(doc);
  assert(vr.find("PROTO P") != std::string::npos);
  // The instance is re-emitted as `P { ... }` (a proto instance), not `Box {`.
  assert(vr.find("P {") != std::string::npos);
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL**

Add to `CMakeLists.txt` after the `x3d_vrml97_proto` block:

```cmake
    add_executable(x3d_proto_writer_parity
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/codecs/tests/proto_writer_parity_test.cpp")
    target_link_libraries(x3d_proto_writer_parity PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_proto_writer_parity COMMAND x3d_proto_writer_parity)
```

Run: `mise run build` then `ctest --preset dev -R x3d_proto_writer_parity --output-on-failure`
Expected: FAIL — neither writer emits proto output.

- [ ] **Step 3: JsonWriter — scene context + instance/declaration emit**

In `runtime/codecs/JsonWriter.hpp`:
1. Add a private `const runtime::Scene *scene_ = nullptr;` member.
2. In `writeDocument`, before `writeChildList(os, doc.scene.rootNodes, 4)`, set `scene_ = &doc.scene;` and emit declarations as the first members of `-children`. The cleanest structure that preserves valid JSON: build the `-children` array including declaration objects, instance objects, and node objects in order. Since `writeChildList` iterates `rootNodes` only, add a sibling helper `writeSceneChildren` that emits, in order: each `protoDeclarations` entry as `{ "ProtoDeclare": {...} }`, each `externProtoDeclarations` entry, then the existing root-node loop (delegating each node to `writeNode`). Wire `writeDocument` to call `writeSceneChildren` instead of `writeChildList` for the scene's `-children`.
3. In `writeNode`, at the very top (after `pad`), add:

```cpp
    if (scene_) {
      auto it = scene_->expandedSources.find(node.get());
      if (it != scene_->expandedSources.end()) {
        writeJsonProtoInstance(os, it->second, depth);
        return;
      }
    }
```

4. Add `writeJsonProtoInstance` (emits `{ "ProtoInstance": { "@name", "@DEF"?, "fieldValue":[{ "@name","@value" }] } }`, looking up each fieldValue's wire type via the instance's declaration interface like XmlWriter's `interfaceField`) and `writeJsonProtoDeclare` (emits `{ "ProtoDeclare": { "@name", "ProtoInterface": { "field":[...] }, "ProtoBody": { "-children":[...] } } }`, re-emitting body template nodes via `writeNode` with a fresh `JsonWriter` whose `scene_` is null so expandedSources does not redirect template nodes). Match the existing JSON formatting helpers (`jstr`, `pad`).

- [ ] **Step 4: VrmlWriter — scene context + instance/declaration emit**

In `runtime/codecs/VrmlWriter.hpp`:
1. Add `const runtime::Scene *scene_ = nullptr;`.
2. In `writeDocument`, set `scene_ = &doc.scene;` and, before the `rootNodes` loop, emit declarations:

```cpp
    for (const auto &d : doc.scene.protoDeclarations)
      if (d) writeVrmlProtoDeclare(os, *d);
    for (const auto &e : doc.scene.externProtoDeclarations)
      if (e) writeVrmlExternProtoDeclare(os, *e);
```

3. In `writeNode`, at the very top (before the `seen_` USE check), add:

```cpp
    if (scene_) {
      auto it = scene_->expandedSources.find(node.get());
      if (it != scene_->expandedSources.end()) {
        writeVrmlProtoInstance(os, it->second, depth);
        return;
      }
    }
```

4. Add the emitters using VRML syntax:
   - `writeVrmlProtoDeclare`: `PROTO <name> [ <accessType> <type> <field> [<value>] ... ] {\n <body nodes> <routes> }\n`. Re-emit body template nodes with a fresh `VrmlWriter` (null `scene_`); emit IS as `<bodyField> IS <protoField>` — for this push, emit IS lines for connections whose node is a top body node (consistent with Task 8's depth note).
   - `writeVrmlExternProtoDeclare`: `EXTERNPROTO <name> [ <interface> ] [ "url" ... ]\n`.
   - `writeVrmlProtoInstance`: `[DEF <DEF>] <name> { <field> <value> ... }\n`, value-typed via the declaration interface.
   Reuse `formatValue`, `pad`, and the inverse type/access name helpers (share the ones added in Task 8 by lifting them into a small shared header, or duplicate the tiny tables — prefer a shared `runtime/codecs/ProtoNameMaps.hpp` with `fieldTypeName`/`accessTypeName` free functions to keep it DRY across the three writers).

- [ ] **Step 5: Run to verify it passes**

Run: `mise run build` then `ctest --preset dev -R x3d_proto_writer_parity --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 6: Commit**

```bash
git add runtime/codecs/JsonWriter.hpp runtime/codecs/VrmlWriter.hpp \
        runtime/codecs/tests/proto_writer_parity_test.cpp CMakeLists.txt \
        runtime/codecs/ProtoNameMaps.hpp
git commit -m "Proto polish: JSON + VRML writers re-emit ProtoDeclare + ProtoInstance"
```

(If you did not create `ProtoNameMaps.hpp`, drop it from the `git add`.)

---

## Task 10: Full verification + corpus smoke

**Files:** none (verification only)

- [ ] **Step 1: Full build + test suite**

Run: `mise run build`
Expected: pytest all green (incl. `tests/test_golden_tree.py`), ctest all green (now includes `x3d_proto_nested_body`, `x3d_json_proto`, `x3d_vrml97_proto`, `x3d_proto_writer_parity`).

- [ ] **Step 2: Confirm the golden tree is byte-identical**

Run: `bash scripts/check_golden.sh` (or inspect the pytest golden-tree test output).
Expected: PASS — this push is runtime-only; the sorted-`*.hpp` sha256 must be unchanged from HEAD before this push. **If it changed, a runtime edit accidentally pulled into a generated template — stop and fix.**

- [ ] **Step 3: Build the corpus sweep harness**

Run:
```bash
g++ -std=c++20 -O1 -I generated_cpp_bindings -I runtime -I runtime/codecs \
  -I runtime/events -I runtime/parse scripts/corpus_sweep.cpp -o build/corpus_sweep
```
Expected: compiles clean.

- [ ] **Step 4: Run the corpus sweep per encoding and compare to baseline**

Run (corpus root `<x3d-render-workspace>/testdata`):
```bash
for ext in x3d x3dv wrl json; do
  ./build/corpus_sweep --ext $ext <x3d-render-workspace>/testdata | tail -3
done
```
Expected — **zero regressions** vs the standing baseline:
- XML (.x3d): 16886/16891
- ClassicVRML (.x3dv): 90/90
- VRML97 (.wrl): 687/688
- JSON (.json): 72/75

Counts must be **>=** baseline (nested-body + JSON-proto fixes may *increase* a bucket; they must not decrease any). If any count drops, bisect with `--dumpfails` to find the newly-failing file and fix before proceeding.

- [ ] **Step 5: Final verification summary commit (docs only, optional)**

If you keep a running progress note, record: tests added, final ctest count, golden sha256 (unchanged), and the four corpus counts. No code commit needed if everything above is green.

---

## Notes for the implementer

- **TDD discipline:** every task writes the test first and confirms RED before GREEN. Do not skip the RED step — it validates the test actually exercises the new path.
- **Golden gate is the safety net:** because nothing here is codegen, the golden sha256 must never move. Treat any movement as a bug in your edit, not an expected output change.
- **`x3d::runtime::ProtoInstance`** must be fully qualified in every new/edited TU (generated-node name collision).
- **Lenient-read ethos:** a malformed/partial PROTO must capture what it can and never throw out of the front door; mistyped value-forwards become `ProtoWarning`s (the engine already wraps them in try/catch — keep new reader code equally tolerant).
- **DRY the inverse name maps:** XmlReader has `mapFieldType`/`mapAccessType` (string→enum). The writers need the inverse (enum→string). Put the inverse in one place (`ProtoNameMaps.hpp` suggested) rather than three copies.
