# Experimental SAI Event Kernel Design

**Status:** Approved direction; firewalled experimental proposal.

## Objective

Add the smallest live-event model that can falsify the difficult SAI semantics
without coupling `x3d::sai::experimental` to the current runtime. The model must
distinguish retained authoring state from timestamped event intent, propagate
ROUTEs causally, reach quiescence at one timestamp, and publish callbacks only
after the cascade is coherent.

The kernel remains a reference model. It does not claim Script execution, node
behavior, clocks, sensors, browser loading, or complete live-profile support.

## Boundary and vocabulary

`scene_edit` remains the only authoring transaction. It writes retained scene
state and never pretends that an `inputOnly` event is storage.

`execution_context::events(event_time)` creates a move-only `event_batch`
against one base revision. `event_time` is a strong type containing X3D time in
seconds. `event_batch::send` represents an external SAI event and accepts only
`inputOnly` and `inputOutput` destinations. As with authoring, scalar node and
node-range values require context-bearing handles.

The initial kernel has no public way to forge a runtime `outputOnly` event.
That authority belongs to a future runtime adapter and will be added only when
an adapter use case can test it. An external write to an `inputOutput` field is
both retained state and an event source, so it is sufficient to exercise ROUTE
propagation without inventing node behavior.

## Cascade semantics

Committing an event batch validates every seed before publication, then runs a
private cascade at one timestamp:

1. Initial sends enter in caller order, but portable semantics do not depend on
   that order.
2. An `inputOnly` delivery is transient and does not alter snapshot storage.
3. An `inputOutput` delivery updates the staged value and emits through its
   outgoing ROUTEs.
4. ROUTEs are visited in authored order for reproducible diagnostics, while the
   public result marks fan-in whose outcome depends on that incidental order.
5. Each ROUTE transmits at most once in one timestamped cascade, breaking loops
   without suppressing distinct fan-in routes.
6. The cascade ends at quiescence. Validation or propagation failure publishes
   neither retained state nor notifications.

Sending the same seed field twice in one batch is rejected as ambiguous input.
Distinct ROUTEs may fan into one field. The reference kernel chooses its stable
authored-route order but records a structured `nonportable_fan_in` issue; users
may inspect the actual trace but cannot mistake the order for a portable X3D
guarantee.

## Result and publication

`event_result` contains:

- the timestamp and base/final revision;
- an ordered diagnostic trace of `event_delivery` records;
- causal predecessor and optional ROUTE identity for each delivery;
- an optional retained-state `change_set` for changed `inputOutput` fields;
- structured portability issues such as value-dependent fan-in.

A successful cascade that changes retained state advances one revision and
publishes one coherent snapshot. A transient-only cascade keeps the revision
but still produces an event result. A batch created from a stale revision fails
atomically with `stale_revision`.

## Event observation

Field-event observation is separate from scene-change observation.
`execution_context::observe(field, callback)` accepts only `outputOnly` or
`inputOutput` fields and returns move-only RAII subscription ownership. Cascade
commit queues matching deliveries; it invokes no user code. The existing
explicit `drain()` dispatches both queues outside locks and reports callback
failures without starving later observers.

Cancellation before drain suppresses queued delivery. Reentrant callbacks may
create a later event batch; a timestamp that is not strictly later than the
last published event is rejected. The kernel creates no threads and does not
choose callback affinity for an adapter.

## Acceptance stories

The slice is acceptable only when executable stories demonstrate:

1. `inputOnly` accepts event intent but remains unreadable and unretained.
2. `inputOutput` changes retained state and emits through a ROUTE.
3. A multi-hop cascade reaches quiescence at one timestamp.
4. A ROUTE cycle terminates because each route fires at most once.
5. Fan-in is delivered and diagnosed as order-dependent rather than presented
   as a portable total order.
6. Any invalid seed or stale base publishes no state and no callback.
7. Field observers run only during explicit drain, survive callback exceptions,
   and may submit a later batch reentrantly.
8. Typed and dynamic event sends have identical traces and errors.

No current runtime change or adapter is part of this slice.
