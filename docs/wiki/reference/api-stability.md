---
title: API stability
summary: What x3d-cpp promises about API, ABI, interface status and CMake target names before 1.0.
tags: [reference, stability, packaging]
updated: 2026-11-20
related:
  - seam-matrix.md
---

# API stability

x3d-cpp is pre-1.0. The promise is narrow.

## Releases

| Release step | API and ABI | Enforced by |
|---|---|---|
| Patch (0.1.0 to 0.1.1) | Intended to be preserved | The CMake package declares `SameMinorVersion`; the shared libraries carry `SOVERSION 0.1` |
| Minor (0.1.x to 0.2.0) | May break both | A consumer built against 0.1 is rejected at configure time and at load time |

Patch compatibility is intent backed by convention. There is no ABI checker in
CI. Pin an exact version if you depend on it.

## Interface status

`include/x3d/sdk.hpp` marks selected features `[EXPERIMENTAL]`: node inspection
and the contextual asset/texture resolver variants. These features may change.
The header has no `[STABLE]` annotations; an unmarked alias is not itself a
compatibility promise.

The [seam matrix](seam-matrix.md) states the status of each interface. Those
per-interface labels do not extend package API or ABI compatibility beyond the
pre-1.0 release policy above.

## CMake targets

| Target | Status |
|---|---|
| `x3d_cpp::sdk`, `x3d_cpp::authoring`, `x3d_cpp::x3d_cpp` | Documented entry points |
| `x3d_cpp::ecmascript` | Documented entry point when built with `X3D_CPP_BUILD_SCRIPT` (default ON) |
| `x3d_cpp::import` | Documented entry point (mesh import); the `ImportScene` IR is experimental and may change |
| `x3d_cpp::duktape` | Exists so `x3d_cpp::ecmascript` resolves; may be restructured |
| `x3d_cpp::headers`, `x3d_cpp::nodes`, `x3d_cpp::authoring_runtime`, `x3d_cpp::runtime` | Exist so the export set resolves; may be restructured |

## Other limits

- The render feed (`RenderDelta` and its descriptors) is an in-process C++
  contract, not a wire format or a C ABI.
- Runtime results are not bit-identical across compilers, architectures or
  backends; see [Architecture](../explanation/architecture.md#floating-point-reproducibility).
- The SAI SDK states its own stability in
  [SAI scope and limits](../sai/explanation/scope-and-limits.md).
