---
title: "ADR-0013: ECMAScript Engine Choice for Script/SAI"
summary: Duktape 2.7.0 was chosen as the reference ECMAScript backend, vendored as a single 2-file bundle, behind a language-agnostic ScriptEngine seam that admits any future engine.
tags: [adr, ecmascript, duktape, script, sai]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/system-script-sai.md
---

# ADR-0013: ECMAScript Engine Choice for Script/SAI

## Status

Accepted — 2026-06-16

## Context

X3D Script nodes execute author-supplied code to drive the event cascade.
ISO/IEC 19775-1 §29.1 is explicit: *"Browsers are not required to support any
specific language… shall adhere to that language binding."*
The only two ISO-normative bindings are ECMAScript (ISO/IEC 19777-1) and Java
(ISO/IEC 19777-2); ECMAScript is the de-facto language of real X3D corpora.

The x3d-cpp-gen runtime has two strong constraints that bear on engine choice:

1. **No IO, no threads, no dynamic deps.** The runtime core is
   IO-free and single-threaded (the event cascade is deterministic, and async
   scripts are explicitly deferred to SCR-ASYNC). Any vendored engine must
   compile with zero external headers beyond the C/C++ standard library.

2. **The bundle-a-permissive-dep pattern** was already established by
   `runtime/parse/tinfl.h` (miniz public-domain subset). A 2-file drop-in is
   the preferred form.

Given those forces, the choices in the realistic set were:

| Engine | Bundle form | License | ES level | Binary overhead |
|---|---|---|---|---|
| **Duktape 2.7.0** | 3 files (`duktape.c`, `duktape.h`, `duk_config.h`), ~3 MB source, ~492 KB compiled object | MIT | ES5.1 + subsets of ES2015 | Minimal |
| QuickJS / quickjs-ng | ~15 files, C99, more complex embedding | MIT | ES2020+ | ~800 KB–1.2 MB object |
| V8 | Not embeddable as a source drop | BSD-3 | Full | ~30 MB .so |
| SpiderMonkey | Not embeddable as a source drop | MPL-2.0 | Full | Large |

The Script/SAI design spec (`docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md`,
§8.2) documents the deliberate choice: *"Duktape 2.7.0 is already vendored…
ES5.1+, no IO deps, matches the `runtime/parse/tinfl.h` bundled-permissive-dep
pattern. QuickJS/V8 remain swap-in via the seam."*

X3D corpora in practice use simple glue scripts — `set_fraction`-style handlers,
field reads, `Browser.addRoute`, `print` — all well within ES5.1.
The ES2020+ features QuickJS supplies are not required by X3D scripts and would
not widen the corpus coverage.

## Decision

We vendored **Duktape 2.7.0** (MIT, 3-file: `runtime/script/vendor/duktape/duktape.c`,
`duktape.h`, and `duk_config.h`) as the reference ECMAScript backend, implemented in
`runtime/script/EcmaScriptBackend.hpp/.cpp`, registered under the `"ecmascript"`
scheme in `ScriptSystem`'s scheme→backend registry.

The backend sits behind a **language-agnostic `ScriptEngine` seam**
(`runtime/script/ScriptEngine.hpp`) that carries no JS types: all values
crossing the seam are boxed as `std::any` tagged by `X3DFieldType`. Duktape
API calls are private to `EcmaScriptBackend`; the seam, `SaiContext`,
`ScriptSystem`, lifecycle ordering, and the `directOutput` gate are
engine-independent and do not need modification to add another backend.

## Consequences

**Positive:**

- **Zero new CMake find_package dependencies.** Duktape is compiled by the
  existing CMake build (`duktape.c` as a plain-C translation unit) with no
  external headers. The build stays self-contained.
- **Small binary footprint.** The compiled Duktape object is approximately
  492 KB, consistent with the runtime's embedded/headless target profile.
- **Full ES5.1 coverage of the X3D corpus.** Real-world X3D Script nodes
  (interpolator glue, Browser API calls, field arithmetic) require no ES2015+
  features; ES5.1 covers 100% of the tested corpus (`x3d_script_corpus_e2e`,
  `CanopyExample.x3d`).
- **Seam keeps the door open.** Registering QuickJS, a Java backend, or a
  Lua backend is a matter of implementing `ScriptEngine` and calling
  `ScriptSystem::registerBackend("ecmascript", ...)` — the lifecycle, cascade
  ordering, and SAI surface are unchanged.
- **Matches the established vendor pattern** (`tinfl.h`): permissive license,
  drop-in source, no external build step, auditable diff.

**Trade-offs / costs:**

- **ES5.1 ceiling.** Scripts that use ES2015+ syntax (`let`, `const`, arrow
  functions, template literals) will throw a Duktape parse error at load time.
  The seam does not hide this; `load()` returns `kInvalidScriptHandle` and
  `ScriptSystem` logs the failure. In practice the X3D corpus does not require
  these features. If a consumer needs ES2020+ they swap in QuickJS via the seam.
- **Duktape is in maintenance mode.** No new feature releases after 2.7.0
  (2023); security patches only. The migration path is: implement
  `EcmaScriptBackend` against QuickJS/quickjs-ng (same `ScriptEngine` API,
  only the internal `duk_context*` table changes) and swap the registered
  backend. No other runtime layer is touched.
- **Larger vendored source file.** `duktape.c` is ~3 MB; it is compiled once
  as its own CMake object and benefits from ccache on subsequent builds.
- **Async scripts (SCR-ASYNC) are deferred for separate reasons**, not
  engine-related: they require threading and non-deterministic timestamp
  assignment, which conflict with the single-thread cascade regardless of which
  engine backs ECMAScript.

## Related

- [Architecture](../architecture.md)
- [Script/SAI subsystem](../subsystems/system-script-sai.md)
- Design spec: `docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md` (§8, §4.2)
- Implementation: `runtime/script/ScriptEngine.hpp`, `runtime/script/EcmaScriptBackend.hpp`, `runtime/script/ScriptSystem.hpp`
- Vendor license: `runtime/script/vendor/duktape/DUKTAPE_LICENSE.txt` (MIT)
- Spec reference: ISO/IEC 19775-1 §29.1 (language-agnostic mandate), §29.2 (lifecycle), ISO/IEC 19777-1 (ECMAScript binding)
