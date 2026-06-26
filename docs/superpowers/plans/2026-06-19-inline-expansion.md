# Parse-time Inline Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `<Inline url='…'/>` resolve at parse/scene-build time — load the referenced X3D file, splice its content into the scene graph (DEF-isolated, internal ROUTEs firing), with a byte-identical writer round-trip — mirroring the existing EXTERNPROTO/AUD-B machinery.

**Architecture:** A new `runtime/InlineExpand.hpp` walks the parsed `Scene`, and for each `Inline` node with `load=TRUE` and a resolvable `url`, replaces it in its parent slot with a synthetic `Group` holding the loaded sub-scene's root nodes. The original `Inline` node is preserved in a new `Scene::expandedInlines` side map so writers re-emit `<Inline url='…'/>` (the AUD-B `expandedSources` redirect pattern). The child's internal ROUTEs are pre-resolved in the child's own DEF scope and carried in `Scene::resolvedInlineRoutes`, which the scene-bridge registers directly. Resolution is done by an injectable `InlineResolver`; the default `localFileInlineResolver` (in `X3DParse.hpp`) does sibling-file I/O with a `thread_local` cycle stack, exactly like `localFileProtoResolver`.

**Tech Stack:** C++17/20 header-only runtime; reflection via `X3DNode::fields()` (`FieldInfo` get/set thunks); `X3DNodeFactory` for node creation; Ninja+ccache build (`mise run build`); standalone-executable ctest tests registered in root `CMakeLists.txt`.

## Global Constraints

- **Golden byte-identity is a hard gate.** `mise run golden` must show zero drift; the full corpus sweep (`mise run corpus`) must show no new crashes/regressions. Verified in the final task.
- **No edits to the generated layer** (`generated_cpp_bindings/`). All work stays node-agnostic via reflection (`fields()`, `findField`, `X3DNodeFactory::create`). The generated `Inline` node is NOT given a content field.
- **Lenient read policy.** A missing / unparsable / cyclic Inline target never throws; it produces an `InlineWarning` and leaves the `Inline` node un-expanded.
- **Scope: Tier 1 only.** No IMPORT/EXPORT cross-boundary routing, no dynamic `load`/`url` events, no cross-UNIT scaling. (See spec `docs/superpowers/specs/2026-06-19-inline-expansion-design.md`.)
- **Mirror precedents exactly:** `expandScene`/`localFileProtoResolver` (`runtime/X3DProtoExpand.hpp`, `runtime/parse/X3DParse.hpp`), `expandedSources` writer redirect (`runtime/codecs/*Writer.hpp`), `resolvedProtoRoutes` bridge registration (`runtime/events/X3DSceneBridge.hpp`).
- **Build/test commands:** build = `mise run build`; single test = `ctest --test-dir build -R <name> --output-on-failure`; golden = `mise run golden`; corpus = `mise run corpus`.

---

## File Structure

- **Create** `runtime/InlineExpand.hpp` — `InlineResolver` type, `expandInlines()` (graph splice + DEF isolation + route hoist). Node-agnostic; depends only on `X3DScene`, reflection, `X3DNodeFactory`.
- **Modify** `runtime/X3DProto.hpp` — add `InlineWarning` struct; add `Scene`-facing structs are in `X3DScene.hpp` (below). (`InlineWarning` lives here next to `ProtoWarning`.)
- **Modify** `runtime/X3DScene.hpp` — add `expandedInlines` map and `resolvedInlineRoutes` vector to `Scene`.
- **Modify** `runtime/X3DDocument.hpp` — add `inlineWarnings` vector (sibling of `protoWarnings`).
- **Modify** `runtime/parse/X3DParse.hpp` — add `localFileInlineResolver` + call `runtime::expandInlines(...)` in `parseDocument` after `expandScene(...)`.
- **Modify** `runtime/events/X3DSceneBridge.hpp` — register `scene.resolvedInlineRoutes` directly (loop mirroring `resolvedProtoRoutes`).
- **Modify** `runtime/codecs/XmlWriter.hpp`, `runtime/codecs/VrmlWriter.hpp`, `runtime/codecs/JsonWriter.hpp` — at the node-emit hook, redirect an `expandedInlines` group → re-emit the preserved `Inline`.
- **Create** tests under `runtime/parse/tests/`: `inline_expand_test.cpp`, `inline_roundtrip_test.cpp`, `inline_routes_test.cpp`, `inline_cycle_test.cpp`, and fixtures under `runtime/parse/tests/data/inline/`.
- **Modify** root `CMakeLists.txt` — register the new test executables.

---

## Task 1: Data carriers (Scene + Document + warning struct)

**Files:**
- Modify: `runtime/X3DProto.hpp` (add `InlineWarning` after `ProtoWarning`, ~line 61)
- Modify: `runtime/X3DScene.hpp` (add two members + include)
- Modify: `runtime/X3DDocument.hpp` (add `inlineWarnings` after `protoWarnings`, ~line 92)
- Test: `runtime/parse/tests/inline_carriers_test.cpp`

