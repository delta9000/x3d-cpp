---
title: "ADR-0065: Adopt a native session into the SAI provider"
summary: Keep one authoritative runtime graph and one host extraction consumer.
tags: [adr, sai, ownership, runtime]
updated: 2026-10-20
related:
  - ../subsystems/sai-provider.md
  - ../subsystems/execution-context.md
---

# ADR-0074: Adopt a native session into the SAI provider

Proposed. CAVEOS loads a RuntimeSession with its own assets, custom geometry,
textures, Inline policy, and sensor systems. Its file viewer cannot use SAI
values on that session; its separate SAI app authors a bounded provider scene.
Copying that graph through another backend would separate editing from native
execution and rendering.

Add `native::adopt_session(unique_ptr<RuntimeSession>, resource_limits)` to the
native companion. It returns an owning portable service, its scene, an active
activation, and checked native host access. Internally the ordinary creation and
activation seams mint frontend authority; activation attaches capture without
reconstructing the runtime. No native pointer enters the portable header.

Require exclusive ownership, idle live callbacks, and no previous tick. Callers
must relinquish mutable aliases: moving a shallow document does not isolate its
graph or author-field store. Native host access checks service identity, thread,
active frontend state, and idle native callbacks. A borrow permits tracking
inputs and extraction between pumps; it does not authorize ticking, field or
topology mutation, or retention across pump/retirement. Owner-thread retirement
must precede any host worker teardown. Later running-session adoption needs a
separate clock, pending-event, and baseline contract.

Register public native identities from root occurrences, root-context DEFs, and
resolved root ROUTE endpoints. Keep order and sharing, including detached DEFs.
Do not validate imported graphs with the bounded authoring containment model.
Stop traversal at PROTO, Inline, IMPORT, and Script boundaries and refuse field
access there by name. Their native runtime structures, internal routes, and
rendering survive. Root names exclude IMPORT aliases. Preserve the whole native
document, including units and META; omit portable metadata capability until its
lossy projection has a defined contract.

Keep the creation whitelist. Adopted fields independently support a small
scalar kind set and identity-preserving node reads. Refuse node writes,
initializeOnly changes, outputOnly writes, and inputOnly reads. Before ingress,
check scalar ranges on disposable native factory storage and compare enum token
round-trips, since generated reflected setters are deliberately lenient. Custom
nodes without factory validation fail closed for writes. Per-field descriptors
remain authoritative; this does not claim complete reflected SAI support.

Place retained batch ingress in X3DExecutionContext. Install the final admitted
inputOutput candidates before observers, handlers, and systems, then run input
occurrences and cascades within one native timestamp. This removes the adapter's
assumption that only interpolator handlers can run. Preserve the ordinary tick
path when no retained ingress is supplied. No rollback is implied after native
execution begins; partial failure faults the portable activation.

The host owns extraction exclusively. Adoption never takes a snapshot or delta;
one pump performs one tick. This preserves a loader baseline and custom packed
geometry, camera, light, fog, and other channels beyond the bounded presentation
projection. Application observations remain owning values delivered by explicit
dispatch after native execution.

The source and relocated `sai_provider_adopt_session` target covers identity,
extraction, routed/deferred changes, Material writes, refusals, opacity,
retirement, and the same-pump TimeSensor interval case. No builds or tests were
run for this proposal. Live ROUTE addition, document convenience adoption,
nested contexts, Script interfaces, and running-session transfer are deferred.

## Amendment (2026-10-20, review fixes)

- **Late identities.** Registration at adoption alone made native SFNode/MFNode
  writes (a Script ROUTE into `addChildren`, host edits) throw "foreign node"
  from the field-write listener and fault the pump. Adopted identity lookup now
  registers a node, and its non-opaque subgraph, the first time a supported
  read, root list, DEF lookup, or observation reaches it. The opacity rules
  are unchanged. The listener reads and records only fields that a live
  observer watches (`backend::observed`, new in x3d-sai).
- **Type names.** Opaque nodes report their real type through `type_name`: the
  PROTO name for an instance, otherwise the native type. Opacity is reported
  only by the field-discovery refusal.
- **Metadata scope.** The service keeps advertising `scene_metadata`. Only the
  adopted scene refuses META projection, so other scenes in the service keep
  it. This replaces "omit portable metadata capability" above.
- **Tick path.** `do_turn` calls `RuntimeSession::tick(now, ingress)` (a new
  overload) instead of `context().tick`, so deferred Inline diagnostics drain
  into `document().inlineWarnings` on both adopted and bounded scenes.
- The `sai_provider_adopt_session` target has now been built and run against
  the x3d-sai checkout.
