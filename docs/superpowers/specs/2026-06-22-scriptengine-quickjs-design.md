# Prove the ScriptEngine seam generic: Duktape + QuickJS — Design

**Date:** 2026-06-22
**Status:** Design proposal — pending human approval before the implementation workflow launches.
**Project card:** "Prove ScriptEngine generic: Duktape + QuickJS" (Seam=ScriptEngine, Phase 0).
**Workflow:** [Card → Done](../../wiki/guides/card-to-done-workflow.md) (substantial card → spec + ADR
first) · [Subagent discipline](../../wiki/guides/workflow-subagent-discipline.md) (five-phase spine).

## Goal

Add a **second, independent** `ScriptEngine` backend (QuickJS) behind the existing seam with
**zero changes to the runtime core**, and prove the seam is genuinely generic via a
**behavioral swap-test**: identical script fixtures run through both backends must produce
**identical observable behavior** (field writes, cascade events, traces).

This is the **Phase-0 pilot** for the product thesis ("unopinionated, pluggable, conformant —
BYO backends, ≥2 external SDKs per seam to prove the interface is generic"). ScriptEngine is the
cheapest place to prove it: the only seam with a real abstract interface *plus* one backend
already. The swap-test built here becomes the **seed of the Seam-harness** that every later seam
(Physics, Audio, Geo, …) reuses.

## The seam as it stands (grounded)

- `runtime/script/ScriptEngine.hpp` — a clean 6-method abstract interface
  (`load` / `initialize` / `shutdown` / `prepareEvents` / `invoke` / `eventsProcessed`).
- **Language-agnostic contract** (the genericity is already designed in): every value crossing
  the seam is the runtime's own field representation — `std::any` boxing the field's C++ type,
  tagged by `X3DFieldType`. **No scripting-language type ever crosses the seam.** Each backend
  privately marshals `std::any ↔ its own idiom`.
- `runtime/script/ScriptSystem.hpp` takes the engine as `std::shared_ptr<ScriptEngine>` —
  **swapping backends is a constructor argument**, nothing more. Scheme→backend registry keys
  "ecmascript"/"javascript"/"vrmlscript"; inline-scheme decode lives in `ScriptSystem`
  (backend-agnostic — QuickJS inherits it for free).
- `runtime/script/EcmaScriptBackend.{hpp,cpp}` — the Duktape impl. The parity surface a second
  backend must reach: **84 `X3DFieldType` marshalling cases** (`pushValue`/`toValue`) + the
  `Browser` global + handler dispatch + per-script context lifecycle.

The interface is already shaped for this. If QuickJS slots in without touching the core, the
seam is proven; if it needs a core `#ifdef` or an interface change, we've found a real leak.

## Deliverable 1 — the genericity test (the proof)

A **swap-test** that drives the *same* fixtures through both backends and asserts identical
observable behavior. We already own the fixtures:
`runtime/script/tests/ecmascript_backend_test.cpp` (T1–T16: lifecycle, marshalling round-trips,
Browser, handler dispatch) and `script_corpus_e2e_test.cpp` (real Savage `.x3d` scenes through
the full parse→expand→attach→tick path).

Mechanism: parametrize these over a backend factory `{ makeDuktape, makeQuickJs }`. Equality is
**observable**, not internal: the set of `(node, field, value)` writes the script drives into the
cascade, and the emitted events, must match across backends for every fixture. This is exactly an
`x3d sim`-style trace diff (and pins the contract for the Seam-harness card).

## Deliverable 2 — the QuickJS backend

`runtime/script/QuickJsBackend.{hpp,cpp}` : `public ScriptEngine`, mirroring `EcmaScriptBackend`:

- **Vendoring / build isolation** — follow the `x3d_physics_jolt` precedent (`CMakeLists.txt:222`):
  a new `option(X3D_CPP_BUILD_QUICKJS … OFF)` → an isolated `x3d_quickjs` STATIC lib that is the
  single TU where QuickJS meets the seam; QuickJS linked **PRIVATE** so its flags/headers never
  leak to consumers. Default build (option OFF) is **unchanged**. Candidate: `quickjs-ng` (MIT,
  actively maintained, ES2023). Vendor in-tree (mirrors `runtime/script/vendor/duktape/`) or
  FetchContent (mirrors Jolt) — decide in U1 by build-reproducibility.
- **Marshalling parity** — `pushValue`/`toValue` for all 84 `X3DFieldType` cases against QuickJS
  `JSValue` (note: QuickJS uses refcounted `JSValue` + explicit `JS_FreeValue`, unlike Duktape's
  value stack — the main idiom difference and the main bug surface).
- **Browser global + handler dispatch + per-script context** — one `JSContext` per loaded script,
  keyed by `ScriptHandle`, mirroring the Duktape `Entry` model.

## Definition of Done (the card's checklist)

- [ ] `ScriptEngine` interface frozen — promote `[EXPERIMENTAL]` → `[STABLE]` in `sdk.hpp`
      (proven by a second backend implementing it unchanged).
- [ ] QuickJS backend wired behind `X3D_CPP_BUILD_QUICKJS`, no core `#ifdef`.
- [ ] **Swap-test green**: every script fixture produces identical observable behavior under
      Duktape and QuickJS.
- [ ] SCR-* findings re-verified under QuickJS (the conformance claims aren't Duktape-specific).
- [ ] Docs: a **seam-status matrix** row (ScriptEngine: frozen ✓ · Duktape ✓ · QuickJS ✓ ·
      swap-test ✓) + `system-script` subsystem page note; ADR-0022.

## Phased implementation plan (five-phase workflow units)

| Unit | Work | Isolation |
|---|---|---|
| **U1** | `QuickJsBackend` skeleton + vendor/CMake (`X3D_CPP_BUILD_QUICKJS` → `x3d_quickjs`) + lifecycle (load/init/shutdown/prepareEvents) — mirror Duktape T1–T11. | New files; CMake edit is sequential. |
| **U2** | Marshalling parity (84 type cases) + `Browser` global + handler dispatch (T12–T16). | New `.cpp`; parallel-safe. |
| **U3** | **Swap-test**: parametrize the script fixtures over both backends; assert identical traces. (Genericity proof + Seam-harness seed.) | Edits shared test files — sequential. |
| **U4** | Freeze interface `[STABLE]`; seam-status doc; SCR-* re-verify; ADR-0022; subsystem-page note. | Docs + one header — sequential. |

Each unit: TDD (red→green), per-unit adversarial review (`buildPassed`/`testsPassed`/
`goldenUntouched`/`approved`), ≤2 fix rounds. Gate: `mise run ci` + swap-test green.

## Golden / codegen rule

**Codegen-free.** No `generated_cpp_bindings/` or template touch — the Script node and emit
methods are already generated; this adds runtime files + an opt-in CMake lib. Golden hash MUST
stay byte-identical. Default build (QuickJS OFF) is behavior-unchanged.

## Risks

1. **JSValue refcounting** — QuickJS requires explicit `JS_FreeValue`; leaks/double-frees are the
   top bug risk. Mitigation: an RAII `JSValue` guard in the backend TU; valgrind the swap-test.
2. **Observable determinism** — both backends must emit identical field values. Float formatting,
   MF iteration order, and number→string coercion can differ. Mitigation: the swap-test compares
   the runtime's marshalled field values (post-`toValue`), not JS-side strings, so coercion stays
   inside each backend.
3. **Build weight / CI** — QuickJS behind an OFF-by-default option keeps the PR fast gate cold-safe;
   the swap-test ctest only builds when the option is ON (a dedicated CI job, like the physics path).
4. **Genericity leak** — if QuickJS forces an interface change, that's a *finding*, not a failure:
   the spec's whole point is to surface it. Document and adjust the seam if so.

## ADR-0022 (to land with U4)

"Second `ScriptEngine` backend (QuickJS) + a behavioral swap-test as the genericity proof" — the
**pattern for every seam card**: an interface is *proven generic* only when a second independent
backend runs identical fixtures to identical observable behavior, gated in CI.
