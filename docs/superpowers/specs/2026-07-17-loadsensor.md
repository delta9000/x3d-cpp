# LoadSensor Runtime Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (or subagent-driven-development) to implement this plan task-by-task. Repo process discipline: `docs/contributor/card-to-done-workflow.md` + `docs/contributor/workflow-subagent-discipline.md`.

**Goal:** Make `LoadSensor` a live X3DNetworkSensorNode: a `LoadSensorSystem` observes watched children's load state per tick via the AssetResolver seam and emits `isActive`/`isLoaded`/`loadTime`/`progress` per spec §9.4.3, closing findings NSN-1..7 and NSN-9.

**Architecture:** Per the approved design `docs/superpowers/specs/2026-07-17-loadsensor-design.md` — time-driven `System` with per-sensor state in a map (never on the node), an injected `extract::AssetResolver` (default: new SEC-3-confined local-file resolver), parse-time pre-seed from `Scene::expandedInlines`, poll-and-diff for NSN-7 resets, and a `ChildLoadPolicy` hook for headed embedders (Anchor cases).

**Tech Stack:** C++20, doctest (`x3d_events_tests` + a new `x3d_fileresolver_test`), CMake dev preset. BUILD RULE: `cmake --build --preset dev`. Test: `ctest --preset dev -R <target>`.

**Key facts an implementer must not rediscover the hard way:**
- Events: `ctx.postEvent(node, "isLoaded", std::any(SFBool{true}))` — post the **base** x3dName; the cascade applies the node's emit-thunk during `tick`, so getters reflect posted values after the tick.
- `Scene::expandedInlines` maps `Group*` → original `Inline*` (parse-time expansion replaces the Inline **inside `LoadSensor.children`** too — the walk is blind over SFNode/MFNode fields).
- `X3DUrlObject` getters: `getUrl()` → `MFString`, `getLoad()` → `SFBool`. Watch-filter: `x3d::nodes::X3DInterfaceRegistry::nodeImplements(*n, x3d::nodes::InterfaceId::X3DUrlObject)`.
- SEC-3 confinement: `x3d::codec::confineLocalIncludePath(url, baseDir, confineRoot)` in `runtime/parse/PathConfine.hpp` (rejects absolute paths and scheme-bearing urls); root is `x3d::runtime::detail::activeConfineRoot()` from `runtime/parse/X3DParse.hpp` (falls back to per-call base when empty).
- `LoadSensor` generated API: `getChildren()/setChildren(MFNode)`, `getTimeOut()`, `getEnabled()` (via X3DSensorNode), outputs read via `getIsActive()/getIsLoaded()/getLoadTime()/getProgress()`.
- Doctest case names are NOT ctest targets (doc-ctest-gate). Wiki pages may only cite real `add_test(NAME …)` targets.

---

### Task 0: Card, issue, branch (process gate)

Per `docs/contributor/card-to-done-workflow.md`. No code exists on `main` until the chain is live.

**Step 1: Create the Project draft card**

```bash
gh project item-create 2 --owner delta9000 \
  --title "Core — LoadSensor runtime system (NSN-1..9)" \
  --body "Wire LoadSensor as an active System over the AssetResolver seam. Design: docs/superpowers/specs/2026-07-17-loadsensor-design.md. Closes findings NSN-1,2,3,4,5,6,7,9."
```
Expected: a draft card appears in Backlog (`scripts/pick-card.sh --list | rg -i loadsensor`). If `gh` lacks the `project` scope, stop and ask the human to create the card.

**Step 2: Convert to issue**

```bash
scripts/pick-card.sh "LoadSensor runtime system"
```
Expected: prints the issue number N and handoff packet. Record N — every commit and the PR reference it.

**Step 3: Branch**

```bash
git checkout -b feat/loadsensor-runtime
```

**Step 4: Commit the design + plan on the branch**

```bash
git add docs/superpowers/specs/2026-07-17-loadsensor-design.md docs/superpowers/specs/2026-07-17-loadsensor.md
git commit -m "docs(loadsensor): runtime design + implementation plan (#N)"
```

---

### Task 1: FileResolver backend (SEC-3-confined local files)

