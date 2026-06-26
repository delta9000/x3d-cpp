# M2c — Binding Stacks Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the X3D binding-stack protocol for Viewpoint/NavigationInfo/Background/Fog — a per-category stack whose top is the bound node, driven by `set_bind` events on the M1 cascade, firing `isBound`/`bindTime` on transitions — in a context-owned side structure (golden byte-identical).

**Architecture:** `runtime/scene/BindingStack.hpp` (one-category stack logic with an Emit callback), `runtime/scene/BindingSystem.hpp` (category map, enrol bindable nodes + wire their `set_bind` handler, default-bind, bound query), wired into `X3DExecutionContext` with a pull API. `set_bind` flows through the existing cascade (its reflection thunk calls `onSet_bind` → the registered handler), which `postEvent`s `isBound`/`bindTime` into the same drain. No `tick` reorder, no codegen.

**Tech Stack:** C++20 header-only runtime; CMake + ctest via `mise run build`. Builds on the M1 cascade/context. **Golden gate** byte-identical: sha256 `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0` (runtime-only — drift is a bug). `mise run build` = cmake+build+ctest; golden pytest `tests/test_golden_tree.py` runs under `uv run pytest`.

**Spec:** `docs/superpowers/specs/2026-06-13-m2c-binding-stacks-design.md`.

**Verified facts:**
- `X3DBindableNode` (global namespace, generated) has `setOnSet_bindHandler(std::function<void(const SFBool&)>)`, `emitIsBound(const SFBool&)`, `emitBindTime(const SFTime&)`, `getIsBound()`, `onSet_bind(const SFBool&)`. The `set_bind` reflection `set` thunk calls `onSet_bind`; the `isBound`/`bindTime` thunks call the emits (so `ctx.postEvent(node,"isBound",std::any(SFBool(b)))` delivers → emit).
- Bindable concrete types: `Viewpoint`, `OrthoViewpoint`, `GeoViewpoint`, `NavigationInfo`, `Background`, `TextureBackground`, `Fog`. Abstract bases (global): `X3DViewpointNode`, `X3DBackgroundNode`, `X3DBindableNode`.
- `SFBool` = `bool`, `SFTime` = `double` (`generated_cpp_bindings/X3Dtypes.hpp`).
- `X3DExecutionContext` (namespace `x3d::runtime`): `postEvent(X3DNode*, const std::string&, std::any)`, `now()`, `process()`, `tick(now)`. Members are private; M2c adds `bindings_` beside `transforms_`/`bounds_` and pull methods.
- Reflection node-graph walk: `for (auto& f : n->fields()) { if(!f.get) continue; if (f.type==X3DFieldType::SFNode) ...; else if (MFNode) ...; }` (guard null getters — the M2a/M2b trap).
- `createX3DNode(type)` builds a node (`X3DNodeFactory.hpp`). Tests live in `runtime/<area>/tests/`, wired in ROOT `CMakeLists.txt`; `runtime/scene` is already on the interface include dirs.
- Generated node types are in the GLOBAL namespace (`X3DNode`, `X3DBindableNode`, `Viewpoint`); runtime classes are in `x3d::runtime`.

---

## File Structure

| File | Responsibility |
|------|----------------|
| `runtime/scene/BindingStack.hpp` (new) | one-category stack: top/bind/unbind/pushDefault, transitions via an Emit callback |
| `runtime/scene/tests/binding_stack_test.cpp` (new) | bind/move/unbind/default + emit sequence |
| `runtime/scene/BindingSystem.hpp` (new) | category map, enrolScene + set_bind handler wiring, bindDefaults, bound() |
| `runtime/scene/tests/binding_system_test.cpp` (new) | enrol viewpoints, fire set_bind, bound switches + isBound |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own BindingSystem; build hook; pull API |
| `runtime/events/tests/m2c_tick_test.cpp` (new) | end-to-end set_bind via tick switches bound + isBound |
| `CMakeLists.txt` (modify) | register 3 new test executables |

---

## Task 1: `BindingStack` — one-category stack logic

