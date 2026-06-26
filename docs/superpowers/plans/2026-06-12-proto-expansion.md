# PROTO / EXTERNPROTO Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eagerly expand `<ProtoInstance>` (local and file-resolved EXTERNPROTO) into concrete node trees at the parse front door, with full IS wiring (value forwarding + body routes + external-route redirection into the event cascade) and `<ProtoInstance>` round-trip preserved.

**Architecture:** Reflection-driven deep clone of the prototype body per instance; value-forward interface fields via the `FieldInfo` get/set thunks; pre-resolve body-internal ROUTEs to a dedicated `Scene.resolvedProtoRoutes` channel (respecting proto-local DEF scope); record interface→body redirects in `Scene.protoRedirects` for the bridge to rewrite external routes. The expanded primary node is spliced into its parent slot via a captured parent back-reference; a scene-side `expandedSources` map keeps the round-trip link off the generated `X3DNode` so the codegen golden stays byte-identical.

**Tech Stack:** C++20, header-only runtime (`runtime/`), generated reflection (`X3DNode::fields()`, `X3DNodeFactory::create`), CTest, the `mise run build` Ninja+ccache `dev` preset, pytest for golden/codegen gates.

**Spec:** `docs/superpowers/specs/2026-06-12-proto-expansion-design.md`

**Global guardrails (apply to every task):**
- **Runtime-only.** No edits under templates / generators / `generated_cpp_bindings/`. The golden tree must stay byte-identical (sha256 `223b73941e93e8bdd1779ffd12d7e35a64c65a986968d01eea6369bd4dd021f2`).
- Build/test with `mise run build` then `ctest --test-dir build --output-on-failure`. Use the job-pool cap (do **not** pass unbounded `-j` — the all-headers TU OOMs).
- New runtime test executables register via the existing CTest pattern (see `runtime/parse/tests/CMakeLists.txt` / the test wiring used by `classic_vrml_reader_test`). Each new `*_test.cpp` gets an `add_executable` + `add_test` entry mirroring a sibling.
- TDD: write the failing test, run it red, implement minimally, run it green, commit.

---

## File Structure

**New files**
- `runtime/X3DProtoClone.hpp` — reflection deep-clone (`deepClone`).
- `runtime/X3DProtoExpand.hpp` — expansion engine (`expandInstance`, `expandScene`, `ExpandGuard`).
- `runtime/parse/X3DProtoResolver.hpp` — `ProtoDeclarationResolver` typedef + `noopProtoResolver`.
- `runtime/tests/proto_clone_test.cpp`, `runtime/tests/proto_expand_test.cpp`, `runtime/tests/proto_roundtrip_test.cpp` — new test TUs (place beside existing runtime tests; mirror their CMake wiring).
- `runtime/parse/tests/data/proto/*` — EXTERN two-file fixtures.

**Modified files**
- `runtime/X3DProto.hpp` — `IsConnection`, `ProtoBody.isConnections`, `ProtoInstance.parent/parentField`, `ProtoWarning`, `ResolvedProtoRoute`, `ProtoRedirect`.
- `runtime/X3DScene.hpp` — `resolvedProtoRoutes`, `protoRedirects`, `expandedSources`.
- `runtime/X3DDocument.hpp` — `protoWarnings`.
- `runtime/parse/ClassicVrmlReader.hpp` — capture IS + instance parent linkage.
- `runtime/codecs/XmlReader.hpp` — capture ProtoDeclare/Interface/Body/IS/Extern/Instance + parent linkage.
- `runtime/parse/X3DParse.hpp` — `baseUrl` param, default `localFileProtoResolver`, `expandScene` call, `protoWarnings` collection.
- `runtime/events/X3DSceneBridge.hpp` — add `resolvedProtoRoutes`, redirect external routes via `protoRedirects`.
- `runtime/codecs/XmlWriter.hpp` — re-emit `<ProtoInstance>` from `expandedSources`.

---

## Task 1: Data-model additions

**Files:**
- Modify: `runtime/X3DProto.hpp`
- Modify: `runtime/X3DScene.hpp:43-50` (proto storage block)
- Modify: `runtime/X3DDocument.hpp:84` (after `rangeWarnings`)
- Test: `runtime/tests/proto_clone_test.cpp` (compile-only assertion of the new shapes)

- [ ] **Step 1: Write the failing test** (a struct-shape smoke test; the real clone test lands in Task 2 in the same file — start the file here).

```cpp
// runtime/tests/proto_clone_test.cpp
#include "X3DDocument.hpp"
#include "X3DProto.hpp"
#include <cassert>
using namespace x3d::runtime;

int main() {
  // Data-model shapes exist and default sanely.
  ProtoBody body;
  body.isConnections.push_back({nullptr, "size", "size"});
  assert(body.isConnections.size() == 1);

  ProtoInstance inst;
  inst.parentField = "geometry";
  assert(inst.parent.expired());            // weak_ptr, empty by default

  Scene scene;
  assert(scene.resolvedProtoRoutes.empty());
  assert(scene.protoRedirects.empty());
  assert(scene.expandedSources.empty());

  X3DDocument doc;
  assert(doc.protoWarnings.empty());

  ProtoWarning w{ProtoWarning::Kind::UnresolvedExtern, "Foo", "no url resolved"};
  assert(w.kind == ProtoWarning::Kind::UnresolvedExtern);
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` (expected: compile error — `isConnections`, `parent`, `resolvedProtoRoutes`, `protoRedirects`, `expandedSources`, `protoWarnings`, `ProtoWarning` undefined).

- [ ] **Step 3: Add the data-model members.**

In `runtime/X3DProto.hpp`, after the `IsConnection`-less `ProtoBody` definition, add the struct and members. Add near the top of the `x3d::runtime` namespace:

```cpp
/// One `field IS protoField` / <connect> mapping captured from a PROTO body.
struct IsConnection {
  std::shared_ptr<X3DNode> node;   // the body node carrying the field
  std::string nodeField;           // its field name
  std::string protoField;          // the interface field it maps to
};

/// A body-internal ROUTE pre-resolved to concrete cloned endpoints. Holds
/// shared_ptr (not raw FieldAddress) so auxiliary body nodes stay alive.
struct ResolvedProtoRoute {
  std::shared_ptr<X3DNode> from;
  std::string fromField;
  std::shared_ptr<X3DNode> to;
  std::string toField;
};

/// One redirect target for an exposed interface event field.
struct ProtoRedirect {
  std::shared_ptr<X3DNode> targetNode;  // cloned body node (keep-alive)
  std::string targetField;
};

/// Non-fatal PROTO diagnostic, collected into X3DDocument.protoWarnings.
struct ProtoWarning {
  enum class Kind {
    UnresolvedExtern, MissingDeclaration, InterfaceMismatch,
    RecursionLimit, UnknownField
  };
  Kind kind;
  std::string instanceName;
  std::string detail;
};
```

Add to `ProtoBody`:

```cpp
  std::vector<IsConnection> isConnections;
```

Add to `ProtoInstance` (public members, near `containerField`):

```cpp
  // Placement: where this instance sits in the graph so expansion can splice
  // the primary node back in. Empty `parent` => the instance is a Scene root.
  std::weak_ptr<X3DNode> parent;
  std::string parentField;          // containerField slot on `parent`
```