**Files:**
- Create: `runtime/io/file/FileResolver.hpp`
- Test: `runtime/io/tests/file_resolver_test.cpp`
- Modify: `CMakeLists.txt` (new `x3d_fileresolver_test` executable near the `x3d_assetresolver_swap` block, ~line 790)

**Step 1: Write the failing test**

```cpp
#include "doctest/doctest.h"
#include "io/file/FileResolver.hpp"
#include <fstream>

using namespace x3d::runtime;
using x3d::runtime::extract::AssetKind;

TEST_CASE("FileResolver: confined existing file is Ready, others Failed") {
  // FIXTURES_DIR is compile-defined; write no files, use committed fixtures.
  auto r = io::file::makeFileResolver(FIXTURES_DIR);
  CHECK(r("f1.bin", AssetKind::Texture).ready());
  CHECK(r("missing.bin", AssetKind::Texture).failed());
  CHECK(r("../escape.bin", AssetKind::Texture).failed());   // outside confine root
  CHECK(r("/etc/hostname", AssetKind::Texture).failed());   // absolute rejected
  CHECK(r("https://example.com/x", AssetKind::Texture).failed()); // scheme not ours
  CHECK(r("f1.bin#frag", AssetKind::Texture).ready());      // fragment stripped
  CHECK_FALSE(r("f1.bin", AssetKind::Texture).pending());   // never Pending
}
```
Reuse the existing committed fixture `runtime/io/tests/fixtures/f1.bin`.

**Step 2: Run to verify it fails**

```bash
cmake --build --preset dev --target x3d_fileresolver_test && ctest --preset dev -R x3d_fileresolver_test
```
Expected: build failure (`io/file/FileResolver.hpp` not found).

**Step 3: Implement**

```cpp
#ifndef X3D_RUNTIME_IO_FILE_RESOLVER_HPP
#define X3D_RUNTIME_IO_FILE_RESOLVER_HPP

// Local-file AssetResolver backend (Networking level 1: file: protocol).
// Answers Ready when `url` resolves inside the SEC-3 confinement root
// (ADR-0038, x3d::codec::confineLocalIncludePath) and is readable; Failed
// otherwise. Never Pending — v1 local I/O is synchronous.

#include "../../extract/AssetResolver.hpp"
#include "../../parse/PathConfine.hpp"
#include "../../parse/X3DParse.hpp" // detail::activeConfineRoot()

#include <fstream>
#include <string>

namespace x3d::runtime::io::file {

inline extract::AssetResolver makeFileResolver(std::string baseDir = "") {
  return [base = std::move(baseDir)](const std::string &urlIn,
                                     extract::AssetKind) -> extract::AssetResult {
    std::string url = urlIn.substr(0, urlIn.find('#')); // strip fragment
    auto confined = x3d::codec::confineLocalIncludePath(url, base, detail::activeConfineRoot());
    if (!confined) return extract::AssetResult::makeFailed();
    std::ifstream f(*confined, std::ios::binary);
    if (!f) return extract::AssetResult::makeFailed();
    return extract::AssetResult::makeReady(
        std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {}));
  };
}

} // namespace x3d::runtime::io::file
#endif
```

CMake (mirror the `x3d_assetresolver_swap` block shape):

```cmake
if(X3D_CPP_BUILD_TESTS)
    add_executable(x3d_fileresolver_test
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/file_resolver_test.cpp")
    target_link_libraries(x3d_fileresolver_test PRIVATE x3d_cpp::sdk x3d_doctest_main)
    target_include_directories(x3d_fileresolver_test PRIVATE runtime)
    target_compile_definitions(x3d_fileresolver_test PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures")
    add_test(NAME x3d_fileresolver_test COMMAND x3d_fileresolver_test)
endif()
```
Check an adjacent block for the exact include-dir convention and copy it.

**Step 4: Run to verify it passes**

`cmake --build --preset dev --target x3d_fileresolver_test && ctest --preset dev -R x3d_fileresolver_test` — Expected: PASS.

**Step 5: Commit** `feat(loadsensor): SEC-3-confined local-file AssetResolver backend (#N)`

---

### Task 2: LoadSensorSystem skeleton + registration