**Interfaces:**
- Produces:
  - `struct x3d::runtime::InlineWarning { enum class Kind { UnresolvedUrl, LoadError }; Kind kind; std::string inlineDEF; std::string detail; };`
  - `Scene::expandedInlines` : `std::unordered_map<X3DNode*, std::shared_ptr<X3DNode>>` (group node ptr → preserved original `Inline` node).
  - `Scene::resolvedInlineRoutes` : `std::vector<ResolvedProtoRoute>` (child-scope pre-resolved routes; reuses the existing `ResolvedProtoRoute` shape from `X3DProto.hpp`).
  - `X3DDocument::inlineWarnings` : `std::vector<InlineWarning>`.

- [ ] **Step 1: Write the failing test**

Create `runtime/parse/tests/inline_carriers_test.cpp`:

```cpp
// Smoke test: the new Inline data carriers compile and are default-empty.
#include "runtime/X3DDocument.hpp"
#include "runtime/X3DScene.hpp"
#include "runtime/X3DProto.hpp"
#include <cassert>
#include <iostream>

int main() {
  using namespace x3d::runtime;
  Scene s;
  assert(s.expandedInlines.empty());
  assert(s.resolvedInlineRoutes.empty());

  X3DDocument doc;
  assert(doc.inlineWarnings.empty());

  InlineWarning w{InlineWarning::Kind::UnresolvedUrl, "MyInline", "no url"};
  doc.inlineWarnings.push_back(w);
  assert(doc.inlineWarnings.size() == 1);
  assert(doc.inlineWarnings[0].kind == InlineWarning::Kind::UnresolvedUrl);
  assert(doc.inlineWarnings[0].inlineDEF == "MyInline");

  std::cout << "inline_carriers_test OK\n";
  return 0;
}
```

- [ ] **Step 2: Add `InlineWarning` to `runtime/X3DProto.hpp`**

After the `ProtoWarning` struct (ends ~line 61), insert:

```cpp
/// Non-fatal Inline diagnostic, collected into X3DDocument.inlineWarnings.
struct InlineWarning {
  enum class Kind { UnresolvedUrl, LoadError };
  Kind kind;
  std::string inlineDEF; // DEF of the Inline node, or "" if anonymous
  std::string detail;    // e.g. the url that failed to resolve
};
```

- [ ] **Step 3: Add members to `runtime/X3DScene.hpp`**

Ensure `#include <unordered_map>` is present (it is). After the `resolvedProtoRoutes` member (~line 54) add:

```cpp
  // Inline expansion (Tier 1): synthetic Group (key) -> the original Inline node
  // it replaced. Writers consult this to re-emit <Inline url=.../> rather than
  // the spliced content (AUD-B redirect pattern; mirrors expandedSources).
  std::unordered_map<X3DNode *, std::shared_ptr<X3DNode>> expandedInlines;

  // Internal ROUTEs of expanded Inlines, pre-resolved against the CHILD scene's
  // own DEF scope (never registered in this scene's `defs`). The bridge
  // registers these directly. Reuses ResolvedProtoRoute (same endpoint shape).
  std::vector<ResolvedProtoRoute> resolvedInlineRoutes;
```

(`ResolvedProtoRoute` is already visible via the existing `#include "X3DProto.hpp"` at the top of `X3DScene.hpp`.)

- [ ] **Step 4: Add `inlineWarnings` to `runtime/X3DDocument.hpp`**

After the `protoWarnings` member (~line 92) add:

```cpp
  // Inline expansion diagnostics (unresolved url, load error), populated by the
  // X3DParse front door via expandInlines(). Sibling of protoWarnings.
  std::vector<InlineWarning> inlineWarnings;
```

Confirm `X3DDocument.hpp` includes `X3DProto.hpp` (it pulls `ProtoWarning`, so `InlineWarning` is visible). If not, add `#include "X3DProto.hpp"`.

- [ ] **Step 5: Register the test in root `CMakeLists.txt`**

Next to the other `runtime/parse/tests` registrations (search for `x3d_proto_expand_audit`), add:

```cmake
    add_executable(x3d_inline_carriers
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_carriers_test.cpp")
    target_link_libraries(x3d_inline_carriers PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_inline_carriers COMMAND x3d_inline_carriers)
```

- [ ] **Step 6: Build and run the test**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_carriers --output-on-failure`
Expected: PASS (`inline_carriers_test OK`).

- [ ] **Step 7: Commit**

```bash
git add runtime/X3DProto.hpp runtime/X3DScene.hpp runtime/X3DDocument.hpp \
        runtime/parse/tests/inline_carriers_test.cpp CMakeLists.txt
