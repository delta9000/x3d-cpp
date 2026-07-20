# ADR-0047: Split the experimental SAI semantic kernel into a sister repository

- Status: Accepted
- Date: 2026-07-19

## Context

The `sai/semantic-kernel` branch carried 40 commits from 2026-07-18 and 2026-07-19,
adding roughly 50,000 lines across 388 files: a compiling proposal for the irreducible
semantics of a modern C++ X3D SAI, plus 338 generated typed bindings and a conformance
register. It was never merged. `main` contained none of it.

Three problems argued against landing it here:

1. The kernel has its own lifecycle and audience. It is a standards proposal reviewed
   against X3D semantics, not an SDK feature reviewed against renderer needs.
2. It expands the SDK's scope and CI surface: roughly 391 lines of CMake, four mise
   tasks, an extra blocking gate in `ci`, and a second conformance register parallel to
   `docs/conformance/findings.yaml`.
3. It contaminates the generated surface. The branch emitted
   `X3DSemanticMetadataRegistry.{hpp,cpp}` into `generated_cpp_bindings/x3d/nodes/`,
   where its only consumer was the SAI metadata bridge.

## Decision

The kernel lives in a separate private repository, `delta9000/x3d-sai`, with a
self-contained generator. It has no build-time, link-time, or Python dependency on
x3d-cpp, and x3d-cpp has no dependency on it.

The generator split is clean rather than a pinned dependency because the X3D Unified
Object Model is not x3d-cpp's artifact. `X3dUnifiedObjectModel-4.0.xml` is published by
the Web3D Consortium; `x3d-sai` vendors and SHA-pins its own copy
(`0f1d9ede593b159469936470c7404629b27b4eb8ccbfef3d07dbb1f27df6443f`). Both projects are
independent consumers of the same upstream specification.

This repository keeps the Script subsystem's in-process SAI context
(`runtime/script/SaiContext.hpp`, `sai_context_test.cpp`,
`docs/wiki/subsystems/system-script-sai.md`). That is a different thing from the
semantic kernel and is unaffected.

## Consequences

**Accepted cost.** Roughly 1,650 lines of parse, model, descriptor, and default logic
are now duplicated between the two generators. The eventual homecoming becomes a
reconciliation rather than a merge. This is deliberate: the SAI value vocabulary is
already divergent by design, with owning storage types and distinct strong types where
C++ primitives would collapse X3D semantics such as time against double and enum
against string. The shared naming helpers were a thin seam, and the reconciliation is a
one-time cost against an ongoing pin-management tax.

**The firewall was aspirational, and is now real.** The kernel library linked only the
standard library, but the test binary linked `x3d_cpp::nodes` and `x3d_doctest_main`,
and the generated registry included `x3d/core/X3DReflection.hpp`. The sister repo
vendors doctest and generates its own reflection header, so nothing is left.

**Reflection parity moved back.** One block in the SAI suite asserted that
`X3DNodeFactory::create(name)->fields()` matched the semantic catalog. That is an SDK
property, not a SAI one, and it was the only such check. It is re-established here as
`tests/test_uom_runtime_reflection_parity.py`, comparing a fresh UOM parse against the
committed generated field tables for all 330 concrete nodes. It inspects generated
source rather than the linked binary, which is a narrower reach than the original in
exchange for full-catalog breadth.

**The service register was cross-implementation, and is now split.** Of its 65
services, 29 were implemented by x3d-cpp and 36 by the kernel or planned for it.
Each repository now owns the rows whose CTest names it can verify. Two loose ends are
tracked rather than papered over:

- x3d-cpp's 29 rows are parked at
  `docs/conformance/sai-services-x3dcpp-rows.pending.yaml`. They do not parse
  standalone, because their merge anchors stayed with the kernel repo.
- Seven invariants in `x3d-sai` have no preserving service. The register was authored
  x3d-cpp-first and never recorded the kernel's implementation of roughly nine ISO
  services, so its conformance gate is non-blocking until those rows are written.

Neither is a claim that coverage exists where it does not.

## Re-entry

The endgame is that the kernel becomes x3d-cpp's authoring API. The trigger is the
strict convergence gate, `mise run sai-conformance` in `x3d-sai`, going green; it is
defined as every registered service and falsification test being complete, which is
exactly the precondition for absorption. A card on
[Project #2](https://github.com/users/delta9000/projects/2) carries the decision.

Re-entry stays a decision made with the evidence available then. This ADR guarantees it
surfaces, not that it is granted.

## Alternatives rejected

**Pin `x3d-cpp-gen` as a dependency of `x3d-sai`.** Avoids duplicating the parse layer,
but leaves SAI-shaped emitters and an unused registry inside the SDK, and makes the
sister repo downstream of an SDK's codegen rather than of the specification.

**Require an adapter proving the kernel can drive x3d-cpp's runtime scene graph before
re-entry.** Stronger evidence, but it reintroduces the x3d-cpp dependency the split
removes.

**Keep the service register whole in one repo.** Either half of it becomes unverifiable
assertion, since neither repo can resolve the other's CTest names.

## Provenance

The pre-split branch head `8e2a000` is preserved as the tag
`archive/sai-semantic-kernel`. That tag also retains 18 non-SAI planning documents that
existed only on that branch.
