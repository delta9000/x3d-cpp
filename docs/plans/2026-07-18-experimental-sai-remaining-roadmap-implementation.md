# Experimental SAI Remaining 40% Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan phase-by-phase. Do not begin a runtime-changing task without its required design checkpoint.

**Goal:** Move `x3d::sai::experimental` from a proven semantic kernel to a
functional runtime-backed C++ SAI, then close document portability, binding,
ABI, and exhaustive conformance gaps.

**Architecture:** Generated semantic metadata becomes authority, while the
experimental kernel remains the reference semantics. A narrow versioned
adapter connects those semantics to the current runtime only after graph,
scope, and lifecycle contracts are executable. Document bridges and binding
hardening build on the same traces rather than introducing parallel behavior.

**Tech Stack:** C++20, generated X3D metadata, x3d-cpp authoring/runtime/event
systems, CMake/CTest, doctest, pytest, YAML conformance registers, sanitizers,
property and fuzz testing.

---

## Program controls

1. Execute one phase at a time. Do not start the next phase until its exit gate
   is green and its result has been reviewed.
2. Keep new public vocabulary in `x3d::sai::experimental` until an explicit
   promotion decision.
3. Write composed user stories and invariant tests before implementation.
4. A runtime file listed below is a review location, not blanket authorization
   to change it. Each runtime-changing task begins with a separate design doc
   and approval.
5. Every rejected operation is non-mutating and returns structured data.
6. Capability reporting fails closed.
7. A service remains partial until behavior, lifecycle, events, errors, and
   tests satisfy its abstract contract.
8. Run Release/Werror and ASan/UBSan at every phase gate; run the full behavior
   and Python suites before each phase-completion commit.

## Phase 1 — Semantic model authority and complete field mechanics

**Target:** 60% → 68%.

### Task 1.1: Generated descriptor authority adapter

**Files:**
- Create: `experimental/sai/include/x3d/sai/experimental/metadata.hpp`
- Create: `experimental/sai/src/metadata.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `CMakeLists.txt`
- Reference: `generated_cpp_bindings/x3d/nodes/X3DInterfaceRegistry.hpp`
- Reference: `generated_cpp_bindings/x3d/nodes/X3DNode.hpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

1. Write a failing story constructing a browser from generated metadata rather
   than a handwritten `type_registry`.
2. Prove representative descriptors preserve node name, ordered fields,
   access, type, default, component, level, accepted-node constraints, and
   units.
3. Add a read-only metadata adapter; do not make generated node classes part
   of semantic identity.
4. Retain handwritten registries only as explicit test fixtures.
5. Add a drift test comparing every generated interface/field against the
   adapter model.
6. Commit: `feat(sai): derive descriptors from generated metadata`.

### Task 1.2: Complete owning value vocabulary

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/types.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Reference: `runtime/codecs/FieldValueIO.hpp`

1. Inventory all ISO scalar and aggregate field kinds represented by generated
   bindings.
2. Add failing round-trip and equality stories for colors, rotations, vectors,
   matrices, images, time, strings, node references, and their MF forms.
3. Add owning value alternatives with explicit numeric widths and no borrowed
   storage.
4. Derive kind checking and defaults from metadata rather than variant index.
5. Add overflow, NaN, signed-zero, and large-image boundary tests.
6. Commit: `feat(sai): complete semantic field values`.

### Task 1.3: Transactional MF editing

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

1. Write failing typed/dynamic stories for size, indexed read/write, insert,
   erase, append, clear, and complete range replacement.
2. Design a revision-bound `multi_field_edit<T>` or equivalent range editor;
   do not expose mutable STL containers from snapshots.
3. Preserve one atomic `scene_edit` publication and authored operation order.
4. Validate every node element before staging an MFNode mutation.
5. Prove equal typed/dynamic changes and external MF event traces.
6. Map the seven planned MF services to stable CTests.
7. Commit: `feat(sai): add transactional multi-field editing`.

