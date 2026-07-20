# Split the experimental SAI semantic kernel into a sister repository

Date: 2026-07-19
Status: design approved, implementation not started

## Problem

The `sai/semantic-kernel` branch holds 40 commits dated 2026-07-18 and 2026-07-19,
adding 50,317 lines across 388 files against 5 deleted. It is a compiling proposal for
the irreducible semantics of a modern C++ X3D SAI: identity, transactional editing,
ordered change streams, declaration and node lifecycles, execution-context apertures.

Three problems argue against landing it in x3d-cpp:

1. The kernel has its own lifecycle and audience. It is a standards proposal reviewed
   against X3D semantics, not an SDK feature reviewed against renderer needs.
2. It expands the SDK's scope and CI surface. The branch adds roughly 391 lines of
   CMake, four mise tasks, an extra blocking gate in `ci`, and a second conformance
   register parallel to `docs/conformance/findings.yaml`.
3. It risks contaminating the SDK's public and generated surface. The branch emits
   `X3DSemanticMetadataRegistry.{hpp,cpp}` into `generated_cpp_bindings/x3d/nodes/`,
   the SDK's own generated tree, where its only consumer is the SAI metadata bridge.

## Key finding: main never had any of it

Verified against `main` on 2026-07-19. Every SAI-kernel artifact is absent:
`experimental/sai/`, `src/x3d_cpp_gen/emit/sai_bindings.py`,
`src/x3d_cpp_gen/emit/semantic_metadata.py`,
`generated_cpp_bindings/x3d/nodes/X3DSemanticMetadataRegistry.hpp`,
`docs/conformance/sai-services.yaml`, `scripts/check_sai_services.py`. The string
`sai` does not appear in `main`'s `mise.toml` at all.

So this is not an extraction from a shipping repo. It is a decision about where an
unmerged branch lands. x3d-cpp has nothing to revert: no CMake removal, no generator
removal, no mise-task removal, no NOTICE edit, no change to `ci`'s depends list.

This does not apply to `runtime/script/SaiContext.hpp`, `sai_context_test.cpp`, or
`docs/wiki/subsystems/system-script-sai.md`. Those are the Script subsystem's
in-process SAI context, they are already on `main`, and they stay. The older
`docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md` describes that
shipped subsystem and also stays.

## Decision

Create a sister repository `x3d-sai`, alongside the existing `../x3d-bgfx`, holding
the semantic kernel and a self-contained generator for its typed bindings. It has no
build-time, link-time, or Python dependency on x3d-cpp.

The endgame is that the kernel eventually becomes x3d-cpp's authoring API. The sister
repo is incubation, not a permanent fork, and the re-entry contract below keeps that
a tracked decision rather than an intention.

## Correction: the firewall is not complete today

The kernel README claims the library "links only the C++ standard library and does not
depend on x3d-cpp's current runtime". That holds for `x3d_sai_experimental` itself. It
does not hold for the metadata bridge or the test binary, both of which the split must
resolve.

`x3d_sai_experimental_metadata` links `x3d_cpp_headers`. This is not a real dependency:
`x3d_cpp_headers` is an `INTERFACE` target supplying include paths, and the only
x3d-cpp header `metadata.cpp` includes is `X3DSemanticMetadataRegistry.hpp`, which moves
to `x3d-sai`. The dependency dissolves with the move.

`x3d_sai_experimental_tests` links `x3d_cpp::nodes` and `x3d_doctest_main`, and
`semantic_kernel_test.cpp` includes `x3d/nodes/X3DNode.hpp` and `X3DNodeFactory.hpp`.
Two separate fixes:

- doctest: `x3d-sai` vendors `runtime/test_support/doctest/doctest.h` and
  `doctest_main.cpp` (MIT, v2.4.11) with its own NOTICE entry.
- Runtime nodes: exactly one call site, `semantic_kernel_test.cpp:595`, in 1 of 91 test
  cases. Lines 594-610 of the test case "generated metadata adapter has exhaustive
  ordered descriptor parity" call `x3d::nodes::X3DNodeFactory::create(name)->fields()`
  and assert that x3d-cpp's live runtime reflection matches the semantic catalog on
  field order, name, type, and access.