**Files:** Create `runtime/scene/BindingStack.hpp`, `runtime/scene/tests/binding_stack_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/scene/tests/binding_stack_test.cpp`:

```cpp
// binding_stack_test.cpp — pure stack logic; nodes are opaque pointers (never
// dereferenced), so fake pointers are fine.
#include "BindingStack.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace x3d::runtime;

int main() {
  int a=0,b=0; // identity stand-ins
  X3DNode* A = reinterpret_cast<X3DNode*>(&a);
  X3DNode* B = reinterpret_cast<X3DNode*>(&b);

  std::vector<std::pair<X3DNode*,bool>> ev;
  BindingStack::Emit emit = [&](X3DNode* n, bool bound){ ev.emplace_back(n,bound); };
  BindingStack s;

  s.pushDefault(A, emit);          // A bound
  assert(s.top()==A);
  assert(ev.size()==1 && ev[0].first==A && ev[0].second==true);

  ev.clear();
  s.bind(B, emit);                 // A->false, B->true
  assert(s.top()==B);
  assert(ev.size()==2 && ev[0]==std::make_pair(A,false) && ev[1]==std::make_pair(B,true));

  ev.clear();
  s.bind(B, emit);                 // already top: no-op
  assert(ev.empty() && s.top()==B);

  ev.clear();
  s.bind(A, emit);                 // A already on stack -> move to top: B->false, A->true
  assert(s.top()==A);
  assert(ev.size()==2 && ev[0]==std::make_pair(B,false) && ev[1]==std::make_pair(A,true));

  ev.clear();
  s.unbind(A, emit);               // top pops: A->false, new top B->true
  assert(s.top()==B);
  assert(ev.size()==2 && ev[0]==std::make_pair(A,false) && ev[1]==std::make_pair(B,true));

  ev.clear();
  s.unbind(A, emit);               // A not on stack (already removed): no-op
  assert(ev.empty() && s.top()==B);

  ev.clear();
  s.unbind(B, emit);               // last node pops: B->false, stack empty
  assert(s.top()==nullptr);
  assert(ev.size()==1 && ev[0]==std::make_pair(B,false));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt` (near the M2b `x3d_bounds_system` block):

```cmake
    add_executable(x3d_binding_stack
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_stack_test.cpp")
    target_link_libraries(x3d_binding_stack PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_binding_stack COMMAND x3d_binding_stack)
```

Run `mise run build` → FAIL (`BindingStack.hpp` not found).

- [ ] **Step 3: Implement `runtime/scene/BindingStack.hpp`:**

```cpp
// BindingStack.hpp — one bindable category's stack. Top = bound node. Transitions
// are reported through an Emit callback (decoupled from the context). The X3D
// binding protocol (ISO/IEC 19775-1 7.2.2). namespace x3d::runtime.
#ifndef X3D_RUNTIME_BINDING_STACK_HPP
#define X3D_RUNTIME_BINDING_STACK_HPP

#include <algorithm>
#include <functional>
#include <vector>

class X3DNode;

namespace x3d::runtime {

class BindingStack {
public:
  // emit(node, true)  => node became bound (caller posts isBound TRUE + bindTime)
  // emit(node, false) => node became unbound (caller posts isBound FALSE)
  using Emit = std::function<void(X3DNode *, bool)>;

  X3DNode *top() const { return stack_.empty() ? nullptr : stack_.back(); }

  // set_bind TRUE: move/push node to top.
  void bind(X3DNode *node, const Emit &emit) {
    if (!stack_.empty() && stack_.back() == node) return; // already bound
    X3DNode *prev = top();
    remove(node);
    stack_.push_back(node);
    if (prev && prev != node) emit(prev, false);
    emit(node, true);
  }

  // set_bind FALSE: pop if top (next becomes bound), else just remove.
  void unbind(X3DNode *node, const Emit &emit) {
    if (stack_.empty()) return;
    if (stack_.back() == node) {
      stack_.pop_back();
      emit(node, false);
      if (X3DNode *nt = top()) emit(nt, true);
    } else {
      remove(node); // non-top removal: no bound transition
    }
  }

  // Startup default-bind: push + announce bound (no previous-top semantics).
  void pushDefault(X3DNode *node, const Emit &emit) {
    stack_.push_back(node);
    emit(node, true);
  }

private:
  void remove(X3DNode *node) {
    stack_.erase(std::remove(stack_.begin(), stack_.end(), node), stack_.end());
  }
  std::vector<X3DNode *> stack_;
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BINDING_STACK_HPP
```

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_binding_stack`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/scene/BindingStack.hpp runtime/scene/tests/binding_stack_test.cpp CMakeLists.txt
git commit -m "M2c: BindingStack one-category bind/move/unbind logic"
```