In `runtime/X3DScene.hpp`, in the proto-storage block (around line 50), add:

```cpp
  // Body-internal ROUTEs of expanded instances, pre-resolved to concrete
  // endpoints (proto-local DEF scope; NOT registered in `defs`).
  std::vector<ResolvedProtoRoute> resolvedProtoRoutes;

  // Exposed interface event fields -> IS-mapped body endpoints, keyed by the
  // expanded primary node pointer then the interface field name. The bridge
  // consults this to redirect external routes that target an instance.
  std::unordered_map<X3DNode *,
                     std::unordered_map<std::string, std::vector<ProtoRedirect>>>
      protoRedirects;

  // Expanded primary node -> its source ProtoInstance, for <ProtoInstance>
  // round-trip on write. Scene-side so the generated X3DNode is untouched.
  std::unordered_map<X3DNode *, ProtoInstance> expandedSources;
```

In `runtime/X3DDocument.hpp`, after `std::vector<RangeDiagnostic> rangeWarnings;`:

```cpp
  // PROTO/EXTERNPROTO expansion diagnostics (unresolved extern, missing decl,
  // interface mismatch, recursion cap), populated by the X3DParse front door.
  std::vector<ProtoWarning> protoWarnings;
```

(`X3DProto.hpp` already includes `<memory>`/`<vector>`/`<string>`; ensure `<unordered_map>` is included where `protoRedirects`/`expandedSources` are declared — `X3DScene.hpp` already includes it.)

- [ ] **Step 4: Run test to verify it passes**

Run: `mise run build && ./build/<path>/proto_clone_test` (expected: exit 0). Wire `proto_clone_test` into CTest mirroring a sibling test's `add_executable`/`add_test`.

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProto.hpp runtime/X3DScene.hpp runtime/X3DDocument.hpp \
        runtime/tests/proto_clone_test.cpp runtime/tests/CMakeLists.txt
git commit -m "Proto expansion: data-model additions (IS, redirects, warnings, parent link)"
```

---

## Task 2: Reflection deep-clone — `X3DProtoClone.hpp`

**Files:**
- Create: `runtime/X3DProtoClone.hpp`
- Test: `runtime/tests/proto_clone_test.cpp` (extend with the real clone test)

- [ ] **Step 1: Write the failing test** (append a second TU is overkill; instead add a `cloneTest()` and call it from `main` in the existing file).

```cpp
// add to runtime/tests/proto_clone_test.cpp, include "X3DProtoClone.hpp"
#include "X3DProtoClone.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DReflection.hpp"
#include <any>

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

static void cloneTest() {
  auto shape = createX3DNode("Shape");
  shape->setDEF("S");
  auto group = createX3DNode("Group");
  // children = [shape, shape]  (a USE: same pointer twice)
  const FieldInfo *kids = fieldByName(*group, "children");
  assert(kids && kids->set);
  kids->set(*group, std::any(std::vector<std::shared_ptr<X3DNode>>{shape, shape}));

  auto clone = deepClone(group);
  assert(clone && clone.get() != group.get());
  const FieldInfo *cKids = fieldByName(*clone, "children");
  auto out = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(cKids->get(*clone));
  assert(out.size() == 2);
  assert(out[0].get() == out[1].get());      // USE sharing preserved
  assert(out[0].get() != shape.get());        // genuinely a clone
  assert(out[0]->getDEF() == "S");            // DEF copied
}
```

Call `cloneTest();` from `main` before `return 0;`.

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` — expected: compile error, `X3DProtoClone.hpp` / `deepClone` not found.

- [ ] **Step 3: Implement `deepClone`.**

```cpp
// runtime/X3DProtoClone.hpp
#ifndef X3D_RUNTIME_PROTO_CLONE_HPP
#define X3D_RUNTIME_PROTO_CLONE_HPP

#include "X3DNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DReflection.hpp"

#include <any>
#include <memory>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

/// Deep-clone a node tree. `cloneMap` (original ptr -> clone) preserves
/// intra-tree DEF/USE shared identity: a node referenced twice clones once.
inline std::shared_ptr<X3DNode>
deepClone(const std::shared_ptr<X3DNode> &src,
          std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> &cloneMap) {
  if (!src) return nullptr;
  auto it = cloneMap.find(src.get());
  if (it != cloneMap.end()) return it->second;       // USE: same clone

  std::shared_ptr<X3DNode> dst = X3DNodeFactory::create(src->nodeTypeName());
  if (!dst) return nullptr;                            // unknown type: drop
  cloneMap[src.get()] = dst;
  dst->setDEF(src->getDEF());

  for (const FieldInfo &f : src->fields()) {
    if (!f.get || !f.set) continue;                    // event-only/read-only
    if (f.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*src));
      f.set(*dst, std::any(deepClone(child, cloneMap)));
    } else if (f.type == X3DFieldType::MFNode) {
      auto kids =
          std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*src));
      std::vector<std::shared_ptr<X3DNode>> out;
      out.reserve(kids.size());
      for (auto &k : kids) out.push_back(deepClone(k, cloneMap));
      f.set(*dst, std::any(std::move(out)));
    } else {
      f.set(*dst, f.get(*src));                         // scalar: copy boxed any
    }
  }
  return dst;
}

inline std::shared_ptr<X3DNode> deepClone(const std::shared_ptr<X3DNode> &src) {
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> m;
  return deepClone(src, m);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_CLONE_HPP
```

- [ ] **Step 4: Run test to verify it passes**

Run: `mise run build && ./build/<path>/proto_clone_test` (expected: exit 0).

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProtoClone.hpp runtime/tests/proto_clone_test.cpp
git commit -m "Proto expansion: reflection deep-clone with USE-sharing preservation"
```

---

## Task 3: Resolver seam typedef — `X3DProtoResolver.hpp`

**Files:**
- Create: `runtime/parse/X3DProtoResolver.hpp`

(The default file-IO resolver, `localFileProtoResolver`, lands in Task 10 inside `X3DParse.hpp` because it calls `parseFile` — defining it here would create an include cycle.)

- [ ] **Step 1: Create the header** (no separate test; exercised by Task 6).

```cpp
// runtime/parse/X3DProtoResolver.hpp
#ifndef X3D_PARSE_PROTO_RESOLVER_HPP
#define X3D_PARSE_PROTO_RESOLVER_HPP

#include "X3DProto.hpp"   // ProtoDeclaration

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace x3d::codec {

/// Resolve an EXTERNPROTO declaration from its `url` list (relative to
/// `baseUrl`). Returns the matched ProtoDeclaration, or null if no candidate
/// resolves (e.g. http/urn with no embedder override). Implementations must
/// not throw on a missing/unreachable url — return null and let the caller
/// record a ProtoWarning (lenient-read policy).
using ProtoDeclarationResolver =
    std::function<std::shared_ptr<x3d::runtime::ProtoDeclaration>(
        const std::vector<std::string> &urls, const std::string &baseUrl)>;

/// Default no-op resolver: resolves nothing. Used when expansion runs without a
/// configured resolver (local PROTOs still expand; EXTERN instances warn).
inline std::shared_ptr<x3d::runtime::ProtoDeclaration>
noopProtoResolver(const std::vector<std::string> &, const std::string &) {
  return nullptr;
}

} // namespace x3d::codec