That last assertion is an x3d-cpp claim, not a SAI claim, and it appears to be the only
runtime-level check of it. `tests/test_reflection.py` covers the generator's emission,
not runtime agreement. So the block is dropped from `x3d-sai` and re-homed: the x3d-cpp
PR gains a runtime test asserting `X3DNodeFactory` reflection matches the UOM-derived
descriptors. The coverage moves rather than evaporating, and the sister repo's firewall
becomes absolute rather than aspirational.

The catalog-to-adapter parity assertions in the same test case, which are the actual SAI
claim, survive intact in `x3d-sai`.

## Repository layout

```
x3d-sai/
  include/x3d/sai/experimental/{kernel,metadata}.hpp
  src/{kernel,metadata}.cpp
  generated_cpp_bindings/x3d/sai/experimental/      338 files, 23.5k LOC, committed
  generated_cpp_bindings/x3d/nodes/X3DSemanticMetadataRegistry.{hpp,cpp}
  src/x3d_sai_gen/                                  the generator package
  tests/ examples/ third_party/{tl,doctest}/
  scripts/            sai_conformance.py, check_sai_services.py, check_sai_invariants.py
  docs/conformance/   sai-services.yaml, sai-invariants.yaml, sai-service-catalog.yaml,
                      SAI-BASELINE.md
  docs/plans/         the 14 SAI design and implementation docs
  CMakeLists.txt  mise.toml  pyproject.toml  NOTICE  LICENSE  README.md
```

The C++ sources flatten out of `experimental/sai/`, since the whole repository is now
the experiment and nothing depends on that prefix.

Everything else keeps its original path. `scripts/sai_conformance.py` resolves
`_repo_root() / "docs" / "conformance"`, and `metadata.cpp` includes
`"x3d/nodes/X3DSemanticMetadataRegistry.hpp"`. Keeping `scripts/`,
`docs/conformance/`, `tests/`, and `generated_cpp_bindings/x3d/...` unchanged means the
move needs no path edits in Python or C++, removing a class of migration bug that a
tidier layout would buy at real risk.

The namespace stays `x3d::sai::experimental`: renaming would churn 34k lines and break
every citation in the plan docs, and the qualifier is still accurate.

`third_party/tl` (tl::expected 1.3.1, CC0-1.0) moves with the kernel, along with its
NOTICE entry. x3d-cpp never needed that entry outside SAI.

## The generator

`x3d-sai` ships its own Python package, `x3d-sai-gen`, with no dependency on
`x3d-cpp-gen`. This is possible because the X3D Unified Object Model is not x3d-cpp's
artifact. `X3dUnifiedObjectModel-4.0.xml` is a 2.5 MB file published by Web3D at
`https://www.web3d.org/specifications/X3dUnifiedObjectModel-4.0.xml`. `x3d-sai`
becomes a second independent consumer of the same upstream standard, which is a
better position for a standards proposal than sitting downstream of an SDK's codegen.

| Piece | Origin | Approx. LOC |
| --- | --- | --- |
| UOM XML | vendored from web3d.org, SHA-pinned | 2.5 MB data |
| `parser.py`, including `build_dependency_graph` | copied | 264 |
| `model/{types,enums,version}.py` | copied | 445 |
| `cpp_str`, `sanitize_field_name` from `naming.py` | copied subset | 25 |
| `semantic_fields.py`: `resolved_node_fields`, `wire_name` | copied | 30 |
| `interfaces_of` from `registry.py` | copied subset | 11 |
| `FIELD_TYPE_MAPPING`, `XS_TYPES` from `generator.py` | copied subset | 40 |
| `emit/{sai_bindings,semantic_metadata}.py` | moved | 369 |

Roughly 1.1k LOC total, of which about 800 is substrate duplicated from x3d-cpp. The
emitters move outright. Only the parse and model layer is copied, and it is copied
because it reads a public spec file.

### Cross-repo coherence

There is no import relationship to keep in sync, so drift reduces to a data question:
are both repositories reading the same UOM revision? The generated catalog already
carries the X3D specification version, a SHA-256 fingerprint of the resolved semantic
model, and the generator version. Each repository gets a CI check asserting its
vendored UOM against a recorded hash, so a spec bump has to be a deliberate act on
both sides.

### Accepted cost