git commit -m "feat(inline): data carriers for Inline expansion (InlineWarning, expandedInlines, resolvedInlineRoutes)"
```

---

## Task 2: `expandInlines` — resolver, Group splice, DEF isolation, cycle guard, wiring

**Files:**
- Create: `runtime/InlineExpand.hpp`
- Modify: `runtime/parse/X3DParse.hpp` (add `localFileInlineResolver`; call `expandInlines` in `parseDocument`)
- Test: `runtime/parse/tests/inline_expand_test.cpp`
- Test fixtures: `runtime/parse/tests/data/inline/parent.x3d`, `child.x3d`, `cycle_a.x3d`, `cycle_b.x3d`

**Interfaces:**
- Consumes: `Scene::expandedInlines`, `Scene::resolvedInlineRoutes` (Task 1); `ResolvedProtoRoute`, `InlineWarning`; reflection `FieldInfo`/`findField`; `X3DNodeFactory::create`.
- Produces (in `namespace x3d::runtime`):
  - `using InlineResolver = std::function<std::shared_ptr<Scene>(const std::vector<std::string>& urls, const std::string& baseUrl)>;`
  - `void expandInlines(Scene& scene, const InlineResolver& resolver, const std::string& baseUrl, std::vector<InlineWarning>& warnings);`
  - (in `namespace x3d::codec`) `std::shared_ptr<runtime::Scene> localFileInlineResolver(const std::vector<std::string>& urls, const std::string& baseUrl);`

- [ ] **Step 1: Write the failing test (composition + DEF isolation, using an injected resolver)**

Create `runtime/parse/tests/inline_expand_test.cpp`. The injected resolver returns a hand-built child `Scene` (no file I/O), so this test is hermetic:

```cpp
#include "runtime/InlineExpand.hpp"
#include "runtime/X3DScene.hpp"
#include "generated_cpp_bindings/X3DNodeFactory.hpp"
#include "generated_cpp_bindings/X3DReflection.hpp"
#include <cassert>
#include <iostream>
#include <memory>

using namespace x3d::runtime;

// Helper: build a Scene whose single root is a DEF'd Shape (stands in for a
// loaded child asset). Returns the scene; registers the Shape under DEF "Geo".
static std::shared_ptr<Scene> makeChildScene() {
  auto s = std::make_shared<Scene>();
  auto shape = X3DNodeFactory::create("Shape");
  shape->setDEF("Geo");
  s->addRootNode(shape); // addRootNode registers the DEF in the child scope
  return s;
}

// Find a node of the given type among a node's MFNode "children", one level deep.
static X3DNode* firstChildOfType(const X3DNode& n, const std::string& type) {
  const FieldInfo* fi = x3d::runtime::findField(n, "children");
  if (!fi || !fi->get) return nullptr;
  for (auto& c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi->get(n)))
    if (c && c->nodeTypeName() == type) return c.get();
  return nullptr;
}

int main() {
  // Parent scene: a Transform (DEF "Root") containing one Inline (url "child").
  Scene parent;
  auto root = X3DNodeFactory::create("Transform");
  root->setDEF("Root");
  auto inl = X3DNodeFactory::create("Inline");
  // set url = ["child"] via reflection
  {
    const FieldInfo* u = findField(*inl, "url");
    assert(u && u->set);
    u->set(*inl, std::any(std::vector<std::string>{"child"}));
  }
  // attach Inline as a child of Root
  {
    const FieldInfo* ch = findField(*root, "children");
    assert(ch && ch->set);
    auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(ch->get(*root));
    kids.push_back(inl);
    ch->set(*root, std::any(std::move(kids)));
  }
  parent.addRootNode(root);

  // Injected resolver: any url -> a fresh child scene.
  InlineResolver resolver = [](const std::vector<std::string>&,
                               const std::string&) { return makeChildScene(); };

  std::vector<InlineWarning> warnings;
  expandInlines(parent, resolver, "", warnings);

  // (1) The Inline under Root was replaced by a Group...
  X3DNode* group = firstChildOfType(*root, "Group");
  assert(group && "Inline should be replaced by a synthetic Group");
  // ...and the Group contains the child's Shape.
  assert(firstChildOfType(*group, "Shape") && "child geometry must hang under the Group");
  // (2) The Inline node is no longer a direct child of Root.
  assert(!firstChildOfType(*root, "Inline") && "Inline node must be spliced out of the live slot");
  // (3) DEF isolation: child DEF "Geo" did NOT leak into the parent scope.
  assert(parent.resolve("Geo") == nullptr && "child DEF must not leak to parent");
  // (4) The preserved Inline is recorded for writer round-trip.
  assert(parent.expandedInlines.count(group) == 1 && "group must map to preserved Inline");
  assert(parent.expandedInlines[group]->nodeTypeName() == "Inline");
  assert(warnings.empty());

  std::cout << "inline_expand_test OK\n";
  return 0;
}
```

- [ ] **Step 2: Run to verify it fails (no `InlineExpand.hpp`)**

Run: `mise run build`
Expected: FAIL — `runtime/InlineExpand.hpp: No such file` (after you add the CMake target in Step 5, the compile fails on the missing header/function).

- [ ] **Step 3: Implement `runtime/InlineExpand.hpp`**

```cpp
// InlineExpand.hpp
// Tier-1 parse-time Inline expansion. For each Inline node with load=TRUE and a
// resolvable url, replace it in its parent slot with a synthetic Group holding
// the loaded child scene's root nodes. The original Inline is preserved in
// Scene::expandedInlines so writers re-emit <Inline url=.../>. The child's
// internal ROUTEs are pre-resolved in the child's DEF scope into
// Scene::resolvedInlineRoutes. Node-agnostic: reflection + X3DNodeFactory only.
//
// Mirrors runtime/X3DProtoExpand.hpp (expandScene) and the AUD-B expandedSources
// writer redirect. DEF isolation per ISO 19775-1 §9.4.2: a child's DEFs are NOT
// exposed to the parent (only IMPORT would, which is out of Tier-1 scope).
#ifndef X3D_RUNTIME_INLINE_EXPAND_HPP
#define X3D_RUNTIME_INLINE_EXPAND_HPP