### Task 1.4: Readability, writability, units, purity, and ordering

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Modify: `docs/conformance/sai-invariants.yaml`
- Modify: `docs/conformance/sai-services.yaml`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

1. Add failing lifecycle tables for `readable()` and `writable(intent)` across
   all access types, generations, adapters, and phases.
2. Add unit declarations and canonical/authored value policies; prove parse,
   assign, route, inspect, and serialize convert exactly once.
3. Snapshot every public range before repeated reads and prove no revision,
   ordering, name, queue, or dirty-state mutation occurs.
4. Rebuild reverse occurrences from committed roots and containment fields.
   Keep `INV-EDIT-3` open until later runtime adapters introduce or expose the
   name maps, route caches, capability summaries, or extraction indexes needed
   for an honest independent reconstruction test; the firewalled kernel does
   not maintain those shadow indexes today.
5. Close `INV-FIELD-5` and `INV-ORDER-1` only when their falsification suites
   are executable. Keep `INV-EDIT-3` open on `sai_edit_derived_indexes`, and
   keep `INV-FIELD-4` open on `sai_field_unit_codec_bridge` until later approved
   bridges provide the missing evidence. The semantic kernel proves authored
   assignment, canonical routing, authored inspection, pure reads, and order.
6. Commit: `feat(sai): make field capabilities predictive`.

### Phase 1 gate

Create `experimental/sai/examples/generated_author_inspect.cpp` using generated
metadata only. It must author and inspect a representative scene containing
SF/MF numeric, string, image, node, and unit-bearing fields without handwritten
descriptors.

Run:

```sh
cmake --build build-sai-ci --target x3d_sai_experimental_tests
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sai-san -R '^sai_' --output-on-failure
mise run sai-conformance-gate
mise run sai-invariants
uv run pytest
ctest --test-dir build-sai -L behavior --output-on-failure
```

Stop/go review: confirm the generated model is authority before Phase 2.

## Phase 2 — Executable declarations, scopes, and graph lifecycle

**Target:** 68% → 76%.

### Task 2.1: Node removal and disposal contract

**Completed 2026-07-19 on `sai/semantic-kernel`.** Strict staged reference
integrity covers roots, shared SFNode/MFNode occurrences, names, exports,
routes, committed imports, and import/removal races. Historical snapshots,
field observers, undrained notifications, stale current handles, monotonic IDs,
stale event batches, ordered changes, and value-local `noexcept` disposal have
executable evidence.

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Reference: `runtime/events/tests/node_lifecycle_audit_test.cpp`

1. Design removal separately from handle disposal: graph detachment, node
   existence, and local wrapper lifetime are not synonyms.
2. Add failing stories for shared occurrences, roots, names, routes, imports,
   event queues, old snapshots, and stale live handles.
3. Reject removal while incoming semantic references remain unless the edit
   explicitly removes or rewrites them atomically.
4. Make `dispose()` noexcept and callback-free; it invalidates only that wrapper
   unless the abstract service requires more.
5. Map `SVC-NODE-DISPOSE` only after C++ disposal obligations pass.
6. Commit: `feat(sai): define node removal and disposal`.

### Task 2.2: Declaration and interface model

**Completed 2026-07-19 on `sai/semantic-kernel`.** Stable declaration identity,
ordered exact interfaces, immutable inspection, transactional lifecycle edits,
exclusive handle-authorized template closures, one heterogeneous change stream,
and honest EXTERNPROTO resolution/retention states have executable evidence.
Prototype instances, IS bindings, URL loading, expansion, and runtime behavior
remain Task 2.3 or later work.

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Reference: `runtime/X3DScene.hpp`
- Reference: `runtime/parse/X3DProtoResolver.hpp`

1. Add owning ordered descriptors for PROTO, EXTERNPROTO, interface fields,
   body roots, URLs, load state, and declaration identity.
2. Add enumerate/find/add/remove/update duals through `scene_edit` and
   `scene_snapshot`.
3. Keep declaration identity separate from declaration name.
4. Preserve unresolved EXTERNPROTO facts without inventing a loaded body.
5. Commit: `feat(sai): model prototype declarations`.

