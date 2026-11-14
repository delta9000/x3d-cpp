---
title: SAI test suites and gates
summary: The SAI gates, the shared provider fixture, the provider and coexistence harnesses and what each establishes.
tags: [sai, reference, testing]
updated: 2026-11-14
related:
  - ../how-to/verify-a-change.md
  - service-register.md
---

# SAI test suites and gates

Which gate to run for which change is in
[Verify a change](../how-to/verify-a-change.md).

## Gates

| Task | Runs | Part of | CI job |
|---|---|---|---|
| `mise run sai-ci` | UOM pin, generator determinism, registers, invariants, generator pytest, C++ build, CTest | `mise run ci` | `sai` |
| `mise run sai-provider` | `scripts/verify_sai_provider.sh`: native versus reference parity, relocated install, host consumer | `mise run ci-all` | `sai-adapter` |
| `mise run sai-coexistence` | `scripts/verify_sai_coexistence.sh`: runtime and SDK in one process, every header and link order | Neither; run by hand | None |
| `mise run sai-conformance` | Opt-in strict register report | Neither | None |

The `sai` job runs on any SAI change, including changes under
`src/x3d_cpp_gen/`. The `sai-adapter` job runs on any C++ or SAI change.

`sai-ci` steps, in order: `sai-uom-pin`, `sai-gen-determinism`,
`sai-conformance-gate`, `sai-invariants`, `sai-pytest`, `sai-build`,
`sai-ctest`. They are sequential on purpose because they share `sai/.venv` and
one build directory. `sai-ctest` also rejects doctest filters that select no
cases.

## SDK CTest suites

Sources are in `sai/tests/`.

