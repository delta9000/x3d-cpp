# Experimental SAI Remaining 40% Roadmap Design

**Status:** Approved direction; optimize for an end-to-end functional runtime
SAI before exhaustive ISO closure.

## Objective

Move the experimental semantic kernel from roughly 60% of a functional SAI
abstraction to a browser-backed, portable, and sustainable C++ SAI. Work is
organized as vertical capability phases rather than clause-by-clause method
completion. Each phase ends in a user-observable capability and an executable
convergence gate.

The percentage ranges are completion weights, not schedule estimates. Service
rows remain partial until their complete abstract behavior is available; the
roadmap does not turn method presence into conformance claims.

## Strategy considered

Three sequencing strategies were considered:

1. **Contract-first vertical slices (selected).** Complete metadata and graph
   authority, then connect one narrow real runtime adapter, then close document
   portability and exhaustive binding coverage. This reaches a useful SAI
   early without allowing adapter mechanics to define semantics.
2. **ISO service waves.** Implement services in clause order. This improves
   catalog counts but produces long stretches of individually callable methods
   without a coherent browser-backed workflow.
3. **Runtime-first adapter.** Wrap current runtime objects immediately. This
   reaches a demo quickly but would bake current pointer ownership, field
   asymmetries, callback behavior, and incomplete context boundaries into the
   public SAI.

## Phase 1: Semantic model authority and complete field mechanics

**Completion target:** 60% → 68%.

Replace handwritten experimental descriptors as semantic authority with an
adapter over the versioned generated metadata. Complete the field vocabulary
and editing mechanics required by representative X3D scenes.

Deliver:

- complete scalar and aggregate X3D value kinds;
- one descriptor across generated, dynamic, typed, parser, runtime, and SAI
  access paths;
- MF size, element read/write, insert, erase, append, clear, and range replace;
- explicit readable/writable queries derived from lifecycle and capability;
- unit metadata with exactly-once conversion;
- pure reads and semantic iteration order;
- derived-index rebuild equivalence;
- generated/runtime/experimental descriptor drift gates.

Exit when representative generated scenes can be authored and inspected
without a handwritten registry and typed/generic operations have equal traces.

## Phase 2: Executable declarations, scopes, and graph lifecycle

**Completion target:** 68% → 76%.

Complete the non-runtime graph semantics needed by real Inline and prototype
scenes.

Deliver:

- node removal, disposal, and explicit stale-handle effects;
- PROTO and EXTERNPROTO declaration handles;
- prototype instance creation;
- interface and IS bindings;
- Inline child-context identity and lifecycle;
- cross-context ROUTE endpoint representation;
- IMPORT/EXPORT remapping over Inline contexts;
- ordered declaration, route, and scope inspection.

Exit when Inline/PROTO scenes can be represented, edited, inspected, and
round-tripped through the semantic graph even though cross-context live events
remain disabled.

## Phase 3: Real runtime adapter and functional SAI milestone

**Completion target:** 76% → 86%.

Connect the semantic kernel to the existing runtime through a narrow versioned
adapter. No runtime mutation is assumed by this roadmap: each required runtime
change receives an explicit design and review before implementation.

Deliver:

- versioned browser/runtime adapter contract;
- runtime identity to semantic-handle mapping;
- atomic edit publication into the runtime;
- runtime-originated outputOnly events;
- cross-context ROUTE cascades;
- clocks, sensors, and Script event participation;
- independent observation cursors;
- documented callback executor and thread affinity;
- real asynchronous load, cancellation, completion, and replacement;
- capability answers backed by actual adapter support.

Exit with an external C++ program that loads a real scene, inspects it, edits
fields and structure, crosses Inline apertures, observes a live cascade, and
replaces the world safely. This is the first genuinely functional SAI
abstraction.

## Phase 4: Document, validation, and capability correctness

**Completion target:** 86% → 94%.

Make browser-backed operation portable across documents and hosts.

Deliver:

- parse and serialize bridges for supported encodings;
- encoding-independent semantic round trips;
- optional source provenance and fidelity;
- strict validation without secret repair;
- explicit lenient-ingestion policy;
- stable structured diagnostics;
- units preserved through parse, assignment, routing, and serialization;
- predictive profile/component negotiation;
- URL/base resolution and transport error taxonomy.

Exit when scenes round-trip without semantic drift and capability answers
predict whether the corresponding operation succeeds.

## Phase 5: Binding completion and product hardening

**Completion target:** 94% → 100%.

Close the service and binding audits and make the abstraction sustainable for
downstream users.

Deliver:

- audit and map all 90 abstract SAI services;
- complete remaining browser utilities, user-data, and matrix services;
- generated node-specific conveniences with generic parity;
- authoring, inspection, live external, and Script profile-parity corpus;
- versioned ABI and adapter boundary;
- property, adversarial, concurrency, and fuzz suites;
- packaging, installation, examples, and migration guidance;
- executable evidence for every invariant and C++ binding obligation.

Exit when strict SAI convergence gates pass and no public convenience exposes
semantics absent from the generic model.

## Critical path and controls

The critical path is Phase 1 → Phase 2 → Phase 3. Phase 4 and Phase 5 must not
delay the functional runtime milestone, but their invariants remain visible in
all earlier interface reviews.

Every phase follows these controls:

1. Add or refine invariants before widening public API.
2. Write composed user stories and falsification tests before implementation.
3. Keep new vocabulary in `x3d::sai::experimental` until review explicitly
   promotes it.
4. Treat generated metadata as authority and runtime state as an adapter
   concern.
5. Fail capability queries closed.
6. Require non-mutation on every rejected operation.
7. Review runtime changes individually rather than treating adapter work as
   blanket authorization.
8. Mark services complete only when abstract behavior, errors, lifecycle,
   event effects, and tests are all present.

## Program-level success measures

- The functional milestone is reached at Phase 3, not deferred until the
  binding is exhaustive.
- No phase increases the count of complete services through naming alone.
- All old snapshots and stale handles retain defined behavior across adapters.
- Typed, dynamic, Script, parser, and runtime paths share semantic traces.
- Cross-context and asynchronous behavior remain causal, generation-aware,
  and non-mutating on failure.
- The final API can be regenerated and adapted without making C++ ABI layout
  part of X3D scene semantics.