---

## Task 2: `BindingSystem` — enrol, wire set_bind, default-bind, query

**Files:** Create `runtime/scene/BindingSystem.hpp`, `runtime/scene/tests/binding_system_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/scene/tests/binding_system_test.cpp`:

```cpp
// binding_system_test.cpp — enrol bindable nodes, default-bind, then fire set_bind
// through the wired handler (which posts isBound via the supplied poster/clock).
// BindingSystem is decoupled from X3DExecutionContext via callbacks; here the
// callbacks drive a real context so the posted isBound events get delivered.
#include "BindingSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include <cassert>
#include <memory>
#include <string>
using namespace x3d::runtime;

static X3DBindableNode* bindable(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

int main() {
  auto vp1 = createX3DNode("Viewpoint");
  auto vp2 = createX3DNode("Viewpoint");
  auto nav = createX3DNode("NavigationInfo");
  Scene scene;
  scene.addRootNode(vp1); scene.addRootNode(vp2); scene.addRootNode(nav);

  X3DExecutionContext ctx;
  BindingSystem bs;
  bs.enrollScene(scene,
      [&](X3DNode* n, const std::string& f, std::any v){ ctx.postEvent(n, f, std::move(v)); },
      [&]{ return ctx.now(); });
  bs.bindDefaults();

  // Defaults: first of each category bound.
  assert(bs.bound("Viewpoint") == vp1.get());
  assert(bs.bound("NavigationInfo") == nav.get());
  assert(bindable(vp1)->getIsBound() == true);
  assert(bindable(vp2)->getIsBound() == false);

  // Fire set_bind TRUE on vp2 via its onSet_bind (as the cascade would), then drain
  // so the posted isBound events are delivered to the emit thunks.
  bindable(vp2)->onSet_bind(true);
  ctx.process();
  assert(bs.bound("Viewpoint") == vp2.get());
  assert(bindable(vp2)->getIsBound() == true);
  assert(bindable(vp1)->getIsBound() == false);
  // NavigationInfo category untouched by a Viewpoint bind.
  assert(bs.bound("NavigationInfo") == nav.get());

  // set_bind FALSE on vp2 -> back to vp1.
  bindable(vp2)->onSet_bind(false);
  ctx.process();
  assert(bs.bound("Viewpoint") == vp1.get());
  assert(bindable(vp1)->getIsBound() == true);
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_binding_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/binding_system_test.cpp")
    target_link_libraries(x3d_binding_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_binding_system COMMAND x3d_binding_system)
```

Run `mise run build` → FAIL (`BindingSystem.hpp` not found).

- [ ] **Step 3: Implement `runtime/scene/BindingSystem.hpp`:**