| CTest | Covers |
|---|---|
| `sai_provider_contract` | Frontend contract: gates, whole occurrences and MF indexed aggregation, shared submission cancellation, known-no-effect versus partial failure, receipt lifetime, ID reuse after retirement, checked close, reentrant metadata cleanup, live ROUTEs, notification budgets, owning SFNode values, grouped-outcome concurrency |
| `sai_provider_reference` | Shared oracle on the reference backend; live ROUTE delivery |
| `sai_provider_schema` | Kernel-free schema proof: generated owner, kind and access checks, malformed value and descriptor rejection, stale descriptors, move-only user data |
| `sai_provider_scene` | Runs the `provider_scene.cpp` example |
| `sai_hosted_logical_turn` | Logical turns: equal times, empty turns, failed turns, inputOnly occurrence delivery |
| `sai_hosted_reference` | Reference backend against the host-driven contract; interpolation clamping; curve validation; configurable limits |
| `sai_hosted_callback` | Callback lifetime: destroying or closing the service from a callback, move-only captures, reentrant capture destruction |
| `sai_hosted_resources` | Byte accounting, admission budgets, bounded history and allocation-failure recovery |
| `sai_external_session` | Reference `external_session`, including [multi-field services](multi-field-services.md) |
| `sai_user_data_services`, `sai_browser_services`, `sai_matrix_operations`, `sai_rotation_constraints` | The matching reference pages |
| `sai_<family>_*` | One CTest per semantic invariant; see [Service register](service-register.md#invariant-test-naming) |
| `sai_generated_semantic_projection` | Generated field projection |

The schema translation unit rejects inclusion of the reference kernel, the
reference metadata adapter and private generated metadata headers.

## Shared provider fixture

`<x3d/sai/experimental/testing/provider_fixture.hpp>` (target
`x3d::sai_provider_fixture`) is one oracle compiled against every backend. It
takes a factory returning `result<unique_ptr<provider::service>>` and uses only
public operations; there is no adapter-owned application façade and no
test-only scene bootstrap.

| Entry point | Runs |
|---|---|
| `run_provider_fixture(factory)` | The complete authoring and live oracle |
| `run_provider_authoring_fixture(factory)` | Setup, ownership and graph cases |
| `run_provider_live_fixture(factory)` | Host-controlled activation and turns |
| `run_provider_user_data_fixture(factory)` | Node and field user data |
| `check_provider_named_nodes(factory)` | Named-node discovery |
| `make_provider_route_scene(service)` | Builds the routed workload with ordinary scene, node, field, root, name and route operations, including nested Shape/Box content |

### Authoring coverage

- scene and context creation, unknown node types, distinct identity, naming
  grammar, duplicate names, ordered repeated roots
- the complete Transform interface against generated descriptors (compared by
  name, without imposing backend enumeration order)
- SFVec3f and SFBool defaults, dynamic and typed equivalence, owning read
  snapshots
- rotation kinds, defaults, axis validity, tolerance, large angles and no
  silent normalization, for both `rotation` and `scaleOrientation`
- negative and zero scale are legal
- wrong values, keys and access; no mutation after failure; initializeOnly
  setup writes; inputOnly unreadable
- foreign owners, empty and disposed wrappers, idempotent local disposal
- owning MFNode lists: order, multiple NULLs, shared nodes, duplicate
  children, invalid references, rejection atomicity
- direct, indirect, rooted and detached cycles; diamond DAGs; a 128-node
  detached chain with a back edge. Graph snapshots verify rejection leaves
  fields, roots, names and scalar values intact
- retained lists and descriptors, wrong-thread access, idempotent close,
  expired authority, capabilities after close
- thread lifetime: a later thread is rejected even if its OS thread ID was
  recycled; a separately closed service is safely destroyed off-thread
- user data: canonical identity, aliases, every semantic field, reentrant
  replacement and clear, exactly-once payload destruction, teardown order

### Live coverage

- scene, context and node identity, roots and names across two services
- exact field metadata, IO aliases, typed keys, access and kind errors
- ROUTE wrong source, kind, destination and context; duplicates; fan-out
  (without imposing sibling ordering or a fan-in winner)
- activation; live author and topology rejection; finite validation
- stateful last-write-wins coalescing; inputOnly whole occurrences stay
  distinct, including equal values
- repeated Begin and unmatched End as successful no-ops; one End releases the
  whole held submission
- other-client FIFO overtaking a held gate; accepted, submitted and processed
- equal host times, distinct turns, decreasing and non-finite time rejection
- deferred observations, retained output, source before routed destination
- cancellation ownership, no replay, subscription cancellation
- retirement cancelling work and notices while an independent activation
  survives
- the default 1,024 pending-request limit, cancellation releasing capacity,
  terminal receipts surviving service destruction
- invalid interpolator curve shapes; repairable setup
- callback exception isolation
- a preflight failure with known no mutation fails only its submission
- a partial or unknown backend failure faults the activation and cancels later
  submissions
- scene units and metadata; named nodes
- the initial-output probe for
  [X3D 4.0 Part 1, 19.3.1](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#X3DInterpolatorNode):
  empty-list default, a nonzero first keyValue in setup and after activation,
  and no synthetic initial event, route delivery or clock step

The fixture does not assert kernel revisions or backend representation
details. The assertion-by-assertion ledger that mapped the earlier separate
offline and hosted fixtures onto this one is in git history at
[`sai/docs/provider-fixture-coverage-ledger.md`](https://github.com/delta9000/x3d-cpp/blob/306e6d3/sai/docs/provider-fixture-coverage-ledger.md).

## Provider harness

`scripts/verify_sai_provider.sh [sai-source] [empty-work-dir]` builds from
`tests/cmake/sai_provider/`. Requirements: C++20, CMake 3.24+ and Ninja. It
fetches nothing, and ordinary x3d-cpp builds gain no SAI dependency.

Each shared or static run builds source consumers, installs both projects and
the adapter, relocates the complete prefix, then configures copied standalone
consumers with package registries disabled. In each mode it runs:

| Executable | Checks |
|---|---|
| `sai_provider_reference` | Shared authoring and live oracle on the reference backend |
| `sai_provider_native` | The same oracle plus native scalar, metadata and graph evidence |
| `sai_provider_parity` | Exact comparison of complete backend-blind reports |
| `sai_provider_native_runtime` | Shared live oracle plus native session, interpolation, ROUTE, world-transform, RenderDelta and failure-effect proofs |
| `sai_provider_presentation` | Immutable frames, ownership, lifetime, deltas |
| `sai_provider_adopt_session` | Session adoption, live ROUTEs, native node writes, Inline diagnostics |
| `sai_provider_allocation_failure` | Native registration after allocation failure; own process |
| `sai_cpu_host` | The [headless host example](host-example.md) as an installed consumer |

- All checks stay active under Release/NDEBUG.
- Native-only link commands are checked for the absence of reference-kernel
  and reference-metadata libraries. The parity executable deliberately links
  both backends.
- Native header guards reject accidental reference implementation includes.
- The adapter tests are plain `main()` programs so an installed-package
  consumer can build them without doctest.
- `native_authoring_fixture.hpp` is a test-only wrapper over the final service.
  It is never installed and is not an application API.

This is bounded experimental evidence, not complete ISO SAI conformance.

## Coexistence check

`scripts/verify_sai_coexistence.sh [sai-source] [empty-work-dir]` builds from
`tests/cmake/sai_coexistence/`. It links the runtime and the SDK as two
separate packages into one process, adds no dependency to either and downloads
nothing.

| Check | Detail |
|---|---|
| Header and link order | Four source consumers: runtime-first and SAI-first headers, crossed with runtime-first and SAI-first CMake link and include-search order |
| Both APIs in one binary | Each consumer includes the SDK and Script `SaiContext` headers, constructs a runtime `Transform` and calls its compiled reflection, then separately creates, edits and reads a typed SAI `Transform` through the metadata bridge |
| Portable service | The same consumer creates and activates a third scene through `provider::service` and checks typed readback and the unchanged independent scenes |
| Private headers | SAI metadata under `x3d_sai/detail` cannot be found on a downstream include path. A revision exporting `x3d/core` or `x3d/nodes` fails |
| Install | Both packages install into one prefix with no private SAI headers and no absolute source or build paths in their CMake exports |
| Relocation | The prefix is moved and the original left absent; a copied consumer finds, builds and runs all four combinations with package registries disabled |

That is eight build-and-run checks per runtime linkage mode. The scene models
stay separate: the check does not translate runtime nodes into SAI handles or
connect event models.

| Setting | Effect |
|---|---|
| Second argument | An **empty** work directory; otherwise a new temporary directory. Builds and logs stay at the printed path |
| `CMAKE_BUILD_PARALLEL_LEVEL` | Compiler concurrency (default 2) |
| `CMAKE_GENERATOR` | Use a generator other than Ninja |
| `X3D_CPP_SHARED_NODES=OFF` | Exercise static runtime symbols |

The source consumer can be configured directly from `tests/cmake/sai_coexistence`
with `-DX3D_SAI_SOURCE_DIR="$PWD/sai"`, building target `sai_coexistence_all`.
Only the full script checks installed relocation.

## Invariant suites

`sai/tests/sai_invariants/` is the landing zone for falsification tests named
by `sai/docs/conformance/sai-invariants.yaml`. A semantic invariant is not
considered proved by a happy-path unit test. Each suite should have four
layers:

1. A minimal example that communicates the invariant to an API user.
2. A deterministic regression for every known historical failure.
3. Property-generated scenes, values, aliases, routes and context boundaries.
4. Adversarial sequences: failure, rollback, disposal, reentrancy,
   cancellation, serialization and replay where relevant.

Rules:

- Compare observable semantic traces and diagnostics. Do not compare private
  container layout, pointer values, scheduler order or serialized bytes unless
  the invariant makes them observable.
- A failing generated case must print its seed and a minimized operation trace
  so it can become a regression.
- Changing invariant prose needs an explicit review-marker digest update in
  the registry.
- Changing a generator or adversarial sequence needs the old counterexamples
  kept, unless the invariant itself changed. Refactors may broaden evidence but
  cannot quietly weaken the contract.