### Task 2.3: Prototype instances and IS bindings

**Files:** Same as Task 2.2.

1. Write a failing composed story defining a prototype interface and body,
   creating two instances, and proving independent instance state with shared
   declaration identity.
2. Model IS as explicit typed interface-to-internal-field bindings.
3. Apply the normal field access state machine to interface writes and events.
4. Reject type/access/context mismatches atomically.
5. Compare the semantic result with existing parser expansion fixtures without
   making expansion clones authoritative.
6. Commit: `feat(sai): author prototype instances and IS bindings`.

### Task 2.4: Inline contexts and cross-context ROUTE topology

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Reference: `runtime/parse/tests/inline_routes_test.cpp`
- Reference: `runtime/parse/tests/inline_cycle_test.cpp`

1. Add explicit parent/child context relationship and Inline load-state data.
2. Extend imported aperture records with stable Inline/declaration provenance.
3. Generalize route endpoints to carry context generation plus local node/field
   identity.
4. Author and inspect cross-context routes but keep execution capability false.
5. Reject cycles in context containment separately from event-route cycles.
6. Commit: `feat(sai): model inline contexts and route endpoints`.

### Phase 2 gate

Add `experimental/sai/examples/scoped_author_inspect.cpp` demonstrating local
PROTO, IS, Inline, EXPORT/IMPORT, node removal, and cross-context route
inspection. Compare topology with existing XML/JSON/VRML fixtures.

Stop/go review: approve the complete non-runtime graph before any runtime
adapter implementation.

## Phase 3 — Real runtime adapter and functional SAI milestone

**Target:** 76% → 86%.

### Task 3.1: Runtime adapter design checkpoint

**Required before code:** Write and approve
`docs/plans/YYYY-MM-DD-sai-runtime-adapter-design.md`.

The design must settle:

- adapter ownership and version negotiation;
- semantic ID mapping without raw-pointer identity;
- snapshot capture and runtime-thread affinity;
- edit publication and rollback boundary;
- callback executor and reentrancy;
- capability discovery;
- adapter error taxonomy;
- which runtime changes are required and why each is safe.

No runtime source changes occur in this task.

### Task 3.2: Read-only browser adapter

**Candidate files, subject to Task 3.1 approval:**
- Create: `experimental/sai/include/x3d/sai/experimental/adapter.hpp`
- Create: `experimental/sai/src/runtime_adapter.cpp`
- Reference: `runtime/X3DScene.hpp`
- Reference: `runtime/events/X3DExecutionContext.hpp`
- Reference: `runtime/events/X3DFieldAddress.hpp`
- Test: `experimental/sai/tests/runtime_adapter_test.cpp`

1. Adapt current scene, generation, descriptors, roots, names, routes,
   declarations, and fields into immutable semantic snapshots.
2. Prove repeated adaptation gives stable semantic IDs across unchanged runtime
   state and never exposes runtime pointers publicly.
3. Keep authoring/live capabilities false.
4. Commit: `feat(sai): adapt runtime snapshots`.

### Task 3.3: Atomic runtime authoring publication

**Required checkpoint:** approve a focused edit-publication design and every
runtime mutation it requires.

1. Translate a validated `scene_edit` change set into one runtime publication.
2. Detect runtime generation/revision conflicts before applying effects.
3. Provide rollback or prevalidation strong enough that rejection is
   non-mutation.
4. Re-adapt the runtime and compare its semantic snapshot/change trace to the
   reference kernel result.
5. Enable authoring capability only for demonstrated operations.
6. Commit: `feat(sai): publish semantic edits to runtime`.

### Task 3.4: Live events and cross-context cascades

**Candidate references, subject to approval:**
- `runtime/events/X3DEventCascade.hpp`
- `runtime/events/X3DEventGraph.hpp`
- `runtime/events/X3DSceneBridge.hpp`
- `runtime/script/ScriptSystem.hpp`
- `runtime/events/TimeSensorSystem.hpp`

