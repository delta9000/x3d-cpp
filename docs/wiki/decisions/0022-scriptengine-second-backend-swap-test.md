---
title: "ADR-0022: Second ScriptEngine Backend (QuickJS) + Behavioral Swap-Test as the Genericity Proof"
summary: An interface is *proven generic* only when a second, independent backend runs identical fixtures to identical observable behavior, gated in CI — established by adding a QuickJS ScriptEngine backend alongside Duktape and a CI-gated swap-test. This is the binding pattern for every seam card.
tags: [adr, seam, genericity, scriptengine, quickjs, swap-test, ci-gate, thesis]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/system-script-sai.md
  - 0013-js-engine-choice.md
  - 0019-physics-seam.md
---

# ADR-0022: Second ScriptEngine Backend (QuickJS) + Behavioral Swap-Test as the Genericity Proof

## Status

Accepted — implemented across the `feat/scriptengine-quickjs` branch (units U1–U4, 2026-06-22..23).

## Context

The product thesis is that x3d-cpp is **unopinionated and pluggable**: every place a
renderer/engine plugs in (scripting, physics, audio, asset/texture/font resolution,
geo-projection, the render-delta consumer) is an abstract **seam**, and the runtime core stays
spec-correct and backend-free. The standing claim of every seam was *"this interface is generic
— bring your own backend."*

But that claim was, until now, **unverified**. Most seams shipped with exactly **one** backend
(Duktape for scripting, Jolt for physics, BuiltinDsp for audio, a single embedder callback for
the IO seams). A one-backend interface is shaped *by* its sole implementation; nothing proves it
isn't quietly leaking that implementation's idioms. The only way to know an interface is generic
is to make a **second, independent** backend satisfy it — and to prove the two are
interchangeable by **behavior**, not by inspection.

`ScriptEngine` was the cheapest place to run this proof first (the Phase-0 pilot): it already
had a clean 6-method abstract interface (`load` / `initialize` / `shutdown` / `prepareEvents` /
`invoke` / `eventsProcessed`), a language-agnostic contract (every value crossing the seam is
the runtime's own field representation, `std::any` tagged by `X3DFieldType` — no
scripting-language type ever crosses), and one backend already shipped (Duktape). QuickJS
(quickjs-ng) is a genuinely independent engine with a different value model — refcounted
`JSValue` + explicit `JS_FreeValue` versus Duktape's value stack — so if the seam survived it
unchanged, the seam is generic; if it forced a core `#ifdef` or an interface change, that would
be a real, surfaced leak. See [ADR-0013](0013-js-engine-choice.md) (the original engine choice;
quickjs-ng was named there as the migration target) and the dated spec
`docs/superpowers/specs/2026-06-22-scriptengine-quickjs-design.md`.

## Decision

**An interface is _proven generic_ only when a second independent backend runs identical
fixtures to identical observable behavior, gated in CI.** A second backend alone is not enough
(it could diverge silently); a swap-test alone is not enough (it could pass by coincidence on
one backend). The proof is the conjunction — two backends + an automated, gated parity test —
and it is the **binding pattern for every seam card** in this project.

Concretely, for the `ScriptEngine` pilot:

- A second backend, **QuickJS** (`runtime/script/QuickJsBackend.{hpp,cpp}`, quickjs-ng
  **v0.15.1**), implements the **unchanged** `ScriptEngine` interface, behind the
  `X3D_CPP_BUILD_QUICKJS` build option, with **no core `#ifdef`** — QuickJS meets the seam in a
  single isolated TU (`x3d_quickjs` static lib, linked PRIVATE so its headers/flags never leak
  to consumers). The default build (option OFF) is byte-identical and behavior-unchanged.
- A **behavioral swap-test** (`x3d_quickjs_swap`) drives the *same* script fixtures through both
  backends and asserts **identical observable behavior** — the `(node, field, value)` writes
  driven into the cascade plus emitted ROUTE-target values must match. Equality is observable
  (the runtime's marshalled field values, post-`toValue`), never internal engine state.
- The swap-test is a **permanent CI merge gate** (`QuickJS seam swap-test` job, flag-gated
  build + `ctest -R 'x3d_quickjs(_backend|_swap)'`).
- Because the interface carried two backends with no signature change, it was promoted
  `[EXPERIMENTAL]` → `[STABLE]` in `include/x3d/sdk.hpp` (the whole Script/SAI surface —
  `ScriptEngine` / `ScriptSystem` / `SaiContext` — as one frozen seam).

A corollary: the existing `SCR-*` conformance findings are claims about the **seam's** observable
behavior, not about Duktape. The swap-test re-verifies they hold identically under QuickJS, so
**SCR conformance is backend-independent** (no finding status changes — the claims simply hold
for a second implementation too).

## Consequences

**Positive:**

- The genericity claim for `ScriptEngine` is now **empirical**, not aspirational — and the
  permanent CI gate keeps it true (any future change that breaks observable parity fails the
  merge).
- A **reusable recipe** exists for hardening every other seam: second backend (no core
  `#ifdef`) → behavioral swap-test → CI gate → freeze `[STABLE]`. This is the seed of the
  Seam-harness card; the [Seam-Status Matrix](../seam-status.md) is its live tracker.
- Freezing the interface `[STABLE]` gives embedders a stability guarantee on the Script/SAI
  surface (breaking change ⇒ major version).
- A genericity *leak* becomes a **finding**, not a silent risk: if a second backend ever forces
  an interface change, the exercise surfaces exactly what part of the interface was
  implementation-shaped.

**Trade-offs / costs:**

- A second backend is real ongoing maintenance: two marshalling implementations (84 field-type
  cases each) must stay in lockstep — though the swap-test is precisely what makes divergence a
  loud CI failure rather than a latent bug.
- The CI gate adds a flag-gated build that FetchContents quickjs-ng (a network + compile cost),
  kept off the fast default `cpp` job so the common PR path stays cold-safe.
- The proof bar is now higher for *every* seam: "one backend + an interface that looks generic"
  no longer counts as done. That is the intended cost — it is the whole point of the thesis.

## Related

- [Seam-Status Matrix](../seam-status.md) — the live tracker this ADR establishes the rule for.
- [Script / SAI Runtime subsystem](../subsystems/system-script-sai.md) — the seam and both backends.
- [ADR-0013: ECMAScript Engine Choice for Script/SAI](0013-js-engine-choice.md) — the original
  Duktape-shipped / quickjs-ng-target decision this ADR follows through on.
- [ADR-0019: Physics via a Flag-Gated Engine Backend](0019-physics-seam.md) — the flag-gated
  isolated-backend precedent (Jolt) this build isolation mirrors; a candidate for the next GREEN row.
- Design spec: `docs/superpowers/specs/2026-06-22-scriptengine-quickjs-design.md`.
