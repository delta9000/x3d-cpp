---
title: "ADR-0001: External-Code Firewall"
summary: Third-party and binary-format code lives behind a one-way firewall so the spec-correct core runtime stays pure and the dialect is clearly separate.
tags: [adr, ext-firewall, binary, geometry, externproto]
updated: 2026-06-20
related:
  - ../architecture.md
---

# ADR-0001: External-Code Firewall

## Status

Accepted

## Context

The binary-geometry and ingestion work (2026-06-19) introduced non-spec codecs (STL reader, `ExternalGeometry` custom node) alongside the spec-correct ISO 19775-1 core. Two tensions drove the design:

**The spec demands purity.** ISO 19775-1 §4.4.5.1 identifies the `ProtoDeclarationResolver` seam as the "implementation-dependent mechanism" for extending a conformant parser — but the generated node bindings, the golden layer, and the conformance gate must never touch non-standard nodes. Any contamination would undermine the generator's core claim: that output is spec-correct by construction.

**The CAVE consumer needs non-spec capability.** The VR CAVE world-model must route mesh data as typed binary (`PackedMesh`) rather than text geometry. There is no portable typed-binary geometry node in X3D 4.0 (ISO 19776-3 Fast-Infoset compresses standard nodes only). So a dialect is unavoidable — the question is how to contain it. This gap is not assumed permanent: a glTF-style binary accessor/buffer-view mechanism has been discussed as a direction for a future X3D binary-geometry facility, which would narrow or close it if adopted.

A spike (2026-06-19) proved the mechanism works end-to-end: a native `ExternalGeometry` node backed by a hand-built `ProtoDeclaration` returned from a custom `ProtoDeclarationResolver` expands through the standard EXTERNPROTO machinery, produces a `PackedMesh` at extract time, and round-trips as `<ExternProtoDeclare>` + `<ProtoInstance>` — without touching any generated file or golden assertion. The prior-art arc (InstantReality `BinaryGeometry` → `ExternalGeometry`/SRC → X3DOM `BufferGeometry`) confirmed the shape; those were native, non-standardized browser nodes. The EXTERNPROTO + opt-in + quarantined approach keeps the content spec-conformant.

The decision was made by the user (2026-06-19, documented in the ingestion-roadmap memory) and ratified after the spike result.

## Decision

We decided that all third-party and binary-format code — STL codec, `ExternalGeometry` node, `ExternalGeometryResolver`, and any future foreign-format codecs — lives behind a four-layer firewall:

1. **Code quarantine.** Own directory tree `runtime/ext/`, namespace `x3d::runtime::ext`, hand-written (never generated). The dependency is strictly one-way: `ext` may include core headers (`PackedMesh.hpp`, `Aabb.hpp`, etc. from `runtime/extract/`); core (`x3d_cpp`, `sdk.hpp`) must never include anything from `runtime/ext/`. This is enforced by the comment in every `runtime/ext/` header: "Core MUST NEVER include this file."

2. **Opt-in CMake flag.** `option(X3D_CPP_BUILD_EXT … OFF)` in `CMakeLists.txt`. When `OFF` (the default), no translation unit from `runtime/ext/` is compiled or linked; `build.ninja` does not reference the directory; ctest registers no ext tests. The standard `mise run build` path is completely unaffected. Build with `cmake --preset dev -DX3D_CPP_BUILD_EXT=ON` to include the ext layer.

3. **Visible in file as EXTERNPROTO.** X3D files that use `ExternalGeometry` must carry an `<ExternProtoDeclare url='"urn:x3d-cpp-gen:ext:ExternalGeometry"'/>`. Any parser that does not install the ext resolver reads and ignores the declaration (graceful degradation). The EXTERNPROTO url list reserves a second slot for a portable `.x3d` fallback (deferred; slot reserved).

4. **Quarantined from gates.** Ext nodes never appear in standard golden files, the conformance audit, or the profile gate. Ext tests are registered exclusively inside the `if(X3D_CPP_BUILD_EXT)` block in `CMakeLists.txt` and run only when the flag is on.

The CMake targets are:

```
add_library(x3d_cpp_ext INTERFACE)           # runtime/ext/ headers
add_library(x3d_cpp::ext ALIAS x3d_cpp_ext)
target_link_libraries(x3d_cpp_ext INTERFACE x3d_cpp::x3d_cpp)  # one-way dep
```