**Files:**
- Create: `runtime/events/LoadSensorSystem.hpp`
- Test: `runtime/events/tests/loadsensor_test.cpp`
- Modify: `runtime/events/X3DSceneBridge.hpp` (add `attachLoadSensors`, extend `attachStandardRuntime`), `CMakeLists.txt` (add the test to `x3d_events_tests` sources, ~line 2453)

**Step 1: Failing test — attaches, activates, first progress**

```cpp
struct Rig {
  Scene scene;
  X3DExecutionContext ctx;
  std::shared_ptr<x3d::nodes::LoadSensor> ls;
  std::shared_ptr<x3d::nodes::ImageTexture> tex;
  std::shared_ptr<LoadSensorSystem> sys;
  explicit Rig(extract::AssetResolver r) {
    ls = std::make_shared<x3d::nodes::LoadSensor>();
    tex = std::make_shared<x3d::nodes::ImageTexture>();
    tex->setUrl(MFString{"a.png"});
    ls->setChildren(MFNode{tex});
    scene.addRootNode(ls);          // match view_dependent_test's scene setup
    ctx.buildSceneGraph(scene);
    sys = std::make_shared<LoadSensorSystem>(std::move(r));
    sys->setScene(&scene);
    sys->attach(ls.get(), ctx);
    ctx.addSystem(sys);
  }
};

TEST_CASE("LoadSensor: pending child activates the sensor") {
  Rig rig([](const std::string &, extract::AssetKind) {
    return extract::AssetResult::makePending();
  });
  rig.ctx.tick(1.0);
  CHECK(rig.ls->getIsActive());
  CHECK_FALSE(rig.ls->getIsLoaded());
  CHECK(rig.ls->getProgress() == 0.0f);
}
```

**Step 2:** build `x3d_events_tests` → fails (header missing).

**Step 3: Implement the skeleton** (state structs + activation; resolution lands in Task 3):

```cpp
#ifndef X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP

#include "X3DSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "../X3DScene.hpp"
#include "../extract/AssetResolver.hpp"
#include "../io/file/FileResolver.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/X3DUrlObject.hpp"
#include "x3d/nodes/X3DInterfaceRegistry.hpp"

namespace x3d::runtime {

enum class ChildStatus { NotStarted, Loading, Ready, Failed };

struct ChildLoadPlan { bool watch = true; bool vacuousReady = false;
                       extract::AssetKind kind = extract::AssetKind::Inline; };
using ChildLoadPolicy = std::function<ChildLoadPlan(X3DNode *child, const Scene &)>;

class LoadSensorSystem : public System {
public:
  explicit LoadSensorSystem(extract::AssetResolver resolver = nullptr,
                            std::string baseUrl = "")
      : resolver_(resolver ? std::move(resolver)
                           : io::file::makeFileResolver(baseUrl)),
        baseUrl_(std::move(baseUrl)) {}

  void setScene(const Scene *s) { scene_ = s; }
  void setChildLoadPolicy(ChildLoadPolicy p) { policy_ = std::move(p); }
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h) { sensorHook_ = std::move(h); }
  void setChildStateHook(std::function<void(X3DNode *, X3DNode *, ChildStatus)> h) { childHook_ = std::move(h); }

  void attach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *ls = dynamic_cast<x3d::nodes::LoadSensor *>(node))
      state_.emplace(ls, SensorState{});
  }

  void update(double now, X3DExecutionContext &ctx) override; // Tasks 3-6 fill this in

private:
  struct ChildState {
    ChildStatus status = ChildStatus::NotStarted;
    std::size_t candidate = 0;       // next MFString index to try
    MFString lastUrl;                // NSN-7 snapshots
    bool lastLoad = true;
    bool preseeded = false;
  };
  struct SensorState {
    std::unordered_map<X3DNode *, ChildState> children;
    bool active = false;
    bool terminal = false;           // burst emitted; idle until an NSN-7 reset
    bool everEvaluated = false;      // R7 first-evaluation
    bool enabled = true;
    double activatedAt = 0.0;
    float lastProgress = -1.0f;
  };

  template <typename T>
  void emit(X3DExecutionContext &ctx, X3DNode *n, const char *field, T v) {
    ctx.postEvent(n, field, std::any(v));
  }

  extract::AssetResolver resolver_;
  std::string baseUrl_;
  const Scene *scene_ = nullptr;
  ChildLoadPolicy policy_;
  std::function<void(X3DNode *, bool, double)> sensorHook_;
  std::function<void(X3DNode *, X3DNode *, ChildStatus)> childHook_;
  std::unordered_map<x3d::nodes::LoadSensor *, SensorState> state_;
  std::unordered_set<std::string> readyMemo_; // ADR-0045: memo Ready only
};

} // namespace x3d::runtime
#endif
```