#include "X3DScene.hpp"
#include "generated_cpp_bindings/X3DNode.hpp"
#include "generated_cpp_bindings/X3DNodeFactory.hpp"
#include "generated_cpp_bindings/X3DReflection.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {

/// Resolve an Inline's url list to a parsed sub-scene (null on failure/cycle).
using InlineResolver = std::function<std::shared_ptr<Scene>(
    const std::vector<std::string> &urls, const std::string &baseUrl)>;

namespace inline_detail {

inline const FieldInfo *field(const X3DNode &n, const std::string &name) {
  return ::x3d::runtime::findField(n, name); // same helper used by attachToParent
}

// Read an Inline's `url` (MFString) via reflection; empty if absent.
inline std::vector<std::string> readUrl(const X3DNode &inl) {
  const FieldInfo *u = field(inl, "url");
  if (!u || !u->get) return {};
  return std::any_cast<std::vector<std::string>>(u->get(inl));
}

// Read an Inline's `load` (SFBool); default TRUE if absent.
inline bool readLoad(const X3DNode &inl) {
  const FieldInfo *l = field(inl, "load");
  if (!l || !l->get) return true;
  return std::any_cast<bool>(l->get(inl));
}

// Build a synthetic Group whose "children" are `content`.
inline std::shared_ptr<X3DNode>
makeGroup(const std::vector<std::shared_ptr<X3DNode>> &content) {
  auto g = X3DNodeFactory::create("Group");
  const FieldInfo *ch = field(*g, "children");
  if (ch && ch->set)
    ch->set(*g, std::any(content));
  return g;
}

// Pre-resolve the child scene's own ROUTEs (against the child's defs) and append
// to dst, plus hoist any routes the child already pre-resolved (nested inlines /
// protos). All become endpoint-concrete, parent-DEF-independent.
inline void hoistChildRoutes(Scene &child,
                             std::vector<ResolvedProtoRoute> &dst) {
  child.resolveRoutes(); // fill Route::from/to weak_ptrs from child.defs
  for (const Route &r : child.routes) {
    auto from = r.from.lock();
    auto to = r.to.lock();
    if (from && to)
      dst.push_back({from, r.fromField, to, r.toField});
  }
  for (const auto &pr : child.resolvedProtoRoutes) dst.push_back(pr);
  for (const auto &ir : child.resolvedInlineRoutes) dst.push_back(ir);
}

// Replace `target` with `replacement` inside an MFNode/SFNode field of `parent`.
// Returns true if a slot was found and rewritten.
inline bool replaceInParent(X3DNode &parent, const X3DNode *target,
                            const std::shared_ptr<X3DNode> &replacement) {
  for (const auto &f : parent.fields()) {
    if (!f.get || !f.set) continue;
    if (f.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(parent));
      if (c.get() == target) {
        f.set(parent, std::any(replacement));
        return true;
      }
    } else if (f.type == X3DFieldType::MFNode) {
      auto kids =
          std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(parent));
      for (auto &k : kids)
        if (k.get() == target) {
          k = replacement;
          f.set(parent, std::any(std::move(kids)));
          return true;
        }
    }
  }
  return false;
}

} // namespace inline_detail

