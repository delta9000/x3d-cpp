# Experimental C++ SAI semantic kernel

This directory is a compiling proposal for the irreducible semantics of a
modern C++ X3D SAI. It is deliberately firewalled in
`x3d::sai::experimental`: the library links only the C++ standard library and
does not modify or depend on x3d-cpp's current runtime.

The proposal makes these design choices executable:

- `node_id` is semantic identity; an `occurrence` is an ordered containment
  edge with its own parent, field, index, and path.
- An `execution_context` owns one generation. `scene_snapshot` is immutable;
  `scene_edit` stages an atomic revision and returns an ordered `change_set`.
- One `field_descriptor` governs dynamic and typed field handles. Access type
  is enforced as a lifecycle rule, and ordinary failure is a `result<T>` with
  structured `sai_error` data.
- Roots, ordered scoped names, and unique routes are inspected and edited
  through the same transaction. IDs found through inspection resolve back to
  handles, and mutation tokens require no private keys.
- Node type and ordered field descriptors are discoverable as owning values.
  Node-valued writes require context-bearing `node` handles (or spans of them),
  preventing a bare ID from aliasing an object in another ownership domain.
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
profiles/components and units, PROTO/EXTERNPROTO and IMPORT/EXPORT apertures,
node removal, runtime-originated outputOnly events, Script integration, URL
transport, concrete browser adapters, presentation scheduling, provenance, and
an ABI-stable boundary. The event kernel is a synchronous reference model, not
a claim that the complete live SAI capability is available.
The load API models only cancellation and stale-completion publication rules.

The type registry is handwritten test metadata for fast iteration. A future
integration must derive it from the same versioned semantic model as generated
node conveniences and runtime reflection. Likewise, typed fields here are
checked views over the dynamic descriptor; generated node-specific sugar
should remain a convenience layer and expose no additional semantics.

No API in this directory should be promoted merely because it compiles. The
invariant tests and composed examples are the review surface: revise the
experimental vocabulary until those user stories remain clear and the
remaining planned invariants can be expressed without privileged access.
