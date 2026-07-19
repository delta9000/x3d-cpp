# Experimental C++ SAI semantic kernel

This directory is a compiling proposal for the irreducible semantics of a
modern C++ X3D SAI. It is deliberately firewalled in
`x3d::sai::experimental`: the library links only the C++ standard library and
does not modify or depend on x3d-cpp's current runtime.

An optional `x3d_sai_experimental_metadata` bridge copies node and field facts
from an instance-free catalog generated from the UOM. The core kernel remains
stdlib-only, and generated runtime node objects never participate in SAI
identity. The adapter currently fails closed for field types not yet present in
the experimental owning value vocabulary. The current generated vocabulary is
complete: all scalar and multi-field kinds have exact tags, owning storage, and
distinct strong types where C++ primitives would otherwise collapse X3D
semantics (notably time/double and enum/string). Catalog provenance carries the X3D
specification version, a deterministic SHA-256 fingerprint of the resolved
semantic model, and the generator version.

The proposal makes these design choices executable:

- `node_id` is semantic identity; an `occurrence` is an ordered containment
  edge with its own parent, field, index, and path.
- An `execution_context` owns one generation. `scene_snapshot` is immutable;
  `scene_edit` stages an atomic revision and returns an ordered `change_set`.
- One `field_descriptor` governs dynamic and typed field handles. Access type
  is enforced as a lifecycle rule, and ordinary failure is a `result<T>` with
  structured `sai_error` data.
- Field storage and event equivalence is the public `same_representation`
  relation: IEEE payload bits survive, signed zeros differ, and identical NaN
  payloads match. Ordinary numerical `operator==` remains numerical.
- Roots, ordered scoped names, and unique routes are inspected and edited
  through the same transaction. IDs found through inspection resolve back to
  handles, and mutation tokens require no private keys.
- Node type and ordered field descriptors are discoverable as owning values.
  Node-valued writes require context-bearing `node` handles (or spans of them),
  preventing a bare ID from aliasing an object in another ownership domain.
- Execution-context boundaries have explicit ordered EXPORT/IMPORT apertures.
  Imported nodes and fields are distinct inspection-only proxy types; parent
  snapshots capture immutable source revisions without claiming a globally
  atomic multi-context snapshot.
- Observation is explicit, queued, and drained outside internal locks.
  `subscription` is move-only RAII; cancellation and destruction invoke no
  callbacks.
- External event intent is a timestamped, move-only batch. `inputOnly` delivery
  is transient, `inputOutput` delivery publishes retained state once, and ROUTE
  propagation records causal predecessors while breaking loops per route.
- Field observation is access-governed and shares the explicit drain boundary.
  Fan-in remains inspectable as a deterministic trace plus a structured
  portability issue instead of silently blessing queue order as semantics.
- World replacement and asynchronous completion are generation/request aware,
  so stale handles and stale completions cannot mutate a replacement world.
- Capabilities fail closed: the kernel advertises authoring and inspection, but
  not live execution, loading, or rendering merely because their publication
  safety protocols can be exercised.

`examples/author_inspect.cpp` is the composed user story. The focused doctest
suite is also registered as stable `sai_*` CTests so each demonstrated
invariant can be cited independently by the convergence register.

## Deliberate limits

This is a semantic reference kernel, not a complete ISO/IEC 19775-2 or
19777-4 implementation. It currently omits parsing and serialization,
profiles and authored unit conversion, PROTO/EXTERNPROTO expansion, node removal,
cross-context field writes and ROUTEs, Inline loading, PROTO/IS
apertures, runtime-originated outputOnly events, Script integration, URL
transport, concrete browser adapters, presentation scheduling, scene-change
provenance, and an ABI-stable boundary. Generated catalog provenance is
explicit. EXPORT/IMPORT currently provides snapshot
inspection,
not multi-context live publication. The event kernel is a synchronous reference
model, not a claim that the complete live SAI capability is available.
The load API models only cancellation and stale-completion publication rules.

Handwritten type registries remain explicit test fixtures for fast iteration.
Production-facing experiments can instead select descriptors from the
generated UOM catalog; unsupported value kinds are reported rather than
silently narrowed if a future model extends the vocabulary. The current 4.0
UOM does not formally annotate per-field
unit categories, so the catalog preserves that absence and the unit-overlay
design remains open. Typed fields here are checked views over the dynamic
descriptor; generated node-specific sugar should remain a convenience layer
and expose no additional semantics.

No API in this directory should be promoted merely because it compiles. The
invariant tests and composed examples are the review surface: revise the
experimental vocabulary until those user stories remain clear and the
remaining planned invariants can be expressed without privileged access.