1. Add an adapter-only authority for runtime-originated outputOnly events.
2. Compare runtime and reference-kernel causal traces for local, cyclic,
   fan-in, Inline, PROTO/IS, sensor, and Script cascades.
3. Preserve one timestamp, loop-breaking rules, and structured portability
   diagnostics across contexts.
4. Never allow external SAI callers to forge runtime output authority.
5. Enable live capability only after trace parity passes.
6. Commit: `feat(sai): bridge live runtime cascades`.

### Task 3.5: Observation cursors, callback executor, and async world changes

**Candidate references, subject to approval:**
- `runtime/script/SaiContext.hpp`
- `runtime/events/X3DExecutionContext.hpp`
- `runtime/parse/X3DParse.hpp`

1. Replace any global dirty-clear assumption with independent revision/event
   cursors for runtime, extraction, checkpoints, and each subscriber.
2. Define callback executor, thread affinity, ordering, cancellation, and
   shutdown behavior.
3. Connect real load, cancel, completion, timeout, and world replacement to
   request/generation identities.
4. Prove stale completions and queued callbacks cannot enter a replacement
   world.
5. Close `INV-EVT-4` only after multi-consumer losslessness is executable.
6. Commit: `feat(sai): add runtime observation and async lifecycle`.

### Phase 3 functional gate

Create `experimental/sai/examples/runtime_browser.cpp` as an external-style
consumer. It must:

1. obtain a browser adapter;
2. load a real scene;
3. inspect roots, descriptors, PROTO/Inline scopes, and imported nodes;
4. atomically edit fields and structure;
5. register field and browser observations;
6. trigger and observe a live cross-context cascade;
7. cancel one load and complete another;
8. replace the world and prove old live handles fail safely.

This gate is the **functional SAI milestone**. Stop for interface review before
starting document/binding completion.

## Phase 4 — Document, validation, and capability correctness

**Target:** 86% → 94%.

### Task 4.1: Parse and serialization bridges

**Files:**
- Create: `experimental/sai/include/x3d/sai/experimental/document.hpp`
- Create: `experimental/sai/src/document.cpp`
- Reference: `runtime/parse/X3DParse.hpp`
- Reference: `runtime/codecs/X3DCodecs.hpp`
- Test: `experimental/sai/tests/document_test.cpp`

1. Parse supported encodings into the semantic model through the same
   descriptor, validation, scope, and identity rules as programmatic authoring.
2. Serialize snapshots without making runtime object layout authoritative.
3. Preserve sharing, occurrences, declarations, routes, scopes, values, units,
   and ordering modulo declared normalization.
4. Commit: `feat(sai): bridge documents to semantic scenes`.

### Task 4.2: Semantic equivalence and optional provenance

1. Define semantic graph equivalence independently of lexical source form.
2. Add optional provenance records for source span, lexical spelling, explicit
   defaults, comments, DEF/USE placement, and statements.
3. Prove discarding provenance cannot change semantic behavior.
4. Close `INV-RT-1` and `INV-RT-2` with XML, JSON, Classic VRML, and VRML97
   corpora.
5. Commit: `feat(sai): preserve semantic and source round trips`.

### Task 4.3: Validation and stable diagnostics

1. Define strict authoring, strict parsing, and lenient ingestion policies.
2. Represent proposed repairs as explicit `scene_edit` values; validators never
   mutate secretly.
3. Extend diagnostics with code, severity, operation, context, identity,
   occurrence, endpoints, source location, and causal chain.
4. Serialize diagnostics without relying on human message wording.
5. Close `INV-VAL-1` and `INV-VAL-2`.
6. Commit: `feat(sai): add explicit validation diagnostics`.

### Task 4.4: Predictive capabilities and portable units

1. Adapt profiles, components, levels, encodings, units, and adapter services
   into revisioned capability data.
2. Generate operation probes from every advertised capability and require them
   to succeed under the same generation/configuration.
3. Run equal fixtures through authoring, inspection, live external, and Script
   paths and compare identity, values, errors, and traces.