Bridge helper in `X3DSceneBridge.hpp` (mirrors `attachKeyDeviceSensors`; return the system so embedders can set the policy/hooks, like `attachInteractive` returns NavigationSystem):

```cpp
inline std::shared_ptr<LoadSensorSystem>
attachLoadSensors(Scene &scene, X3DExecutionContext &ctx,
                  extract::AssetResolver resolver = nullptr,
                  std::string baseUrl = "") {
  auto sys = std::make_shared<LoadSensorSystem>(std::move(resolver), std::move(baseUrl));
  sys->setScene(&scene);
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
  return sys;
}
```
Extend `attachStandardRuntime(Scene&, X3DExecutionContext&,
extract::AssetResolver assetResolver = nullptr, std::string baseUrl = "")` to call
`attachLoadSensors(scene, ctx, std::move(assetResolver), std::move(baseUrl))` after
`attachKeyDeviceSensors`. Update its doc comment (adds §9 LoadSensor to the list).

**Step 3b:** minimal `update()` that activates on any loadable child (Task 3 replaces it):

```cpp
inline void LoadSensorSystem::update(double now, X3DExecutionContext &ctx) {
  for (auto &[ls, st] : state_) {
    if (!ls->getEnabled()) continue;
    for (auto &childAny : ls->getChildren()) {
      X3DNode *c = childAny.get();
      if (!c || !x3d::nodes::X3DInterfaceRegistry::nodeImplements(
                    *c, x3d::nodes::InterfaceId::X3DUrlObject)) continue;
      if (!st.active) {
        st.active = true; st.activatedAt = now;
        emit(ctx, ls, "isActive", SFBool{true});
        emit(ctx, ls, "progress", SFFloat{0.0f});
        if (sensorHook_) sensorHook_(ls, true, now);
      }
    }
  }
}
```

**Step 4:** build + `ctest --preset dev -R x3d_events_tests` → PASS (all prior suites unaffected).

**Step 5: Commit** `feat(loadsensor): system skeleton + standard-runtime registration (#N)`

---

### Task 3: Child resolution core — poll, fallback, pre-seed, success burst, progress

**Files:** Modify `runtime/events/LoadSensorSystem.hpp`, `runtime/events/tests/loadsensor_test.cpp`

This task replaces the skeleton `update()` with the real per-tick algorithm:

```
for each (ls, st):
  1. enabled gate: !getEnabled() -> if st.active: emit isActive FALSE + hook; st = SensorState{}; st.enabled=false; continue.
  2. refresh watch set from ls->getChildren():
     - ptr in scene_->expandedInlines  -> ensure ChildState{Ready, preseeded=true}
     - nodeImplements X3DUrlObject     -> ensure ChildState; apply policy_ (default plan below);
                                          plan.watch==false -> skip; plan.vacuousReady -> Ready
     - else                            -> not watched
     - erase ChildStates whose pointer left the watch set (membership diff)
     - NSN-7 diff (non-preseeded): read getUrl()/getLoad();
         url != lastUrl     -> status=Loading, candidate=0, st.terminal=false, restartTimeout=true
         load TRUE->FALSE   -> status=NotStarted; FALSE->TRUE -> status=NotStarted (starts this tick)
  3. if st.terminal: continue (idle until reset)
  4. poll each watched child (dedupe by pointer):
     - preseeded/Ready/Failed: skip
     - load==FALSE: stays NotStarted
     - url empty: Ready (R3)
     - walk candidates from child.candidate: embedded scheme (ecmascript:/javascript:/data:) -> Ready;
       readyMemo_ hit -> Ready; else ONE resolver call: Ready -> memo + Ready;
       Failed -> ++candidate (exhausted -> child Failed); Pending -> child Loading, stop walking
  5. restartTimeout -> st.activatedAt = now
  6. aggregate over unique watched children:
     watched==0                      -> vacuous success (R6), only when !st.everEvaluated
     any Failed                      -> terminal failure burst
     all Ready                       -> terminal success burst
     any Loading / any loadable      -> ensure activated (isActive TRUE + progress), progress emit-on-change
  7. timeout: st.active && timeOut>0 && now-st.activatedAt > timeOut -> terminal failure burst
  8. st.everEvaluated = true
```

