# Unified provider fixture coverage ledger

This ledger tracks the migration from the frozen offline and hosted pilot proofs
to the single `experimental::provider` contract. It is an assertion/category
ledger, not an assertion-count equivalence claim or a claim of full ISO SAI
conformance. The same `testing/provider_fixture.hpp` is compiled against the
reference and native backends.

## Entry points and ownership

* `run_provider_fixture(factory)` runs the complete authoring and live oracle
* `run_provider_authoring_fixture(factory)` runs setup, ownership and graph cases
* `run_provider_live_fixture(factory)` runs host-controlled activation and turns
* `make_provider_route_scene(service)` uses only ordinary public scene, node,
  field, root, name and route operations, including nested Shape/Box content
* The factory returns `result<unique_ptr<provider::service>>`; no adapter-owned
  application façade or offline/live compatibility interface remains
* Internal test lookup helpers compose `get_field` with the same public
  `read`, `author` or `user_data`; they store no backend state or authority

## Retained authoring assertions

The origin ranges refer to the frozen pre-consolidation
`testing/provider_fixture.hpp`, not the current line numbers.

| Old assertions/categories | Unified replacement and status |
| --- | --- |
| Lines 97–135: creation, canonical field identity, node/field metadata, exact type, owning metadata, named/rooted reacquisition, node separation | `run_provider_user_data_fixture`; retained through the one service and owner-bearing handles |
| Lines 140–191: metadata for every semantic field, generated keys, aliases, unknown names, wrong owner/kind/access | Same helper; metadata uses canonical field lookup and remains independent of value support |
| Lines 193–243: foreign, disposed, expired, empty and wrong-thread metadata/field operations | Same helper; explicit disposed error replaces pilot stale error, while stale authority remains separately tested |
| Lines 244–307: reentrant replacement/clear/close, owning reads outliving association, exact-once final payload destruction | Same helper; preserved as behavioral assertions, including cleanup observing replacement state |
| Lines 308–352: authority retirement before cleanup, explicit/implicit teardown and wrong-thread close | Same helper; open-service foreign-thread destruction is intentionally disallowed by the unified lifecycle contract; owner close followed by cross-thread destruction is the replacement |
| Lines 374–420: scene creation, unknown node types, distinct identity, naming grammar, duplicate names/renaming, ordered repeated roots | `run_provider_authoring_fixture`; explicit scene/context creation added; repeated root occurrences and owner identity retained |
| Lines 422–458: complete Transform interface and generated descriptor correspondence | All generated Transform keys must be found with exact kind/access; report is normalized by name instead of imposing backend enumeration order |
| Lines 460–519: SFVec3f/SFBool defaults, dynamic/typed equivalence, owning read snapshots and distinct storage | Same authoring fixture; retained |
| Lines 521–576: SFRotation kinds, defaults, axis validity, finite components, tolerance, large angles and no silent normalization | Same authoring fixture; retained for both rotation and scaleOrientation |
| Lines 548–555: negative and zero scale are legal | Retained |
| Lines 578–662: wrong values/keys/access, unknown document syntax, no mutation after failure | Retained; initializeOnly setup writes now positively tested, inputOnly remains unreadable; raw node IDs are rejected at compile time |
| Lines 665–693: foreign owner, empty/disposed wrappers, local/idempotent disposal and generated-key authority | Retained with explicit disposed taxonomy |
| Lines 700–866: owning MFNode lists, order, multiple NULLs, shared nodes, duplicate children, invalid references, owner/key checks and rejection atomicity | Retained through typed `node_values` projection; backend-specific children uniqueness remains checked on both actual backends |
| Lines 868–910: direct/indirect/rooted/detached cycles, clearing, retained names, diamond DAGs and 128-node detached chain/back-edge | Retained; graph snapshots verify rejection leaves fields, roots, names and scalar values intact |
| Lines 913–983: owning retained lists/descriptors, wrong-thread access, close idempotence, expired authority, capabilities after close | Retained; capabilities now compare versioned services/nodes/kinds instead of obsolete booleans |
| Lines 984–1016: creator thread lifetime and cross-thread destruction | Wrong-thread operations and close remain covered; creator closes before exiting, then a later thread is rejected even if its OS thread ID was recycled; a separately closed service is safely destroyed off-thread |

## Retained hosted assertions and deliberate policy replacements

| Old category | Unified replacement and status |
| --- | --- |
| Backend-created route fixture | Ordinary public authoring builds the workload, avoiding a backend bootstrap requirement |
| Scene/context/node identity, roots and names across two services/scenes | Retained in shared live fixture |
| Exact field metadata, IO aliases, typed keys, access and kind errors | Retained; vocabulary now includes supported authoring rotations and owning SF/MFNode values |
| Read/write setup values, owning arrays, initializeOnly setup access | Retained; setup/live share the same field handles |
| ROUTE wrong source/kind/destination/context, duplicates and fan-out | Retained without imposing sibling ordering or fan-in winner |
| Activation, live author/topology rejection, live access/kind/finite validation, empty queue rejection | Retained |
| Duplicate canonical stateful seed rejection | Replaced by the approved last-write-wins stateful coalescing assertions; inputOnly whole occurrences remain distinct, including equal values |
| Nested Begin rejected | Replaced by repeated Begin success/no-op and unmatched End success/no-op; one End releases the whole held submission |
| Other-client FIFO overtaking a held gate; accepted/submitted/processed distinction | Retained; per-call receipts additionally identify their shared released submission |
| Equal host times, distinct turns, decreasing/nonfinite time rejection | Retained |
| Deferred observations, retained output, source-before-routed-destination and owning payloads | Retained; no assertion requires each duplicate input occurrence to generate a separate output/ROUTE cascade |
| Cancellation ownership, terminal cancellation, no replay, subscription cancellation | Retained |
| Retirement cancels work/notices and invalidates handles, independent activation survives | Retained |
| Wrong-thread pump/enqueue/read, close invalidation | Retained |
| Default 1024 pending requests, resource-limit rejection, cancellation releasing capacity, terminal receipts surviving service destruction | Retained; configurable limits and payload accounting are additionally covered in frontend resource tests |
| Invalid curve shape/order/duplicates/nonfinite representability, repairable setup and configuration-first batch | Retained against both real backends |
| Callback exception isolation and no replay | Retained in shared fixture and deeper callback-only tests |
| Execution preflight with known no mutation faults activation/cancels later work | Intentionally changed: only that submission fails; activation and later FIFO work remain active and processable |
| Prior completed notifications survive a later fault | Retained for harmless failure in shared fixture; partial/unknown failure and preserved completed history are asserted in focused frontend contract tests |
| Partial/unknown backend execution failure | Faults activation and cancels later submissions; no rollback or automatic retry claim |