4. Close `INV-CAP-1` and `INV-PORT-1` only after predictive/parity gates pass.
5. Commit: `feat(sai): make capabilities and profiles predictive`.

### Phase 4 gate

Round-trip the supported conformance corpus through every encoding and compare
semantic graphs, then run capability probes against the real adapter. Record
unsupported features explicitly rather than normalizing them away.

## Phase 5 — Binding completion and product hardening

**Target:** 94% → 100%.

### Task 5.1: Complete the service and obligation audits

**Files:**
- Modify: `docs/conformance/sai-service-catalog.yaml`
- Modify: `docs/conformance/sai-services.yaml`
- Modify: `docs/conformance/sai-invariants.yaml`
- Regenerate: `docs/conformance/SAI-BASELINE.md`

1. Audit all 90 ISO 19775-2 abstract services.
2. Finish the ISO 19777-4 C++ obligation audit.
3. Implement or explicitly mark unsupported every remaining browser utility,
   node/field user-data, matrix, and declaration service.
4. Require a stable CTest for each non-planned row.
5. Commit: `docs(sai): complete service and binding audit`.

### Task 5.2: Generated typed conveniences and profile parity

Phase 1 now generates owner-specific field keys from the same resolved UOM
descriptor path. `sai_generated_binding_drift` compares every actual generated
key with the complete ordered metadata catalog, while
`sai_generated_registry_provenance` seals typed creation to the same model
fingerprint and `sai_generated_registry_mutation` proves that extending a
generated registry revokes it. Remaining:

1. Run one fixture corpus through authoring, inspection, live external, and
   Script profiles and compare semantic traces.
2. Close `INV-GEN-1` only after preventing dangling descriptor, generated-key,
   documentation, and serializer references across the complete catalog.

### Task 5.3: Versioned ABI and adapter boundary

**Required checkpoint:** approve a separate ABI design before implementation.

1. Keep STL layout, exception ABI, inheritance, and allocator choices out of
   scene semantics.
2. Define adapter version negotiation and semantic trace interchange.
3. Test compatible and incompatible adapter versions across a shared-library
   boundary.
4. Close `INV-CXX-5`.
5. Commit: `feat(sai): version the adapter ABI`.

### Task 5.4: Property, adversarial, concurrency, and fuzz gates

1. Generate valid and invalid scene graphs from descriptor constraints.
2. Generate edit/event/load/replacement/disposal sequences and compare the
   reference kernel with the runtime adapter.
3. Stress immutable snapshot reads, conflicting commits, concurrent drains,
   cancellation, and shutdown under ThreadSanitizer where supported.
4. Fuzz document ingestion, value conversion, descriptor lookup, and adapter
   error translation.
5. Promote property/adversarial placeholders in every invariant.
6. Commit: `test(sai): add semantic generative gates`.

### Task 5.5: Packaging, documentation, and strict convergence

1. Install the public experimental headers, library, adapter interface, CMake
   targets, and composed examples without leaking build-tree paths.
2. Document threading, ownership, snapshots, edits, events, errors,
   capabilities, adapter versions, and deliberate unsupported services.
3. Add migration guidance from ISO 19777-4 idioms and current x3d-cpp runtime
   APIs.
4. Run install/embed consumers against shared and static builds.
5. Run the strict SAI convergence gate and require zero planned services,
   invariants, or binding obligations unless explicitly classified unsupported.
6. Commit: `docs(sai): complete functional C++ SAI binding`.

## Final acceptance

The remaining program is complete only when:

- the Phase 3 external consumer works against a real browser runtime;
- semantic and runtime traces agree for authoring, fields, scopes, events,
  observation, async lifecycle, and failure;
- documents round-trip without semantic drift;
- capability reports predict operation results;
- all 90 abstract services and all C++ obligations are audited;
- generated conveniences expose no semantics absent from generic inspection;
- ABI/version mechanics remain separate from scene semantics;
- Release/Werror, ASan/UBSan, concurrency, property, fuzz, behavior, Python,
  conformance, install, and strict SAI gates are green.