Emission rules (design §3 table — implement exactly):

- first-eval all-Ready (incl. vacuous): `isLoaded=TRUE, loadTime=now, progress=1` — NO isActive pulse (R7).
- first-eval failure with no prior activation: `isLoaded=FALSE` only.
- activation: `isActive=TRUE` + `progress=readyCount/watchedCount`.
- while active: `progress` when changed.
- terminal success after activation: `isLoaded=TRUE, loadTime=now, progress=1, isActive=FALSE`.
- terminal failure after activation: `isLoaded=FALSE, isActive=FALSE`.

**Step 1: Failing tests** (scripted resolver: `std::map<std::string, std::deque<extract::AssetResult>>`):

1. single-child Ready → first tick: full NSN-9 burst, `isActive` still FALSE (R7).
2. Pending,Pending,Ready over ticks 1..3 → isActive TRUE at t1; success burst at t3; `getLoadTime()==3.0`.
3. two children, one pre-Ready (scripted) one Pending → activation progress 0.5; 1.0 on completion.
4. MFString fallback: url `["bad.png","good.png"]`, bad→Failed, good→Ready → child Ready, success.
5. pre-seed: `parseDocument` a small scene with a real Inline fixture (add `runtime/events/tests/fixtures/inline_child.x3d` + a parent string parsed with baseUrl) → `attachLoadSensors` → first tick burst, no isActive pulse, and `state_` shows the Group pre-seeded (assert via childHook).
6. all-candidates-Failed → `isLoaded=FALSE`, `isActive` FALSE after activation.

**Step 2:** run → fail. **Step 3:** implement the algorithm. **Step 4:** run → pass.

**Step 5: Commit** `feat(loadsensor): child load-state machine + aggregate event bursts (#N)`

---

### Task 4: Timeout and enabled lifecycle

**Files:** same two.

**Step 1: Failing tests**

1. `timeOut=5`, resolver always Pending: t1 active; tick at t=7 → `isLoaded=FALSE`, `isActive=FALSE`, no `loadTime` (stays 0).
2. `timeOut=0` (default), always Pending: t=1000 still active, no failure events.
3. `setEnabled(false)` mid-load: next tick → `isActive=FALSE`, no `isLoaded`; re-enable → fresh cycle (activation re-fires; timeout window restarts).
4. timeout window starts at activation, not at scene start: sensor attached at t=10, timeOut=5 → failure lands after t=15, not after t=5.

**Steps 2-4:** fail → implement (timeout check + enabled gate per algorithm) → pass.

**Step 5: Commit** `feat(loadsensor): timeOut deadline + enabled lifecycle (#N)`

---

### Task 5: NSN-7 resets

**Files:** same two.

**Step 1: Failing tests**

1. url change after success: child Ready at t1 (burst); `tex->setUrl({"b.png"})` at t2; tick t3 → sensor re-activates (`isActive=TRUE` again), `isLoaded` NOT re-emitted FALSE (R4 — assert via hook log: no isLoaded event between), new resolver answer Ready → second success burst at t4.
2. url change restarts the timeout window: timeOut=5, activate t1, change url at t4, always-Pending → failure lands after t9, not t5.
3. `load` TRUE→FALSE after success: child drops to NotStarted; sensor idles (no events); FALSE→TRUE → re-activation and re-resolution.
4. membership: `setChildren({})` after success → no further events; add a new child → fresh evaluation.
5. USE dedup (R5): `setChildren({tex, tex})` → progress denominator 1 (success burst at 1.0 directly).