## Malformed schema and source-boundary proof

`tests/provider_schema_test.cpp` retains exact generated owner/kind/access checks,
malformed value rejection before extraction, invalid rotation rejection before
mutation, metadata on unsupported fields, exact alias-shaped field precedence,
independent alias-shaped metadata, stale descriptor rejection, unknown canonical
field rejection, and move-only user data. It adds empty/duplicate/invalid-enum
descriptor rejection and resolves backend node IDs before exporting authority.

The old separate `read_nodes` and scalar node-value rejection expectations are
replaced by compile-time exclusion of raw IDs from `provider::payload`, typed
owning SF/MFNode results, and dangling backend-reference rejection. Generic
MFNode projection preserves duplicates; the actual Transform children semantic
rule rejects duplicate non-NULL children in the common behavioral fixture.
The old `read_nodes(node, "translation")` rejection has no corresponding call
in the unified API: dynamic `read` legitimately returns that field's scalar
payload. Its replacement is generated-key exact-kind checking and malformed
backend payload/descriptor mismatch rejection; no test helper invents an error
on the provider's behalf.
The old hard-coded rejection of a backend-declared `inventedChildren` MFNode
field is replaced by positive generic owning read/write projection; field
capabilities and backend semantics, rather than a frontend spelling allowlist,
decide support. Shape and Box are also positively created now instead of treating
Shape as an unimplemented node; unknown-type rejection is still retained.

The schema translation unit still rejects inclusion of the reference kernel,
reference metadata adapter and private generated metadata headers. Native-only
link/symbol proof must additionally show that the portable frontend and shared
fixture do not pull in the reference kernel.

## Backend-owned and focused evidence

* Native direct-authority/read-repair/runtime evidence stays with the native
  provider: five direct-native authoring proofs (including the 4,096-node
  detached graph), six former runtime cases, and SFNode/unnamed-ROUTE/occurrence
  evidence are in `tests/cmake/sai_provider/native.cpp` and
  `adapters/sai_provider/tests/native_runtime_test.cpp`
* Reference callback failure/reentrancy and narrow kernel input occurrence tests
  remain backend-owned; common portable tests do not assert kernel revisions or
  backend representation details
* Focused frontend contract tests cover gates, whole occurrences and MF indexed
  aggregation, shared submission cancellation, known-no-effect versus partial
  failure, receipt lifetime, ID reuse after retirement, checked close and
  reentrant metadata cleanup
* The frontend resource suite retains its 12 cases and grows from 4,230 to
  4,256 assertions. The 32,000-operation bounded-history, 1,000-client,
  receipt/thread/lifetime and admission-budget scenarios remain. Its deliberate
  replacements are successful repeated Begin/unmatched End, one coalesced
  stateful submission with shared outcomes, submission-wide released
  cancellation, and exact accounting by `sizeof(provider::write)` plus field
  and dynamic payload bytes
* The separate frontend contract suite adds 17 cases/2,170 assertions for
  notification budgets, owning SFNode values, mixed MF/event policies,
  reentrant cleanup and grouped-outcome concurrency

## Added initial-output semantic probe

The final shared fixture adds 18 checks for the pre-input getter requirement in
[X3D 4.0 Part 1, 19.3.1](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#X3DInterpolatorNode).
It verifies empty-list default, a nonzero first keyValue during setup and after
activation, clearing/restoring keyValue, and continued outputOnly application
write denial. Detached unnamed nodes exercise a normal public route. No
synthetic initial event, route delivery or clock step is accepted: an empty
first turn at time -100 leaves the initialized value intact, and only a real
fraction input produces the later interpolation/route/notification. Original
normalized observation, request and notification traces remain unchanged.

## Validation status

Results after final provider backend-hook removal and the additional
18-check initial-output probe:

* Reference common oracle: 1,422 checks in the final fresh CMake reference build
  (the preceding 1,404-check version also passed an independent targeted build)
* Kernel-free malformed schema proof: 42 checks
* Strict syntax checks: fixture/reference, malformed schema and package consumer
  pass `-Wall -Wextra -Wpedantic -Werror`
* The schema proof's defined symbols contain no reference execution-context,
  scene-snapshot, scene-edit, external-session or type-registry implementations
* Fresh ABI-consistent static source integration: all four tests pass;
  native and reference complete reports compare equal at 1,422 checks per
  backend, and the 203-check live oracle plus all nine native runtime evidence
  categories pass
* Relocated static and shared-mode verification continue in the serialized
  integration build; final packaging/link audit results are recorded by the
  integration verification report

Counts are useful diagnostics; the category mappings above define the
preservation claim. Full ISO binding/profile conformance remains outside this
proof.
