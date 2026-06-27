---
title: "ADR-0029: directOutput Writes Are Non-Routable (SAI §4.5.2 Governs)"
summary: "A directOutput Script write to another node mutates the target field's value but does not generate an event or seed the event cascade; only a Script's own outputOnly writes are routable."
tags: [adr, scripting, event-cascade, sai, directoutput, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../subsystems/system-script-sai.md
  - ../../conformance/findings.yaml
---

# ADR-0029: directOutput Writes Are Non-Routable (SAI §4.5.2 Governs)

## Status

Proposed — 2026-06-25. Conformance finding `DO-CASCADE`.

## Context

Two normative parts of X3D disagree on whether a `directOutput` Script write to another node's
field is a routable event:

- **19775-1 §29.2.6** ("Scripts with direct outputs", `scripting.md:118`): such writes are sent
  "by sending input events to the corresponding fields. These events **shall be part of the current
  event cascade**." (routable)
- **19775-2 SAI §4.5.2**: an internal interaction that directly writes another node's field
  "generates **no event** and therefore **does not form part of the event cascade**." (non-routable)

The implementor ecosystem follows the SAI reading: Joe Williams reports implementations do not
produce a routable event from the direct write (downstream must watch the target field), and Andreas
Plesch's x3dom tests show `setFieldValue` produced no ROUTE-triggering event while `postMessage` did.
No NIST conformance example exercises cascade generation from a directOutput write.

x3d-cpp currently implements the **§29.2.6 (routable) reading**: `SaiContext::setField`
(`runtime/script/SaiContext.hpp:101`) calls `ctx_.postEvent(node, field, …)` for a permitted
cross-node write, which `EventCascade::postEvent` (`runtime/events/X3DEventCascade.hpp:56-62`)
enqueues as a cascade seed and fans out along the written field's ROUTEs. This is the side now judged
defective, and it is a surprising side-channel (a directOutput write can silently re-drive arbitrary
ROUTEs mid-cascade).

## Decision

A `directOutput` cross-node write **mutates the target field's stored value via the reflection
setter, but does not call `postEvent` — no event is generated and no ROUTE fans out from the written
field.** A self-write to the Script's own `outputOnly` field keeps the routable `postEvent` path
(the sanctioned output channel). Downstream observers see a directOutput effect as the field's
last value (`getField`/render/extract) or must be ROUTEd from the Script's own output field.

19775-2 (SAI) is the more specific text for the field-access seam, §4.4.8.3 already defers to it for
Script/SAI execution-model steps, and the ecosystem behaves this way — so §29.2.6 is the clause to
be corrected upstream, not the engine.

## Consequences

**Positive:**
- Conforms to SAI §4.5.2 and matches x3dom/X_ITE behavior (portability).
- Removes the mid-cascade re-drive side-channel; event flow is predictable.
- Low risk: the existing `sai_context_test.cpp:107-127` value-landing assertion still holds (only the
  routing semantics change).

**Trade-offs:**
- Content that (incorrectly) relied on directOutput writes triggering ROUTEs will not fire those
  ROUTEs — but such content was non-portable already.
- `sai_context_test.cpp:107-127` must be extended to assert no ROUTE fans out from the written field.

## X3D 4.1 alignment

Unresolved upstream in 4.1 (the §29.2.6 vs SAI §4.5.2 contradiction persists). The engine adopts the
SAI direction ahead of the spec; an upstream erratum against §29.2.6 is owed.