#endif // X3D_PARSE_PROTO_RESOLVER_HPP
```

- [ ] **Step 2: Build to verify it compiles**

Run: `mise run build` (expected: success; header included by nothing yet — add a throwaway `#include` in `proto_clone_test.cpp` to force compilation, then remove, OR rely on Task 4 picking it up).

- [ ] **Step 3: Commit**

```bash
git add runtime/parse/X3DProtoResolver.hpp
git commit -m "Proto expansion: ProtoDeclarationResolver seam typedef + noop default"
```

---

## Task 4: Expansion engine — local PROTO value forwarding

**Files:**
- Create: `runtime/X3DProtoExpand.hpp`
- Test: `runtime/tests/proto_expand_test.cpp`

- [ ] **Step 1: Write the failing test.**

```cpp
// runtime/tests/proto_expand_test.cpp
#include "X3DDocument.hpp"
#include "X3DProto.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DReflection.hpp"
#include <any>
#include <cassert>
using namespace x3d::runtime;
using x3d::codec::noopProtoResolver;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

// Local PROTO "Param": interface size (SFVec3f, initializeOnly, default 2,2,2),
// body = [ Box ] with size IS size. Instance overrides size = 5,5,5.
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

  ProtoInstance inst;
  inst.name = "Param";
  inst.declaration = decl;
  ProtoFieldValue fv; fv.name = "size";
  fv.value = std::any(SFVec3f{5.f, 5.f, 5.f});
  inst.fieldValues.push_back(fv);

  Scene scene;
  ExpandGuard guard;
  std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, guard, warnings);

  assert(primary && primary->nodeTypeName() == "Box");
  auto sz = std::any_cast<SFVec3f>(fieldByName(*primary, "size")->get(*primary));
  assert(sz.x == 5.f && sz.y == 5.f && sz.z == 5.f);   // override forwarded
  assert(warnings.empty());
}

int main() { localValueForwardTest(); return 0; }
```

(Confirm the `SFVec3f` value type and accessor names against `X3Dtypes.hpp`; adjust the boxed type if the project's SFVec3f differs.)

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` — expected: `X3DProtoExpand.hpp` / `expandInstance` / `ExpandGuard` undefined.

- [ ] **Step 3: Implement the engine (local-only slice).**

```cpp
// runtime/X3DProtoExpand.hpp
#ifndef X3D_RUNTIME_PROTO_EXPAND_HPP
#define X3D_RUNTIME_PROTO_EXPAND_HPP

#include "X3DProtoClone.hpp"
#include "X3DScene.hpp"
#include "parse/X3DProtoResolver.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

/// Recursion/cycle guard threaded through nested expansion.
struct ExpandGuard {
  int depth = 0;
  int maxDepth = 32;
};

namespace proto_detail {

inline const FieldInfo *findField(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

inline const ProtoField *interfaceField(const ProtoDeclaration &d,
                                        const std::string &name) {
  for (const auto &p : d.interface) if (p.name == name) return &p;
  return nullptr;
}

inline const ProtoFieldValue *instanceValue(const ProtoInstance &inst,
                                             const std::string &name) {
  for (const auto &v : inst.fieldValues) if (v.name == name) return &v;
  return nullptr;
}

} // namespace proto_detail

/// Expand a single instance into its primary cloned node. Local declarations
/// only at this stage (EXTERN resolution + routes/redirects land in later
/// tasks). Returns null if no declaration is available.
inline std::shared_ptr<X3DNode>
expandInstance(ProtoInstance &inst, Scene &scene,
               const x3d::codec::ProtoDeclarationResolver &resolver,
               ExpandGuard &guard, std::vector<ProtoWarning> &warnings) {
  (void)resolver; // used in Task 6
  std::shared_ptr<ProtoDeclaration> decl = inst.declaration;
  if (!decl) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "no local declaration"});
    return nullptr;
  }

  // Clone the body; primary = clone of the first body node.
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> cloneMap;
  if (decl->body.nodes.empty()) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "empty proto body"});
    return nullptr;
  }
  std::shared_ptr<X3DNode> primary;
  for (const auto &bn : decl->body.nodes) {
    auto c = deepClone(bn, cloneMap);
    if (!primary) primary = c;
  }
  if (primary) primary->setDEF(inst.DEF);

  // Value-forward initializeOnly / inputOutput interface fields onto the
  // cloned body fields named by each IS connection.
  for (const IsConnection &is : decl->body.isConnections) {
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end()) continue;
    X3DNode &cloned = *cit->second;
    const ProtoField *pf = proto_detail::interfaceField(*decl, is.protoField);
    if (!pf) {
      warnings.push_back(
          {ProtoWarning::Kind::UnknownField, inst.name, is.protoField});
      continue;
    }
    if (pf->access != AccessType::InitializeOnly &&
        pf->access != AccessType::InputOutput)
      continue; // event fields handled by redirects (Task 5)
    const FieldInfo *fi = proto_detail::findField(cloned, is.nodeField);
    if (!fi || !fi->set) continue;

    const ProtoFieldValue *ov = proto_detail::instanceValue(inst, pf->name);
    if (ov && ov->value.has_value()) {
      fi->set(cloned, ov->value);
    } else if (ov && !ov->nodeValue.empty()) {
      // SFNode/MFNode override from the instance (already real nodes).
      if (fi->type == X3DFieldType::SFNode)
        fi->set(cloned, std::any(ov->nodeValue.front()));
      else if (fi->type == X3DFieldType::MFNode)
        fi->set(cloned, std::any(ov->nodeValue));
    } else if (pf->value.has_value()) {
      fi->set(cloned, pf->value);
    } else if (!pf->nodeDefault.empty()) {
      if (fi->type == X3DFieldType::SFNode)
        fi->set(cloned, std::any(pf->nodeDefault.front()));
      else if (fi->type == X3DFieldType::MFNode)
        fi->set(cloned, std::any(pf->nodeDefault));
    }
  }
  return primary;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_EXPAND_HPP
```

- [ ] **Step 4: Run test to verify it passes**

Run: `mise run build && ./build/<path>/proto_expand_test` (expected: exit 0). Wire `proto_expand_test` into CTest.

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProtoExpand.hpp runtime/tests/proto_expand_test.cpp runtime/tests/CMakeLists.txt
git commit -m "Proto expansion: local instance expansion + IS value forwarding"
```

---

## Task 5: Body-internal routes + interface redirects

**Files:**
- Modify: `runtime/X3DProtoExpand.hpp` (extend `expandInstance`)
- Test: `runtime/tests/proto_expand_test.cpp`

- [ ] **Step 1: Write the failing test** (add `routesAndRedirectsTest()` and call it from `main`).

