# Build and CI Compile-Structure Design

**Date:** 2026-07-13  
**Status:** Approved for planning  
**Scope:** CMake target structure, C++ compile topology, Python generator tests, and GitHub Actions policy

## 1. Problem

The repository compiles mostly legitimate production and test code, but its build graph repeats expensive work and does not make target purpose explicit.

Measured on the current branch:

| Measurement | Result |
|---|---:|
| Cold default C++ build, 16 cores | 80.65 s wall / 1081.60 s user CPU |
| CI CTest phase, including header isolation | 28.88 s wall / 293.87 s user CPU |
| Default executable targets | 50 |
| Default executables actually exercised by CTest | 48 |
| CI-preset CTest entries | 457 |
| Per-header compile-contract entries | 407 |
| Recent uncached GitHub sanitizer jobs | 30.6-36.2 min |
| Recent GitHub fast C++ jobs | 9.1-10.3 min |
| Recent GitHub pytest jobs | 14.1-15.0 min |

The five grouped doctest targets account for about 656 seconds of the cold build's cumulative compiler time. They save links, but their 145 source files remain independent header-heavy translation units. Two Python namespace-emission tests also invoke the generator's optional C++ smoke compilation even though they only inspect emitted text; those two tests take about 189 seconds each locally.

The current GitHub cache behavior masks the problem when a PR happens to have a warm branch-scoped cache and exposes it again on a new PR. A sanitizer job has varied from 1.9 minutes warm to 36.2 minutes cold. Correctness and acceptable latency cannot depend on that cache surviving.

## 2. Goals

1. Every default-compiled target has an explicit production, behavioral-test, compile-contract, or opt-in-tool purpose.
2. PR feedback remains comprehensive but does not compile unrelated configurations.
3. Header self-containment remains enforced without hundreds of nested build-tool invocations.
4. Sanitizer coverage applies to executable behavior, not unused gate tools or compile-only contracts.
5. Heavy hand-written runtime implementations are compiled once into libraries rather than recompiled in every consumer translation unit.
6. The installed `x3d_cpp::sdk`, `x3d_cpp::authoring`, and `x3d_cpp::x3d_cpp` contracts remain source-compatible.
7. The slim authoring target remains free of parse, execution, extraction, physics, script, sound, and backend symbols.

## 3. Non-goals

- Do not weaken generated-tree drift checks, behavioral tests, or installed-consumer verification.
- Do not apply global unity builds to hand-written tests. A throwaway unity build produced extensive collisions among file-local helpers such as `feq`, `setF`, `failures`, and `kScene`.
- Do not make PCH the primary strategy. The repository's prior measurements found template instantiation and code generation, rather than parsing alone, to dominate.
- Do not move templates, small value-type accessors, `constexpr` functions, or callback seams out of headers merely to reduce line counts.
- Do not change generated node API or golden output in this effort.

## 4. Target taxonomy

Every repository-owned target belongs to one of four categories:

| Category | Meaning | Default build |
|---|---|---|
| Production | Installed library or shipped CLI | Yes |
| Behavior | Executable run by the normal CTest suite | Yes in top-level developer builds |
| Compile contract | Header isolation, install/embed, or API-surface proof | Explicit aggregate target/job |
| Opt-in gate/tool | External-corpus gate, backend seam, example renderer, fuzz target | No; its owning task/job builds it explicitly |

`x3d_cli_gate` and `x3d_canon_gate` become opt-in gate tools. They are currently built by every default and sanitizer build but are not run by GitHub's C++ job.

## 5. Phase one: make the existing graph intentional

### 5.1 Behavioral and gate aggregates

CMake will expose explicit aggregate targets:

- `x3d_behavior_tests`: all executables used by the normal behavioral CTest suite.
- `x3d_header_isolation`: every generated and hand-written public header compiled as its own source file under one object-library target.
- `x3d_sanitizer_tests`: runtime behavioral executables that provide meaningful ASan/UBSan execution coverage.

The header-isolation implementation will collect generated one-include source files into one `OBJECT EXCLUDE_FROM_ALL` target. Ninja still performs one compilation per header and reports the exact failing source, but CMake generates one target and CTest launches one build instead of creating and invoking 407 targets individually.

### 5.2 Default-target cleanup

`x3d_cli_gate` and `x3d_canon_gate` will use `EXCLUDE_FROM_ALL`. Their mise tasks will build the exact required targets before executing them. The normal build remains responsible for the installed libraries, CLI, and behavioral tests.

### 5.3 Sanitizers

The sanitizer preset changes from `RelWithDebInfo` to `Debug`. ASan/UBSan does not require optimizer-heavy compilation to find the memory, lifetime, overflow, and undefined-behavior failures this gate targets. The job builds `x3d_sanitizer_tests` rather than the unconstrained default graph, then runs the behavioral CTest label.

### 5.4 Python generator tests

Namespace-emission tests pass `--no-test` to the generator. They continue to generate the complete output tree and inspect namespace/factory content. C++ compilation remains covered by the CMake behavior, all-headers, header-isolation, and installed-consumer gates.