**Step 5: Commit** `feat(loadsensor): watched-child change resets (NSN-7) (#N)`

---

### Task 6: Edge rulings, policy hook, ROUTE end-to-end

**Files:** same two.

**Step 1: Failing tests**

1. R3: child with empty url + load=TRUE → vacuous Ready (success burst).
2. R6: `children=[]` → vacuous success burst on first tick (documented ruling).
3. Embedded scheme: Script-like child url `{"ecmascript: ..."}` → Ready with zero resolver calls (assert call count).
4. Anchor ruling: Anchor child url `{"#Doorway"}` + a `Viewpoint` DEF'd `Doorway` in the scene → Ready; missing Viewpoint → resolver asked (Failed by file resolver → child Failed).
5. Policy hook: `setChildLoadPolicy` returning `watch=false` for a child → child ignored; a policy returning vacuousReady → burst without resolver calls.
6. ROUTE e2e (standard_runtime_test pattern): XML string with `LoadSensor` + `ROUTE LS.loadTime TO script/field`-style target (use a TimeSensor-stopTime or another settable SFTime sink as in existing tests) → parse → `buildFrom` → `attachStandardRuntime` (resolver injected) → tick → assert the sink received the value.

**Step 5: Commit** `feat(loadsensor): edge rulings + child-load policy hook + ROUTE e2e (#N)`

---

### Task 7: CLI + RuntimeSession pass-through

**Files:** Modify `tools/x3d-cli/sim_runtime.hpp` (`attachFullRuntime`, ~line 119), `runtime/extract/RuntimeSession.hpp` (~line 150)

- `SessionOptions` gains `extract::AssetResolver assetResolver = nullptr; std::string baseUrl = "";` (doc comment: null → SEC-3 local-file default).
- `RuntimeSession` passes them to `attachStandardRuntime`.
- `attachFullRuntime` calls `attachLoadSensors(scene, ctx)` (defaults) — `x3d sim` gets LoadSensor behavior out of the box for local files.
- Update the `attachStandardRuntime` doc-comment system list in `RuntimeSession.hpp` line 10 if it enumerates clauses.

**Test:** extend `standard_runtime_test.cpp`-style smoke: session over a LoadSensor scene with default options (file resolver, baseUrl = fixture dir) → burst observed. If the CLI sim harness exposes a simpler seam, use it — do not build new harness.

**Commit** `feat(loadsensor): CLI + RuntimeSession resolver pass-through (#N)`

---

### Task 8: Docs, findings, gates (DoD — all in the same PR)

Per CLAUDE.md anti-drift discipline; nothing is "done" until these land.

1. `docs/conformance/findings.yaml`: NSN-1,2,3,4,5,6,7,9 → `status: closed` + `commit: <sha>`. Then `mise run conformance` (never hand-edit generated `.md`).
2. New `docs/wiki/subsystems/system-loadsensor.md`: architecture, event table, rulings R3–R7, policy hook, resolver injection, test index (cite only real ctest targets: `x3d_events_tests`, `x3d_fileresolver_test`).
3. `mkdocs.yml` nav entry; `docs/wiki/coverage.md` row (+counts); `docs/wiki/seam-status.md` NSN-* line.
4. New ADR `docs/wiki/decisions/00XX-loadsensor-assetresolver-oracle.md` (next contiguous number — check `ls docs/wiki/decisions/`): first SDK-side AssetResolver caller, Ready-only memo, R3–R7 rulings, Anchor default-policy deviation. Add its coverage row (coverage-gate).
5. Record the Anchor default-policy deviation in findings.yaml (new minor entry, status deferred, pointing at the policy hook).
6. `mise run docs-drift` → review CITES hits, update flagged pages in the same PR.
7. `mise run docs-build` (strict), `mise run ci` — both green.
8. PR: body `Closes #N` + DoD checklist. Commit style: `feat(loadsensor): … (#N)`.

---

## Execution handoff

Two execution options:

1. **Subagent-driven (this session)** — fresh subagent per task, review between tasks (repo's `workflow-subagent-discipline.md`).
2. **Parallel session** — new session runs executing-plans in a worktree.

Which approach?