/// Expand every load=TRUE Inline in `scene`. Lenient: failures become warnings.
inline void expandInlines(Scene &scene, const InlineResolver &resolver,
                          const std::string &baseUrl,
                          std::vector<InlineWarning> &warnings) {
  using namespace inline_detail;

  // Collect (parent, inlineNode) pairs first, so we don't mutate fields mid-walk.
  // Walk root-level + every node-typed field, recording the containing node.
  struct Site { X3DNode *parent; std::shared_ptr<X3DNode> inl; };
  std::vector<Site> sites;
  std::vector<std::shared_ptr<X3DNode>> rootInlines; // Inlines that ARE scene roots

  std::function<void(const std::shared_ptr<X3DNode> &)> walk =
      [&](const std::shared_ptr<X3DNode> &n) {
        if (!n) return;
        for (const auto &f : n->fields()) {
          if (!f.get) continue;
          if (f.type == X3DFieldType::SFNode) {
            auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
            if (!c) continue;
            if (c->nodeTypeName() == "Inline") sites.push_back({n.get(), c});
            else walk(c);
          } else if (f.type == X3DFieldType::MFNode) {
            for (auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n))) {
              if (!c) continue;
              if (c->nodeTypeName() == "Inline") sites.push_back({n.get(), c});
              else walk(c);
            }
          }
        }
      };
  for (auto &r : scene.rootNodes) {
    if (r && r->nodeTypeName() == "Inline") rootInlines.push_back(r);
    else walk(r);
  }

  auto expandOne = [&](const std::shared_ptr<X3DNode> &inl,
                       X3DNode *parent /* null => scene root */) {
    if (!readLoad(*inl)) return; // load=FALSE: leave node un-expanded (Tier 3)
    std::vector<std::string> urls = readUrl(*inl);
    std::shared_ptr<Scene> child = resolver(urls, baseUrl);
    if (!child) {
      warnings.push_back({InlineWarning::Kind::UnresolvedUrl, inl->DEF(),
                          urls.empty() ? "no url" : urls.front()});
      return; // node stays un-expanded; round-trips normally (no map entry)
    }
    // Build the Group from the child's root nodes; isolate child DEFs (we never
    // copy child.defs into scene.defs).
    auto group = makeGroup(child->rootNodes);
    hoistChildRoutes(*child, scene.resolvedInlineRoutes);
    scene.expandedInlines[group.get()] = inl; // preserve for writer round-trip
    if (parent) {
      replaceInParent(*parent, inl.get(), group);
    } else {
      for (auto &r : scene.rootNodes)
        if (r.get() == inl.get()) { r = group; break; }
    }
  };

  for (auto &site : sites) expandOne(site.inl, site.parent);
  for (auto &r : rootInlines) expandOne(r, nullptr);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INLINE_EXPAND_HPP
```

> NOTE on `inl->DEF()`: read the Inline's DEF name for the warning. If `X3DNode` exposes the getter under a different name (check `generated_cpp_bindings/X3DNode.hpp` — `setDEF` exists at line 136; find the matching reader, likely `DEF()` or `getDEF()`), use that. If none is public, pass `""`.

- [ ] **Step 4: Create test fixtures**

`runtime/parse/tests/data/inline/child.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <head><meta name="title" content="child"/></head>
  <Scene>
    <Shape DEF="Geo"><Box size="1 1 1"/></Shape>
  </Scene>
</X3D>
```

`runtime/parse/tests/data/inline/parent.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <head><meta name="title" content="parent"/></head>
  <Scene>
    <Transform DEF="Root">
      <Inline DEF="Asset" url="child.x3d"/>
    </Transform>
  </Scene>
</X3D>
```

`cycle_a.x3d` (url → `cycle_b.x3d`) and `cycle_b.x3d` (url → `cycle_a.x3d`):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Inline url="cycle_b.x3d"/>
</Scene></X3D>
```

(and the mirror for `cycle_b.x3d` pointing at `cycle_a.x3d`).

- [ ] **Step 5: Register the test**

In root `CMakeLists.txt`, next to the other inline targets:

```cmake
    add_executable(x3d_inline_expand
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_expand_test.cpp")
    target_link_libraries(x3d_inline_expand PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_inline_expand COMMAND x3d_inline_expand)
```

- [ ] **Step 6: Add the default resolver + wiring in `runtime/parse/X3DParse.hpp`**

Add `#include "../InlineExpand.hpp"` to the includes. Add the forward declaration alongside the existing ones (near line 80):

```cpp
inline std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl);
```

In `parseDocument`, immediately AFTER the existing `runtime::expandScene(...)` call (line ~120), add:

```cpp
  // Expand load=TRUE Inlines: resolve each url to a sub-scene and splice it in.
  // Mirrors the PROTO pass above; nested Inlines recurse via parseFile.
  runtime::expandInlines(doc.scene, localFileInlineResolver, baseUrl,
                         doc.inlineWarnings);
```

Define `localFileInlineResolver` after `localFileProtoResolver` (it is the direct sibling — copy its structure, including the `thread_local` cycle stack, but return a parsed sub-`Scene`):

```cpp
/// Default Inline resolver: resolve file-like urls relative to `baseUrl`, parse
/// the target, return its Scene. http/https/urn skipped (embedder territory).
/// A thread_local active-file stack makes self-referential / indirect Inline
/// loops terminate (ISO 19775-1 §9.4.2: browsers shall not honor such loops).
inline std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u;
    if (auto h = url.find('#'); h != std::string::npos) url = url.substr(0, h);
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue;
    std::string path = url;
    if (!baseUrl.empty() && !path.empty() && path.front() != '/')
      path = baseUrl + "/" + path;
    if (std::find(activeFiles.begin(), activeFiles.end(), path) !=
        activeFiles.end())
      continue; // cycle: this file is already being resolved up the stack
    activeFiles.push_back(path);
    std::shared_ptr<runtime::Scene> result;
    try {
      runtime::X3DDocument sub = parseFile(path); // recurses (expands sub-Inlines)
      result = std::make_shared<runtime::Scene>(std::move(sub.scene));
    } catch (const std::exception &) {
      // lenient: try the next candidate url
    }
    activeFiles.pop_back();
    if (result) return result;
  }
  return nullptr;
}
```