```cpp
// BindingSystem.hpp — owns one BindingStack per bindable category, enrols bindable
// nodes from a Scene, wires each node's set_bind handler to drive its stack, and
// default-binds the first of each category. Decoupled from X3DExecutionContext via
// a Poster (post an event) + Clock (current time) callback — so there is NO include
// cycle with the context. Context-owned, purely event-driven. namespace x3d::runtime.
#ifndef X3D_RUNTIME_BINDING_SYSTEM_HPP
#define X3D_RUNTIME_BINDING_SYSTEM_HPP

#include "BindingStack.hpp"
#include "X3DBackgroundNode.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"
#include "X3DViewpointNode.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

class BindingSystem {
public:
  using Poster = std::function<void(X3DNode *, const std::string &, std::any)>;
  using Clock = std::function<double()>;

  // Enrol every bindable node; wire its set_bind handler to post via `poster`.
  void enrollScene(const Scene &scene, Poster poster, Clock clock) {
    poster_ = std::move(poster);
    clock_ = std::move(clock);
    for (const auto &root : scene.rootNodes)
      if (root) walk(root.get());
  }

  // Bind the first enrolled node of each category that has no current top.
  // Runs at build time (no active cascade) -> set isBound directly via reflection.
  void bindDefaults() {
    BindingStack::Emit emit = [](X3DNode *t, bool bound) { setIsBound(t, bound); };
    for (auto &kv : enrolled_) {
      const std::string &cat = kv.first;
      if (!kv.second.empty() && !stacks_[cat].top())
        stacks_[cat].pushDefault(kv.second.front(), emit);
    }
  }

  X3DNode *bound(const std::string &category) const {
    auto it = stacks_.find(category);
    return it == stacks_.end() ? nullptr : it->second.top();
  }

  // Category of a bindable node: viewpoint family / background family / own type.
  static std::string category(X3DNode *n) {
    if (dynamic_cast<X3DViewpointNode *>(n)) return "Viewpoint";
    if (dynamic_cast<X3DBackgroundNode *>(n)) return "Background";
    return n->nodeTypeName();
  }

private:
  static void setIsBound(X3DNode *n, bool bound) {
    for (const auto &f : n->fields())
      if (f.x3dName == "isBound" && f.set) { f.set(*n, std::any(SFBool(bound))); return; }
  }

  void enroll(X3DNode *node) {
    auto *b = dynamic_cast<X3DBindableNode *>(node);
    if (!b) return;
    const std::string cat = category(node);
    enrolled_[cat].push_back(node);
    b->setOnSet_bindHandler([this, node, cat](const SFBool &v) {
      BindingStack::Emit emit = [this](X3DNode *t, bool bound) {
        poster_(t, "isBound", std::any(SFBool(bound)));
        if (bound) poster_(t, "bindTime", std::any(SFTime(clock_())));
      };
      if (v) stacks_[cat].bind(node, emit);
      else   stacks_[cat].unbind(node, emit);
    });
  }

  void walk(X3DNode *n) {
    enroll(n);
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) walk(c.get());
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get());
      }
    }
  }

  Poster poster_;
  Clock clock_;
  std::unordered_map<std::string, BindingStack> stacks_;
  std::unordered_map<std::string, std::vector<X3DNode *>> enrolled_;
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BINDING_SYSTEM_HPP
```

Note: include whatever header declares `SFBool`/`SFTime`/`X3DFieldType`/`FieldInfo` if not transitively available — `X3DNode.hpp` pulls in reflection + types here; if a symbol is missing add `#include "X3DReflection.hpp"` and `#include "X3Dtypes.hpp"`.

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_binding_system`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/scene/BindingSystem.hpp runtime/scene/tests/binding_system_test.cpp CMakeLists.txt
git commit -m "M2c: BindingSystem enrol + set_bind wiring + default-bind + bound query"
```

---

## Task 3: `X3DExecutionContext` integration + pull API

**Files:** Modify `runtime/events/X3DExecutionContext.hpp`; create `runtime/events/tests/m2c_tick_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/events/tests/m2c_tick_test.cpp`:

```cpp
// m2c_tick_test.cpp — end-to-end: buildSceneGraph default-binds the first Viewpoint;
// a set_bind event delivered through tick switches the bound node and fires isBound.
#include "X3DExecutionContext.hpp"
#include "X3DBindableNode.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include <cassert>
#include <memory>
using namespace x3d::runtime;
static X3DBindableNode* bnd(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

int main() {
  auto vp1 = createX3DNode("Viewpoint");
  auto vp2 = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp1); scene.addRootNode(vp2);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  assert(ctx.boundViewpoint() == vp1.get());
  assert(bnd(vp1)->getIsBound() == true);

  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(true)));
  ctx.tick(1.0);
  assert(ctx.boundViewpoint() == vp2.get());
  assert(bnd(vp2)->getIsBound() == true);
  assert(bnd(vp1)->getIsBound() == false);

  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(false)));
  ctx.tick(2.0);
  assert(ctx.boundViewpoint() == vp1.get());
  assert(bnd(vp1)->getIsBound() == true);
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_m2c_tick
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2c_tick_test.cpp")
    target_link_libraries(x3d_m2c_tick PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_m2c_tick COMMAND x3d_m2c_tick)
```