Two independently evolving parsers make the eventual homecoming a reconciliation
rather than a merge. This is accepted deliberately. The SAI value vocabulary is
already divergent by design: owning `vec3f` and `color4f` storage, and distinct
strong types where C++ primitives would otherwise collapse X3D semantics, notably
time against double and enum against string. The shared naming helpers were a thin
seam. The reconciliation is a one-time cost at re-entry, against an ongoing
pin-management tax if the dependency were kept.

## Migration

### x3d-sai

`git filter-repo --refs sai/semantic-kernel` over a scratch clone, keeping only SAI
paths and renaming them into the layout above. The 40 commits survive as real
history; they read as a deliberate design, plan, implement, falsify sequence, and the
plan docs cite them. filter-repo drops commits that become empty.

Three seed commits follow, for content that cannot be filtered because it would drag
SDK material along:

1. Build system: a fresh root `CMakeLists.txt` (the 391 branch lines rewritten as a
   root project rather than a subdirectory), `mise.toml`, `LICENSE`, `NOTICE`, and
   `README.md` promoted from `experimental/sai/README.md`.
2. Vendored parse layer: the 800 LOC substrate and the UOM XML, with a provenance
   note. Correctly excluded from filtered history, since those files' history in
   x3d-cpp is about SDK codegen.
3. Generator wiring: the `x3d-sai-gen` package, `mise run gen`, and the
   regen-determinism check.

The README must state that history before the seed commits is provenance and is not
bisectable. The imported commits cannot build without a build system.

### x3d-cpp

A small PR:

- Tag `archive/sai-semantic-kernel` at `8e2a000`, then delete the branch and the
  `.worktrees/sai-semantic-kernel` worktree.
- Add an ADR under `docs/wiki/decisions/` recording the split, the endgame, the
  re-entry trigger, and the accepted parser-duplication cost.
- Add a short `docs/wiki/` pointer page, a `docs/wiki/coverage.md` row, and an
  `mkdocs.yml` nav entry. `mise run docs-build` is strict on dead links and nav
  orphans, so these are required for the gate, not optional polish.
- Re-home the runtime reflection parity check described in the correction section, as a
  new runtime test asserting `X3DNodeFactory::create(name)->fields()` agrees with the
  UOM-derived descriptors on order, name, type, and access.

## Gates

x3d-cpp's `ci` is untouched. `x3d-sai` gets its own:

- Build and ctest, covering the roughly 10 named falsification tests
  (`sai_id_shared_node`, `sai_own_domain`, `sai_field_one_descriptor`,
  `sai_generated_metadata_drift`, and the rest). On the branch these build
  unconditionally inside the SDK's test block; in the sister repo they are
  first-class.
- `sai-conformance-gate`: register validation, cross-links, baseline drift. Blocking.
- Regen determinism: `mise run gen && git diff --exit-code`, since 23.5k lines of
  bindings are committed.
- UOM SHA pin check.
- `sai-conformance --strict`: the convergence gate. Non-blocking, expected to fail
  until the SAI is complete.

## Re-entry contract

The strict convergence gate already exists and is already defined as requiring every
registered SAI service and falsification test to be complete. That is exactly the
precondition for absorbing the kernel into x3d-cpp's authoring API, so it serves as
the re-entry trigger.

- The x3d-cpp ADR states the endgame, names `sai-conformance --strict` going green as
  the trigger, and records the accepted cost from the generator section.
- A card on [Project #2](https://github.com/users/delta9000/projects/2), "evaluate SAI
  kernel re-entry", carries the decision, blocked on that gate.
- The `x3d-sai` README states the same intent, so a contributor there knows the work
  is a future part of x3d-cpp rather than an orphan.

Re-entry stays a decision made with the evidence available at the time. The contract
guarantees it surfaces, not that it is granted.

An alternative was considered and rejected: additionally requiring a working adapter
in `x3d-sai` proving the kernel can drive x3d-cpp's runtime scene graph. It would
demonstrate the homecoming before committing to it, but it puts an x3d-cpp dependency
back into the sister repo and breaks the firewall the split establishes.

## Smaller decisions

- Repository name is `x3d-sai`, hosted alongside x3d-cpp under the same owner.
- `x3d-sai` keeps `docs/plans/` as plain files initially. It does not adopt mkdocs.
  The audience is contributors reading the proposal in the repo, and a published
  wiki can be added later if the reader set widens.
