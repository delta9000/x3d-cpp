---
title: API stability
summary: What x3d-cpp promises about API, ABI, seam markers and CMake target names before 1.0.
tags: [reference, stability, packaging]
updated: 2026-11-14
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

## Header markers

| Marker in `include/x3d/sdk.hpp` | Meaning |
|---|---|
| `[STABLE]` | Frozen before v2; a breaking change is a major bump |
| `[EXPERIMENTAL]` | Usable; may gain fields |

There is no blanket answer for the seams. The [seam matrix](seam-matrix.md)
states each one.

## CMake targets

| Target | Status |
|---|---|
| `x3d_cpp::sdk`, `x3d_cpp::authoring`, `x3d_cpp::x3d_cpp` | Documented entry points |
| `x3d_cpp::headers`, `x3d_cpp::nodes`, `x3d_cpp::authoring_runtime`, `x3d_cpp::runtime` | Exist so the export set resolves; may be restructured |

## Other limits

- The render feed (`RenderDelta` and its descriptors) is an in-process C++
  contract, not a wire format or a C ABI.
- Runtime results are not bit-identical across compilers, architectures or
  backends; see [Architecture](../explanation/architecture.md#floating-point-reproducibility).
- The SAI SDK states its own stability in
  [SAI scope and limits](../sai/explanation/scope-and-limits.md).
