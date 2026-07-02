---
title: "ADR-0042: Slim Authoring Target & Ingestion Pipeline"
summary: Introduce x3d_cpp::authoring as a slim serialization/authoring library that excludes parse/runtime subsystems, backed by a footprint gate to prevent code bloat.
tags: [adr, authoring, footprint, assets, cmake, CLI]
updated: 2026-07-01
related:
  - ../architecture.md
  - ../subsystems/asset-import.md
---

# ADR-0042: Slim Authoring Target & Ingestion Pipeline

## Status

Accepted

## Context

The asset-import and scene-authoring workflow (Wave 3) introduced a requirement to construct X3D scene graphs programmatically from external assets (such as OBJ models or synthetic fixtures) and serialize them. 

However, two architectural concerns arose:
1. **Binary Footprint Bloat**: Importers or asset converters built on top of the SDK do not require full runtime behavior, event cascade propagation, scripting, audio, or physics engines. Pulling in these subsystems increases compile times, executable footprint, and memory overhead unnecessarily.
2. **Subsystem Decoupling**: The serialization/authoring layer (document model, codec writers, range validation) should remain cleanly separated from the parsing/execution layer. A programmatically constructed scene should be serializable without pulling in parser dependencies (such as XML/JSON readers or PROTO resolvers).

To address these tensions, we needed a dedicated target structure and a gate system to guarantee that compile-time and link-time separation remains strictly enforced.

## Decision

We decided to partition the codebase to expose a slim, header-only target for authoring and implement a symbol/size footprint gate to prevent regression:

1. **The `x3d_cpp::authoring` Target**: 
   Introduced in `CMakeLists.txt` as an `INTERFACE` library. It exposes only the document model, the codecs (writers), the generated node reflection, and `collectRangeWarnings` (for self-validation). It deliberately omits parser, event/execution, scripting, audio, and physics include directories so that downstream users linking against `x3d_cpp::authoring` cannot transitively pull in these APIs.
   
2. **Footprint Gate (`authoring-footprint.sh`)**:
   An automated gate runs under `mise run authoring-footprint` and CI. It compiles the `x3d_authoring_smoke` executable, which only links `x3d_cpp::authoring`. 
   * **Symbol scan**: Using `nm`, the gate asserts that no symbols from forbidden subsystems (such as `SceneExtractor`, `X3DExecutionContext`, `parseDocument`, `parseFile`, `PhysicsWorld`, `ScriptSystem`, or `MeshBuilder`) are present in the compiled binary.
   * **Size baseline**: The gate strips the binary and verifies that the `.text` section size does not exceed the baseline by more than 10%, flagging any transitive inclusion drift.

3. **`x3d_asset_import` Integration CLI**:
   A command-line tool `x3d_asset_import` was created in `examples/asset_import/main.cpp` to tie the components together (backend loading → texture planning → scene graph emission → profile fitting → self-validation → serialization). For verification purposes, it implements a `--verify` flag which *does* use the full SDK parser (`x3d::codec::parseFile`) to round-trip and verify the generated X3D files. This dependency is quarantined solely in the tool's CLI wrapper (`main.cpp`), keeping the underlying `emit` library completely free of parser dependencies.

4. **Divergence from ISO/IEC 19777-4**:
   Rather than implementing the full, verbose ISO/IEC 19777-4 C++ binding style (which relies on heavyweight IDL abstractions and interfaces), we opted for a direct, performance-optimized, and type-safe modern C++20 object model. This is wrapped with naming guardrails that retain clean, self-documenting APIs (like `std::shared_ptr` node references, raw vectors for arrays, and fluent setter chaining).

## Consequences

### Positive:
- **Ultra-slim Authoring Binary**: Consumers who only need to build and export X3D files (e.g. offline converters or CAD exporters) pay no penalty for execution, scripting, sound, or physics systems.
- **Architecturally Enforced Separation**: The footprint gate catches any developer error that transitively couples parsing/runtime components back into the authoring target.
- **Improved Compilation Speed**: Linking only `x3d_cpp::authoring` avoids compiling or linking large parts of the execution runtime.
- **Direct & Clean API**: The C++20 object model is highly readable and integrates naturally with standard STL types, making it much easier to write scene exporters than the ISO/IEC 19777-4 equivalent.

### Trade-offs / costs:
- **Verification Quarantine**: The `--verify` round-trip re-parse functionality in the CLI tool requires linking `x3d_cpp::sdk` (the parser), meaning the CLI binary itself does not have a minimal footprint. This is acceptable since the CLI is an integration keystone and not the slim library itself.
- **Divergence from Standards**: Developers familiar with the ISO/IEC 19777-4 C++ standard bindings will need to learn our simplified C++20 model, though the mapping remains intuitive.
- **Baseline Maintenance**: Refactoring core node classes or reflection can cause legitimate growth in the `.text` size. The baseline must be updated manually with `scripts/authoring-footprint.sh --write-baseline` when this happens.

## Related

- [Architecture](../subsystems/generated-bindings.md)
- Ingestion roadmap: `docs/superpowers/specs/2026-07-01-asset-import-authoring-target-design.md`
- Subsystem documentation: `docs/wiki/subsystems/asset-import.md`
- `scripts/authoring-footprint.sh` — symbol scan and size gate script
- `examples/asset_import/footprint/baseline.tsv` — committed size baseline
- `examples/asset_import/CMakeLists.txt` — compile/link option flags for `x3d_authoring_smoke`