The activation seam is `x3d::runtime::ext::install(base)` (`runtime/ext/ExtResolver.hpp`), which wraps a `ProtoDeclarationResolver` to intercept `urn:x3d-cpp-gen:ext:ExternalGeometry` and delegate everything else to `base`. No new parameters on `parseDocument` or `parseFile`; the existing resolver seam is the exact opt-in gap.

A companion process-global hook `x3d::runtime::fallbackNodeCreator()` (`runtime/X3DProtoClone.hpp`) handles `deepClone` for ext nodes (the generated factory returns `nullptr` on unrecognized type names). `install()` registers the `ExternalGeometry` creator exactly once via `std::once_flag`. This hook is sticky and set-once — configured at single-threaded setup time before any parsing. The generated layer was not touched.

The STL write path (`tools/x3d-cli/stl_write.hpp`) stays core-side: it takes `PackedMesh` data (a core type) and writes standard binary STL — no ext type is needed. Only the read direction (byte buffer → `PackedMesh`) lives in ext, because the codec (`StlReader.hpp`) does not add X3D nodes.

## Consequences

**Positive:**

- The generated bindings, golden layer, conformance gate, and the default `mise run build` + `ctest` invocation are completely unaffected by the ext layer. `flag OFF: ctest 145/145, golden zero-drift` was verified when the firewall scaffold shipped.
- The dialect is clearly signalled in any X3D file that uses it (`<ExternProtoDeclare>`), degrades gracefully in other parsers, and is isolated to an explicit opt-in compile flag — zero hidden coupling.
- The one-way dependency constraint is architecturally enforced (compiler will error if a core header tries to include an ext header) and documented in every ext file header, making violations immediately visible in code review.
- Adding future codecs (PLY, OBJ single-mesh, glTF per-mesh) is additive: drop a new codec in `runtime/ext/codecs/`, register its format sniff in `ExternalGeometryResolver.hpp`, add ext-gated tests. No core change required.
- The STL writer staying in core means the `x3d extract` CLI path (X3D → STL) needs only `x3d_cpp::sdk` with no ext dependency — embedders that only write, never read, non-spec formats pay nothing.

**Trade-offs / costs:**

- Embedders that want `ExternalGeometry` support must explicitly call `x3d::runtime::ext::install()` and pass the returned resolver to `parseDocument`. This is intentional friction (opt-in = visible), but it is friction.
- The `fallbackNodeCreator` hook is a process-global, sticky, set-once slot. Concurrent `install()` calls after setup are not synchronized and not supported. This is documented but slightly unusual.
- The portable `.x3d` fallback baker (second slot in the EXTERNPROTO url list) is deferred — files authored with `ExternalGeometry` today will not degrade to text geometry in a standard parser; they produce an unresolved `ProtoInstance` (readable, but lossless round-trip only, not renderable elsewhere). This was an explicit deferral: "reserve the slot; defer the baker until an interchange consumer asks." A mechanism for exactly this kind of fallback slot already exists in EXTERNPROTO practice: multi-URN EXTERNPROTO declarations can list two or more URNs, with a spec-compliant implementation first and a PROTO-based compatible fallback last — the same chaining shape the reserved second slot would use once the baker is built.
- Tests that use `StlReader` to validate output (e.g. `x3d_stl_write_test`, `x3d_extract_oracle_test`) are ext-gated even though they test core writer output, because they need the reader for the self-oracle check. The core write path itself is not gated.

## Related

- [Architecture](../architecture.md)
- Ingestion roadmap memory: `~/.claude/projects/.../memory/x3d-cpp-gen-ingestion-roadmap.md` — full decision trail including the spike, the abstraction pivot (BinaryGeometry → ExternalGeometry), and ratification
- Dated design spec: `docs/superpowers/specs/2026-06-19-binary-geometry-extension-design.md` — the consolidated design document; cites ISO 19775-1 §4.4.5.1 and the EXTERNPROTO extension point
- `runtime/ext/ExternalGeometry.hpp` — the firewall boundary comment ("Core MUST NEVER include this file") and namespace declaration
- `runtime/ext/ExtResolver.hpp` — `install()` function and URN interception
- `runtime/ext/ExternalGeometryResolver.hpp` — lazy materialization seam; bridges firewall via core-typed `std::function`
- `runtime/ext/codecs/StlReader.hpp` — first codec; the one-way dep comment ("ext→core") is load-bearing documentation
- `CMakeLists.txt` lines 207–213 (option declaration) and 1652–1716 (quarantine block + ext tests)