```cpp
static void routesAndRedirectsTest() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Anim";
  // interface: value_changed (SFVec3f, outputOnly)
  ProtoField out; out.name = "value_changed";
  out.type = X3DFieldType::SFVec3f; out.access = AccessType::OutputOnly;
  decl->interface.push_back(out);

  auto ts = createX3DNode("TimeSensor");        ts->setDEF("TS");
  auto pi = createX3DNode("PositionInterpolator"); pi->setDEF("PI");
  decl->body.nodes.push_back(ts);
  decl->body.nodes.push_back(pi);
  // internal ROUTE TS.fraction_changed -> PI.set_fraction
  decl->body.routes.push_back(Route{"TS", "fraction_changed", "PI", "set_fraction"});
  // PI.value_changed IS value_changed
  decl->body.isConnections.push_back({pi, "value_changed", "value_changed"});

  ProtoInstance inst; inst.name = "Anim"; inst.declaration = decl; inst.DEF = "A";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(inst, scene, noopProtoResolver, guard, warnings);
  assert(primary);

  // body route pre-resolved to concrete cloned endpoints
  assert(scene.resolvedProtoRoutes.size() == 1);
  const auto &r = scene.resolvedProtoRoutes[0];
  assert(r.from && r.from->nodeTypeName() == "TimeSensor" && r.fromField == "fraction_changed");
  assert(r.to && r.to->nodeTypeName() == "PositionInterpolator" && r.toField == "set_fraction");

  // redirect: interface value_changed -> cloned PI.value_changed
  auto &byField = scene.protoRedirects[primary.get()];
  assert(byField.count("value_changed") == 1);
  assert(byField["value_changed"].size() == 1);
  assert(byField["value_changed"][0].targetField == "value_changed");
}
```