### 5.5 CI change classes

The change detector distinguishes:

- `cpp`: CMake/presets, generated bindings, public headers, C/C++ runtime, C++ tools, and examples.
- `generator`: Python package, generator tests, UOM fixtures, lock/config files, and golden scripts.
- `docs`: documentation-only changes.
- Existing seam-specific classes.

Sanitizer, fuzz, texture, and example-consumer jobs depend on `cpp`, not the current broad "any non-doc file" class. A generator change that alters committed generated output also changes `generated_cpp_bindings/` and therefore correctly enters the C++ class.

## 6. Phase two: compile the hand-written runtime once

### 6.1 Dependency topology

The current `x3d_cpp_nodes PUBLIC x3d_cpp` plus `x3d_cpp INTERFACE x3d_cpp_nodes` relationship cannot simply accept another compiled runtime dependency without creating a real cycle. The build will first establish these acyclic layers:

```text
x3d_cpp_headers (INTERFACE: include dirs + C++20)
        |
        +--> x3d_cpp_nodes (STATIC: generated node definitions)
                    |
                    +--> x3d_cpp_authoring_runtime
                    |      (STATIC: document model, range validation, writers)
                    |
                    +--> x3d_cpp_runtime
                           (STATIC: readers, scene/events, extraction, script/sound core)

x3d_cpp::authoring --> headers + nodes + authoring_runtime
x3d_cpp::x3d_cpp   --> headers + nodes + authoring_runtime + runtime
x3d_cpp::sdk       --> x3d_cpp::x3d_cpp + include/x3d
```

`x3d_cpp_headers` is exported because installed exported targets may not reference a build-only target. It is an implementation target, not a new recommended consumer surface.

Static-library composition ensures consumers only pull referenced object code. The separate authoring library prevents the slim authoring façade from acquiring parser, runtime, extraction, script, sound, or backend symbols.

### 6.2 What moves out of headers

Move non-template, non-`constexpr`, behavior-heavy definitions into `.cpp` files while preserving declarations and public types in their existing headers. Prioritize by measured fan-out and compile cost:

1. Codecs and readers: `FieldValueIO`, XML/JSON/VRML writers, canonical writer, XML/JSON/ClassicVRML readers, `NodeBuilder`, and `X3DParse` front door.
2. Extraction: `MeshBuilder`, `SceneExtractor`, material/texture/text/light helpers, and NURBS evaluation.
3. Scene and events: execution context, scene bridge, event cascade/graph, transform/bounds/pick systems, and high-fan-out interaction systems.
4. Document/authoring runtime: document, scene, PROTO expansion, and range-validation bodies needed by both full SDK and authoring.

Move these implementation details behind declarations or private `.cpp` helpers:

- Large switch statements and reflection walks.
- Parser/writer state machines.
- Mesh construction and traversal algorithms.
- Non-template class method bodies.
- Large static lookup tables that do not require constant evaluation in consumers.

Keep these in headers:

- Templates and generic callbacks.
- `constexpr` data needed at compile time.
- Small POD/value-type methods where an out-of-line call would be counterproductive.
- Seam type aliases and public aggregate definitions.
- Methods whose source compatibility genuinely depends on inlining.

### 6.3 Migration waves and stop conditions

Each subsystem wave follows the same loop:

1. Record clean cold-build and affected-target timings with ccache disabled.
2. Add link-level regression coverage before moving definitions.
3. Move one coherent subsystem.
4. Run behavior, header isolation, install/embed, authoring-footprint, and downstream example gates.
5. Repeat the timing under identical compiler/build settings.

A wave is retained only if it materially reduces cumulative compiler time or memory without growing the public surface. A practical threshold is at least 10% improvement in its affected target group or a clear reduction in repeated high-cost template/code generation. Small headers that fail this threshold stay header-only.

## 7. Verification and acceptance

Correctness gates:

- Python suite and golden drift pass.
- C++ behavioral suite passes in normal, Werror, and sanitizer builds.
- `x3d_header_isolation` passes.
- Installed `find_package`/embed smoke passes.
- `authoring-footprint.sh` remains within its symbol and size budgets.
- Optional examples and seam jobs build only their required targets and pass.
- No generated binding drift unless separately intended.

Performance acceptance:

- Pytest no longer launches generated C++ smoke compilation in namespace-only tests.
- The header contract uses one build invocation and one CTest entry while preserving one TU per header.
- An uncached sanitizer PR job no longer has a 30-minute critical path.
- The normal cold C++ build shows a measured reduction after each phase; results are recorded in the build guide rather than estimated from cache-hit runs.
- No target is present in the default graph without a documented taxonomy classification.

## 8. Documentation

Update `docs/wiki/guides/build-and-mise.md`, `docs/wiki/architecture.md`, `CMakePresets.json` descriptions, `mise.toml` task descriptions, and CI comments alongside the code. Replace stale statements that describe roughly 70 executables, roughly 800 header tests, or historical cold-build timings as current facts.