> NOTE: `runtime::Scene` must be movable into a `shared_ptr` (it is — it's an aggregate of vectors/maps). If a move ctor is unavailable, copy instead (`std::make_shared<runtime::Scene>(sub.scene)`).

- [ ] **Step 7: Build and run**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_expand --output-on-failure`
Expected: PASS (`inline_expand_test OK`).

- [ ] **Step 8: Commit**

```bash
git add runtime/InlineExpand.hpp runtime/parse/X3DParse.hpp \
        runtime/parse/tests/inline_expand_test.cpp \
        runtime/parse/tests/data/inline/ CMakeLists.txt
git commit -m "feat(inline): expandInlines — Group splice, DEF isolation, default resolver + cycle guard"
```

> **Heads-up for later tasks:** after this task the live graph mutates for resolvable Inlines, so the full golden/corpus suite may show Inline-bearing files expanding. Task 3 (writer redirect) restores byte-identity; the full golden gate runs in Task 5. Per-task unit tests stay green meanwhile.

---

## Task 3: Writer round-trip — re-emit `<Inline>` via `expandedInlines`

**Files:**
- Modify: `runtime/codecs/XmlWriter.hpp` (node-emit hook, ~line 185 where `expandedSources` is checked)
- Modify: `runtime/codecs/VrmlWriter.hpp` (node-emit hook, ~line 360)
- Modify: `runtime/codecs/JsonWriter.hpp` (matching node-emit hook)
- Test: `runtime/parse/tests/inline_roundtrip_test.cpp`

**Interfaces:**
- Consumes: `Scene::expandedInlines` (Task 1); the writers' existing `scene_` pointer + `writeNodeElement`/`writeNode` entry points.
- Produces: byte-identical round-trip for documents containing resolvable Inlines.

- [ ] **Step 1: Write the failing test**

Create `runtime/parse/tests/inline_roundtrip_test.cpp`. It parses `parent.x3d` (which expands `child.x3d`) and asserts the re-serialized XML still contains the `<Inline>` and NOT the child `Box`:

```cpp
#include "x3d/sdk.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  assert(argc >= 2 && "pass the inline fixtures dir as argv[1]");
  std::string dir = argv[1];
  auto doc = x3d::sdk::parseFile(dir + "/parent.x3d");

  // After expansion, the Inline content (Box) is in the live graph...
  // ...but the writer must re-emit <Inline url=...> and NOT the Box.
  std::string xml = x3d::codec::XmlWriter{}.writeDocument(doc);
  assert(xml.find("<Inline") != std::string::npos && "Inline must re-emit");
  assert(xml.find("child.x3d") != std::string::npos && "url must be preserved");
  assert(xml.find("<Box") == std::string::npos &&
         "expanded child content must NOT be serialized");

  // Same for VRML + JSON encodings.
  std::string vrml = x3d::codec::VrmlWriter{}.writeDocument(doc);
  assert(vrml.find("Inline") != std::string::npos);
  assert(vrml.find("Box") == std::string::npos);

  std::string json = x3d::codec::JsonWriter{}.writeDocument(doc);
  assert(json.find("Inline") != std::string::npos);
  assert(json.find("Box") == std::string::npos);

  std::cout << "inline_roundtrip_test OK\n";
  return 0;
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_roundtrip --output-on-failure` (after registering in Step 6)
Expected: FAIL — `<Box` IS present (the synthetic Group's content serializes because no redirect exists yet).

- [ ] **Step 3: Add the redirect to `XmlWriter.hpp`**

At the node-emit hook where `expandedSources` is checked (~line 185), add a sibling check BEFORE it (so a Group that maps to an Inline re-emits the Inline):

```cpp
  // Inline round-trip: if this node is the synthetic Group of an expanded
  // Inline, re-emit the original <Inline url=.../> and do NOT descend.
  if (scene_) {
    auto il = scene_->expandedInlines.find(node.get());
    if (il != scene_->expandedInlines.end())
      return writeNodeElement(il->second, containerOverride);
  }
```

(`writeNodeElement(theInlineNode, …)` emits a normal `<Inline>` element with its `url`/`bbox`/etc. — the Inline has no content field, so nothing extra is written.)

- [ ] **Step 4: Add the redirect to `VrmlWriter.hpp`**

At the top of `writeNode` (~line 360, alongside the `expandedSources` check), add:

```cpp
  if (scene_) {
    auto il = scene_->expandedInlines.find(node.get());
    if (il != scene_->expandedInlines.end()) {
      writeNode(os, il->second, depth); // re-emit the original Inline
      return;
    }
  }
```

- [ ] **Step 5: Add the redirect to `JsonWriter.hpp`**

Find the JSON writer's node-emit entry point (the analog of `writeNodeElement`/`writeNode` — search for `expandedSources` in `JsonWriter.hpp`; mirror it). Add the same `expandedInlines` lookup that redirects to emitting the preserved Inline node and skips the Group's subtree. If `JsonWriter` does not yet handle `expandedSources`, model the redirect on however it emits a single node, ensuring the Group is replaced by the Inline and its children are not visited.

- [ ] **Step 6: Register the test**

```cmake
    add_executable(x3d_inline_roundtrip
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_roundtrip_test.cpp")
    target_link_libraries(x3d_inline_roundtrip PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_roundtrip
        COMMAND x3d_inline_roundtrip
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")
```

- [ ] **Step 7: Build and run**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_roundtrip --output-on-failure`
Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add runtime/codecs/XmlWriter.hpp runtime/codecs/VrmlWriter.hpp \
        runtime/codecs/JsonWriter.hpp \
        runtime/parse/tests/inline_roundtrip_test.cpp CMakeLists.txt
git commit -m "feat(inline): writer round-trip — re-emit <Inline> via expandedInlines redirect"
```

---

## Task 4: Internal route hoisting registered by the scene-bridge (self-animation)

**Files:**
- Modify: `runtime/events/X3DSceneBridge.hpp` (register `resolvedInlineRoutes`, ~line 116 where `resolvedProtoRoutes` is registered)
- Test: `runtime/parse/tests/inline_routes_test.cpp`
- Test fixture: `runtime/parse/tests/data/inline/animated_child.x3d`, `animated_parent.x3d`

**Interfaces:**
- Consumes: `Scene::resolvedInlineRoutes` (populated by `expandInlines`, Task 2); `X3DExecutionContext::addRoute` + `buildFrom`.
- Produces: the inlined scene's internal ROUTEs registered on the execution context so self-animating assets tick.

- [ ] **Step 1: Write the failing test**

`animated_child.x3d` — a Transform driven by a TimeSensor→PositionInterpolator (its own internal ROUTEs):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Transform DEF="Mover">
    <Shape><Box/></Shape>
  </Transform>
  <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
  <PositionInterpolator DEF="Path" key="0 1"
      keyValue="0 0 0  10 0 0"/>
  <ROUTE fromNode="Clock" fromField="fraction_changed"
         toNode="Path" toField="set_fraction"/>
  <ROUTE fromNode="Path" fromField="value_changed"
         toNode="Mover" toField="set_translation"/>
</Scene></X3D>
```

`animated_parent.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Inline url="animated_child.x3d"/>
</Scene></X3D>
```

`runtime/parse/tests/inline_routes_test.cpp`:

```cpp
#include "x3d/sdk.hpp"
#include "runtime/events/X3DExecutionContext.hpp"
#include "runtime/events/X3DSceneBridge.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  assert(argc >= 2);
  std::string dir = argv[1];
  auto doc = x3d::sdk::parseFile(dir + "/animated_parent.x3d");

  // The child's two internal ROUTEs were hoisted, pre-resolved.
  assert(doc.scene.resolvedInlineRoutes.size() == 2 &&
         "both child ROUTEs hoisted with concrete endpoints");
  for (auto& r : doc.scene.resolvedInlineRoutes) {
    assert(r.from && r.to && "endpoints resolved in child scope");
  }

  // Bridge + tick: the inlined Transform should move.
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  auto bridge = ctx.buildFrom(doc.scene);
  assert(bridge.routesAdded >= 2 && "inline routes registered on the context");

  std::cout << "inline_routes_test OK\n";
  return 0;
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_routes --output-on-failure` (after Step 4)
Expected: FAIL — `bridge.routesAdded` does not include the 2 inline routes (bridge ignores `resolvedInlineRoutes`).

- [ ] **Step 3: Register `resolvedInlineRoutes` in `X3DSceneBridge.hpp`**

Immediately after the `resolvedProtoRoutes` registration loop (~line 122), add the parallel loop:

```cpp
  // Pre-resolved internal ROUTEs of expanded Inlines: endpoints are concrete
  // nodes in the inlined child's DEF scope (never in scene.defs), so register
  // them directly rather than via DEF-name lookup (mirrors resolvedProtoRoutes).
  for (const ResolvedProtoRoute &ir : scene.resolvedInlineRoutes) {
    if (!ir.from || !ir.to) continue;
    ctx.addRoute({ir.from.get(), ir.fromField}, {ir.to.get(), ir.toField});
    ++result.routesAdded;
  }
```

- [ ] **Step 4: Register the test**

```cmake
    add_executable(x3d_inline_routes
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_routes_test.cpp")
    target_link_libraries(x3d_inline_routes PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_routes
        COMMAND x3d_inline_routes
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")
```

- [ ] **Step 5: Build and run**

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_routes --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add runtime/events/X3DSceneBridge.hpp \
        runtime/parse/tests/inline_routes_test.cpp \
        runtime/parse/tests/data/inline/ CMakeLists.txt
git commit -m "feat(inline): hoist + register inlined internal ROUTEs (self-animating assets tick)"
```

---

## Task 5: Cycle test + full verification gate (golden + corpus + ctest)

**Files:**
- Test: `runtime/parse/tests/inline_cycle_test.cpp` (uses the `cycle_a.x3d`/`cycle_b.x3d` fixtures from Task 2)
- No production changes expected unless the gate surfaces a regression.

**Interfaces:**
- Consumes: everything above; the default `localFileInlineResolver` cycle guard.

- [ ] **Step 1: Write the cycle test**

`runtime/parse/tests/inline_cycle_test.cpp`:

```cpp
#include "x3d/sdk.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  assert(argc >= 2);
  std::string dir = argv[1];
  // a -> b -> a. Must terminate (no stack overflow / hang) and not crash.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_a.x3d");
  // The outermost Inline resolves b; b's Inline back to a is refused by the
  // cycle guard (returns null) -> b loads with an un-expanded Inline, which the
  // outer a splices in. Either way: we got here, so it terminated.
  std::cout << "inline_cycle_test OK (terminated, scene roots="
            << doc.scene.rootNodes.size() << ")\n";
  return 0;
}
```

- [ ] **Step 2: Register + run the cycle test**

```cmake
    add_executable(x3d_inline_cycle
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/inline_cycle_test.cpp")
    target_link_libraries(x3d_inline_cycle PRIVATE x3d_cpp::sdk)
    add_test(NAME x3d_inline_cycle
        COMMAND x3d_inline_cycle
                "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/data/inline")
```

Run: `mise run build` then `ctest --test-dir build -R x3d_inline_cycle --output-on-failure`
Expected: PASS (terminates; no hang).

- [ ] **Step 3: Run the FULL ctest suite**

Run: `mise run build` (it builds + runs ctest in parallel)
Expected: all tests PASS, including the prior 110+ and the 5 new `x3d_inline_*` tests.

- [ ] **Step 4: Golden byte-identity gate**

Run: `mise run golden`
Expected: zero drift. (Golden covers generated `*.hpp`; the writer redirect in Task 3 keeps Inline-bearing serialization byte-identical.)

- [ ] **Step 5: Corpus sweep**

Run: `mise run corpus`
Expected: no new crashes; OK-rate at or above the pre-change baseline (≈99.95%). Inline-bearing archive files now compose instead of leaving an inert `Inline`. If the sweep surfaces a crash, debug with the systematic-debugging skill, fix, and re-run before committing.

- [ ] **Step 6: Commit**

```bash
git add runtime/parse/tests/inline_cycle_test.cpp CMakeLists.txt
git commit -m "test(inline): cycle termination + full verification gate (ctest/golden/corpus green)"
```

---

## Self-Review

**Spec coverage** (against `2026-06-19-inline-expansion-design.md`):

| Spec requirement | Task |
|---|---|
| Parse-time expansion for `load=TRUE` via resolver | Task 2 |
| New `runtime/InlineExpand.hpp`, hooked after `expandScene` in `parseDocument` | Task 2 |
| `InlineResolver` mirroring `ProtoDeclarationResolver`; default `localFileInlineResolver` | Task 2 |
| DEF/USE isolation (child defs not in parent) | Task 2 (asserted) |
| Internal ROUTEs pre-resolved in child scope → `resolvedInlineRoutes`; bridge registers | Tasks 1, 4 |
| Self-reference / cycle detection (thread_local stack) | Tasks 2, 5 |
| `load=FALSE` → skip; retain url | Task 2 (`readLoad`) |
| `url` MFString → first-resolvable wins | Task 2 (`localFileInlineResolver` loop) |
| Writer round-trip byte-identical (re-emit `<Inline>`) | Tasks 3, 5 |
| `inlineWarnings` sibling of `protoWarnings` | Task 1 |
| Lenient error handling | Task 2 |
| Tests: composition, DEF isolation, self-animation, cycle, round-trip, corpus | Tasks 2–5 |
| `bbox`/`visible`/`global` passthrough | Implicit — preserved Inline node carries these fields unchanged; the synthetic Group is transform-free so content stays in the Inline's frame. No code needed. |
| child UNIT differs → warning + defer scaling | **Deferred (documented Tier-1 carve-out).** Not implemented; default units cover real content. Not a task. |

**Placeholder scan:** No TBD/TODO. The two NOTE callouts (DEF-getter name in Task 2 Step 3; JsonWriter hook in Task 3 Step 5) point the implementer at a specific file/line to confirm a name — concrete, not vague.

**Type consistency:** `expandInlines` / `InlineResolver` / `localFileInlineResolver` signatures match across Tasks 2, 4. `ResolvedProtoRoute{from, fromField, to, toField}` used consistently (Task 1 declares, Tasks 2/4 consume). `expandedInlines` keyed by `X3DNode*` → `shared_ptr<X3DNode>` consistent across Tasks 1, 2, 3. `Scene::resolveRoutes()` / `Route::from/to` weak_ptr usage matches `X3DRoute.hpp`.

**Known interim state:** between Task 2 and Task 3 the full golden/corpus suite may show Inline expansion not yet round-trip-safe; per-task unit tests stay green and the gate is enforced in Task 5. Flagged in Task 2's closing note.