Run `mise run build` → FAIL (`boundViewpoint` not a member).

- [ ] **Step 3: Wire BindingSystem into the context.** In `runtime/events/X3DExecutionContext.hpp`:

Add include: `#include "BindingSystem.hpp"`. Add a private member next to `bounds_`:
```cpp
  BindingSystem bindings_;
```

In `buildSceneGraph`, after the bounds build, add (the lambdas adapt the context's
`postEvent`/`now` to BindingSystem's Poster/Clock — no include cycle):
```cpp
    bindings_.enrollScene(scene,
        [this](X3DNode *n, const std::string &f, std::any v) { postEvent(n, f, std::move(v)); },
        [this] { return now(); });
    bindings_.bindDefaults();
```

Add the pull API (public), next to M2b's `worldBounds`:
```cpp
  /// Pull surface: the currently bound node per bindable category (null if none).
  X3DNode *boundBindable(const std::string &category) const { return bindings_.bound(category); }
  X3DNode *boundViewpoint() const { return bindings_.bound("Viewpoint"); }
  X3DNode *boundNavigationInfo() const { return bindings_.bound("NavigationInfo"); }
  X3DNode *boundBackground() const { return bindings_.bound("Background"); }
  X3DNode *boundFog() const { return bindings_.bound("Fog"); }
```

No `tick` change is needed: `set_bind` is delivered by the existing `cascade_.process()`,
invoking the wired handler, which posts `isBound`/`bindTime` (via the Poster) into the
same drain. `BindingSystem.hpp` does NOT include `X3DExecutionContext.hpp` (it uses the
Poster/Clock callbacks), so owning a `BindingSystem` member in the context is cycle-free.

- [ ] **Step 4: Run PASS (no regression).** `mise run build` then `ctest --preset dev -R 'x3d_m2c_tick|x3d_binding|x3d_m2a_tick|x3d_m2b_tick|x3d_event|animation|scene_bridge' --output-on-failure`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/events/X3DExecutionContext.hpp runtime/events/tests/m2c_tick_test.cpp CMakeLists.txt
git commit -m "M2c: wire BindingSystem into the execution context (build hook + bound* pull API)"
```

---

## Task 4: Full verification

**Files:** none.

- [ ] **Step 1: Full build + suite.** `mise run build` → ctest all green (adds `x3d_binding_stack`, `x3d_binding_system`, `x3d_m2c_tick`); `uv run pytest` green incl. `tests/test_golden_tree.py`. No M2a/M2b/event regression.
- [ ] **Step 2: Golden byte-identical.** `bash scripts/check_golden.sh` → "byte-for-byte"; sha256 unchanged from `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. If changed, a runtime edit leaked into a generated template — stop and fix.
- [ ] **Step 3: Record outcome.** New ctest count, golden unchanged, M2c (binding) in place; the bound-node-effect (camera pose, fog/background params) remains the M2.5 extraction concern; per-Layer stacks are backlog M2C-BIND-1.

---

## Notes for the implementer

- **TDD discipline:** test first, confirm RED, then GREEN.
- **Golden gate is the safety net** — nothing here is codegen; sha256 must never move.
- **dynamic_cast is intentional** here: BindingSystem keys off the generated `X3DBindableNode`/`X3DViewpointNode`/`X3DBackgroundNode` families (binding is defined precisely on them).
- **No include cycle:** `BindingSystem.hpp` is decoupled from the context via the Poster/Clock callbacks — it never includes `X3DExecutionContext.hpp`. The context includes `BindingSystem.hpp` and owns it directly. Keep it that way.
- **Default-bind has no cascade** — `bindDefaults`' Emit sets `isBound` via the reflection `set` thunk, not `postEvent`.
- **Don't touch codegen** — all edits in `runtime/**` + `CMakeLists.txt`.
```