(If `Route`'s constructor signature differs, match `runtime/X3DRoute.hpp` — it is `Route(fromN, fromF, toN, toF)`.)

- [ ] **Step 2: Run test red** — `mise run build && ./build/<path>/proto_expand_test` → fails the new asserts.

- [ ] **Step 3: Extend `expandInstance`** — before `return primary;`, add:

```cpp
  // Build a body-local DEF table from the cloned nodes (proto-local scope).
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> localDefs;
  for (const auto &kv : cloneMap) {
    const std::string def = kv.first->getDEF();
    if (!def.empty()) localDefs[def] = kv.second;
  }

  // Pre-resolve body-internal ROUTEs to concrete endpoints.
  for (const Route &br : decl->body.routes) {
    auto fIt = localDefs.find(br.fromNode);
    auto tIt = localDefs.find(br.toNode);
    if (fIt == localDefs.end() || tIt == localDefs.end()) continue;
    scene.resolvedProtoRoutes.push_back(
        {fIt->second, br.fromField, tIt->second, br.toField});
  }

  // Interface event redirects (inputOnly / outputOnly / inputOutput).
  for (const IsConnection &is : decl->body.isConnections) {
    const ProtoField *pf = proto_detail::interfaceField(*decl, is.protoField);
    if (!pf) continue;
    if (pf->access == AccessType::InitializeOnly) continue; // value-only
    auto cit = cloneMap.find(is.node.get());
    if (cit == cloneMap.end()) continue;
    scene.protoRedirects[primary.get()][is.protoField].push_back(
        {cit->second, is.nodeField});
  }
```

- [ ] **Step 4: Run test green** — `mise run build && ./build/<path>/proto_expand_test` → exit 0.

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProtoExpand.hpp runtime/tests/proto_expand_test.cpp
git commit -m "Proto expansion: pre-resolved body routes + interface event redirects"
```

---

## Task 6: EXTERNPROTO resolution + recursion guard

**Files:**
- Modify: `runtime/X3DProtoExpand.hpp` (`expandInstance` declaration branch)
- Test: `runtime/tests/proto_expand_test.cpp`

- [ ] **Step 1: Write the failing test** (stub resolver — no file IO).

```cpp
static void externResolveTest() {
  // EXTERN instance with no local declaration; a stub resolver supplies one.
  auto inst = std::make_shared<ProtoInstance>();
  inst->name = "ExtBox";
  auto ext = std::make_shared<ExternProtoDeclaration>();
  ext->name = "ExtBox"; ext->url = {"shapes.x3d#ExtBox"};
  inst->externDeclaration = ext;

  auto resolver = [](const std::vector<std::string> &urls,
                     const std::string &) -> std::shared_ptr<ProtoDeclaration> {
    assert(!urls.empty());
    auto d = std::make_shared<ProtoDeclaration>();
    d->name = "ExtBox";
    d->body.nodes.push_back(createX3DNode("Box"));
    return d;
  };

  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> warnings;
  auto primary = expandInstance(*inst, scene, resolver, guard, warnings);
  assert(primary && primary->nodeTypeName() == "Box");
  assert(warnings.empty());

  // Unresolved (resolver returns null) -> warning, no throw, null primary.
  Scene s2; ExpandGuard g2; std::vector<ProtoWarning> w2;
  auto none = expandInstance(*inst, s2, x3d::codec::noopProtoResolver, g2, w2);
  assert(!none && w2.size() == 1 &&
         w2[0].kind == ProtoWarning::Kind::UnresolvedExtern);

  // Depth cap -> RecursionLimit warning.
  Scene s3; ExpandGuard g3; g3.depth = g3.maxDepth; std::vector<ProtoWarning> w3;
  auto capped = expandInstance(*inst, s3, resolver, g3, w3);
  assert(!capped && w3.size() == 1 &&
         w3[0].kind == ProtoWarning::Kind::RecursionLimit);
}
```

- [ ] **Step 2: Run test red.**

- [ ] **Step 3: Implement EXTERN branch + guard.** Replace the `if (!decl)` block at the top of `expandInstance` with:

```cpp
  if (guard.depth >= guard.maxDepth) {
    warnings.push_back(
        {ProtoWarning::Kind::RecursionLimit, inst.name, "max expansion depth"});
    return nullptr;
  }
  std::shared_ptr<ProtoDeclaration> decl = inst.declaration;
  if (!decl && inst.externDeclaration) {
    decl = resolver(inst.externDeclaration->url, /*baseUrl set by caller*/ "");
    if (!decl) {
      warnings.push_back(
          {ProtoWarning::Kind::UnresolvedExtern, inst.name,
           inst.externDeclaration->url.empty() ? "no url"
                                               : inst.externDeclaration->url.front()});
      return nullptr;
    }
  }
  if (!decl) {
    warnings.push_back(
        {ProtoWarning::Kind::MissingDeclaration, inst.name, "no declaration"});
    return nullptr;
  }
```

Note: `baseUrl` threading into the resolver from `expandScene` is wired in Task 7 (add a `const std::string& baseUrl` parameter to `expandInstance` then). For this task the stub ignores `baseUrl`, so pass `""`.

- [ ] **Step 4: Run test green.**

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProtoExpand.hpp runtime/tests/proto_expand_test.cpp
git commit -m "Proto expansion: EXTERNPROTO resolver branch + recursion guard"
```

---

## Task 7: `expandScene` walk + splice into parent slot

**Files:**
- Modify: `runtime/X3DProtoExpand.hpp` (add `expandScene`; add `baseUrl` param to `expandInstance`)
- Test: `runtime/tests/proto_expand_test.cpp`

- [ ] **Step 1: Write the failing test** (root + nested placement, and round-trip source recorded).

```cpp
static void expandSceneSpliceTest() {
  Scene scene;
  // Shared local declaration.
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "P"; decl->body.nodes.push_back(createX3DNode("Box"));
  scene.protoDeclarations.push_back(decl);

  // Root-level instance.
  ProtoInstance rootInst; rootInst.name = "P"; rootInst.declaration = decl;
  rootInst.DEF = "R";
  scene.protoInstances.push_back(rootInst);

  // Nested instance: child of a Shape in the `geometry` slot.
  auto shape = createX3DNode("Shape");
  scene.addRootNode(shape);
  ProtoInstance nested; nested.name = "P"; nested.declaration = decl;
  nested.parent = shape; nested.parentField = "geometry";
  scene.protoInstances.push_back(nested);

  std::vector<ProtoWarning> warnings;
  expandScene(scene, x3d::codec::noopProtoResolver, "", warnings);

  // Root instance -> a Box root node, DEF "R".
  bool foundRootBox = false;
  for (auto &n : scene.rootNodes)
    if (n && n->nodeTypeName() == "Box" && n->getDEF() == "R") foundRootBox = true;
  assert(foundRootBox);

  // Nested -> spliced into shape.geometry.
  const FieldInfo *geo = fieldByName(*shape, "geometry");
  auto g = std::any_cast<std::shared_ptr<X3DNode>>(geo->get(*shape));
  assert(g && g->nodeTypeName() == "Box");

  // round-trip source recorded for both primaries
  assert(scene.expandedSources.size() == 2);
}
```

- [ ] **Step 2: Run test red.**

- [ ] **Step 3: Add `baseUrl` to `expandInstance`** (signature: insert `const std::string &baseUrl` before `ExpandGuard &guard`; pass it to `resolver(...)` instead of `""`). Then add `expandScene`:

```cpp
namespace proto_detail {

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

} // namespace proto_detail

/// Expand every captured ProtoInstance in `scene`, splicing primaries into
/// place. Front-door entry point. Collects diagnostics into `warnings`.
inline void expandScene(Scene &scene,
                        const x3d::codec::ProtoDeclarationResolver &resolver,
                        const std::string &baseUrl,
                        std::vector<ProtoWarning> &warnings) {
  for (ProtoInstance &inst : scene.protoInstances) {
    ExpandGuard guard;
    auto primary = expandInstance(inst, scene, baseUrl, guard, warnings);
    if (!primary) continue;
    auto parent = inst.parent.lock();
    if (!parent)
      scene.addRootNode(primary);
    else
      proto_detail::attachToParent(parent, inst.parentField, primary);
    scene.expandedSources[primary.get()] = inst;
  }
}
```

Update the Task 4/6 tests' direct `expandInstance(...)` calls to pass `""` for the new `baseUrl` argument.

> **Nested-in-body instances note:** instances that appear *inside another PROTO's body* are an edge case. They are not in `scene.protoInstances` (they live in `decl->body`). Handling them requires recursing in `expandInstance` after cloning; this is deferred to a fast-follow and recorded as a `ProtoWarning` only if encountered. The common cases (root-level and node-tree-nested instances) are covered here.

- [ ] **Step 4: Run test green.**

- [ ] **Step 5: Commit**

```bash
git add runtime/X3DProtoExpand.hpp runtime/tests/proto_expand_test.cpp
git commit -m "Proto expansion: expandScene walk + splice into parent slot + source link"
```

---

## Task 8: VRML/ClassicVRML capture — IS + instance parent linkage

**Files:**
- Modify: `runtime/parse/ClassicVrmlReader.hpp` (IS site ~line 409; proto-instance site ~line 272-277)
- Test: `runtime/parse/tests/classic_vrml_reader_test.cpp`

- [ ] **Step 1: Write the failing test** (a `.x3dv` snippet string parsed via the reader; assert IS captured and instance parent recorded).

```cpp
// add to classic_vrml_reader_test.cpp
static void protoIsCaptureTest() {
  const char *src =
    "#X3D V4.0 utf8\n"
    "PROTO Param [ initializeOnly SFVec3f size 2 2 2 ] {\n"
    "  Box { size IS size }\n"
    "}\n"
    "Transform { children [ Param { size 5 5 5 } ] }\n";
  ClassicVrmlReader reader;
  auto doc = reader.readDocument(src);
  auto &scene = doc.getScene();
  assert(scene.protoDeclarations.size() == 1);
  auto &decl = *scene.protoDeclarations[0];
  assert(decl.body.isConnections.size() == 1);
  assert(decl.body.isConnections[0].nodeField == "size");
  assert(decl.body.isConnections[0].protoField == "size");
  assert(decl.body.isConnections[0].node &&
         decl.body.isConnections[0].node->nodeTypeName() == "Box");
  // instance recorded with its parent Transform + children slot
  assert(scene.protoInstances.size() == 1);
  auto &inst = scene.protoInstances[0];
  assert(!inst.parent.expired());
  assert(inst.parent.lock()->nodeTypeName() == "Transform");
  assert(inst.parentField == "children");
}
```

- [ ] **Step 2: Run test red.**

- [ ] **Step 3: Capture IS.** At `ClassicVrmlReader.hpp:409` the reader currently does `expectWord(tok, "IS proto-field name"); // consumed; mapping not wired`. The enclosing `parseNodeBody` knows the current node being built and must have access to the ProtoBody under construction. Thread a `ProtoBody *currentProtoBody` (default null when not inside a PROTO) through `parseNode`/`parseNodeBody`, set non-null while parsing a `parseProtoDeclare` body. Replace the consume-and-drop with:

```cpp
      if (tok.peek().isWord("IS")) {
        tok.next();
        std::string protoField = expectWord(tok, "IS proto-field name");
        if (currentProtoBody)
          currentProtoBody->isConnections.push_back(
              {currentNode, fieldName, protoField});
        continue;
      }
```

(`currentNode` = the `shared_ptr<X3DNode>` for the body being filled; `fieldName` = the field name already read at the head of the statement. Match the exact local variable names in `parseNodeBody`.)

- [ ] **Step 4: Capture instance parent linkage.** At `ClassicVrmlReader.hpp:272-277`, `parseProtoInstance` records the instance but with no parent. Pass the parent context into `parseNode` (the caller in `parseNodeBody` knows `currentNode` + the containerField slot it is filling; the root loop passes null). After `parseProtoInstance` appends to `scene.protoInstances`, set the just-added instance's `parent`/`parentField` from the call site. Representative:

```cpp
    if (scene.findProto(rawTypeName) || findExternProto(scene, rawTypeName)) {
      parseProtoInstance(tok, scene, def, rawTypeName);
      if (!scene.protoInstances.empty()) {
        scene.protoInstances.back().parent = parentNode;   // null at root
        scene.protoInstances.back().parentField = parentField;
      }
      return nullptr;
    }
```

where `parentNode`/`parentField` are new parameters to `parseNode` (defaulted empty for the root call at line 242).

- [ ] **Step 5: Run test green, then full reader suite.**

Run: `mise run build && ./build/<path>/classic_vrml_reader_test` (expected: exit 0, no regressions).

- [ ] **Step 6: Commit**

```bash
git add runtime/parse/ClassicVrmlReader.hpp runtime/parse/tests/classic_vrml_reader_test.cpp
git commit -m "Proto expansion: ClassicVRML captures IS mappings + instance parent linkage"
```

---

## Task 9: XML capture — ProtoDeclare / Interface / Body / IS / Extern / Instance

**Files:**
- Modify: `runtime/codecs/XmlReader.hpp` (element dispatch; currently skips unknown at ~line 174)
- Test: `runtime/codecs/tests/<xml reader test>.cpp` (mirror existing XmlReader test TU)

- [ ] **Step 1: Write the failing test.**

```cpp
static void xmlProtoCaptureTest() {
  const char *xml =
    "<X3D profile='Interchange' version='4.0'><Scene>"
    "<ProtoDeclare name='Param'>"
    "  <ProtoInterface>"
    "    <field name='size' type='SFVec3f' accessType='initializeOnly' value='2 2 2'/>"
    "  </ProtoInterface>"
    "  <ProtoBody>"
    "    <Box><IS><connect nodeField='size' protoField='size'/></IS></Box>"
    "  </ProtoBody>"
    "</ProtoDeclare>"
    "<Transform><ProtoInstance name='Param' containerField='children'>"
    "  <fieldValue name='size' value='5 5 5'/>"
    "</ProtoInstance></Transform>"
    "</Scene></X3D>";
  XmlReader reader;                       // match the actual reader class
  auto doc = reader.readDocument(xml);
  auto &scene = doc.getScene();
  assert(scene.protoDeclarations.size() == 1);
  auto &decl = *scene.protoDeclarations[0];
  assert(decl.interface.size() == 1 && decl.interface[0].name == "size");
  assert(decl.body.nodes.size() == 1 && decl.body.nodes[0]->nodeTypeName() == "Box");
  assert(decl.body.isConnections.size() == 1 &&
         decl.body.isConnections[0].protoField == "size");
  assert(scene.protoInstances.size() == 1);
  auto &inst = scene.protoInstances[0];
  assert(inst.name == "Param" && inst.fieldValues.size() == 1);
  assert(!inst.parent.expired() &&
         inst.parent.lock()->nodeTypeName() == "Transform");
  assert(inst.parentField == "children");
}
```

- [ ] **Step 2: Run test red.**

- [ ] **Step 3: Implement XML capture.** In `XmlReader.hpp`, where the element name is dispatched (the node-vs-unknown decision near line 167-174), add element-name branches **before** the unknown-skip fallback. Reuse the reader's existing child-iteration / attribute-reading helpers (match local names — e.g. how it reads attributes and recurses into child elements). Structure:

```cpp
  // --- PROTO statements (checked before factory/unknown handling) ---
  if (elemName == "ProtoDeclare") {
    auto decl = std::make_shared<runtime::ProtoDeclaration>();
    decl->name = attr(el, "name");
    for (child in el) {
      if (child.name == "ProtoInterface") readProtoInterface(child, decl->interface);
      else if (child.name == "ProtoBody")  readProtoBody(child, decl->body, scene);
    }
    scene.protoDeclarations.push_back(decl);
    return nullptr; // not a graph node
  }
  if (elemName == "ExternProtoDeclare") {
    auto ext = std::make_shared<runtime::ExternProtoDeclaration>();
    ext->name = attr(el, "name");
    ext->url  = splitMFString(attr(el, "url"));
    readProtoInterface(el, ext->interface); // <field> children
    scene.externProtoDeclarations.push_back(ext);
    return nullptr;
  }
  if (elemName == "ProtoInstance") {
    runtime::ProtoInstance inst;
    inst.name = attr(el, "name");
    inst.DEF = attr(el, "DEF");
    inst.USE = attr(el, "USE");
    if (hasAttr(el, "containerField")) inst.containerField = attr(el, "containerField");
    for (child in el) if (child.name == "fieldValue") {
      runtime::ProtoFieldValue fv;
      fv.name = attr(child, "name");
      // scalar value attr OR child node(s) for SFNode/MFNode field values
      if (hasAttr(child, "value")) fv.value = parseFieldValueAny(/*per declared type if known*/ child);
      else fv.nodeValue = readChildNodes(child, scene);
      inst.fieldValues.push_back(std::move(fv));
    }
    inst.parent = parentNode;          // set by caller (null at Scene root)
    inst.parentField = inst.containerField;
    scene.protoInstances.push_back(std::move(inst));
    return nullptr;
  }
```

Helper `readProtoInterface` reads each `<field name type accessType value>` into a `ProtoField` (boxing `value` per `type`; for SFNode/MFNode default, read child nodes into `nodeDefault`). `readProtoBody` reads child elements as nodes into `body.nodes` and `<ROUTE>` into `body.routes`; when it encounters a node's `<IS>` element, read each `<connect nodeField protoField/>` into `body.isConnections` bound to the **current body node** being read.

> The exact helper names (`attr`, `readChildNodes`, `splitMFString`, `parseFieldValueAny`) must match the existing XmlReader/XmlLite plumbing — read the surrounding code in `XmlReader.hpp` and reuse what is already there for attribute and child-node reading. Do not introduce a parallel parser.

- [ ] **Step 4: Run test green, then full XML + roundtrip suites.**

Run: `mise run build && ctest --test-dir build -R "xml|roundtrip" --output-on-failure` (expected: pass, no regressions).

- [ ] **Step 5: Commit**

```bash
git add runtime/codecs/XmlReader.hpp runtime/codecs/tests/<file>.cpp
git commit -m "Proto expansion: XmlReader captures ProtoDeclare/Extern/Instance/IS"
```

---

## Task 10: Front-door integration — `X3DParse` baseUrl + default resolver + expandScene

**Files:**
- Modify: `runtime/parse/X3DParse.hpp` (`parseDocument`, `parseFile`)
- Test: `runtime/parse/tests/<parse front-door test>.cpp` + EXTERN fixtures under `runtime/parse/tests/data/proto/`

- [ ] **Step 1: Create EXTERN fixtures.**

`runtime/parse/tests/data/proto/shapes.x3d`:
```xml
<X3D profile='Interchange' version='4.0'><Scene>
<ProtoDeclare name='ExtBox'><ProtoBody><Box/></ProtoBody></ProtoDeclare>
</Scene></X3D>
```
`runtime/parse/tests/data/proto/main.x3d`:
```xml
<X3D profile='Interchange' version='4.0'><Scene>
<ExternProtoDeclare name='ExtBox' url='shapes.x3d#ExtBox'/>
<ProtoInstance name='ExtBox'/>
</Scene></X3D>
```

- [ ] **Step 2: Write the failing test.**

```cpp
static void frontDoorExpandTest() {
  // Local PROTO expands end-to-end through the front door.
  const char *xml =
    "<X3D version='4.0'><Scene>"
    "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
    "<ProtoInstance name='P'/></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  bool box = false;
  for (auto &n : doc.getScene().rootNodes)
    if (n && n->nodeTypeName() == "Box") box = true;
  assert(box);
  assert(doc.protoWarnings.empty());

  // EXTERN resolves a sibling file via the default local-file resolver.
  auto ext = x3d::codec::parseFile("runtime/parse/tests/data/proto/main.x3d");
  bool extBox = false;
  for (auto &n : ext.getScene().rootNodes)
    if (n && n->nodeTypeName() == "Box") extBox = true;
  assert(extBox);
}
```

(Adjust the fixture path to however sibling tests locate `data/` — match the existing pattern, e.g. a compile-time `TESTDATA_DIR` macro.)

- [ ] **Step 3: Implement.** In `X3DParse.hpp`:
  1. `#include "X3DProtoExpand.hpp"` and `#include "X3DProtoResolver.hpp"`.
  2. Define `localFileProtoResolver` (here, where `parseFile` is visible):

```cpp
/// Default EXTERNPROTO resolver: resolves file-like URLs relative to baseUrl,
/// parses the target, and returns its matching ProtoDeclare. http/urn -> null.
inline std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl) {
  for (const std::string &u : urls) {
    std::string url = u, frag;
    if (auto h = url.find('#'); h != std::string::npos) {
      frag = url.substr(h + 1);
      url = url.substr(0, h);
    }
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue;                                  // embedder override territory
    std::string path = url;
    if (!baseUrl.empty() && !path.empty() && path.front() != '/')
      path = baseUrl + "/" + path;               // resolve relative to base dir
    try {
      runtime::X3DDocument sub = parseFile(path);
      const std::string want = frag.empty() ? std::string() : frag;
      if (auto d = sub.scene.findProto(want.empty() ? sub_first_proto_name(sub)
                                                    : want))
        return d;                                // see note below
    } catch (const std::exception &) {
      continue;                                  // try next candidate, lenient
    }
  }
  return nullptr;
}
```

  Simplify the fragment lookup: `findProto(name)` already exists on `Scene`. When `frag` is empty, fall back to the first declaration: iterate `sub.scene.protoDeclarations` and return the first (or match the EXTERN's own name — the caller knows it; pass the extern name through if you prefer). Keep it minimal: if `frag` non-empty use `sub.scene.findProto(frag)`, else return `sub.scene.protoDeclarations.empty() ? nullptr : sub.scene.protoDeclarations.front()`.

  3. Add an optional `baseUrl` + resolver to `parseDocument`:

```cpp
inline runtime::X3DDocument parseDocument(
    const std::string &text, Encoding hint = Encoding::Unknown,
    const std::string &baseUrl = "",
    const ProtoDeclarationResolver &resolver = localFileProtoResolver) {
  ... existing body up to range-warning collection ...
  // Expand PROTOs after range-warning collection.
  runtime::expandScene(doc.scene, resolver, baseUrl, doc.protoWarnings);
  return doc;
}
```

  4. In `parseFile`, derive the base directory and pass it:

```cpp
  std::string base;
  if (auto slash = path.find_last_of("/\\"); slash != std::string::npos)
    base = path.substr(0, slash);
  return parseDocument(bytes, enc, base);
```

> **Cross-file cycle bound:** `localFileProtoResolver` calls `parseFile`, which expands again. A.x3d↔B.x3d EXTERN cycle would recurse without bound. Add a `thread_local std::vector<std::string> activeFiles;` guard inside `localFileProtoResolver` (push the resolved absolute path before `parseFile`, pop after; if already present, `continue`). Keep this local to the default resolver. Cover with a two-file mutually-referential fixture asserting a bounded result + no hang.

- [ ] **Step 4: Run test green; then full suite.**

Run: `mise run build && ctest --test-dir build --output-on-failure`.

- [ ] **Step 5: Commit**

```bash
git add runtime/parse/X3DParse.hpp runtime/parse/tests/
git commit -m "Proto expansion: front-door baseUrl + local-file resolver + expandScene"
```

---

## Task 11: Bridge wiring — resolved proto routes + external-route redirect

**Files:**
- Modify: `runtime/events/X3DSceneBridge.hpp:92-155` (`buildRoutes`)
- Test: `runtime/events/tests/<bridge or cascade test>.cpp`

- [ ] **Step 1: Write the failing test** (an IS live-event: external route into an instance's exposed inputOnly field reaches the body interpolator).

```cpp
static void protoRouteRedirectTest() {
  // Build a scene with one expanded instance exposing set_fraction (inputOnly)
  // IS-mapped to a PositionInterpolator, plus a body route, then an EXTERNAL
  // route TS.fraction_changed -> instance.set_fraction.
  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Anim";
  ProtoField in; in.name = "set_fraction"; in.type = X3DFieldType::SFFloat;
  in.access = AccessType::InputOnly; decl->interface.push_back(in);
  auto pi = createX3DNode("PositionInterpolator"); pi->setDEF("PI");
  decl->body.nodes.push_back(pi);
  decl->body.isConnections.push_back({pi, "set_fraction", "set_fraction"});
  scene.protoDeclarations.push_back(decl);

  auto ts = createX3DNode("TimeSensor"); ts->setDEF("TS");
  scene.addRootNode(ts);
  ProtoInstance inst; inst.name = "Anim"; inst.declaration = decl; inst.DEF = "A";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, x3d::codec::noopProtoResolver, "", w);
  // external route names the instance DEF + interface field
  scene.routes.push_back(Route{"TS", "fraction_changed", "A", "set_fraction"});

  X3DExecutionContext ctx;
  auto res = ctx.buildFrom(scene);
  // The redirect produced a route into the cloned PI.set_fraction; the bad-field
  // rejection must NOT fire for the interface field.
  bool rejectedInterfaceField = false;
  for (auto &r : res.rejected)
    if (r.reason.find("set_fraction") != std::string::npos) rejectedInterfaceField = true;
  assert(!rejectedInterfaceField);
  assert(res.routesAdded >= 1);
}
```

(Match `BridgeResult` field names — `rejected[].reason`, `routesAdded` — against `X3DSceneBridge.hpp`.)

- [ ] **Step 2: Run test red.** (Without redirect, the external route's `set_fraction` is an unknown field on the spliced primary → rejected.)

- [ ] **Step 3: Implement.** In `buildRoutes`:

  (a) Before/after the main loop, register pre-resolved proto routes directly:

```cpp
  for (const ResolvedProtoRoute &pr : scene.resolvedProtoRoutes) {
    if (!pr.from || !pr.to) continue;
    ctx.addRoute({pr.from.get(), pr.fromField}, {pr.to.get(), pr.toField});
    ++result.routesAdded;
  }
```

  (b) In the unknown-field rejection paths (steps (2) for `fromField` and `toField`), before pushing a rejection, consult `protoRedirects` keyed by the resolved node pointer + field name, and if present, add a route per redirect target instead of rejecting. Factor a helper:

```cpp
  // Returns true if (node, field) was an exposed interface field and routes
  // were added via the redirect table (so the caller skips normal handling).
  auto redirectEndpoint = [&](X3DNode *node, const std::string &field,
                              bool asSource, const FieldAddress &other) -> bool {
    auto nIt = scene.protoRedirects.find(node);
    if (nIt == scene.protoRedirects.end()) return false;
    auto fIt = nIt->second.find(field);
    if (fIt == nIt->second.end()) return false;
    for (const ProtoRedirect &t : fIt->second) {
      if (asSource) ctx.addRoute({t.targetNode.get(), t.targetField}, other);
      else          ctx.addRoute(other, {t.targetNode.get(), t.targetField});
      ++result.routesAdded;
    }
    return true;
  };
```

  Use it: when `findField` returns null for the sink, try `redirectEndpoint(toNode.get(), route.toField, /*asSource=*/false, {fromNode.get(), route.fromField})` and `continue` if it returns true; symmetrically for the source side. (Resolve the *opposite* endpoint normally first; if both endpoints are interface fields, resolve source redirect to concrete then feed each as `other`. For the common single-interface case the lambda suffices.)

- [ ] **Step 4: Run test green; then full event suite.**

Run: `mise run build && ctest --test-dir build -R "cascade|bridge|animation|event" --output-on-failure`.

- [ ] **Step 5: Commit**

```bash
git add runtime/events/X3DSceneBridge.hpp runtime/events/tests/<file>.cpp
git commit -m "Proto expansion: bridge registers proto routes + redirects external routes"
```

---

## Task 12: Round-trip — re-emit `<ProtoInstance>`

**Files:**
- Modify: `runtime/codecs/XmlWriter.hpp`
- Test: `runtime/tests/proto_roundtrip_test.cpp`

- [ ] **Step 1: Write the failing test.**

```cpp
// runtime/tests/proto_roundtrip_test.cpp
#include "X3DParse.hpp"
#include "XmlWriter.hpp"
#include <cassert>
#include <string>
using namespace x3d;

int main() {
  const char *xml =
    "<X3D version='4.0'><Scene>"
    "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
    "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);          // expands to a Box root
  std::string out = codec::writeXml(doc);        // match the actual writer API
  // The expansion's Box must NOT be serialized; the original instance is.
  assert(out.find("<ProtoInstance") != std::string::npos);
  assert(out.find("name=\"P\"") != std::string::npos ||
         out.find("name='P'") != std::string::npos);
  assert(out.find("<Box") == std::string::npos); // suppressed via sourceLink
  return 0;
}
```

(Match the writer entry point — `writeXml(doc)` or an `XmlWriter` visitor; read `XmlWriter.hpp` for the actual API.)

- [ ] **Step 2: Run test red.**

- [ ] **Step 3: Implement.** The XML writer walks nodes (a `NodeVisitor` or recursive emit). Give it access to `scene.expandedSources`. Before emitting a node, look up `scene.expandedSources.find(node.get())`; if found, emit `<ProtoInstance name=... [DEF=...] [containerField=...]>` with a `<fieldValue name=... value=.../>` per `source.fieldValues` and **do not** descend into the expanded subtree. Representative:

```cpp
  if (auto it = scene.expandedSources.find(&node); it != scene.expandedSources.end()) {
    const runtime::ProtoInstance &src = it->second;
    out << "<ProtoInstance name=\"" << escape(src.name) << "\"";
    if (!src.DEF.empty()) out << " DEF=\"" << escape(src.DEF) << "\"";
    if (src.containerField != "children")
      out << " containerField=\"" << escape(src.containerField) << "\"";
    out << ">";
    for (const auto &fv : src.fieldValues)
      out << "<fieldValue name=\"" << escape(fv.name) << "\" value=\""
          << formatAny(fv.value) << "\"/>";       // reuse the writer's value formatter
    out << "</ProtoInstance>";
    return;                                        // skip the expansion subtree
  }
```

`ProtoDeclare`/`ExternProtoDeclare` emission from `scene.protoDeclarations` / `externProtoDeclarations` is unchanged (verify the writer already emits them; if not, add a pass mirroring the existing route/declaration emission — out of scope only if already present).

> The writer needs the `Scene&` in scope at the node-emit site. If the current writer is a free function over the document, this is already available; if it is a `NodeVisitor` without scene access, pass the scene into the visitor's constructor.

- [ ] **Step 4: Run test green; then full roundtrip suite.**

Run: `mise run build && ctest --test-dir build -R "roundtrip" --output-on-failure`.

- [ ] **Step 5: Commit**

```bash
git add runtime/codecs/XmlWriter.hpp runtime/tests/proto_roundtrip_test.cpp runtime/tests/CMakeLists.txt
git commit -m "Proto expansion: writer re-emits <ProtoInstance> from expandedSources"
```

---

## Task 13: Full verification + corpus smoke

**Files:** none (verification only) — plus optionally a recorded sweep note.

- [ ] **Step 1: Golden gate (codegen must be untouched).**

Run: `python -m pytest tests/test_golden_tree.py -q` (or `scripts/check_golden.sh`).
Expected: PASS, golden sha256 unchanged (`223b7394…`). If it fails, a generated file was touched — revert that; all work is runtime-only.

- [ ] **Step 2: Full pytest.**

Run: `python -m pytest -q`. Expected: all green (≥ 188 as of Step 0).

- [ ] **Step 3: Full ctest.**

Run: `mise run build && ctest --test-dir build --output-on-failure`. Expected: all green, including the new `proto_clone_test`, `proto_expand_test`, `proto_roundtrip_test`, and the reader/bridge additions.

- [ ] **Step 4: Corpus smoke.** Build the corpus sweep harness and run it over a PROTO-heavy subset to confirm no new parse regressions and that instances expand.

Run (adapt paths to the existing `scripts/corpus_sweep.cpp` build line in memory/build notes):
```bash
g++ -std=c++20 -O1 -I generated_cpp_bindings -I runtime -I runtime/codecs \
    -I runtime/events -I runtime/parse scripts/corpus_sweep.cpp -o build/corpus_sweep
./build/corpus_sweep --ext x3d --show <x3d-render-workspace>/testdata | tail -20
```
Expected: PROTO files parse + expand without throwing; failure-bucket counts no worse than the pre-task baseline. Record the delta in the commit message / memory.

- [ ] **Step 5: Final commit (if any sweep harness tweak) + update memory.**

```bash
git add -A
git commit -m "Proto expansion: full verification (golden byte-identical, ctest/pytest green, corpus smoke)"
```

Update the modernization memory with: M1 PROTO closeout complete, what expands (local + file-EXTERN, full IS wiring), what's deferred (http/urn EXTERN → embedder; nested-in-body instances → fast-follow), and the verified counts.

---

## Self-Review

**Spec coverage:**
- §5.1 data model → Task 1. §5.2 capture (VRML IS + parent) → Task 8; (XML) → Task 9.
- §5.3 deepClone → Task 2. §5.4 expand (value-forward, decl resolve, splice) → Tasks 4, 6, 7.
- §5.4 step 5/6 (body routes, redirects) → Task 5. §E proto-local scope (pre-resolved routes) → Task 5/Task 1 `resolvedProtoRoutes`.
- §5.5 resolver seam → Task 3 (typedef) + Task 10 (`localFileProtoResolver`).
- §5.6 front-door (baseUrl, expandScene, warnings) → Task 10. §5.7 bridge redirect → Task 11.
- §5.8 round-trip → Task 12. §H golden-safe source map → Task 1 (`expandedSources` scene-side). §6 ProtoWarning → Task 1 + collected in Task 10. §7 tests → per-task TDD + Task 13. §8 lifetime (shared_ptr in routes/redirects) → Task 1 struct choices. §9 DoD → Task 13.

**Placeholder scan:** Reader/writer tasks (8, 9, 12) intentionally use representative code with explicit "match the existing local helper names" notes because those edits weave into hand-written parsers whose internal helper names must be read in-file; every such note names the exact file + anchor line and the precise data to capture. No `TBD`/`implement later`. The `<path>` in test-run commands is the CTest binary dir, resolved once when the first test is wired.

**Type consistency:** `expandInstance` signature evolves across tasks — Task 4 introduces it `(inst, scene, resolver, guard, warnings)`; Task 7 inserts `baseUrl` before `guard` → `(inst, scene, baseUrl, guard, warnings)`. Tasks 6/7 explicitly call out updating earlier call sites. `ProtoWarning::Kind` enum, `ResolvedProtoRoute` (from/fromField/to/toField), `ProtoRedirect` (targetNode/targetField), and `protoRedirects[node][field] -> vector<ProtoRedirect>` are used consistently in Tasks 1/5/11. `deepClone` two-arg + one-arg overloads consistent across Tasks 2/4.
