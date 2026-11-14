---
title: EventUtility System
summary: EventUtility nodes — BooleanFilter/Sequencer/Toggle/Trigger and IntegerSequencer for wiring logic without scripting.
tags: [subsystem, event-utility, boolean-filter, sequencer, toggle, trigger]
updated: 2026-09-26
related:
  - ../architecture.md
  - ../subsystems/event-cascade.md
  - ../subsystems/routes.md
  - ../subsystems/execution-context.md
---

# EventUtility System

The EventUtility System implements the ISO/IEC 19775-1 §30 Event Utilities
component — a cluster of seven node types that wire boolean/integer logic and
stepwise sequencing directly via ROUTEs, without requiring an ECMAScript
`Script`.

## Purpose

The subsystem attaches handlers to these generated node types:

| Node | Spec clause | Behavior |
|---|---|---|
| `BooleanTrigger` | §30.4.4 | Any `set_triggerTime` → emits `triggerTrue=TRUE` |
| `IntegerTrigger` | §30.4.6 | `set_boolean=TRUE` → emits `triggerValue=integerKey`; FALSE ignored. Writing `integerKey` (even to the same value) also emits `triggerValue` with that value, via a field-write listener |
| `TimeTrigger` | §30.4.7 | Any `set_boolean` (value irrelevant) → emits `triggerTime=now` |
| `BooleanFilter` | §30.4.1 | Routes `inputTrue`/`inputFalse` by value; always emits `inputNegate` |
| `BooleanToggle` | §30.4.3 | Each `set_boolean=TRUE` flips `toggle`; FALSE is a no-op; one `toggle_changed` per cascade carries the final state |
| `BooleanSequencer` | §30.4.2 | Stepwise `value_changed` (SFBool) on `set_fraction`, once per key interval; `next`/`previous` step with wrap |
| `IntegerSequencer` | §30.4.5 | Stepwise `value_changed` (SFInt32) on `set_fraction`, once per key interval; `next`/`previous` step with wrap |

All seven are event-driven: they register input handlers in `attach()` and
leave `update()` a no-op. The six nodes with outputOnly fields use
`ctx.postOutputEvent(...)`; admission precedes source storage, observers and
ROUTEs. BooleanToggle's `toggle` is inputOutput, so it has its own contract:
each TRUE flips the stored state immediately (a second TRUE in the same cascade
flips the first one's result), and one re-read seed per cascade publishes the
state as it stands when drained. Storage, observers and ROUTEs therefore agree
on one `toggle_changed` with the final value. An explicit `set_toggle`
is ordinary inputOutput delivery; when it and a later TRUE land in one cascade,
the outgoing ROUTE keeps the per-timestamp cap every inputOutput field has.

Sequencers send one `value_changed` per key interval (§30.2.4). The
system remembers the interval of the last fraction that emitted; a later
fraction in the same interval is silent. `next`/`previous` and edits to `key` or
`keyValue` forget that interval, so the next fraction emits even if it lands in
the same interval again. A fraction whose output loses per-cascade admission
still selects its interval and private index.

The sequencer stepwise-selection rule (`sequencerStepIndex`) deserves explicit
notation: it is the largest index `i` such that `key[i] <= t`, boundary-clamped
at both ends. On duplicate key values, the lowest index wins (SEQ-7: "first
definition wins") — implemented by walking back while `key[i] == t && key[i-1]
== key[i]`. The final-key clamp also walks back over equal final keys for fractions
at or beyond that key. Selection is stepwise.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/EventUtilitySystem.hpp` | All seven System classes + `sequencerStepIndex`; the entire subsystem is header-only |
| `runtime/events/X3DSceneBridge.hpp` | `attachEventUtilities(scene, ctx)` — production wiring; iterates every node in the scene, calls `attach()` on each system, then registers all seven with `ctx.addSystem` |
| `runtime/events/X3DSystem.hpp` | `System` base class (`attach()` / `update()`) that all seven classes extend |
| `runtime/events/X3DExecutionContext.hpp` | `postOutputEvent` / `postEvent` + `addSystem` + `tick` — the cascade/context seams the systems call |
| `generated_cpp_bindings/x3d/nodes/BooleanFilter.hpp` | `setOnSet_booleanHandler` / `emitInput*` accessors |
| `generated_cpp_bindings/x3d/nodes/BooleanToggle.hpp` | `setOnSet_booleanHandler` / `getToggle` |
| `generated_cpp_bindings/x3d/nodes/BooleanTrigger.hpp` | `setOnSet_triggerTimeHandler` / `emitTriggerTrue` |
| `generated_cpp_bindings/x3d/nodes/IntegerTrigger.hpp` | `setOnSet_booleanHandler` / `getIntegerKey` / `emitTriggerValue` |
| `generated_cpp_bindings/x3d/nodes/TimeTrigger.hpp` | `setOnSet_booleanHandler` / `emitTriggerTime` |
| `generated_cpp_bindings/x3d/nodes/BooleanSequencer.hpp` | `setOnSet_fractionHandler` / `setOnNextHandler` / `setOnPreviousHandler` / `getKey` / `getKeyValue()` |
| `generated_cpp_bindings/x3d/nodes/IntegerSequencer.hpp` | Same handler pattern; `getKeyValue()` returns `MFInt32` |
| `runtime/events/tests/event_utility_test.cpp` | Behavioral conformance tests |

## Interfaces and seams

### Exposed interface

The primary entry points are the seven `System` subclasses and the production
wiring helper:

```cpp
namespace x3d::runtime {

// Individual systems — attach + register manually if fine-grained control
// is needed:
class BooleanTriggerSystem  : public System {... };
class IntegerTriggerSystem  : public System {... };
class TimeTriggerSystem     : public System {... };
class BooleanFilterSystem   : public System {... };
class BooleanToggleSystem   : public System {... };

template <typename NodeT, typename ValueT>
class SequencerSystem       : public System {... };

// Step-index helper (exposed inline; also used by the sequencer systems):
std::size_t sequencerStepIndex(const MFFloat &key, float t);

} // namespace x3d::runtime

// Production wiring, declared in X3DSceneBridge.hpp:
void attachEventUtilities(Scene &scene, X3DExecutionContext &ctx);
```

`attachEventUtilities` constructs one instance of each system, walks every node
in the scene calling `sys->attach(n, ctx)` (each system guards with a
`dynamic_cast` and returns immediately on a type miss), then calls
`ctx.addSystem(std::move(sys))` so the context retains ownership.

### Seam points

- `System::attach(node, ctx)` — the only entry point each system needs;
  wires the node's `inputOnly` handler closure. No `update()` override is needed
  (all seven nodes are event-driven). The `System` base is in
  `runtime/events/X3DSystem.hpp`.

- `ctx.postOutputEvent(node, field, value)` — outputOnly producers queue a
  candidate without pre-writing its backing field. The first admitted output per
  field in a logical cascade reaches storage, observers and ROUTEs. All incoming
  inputOnly occurrences still run their handlers. This is the existing native
  selection policy, not a uniquely prescribed ordering for simultaneous events.
  `postEvent` remains external ingress and BooleanToggle's inputOutput path.

- `ctx.now()` — `TimeTriggerSystem` reads this to stamp `triggerTime`. The
  clock is advanced by `ctx.tick(now)` before systems run, so `now()` is the
  current-tick timestamp.

- `setOnSet_<field>Handler(fn)` — the generated binding API that the systems
  call in `attach()`. Each `inputOnly` field on the generated node class has a
  corresponding `setOnXxxHandler` registration method; the cascade delivers into
  the corresponding `onXxx` dispatch method, which calls the stored functor.

- `SequencerSystem::index_` — a per-system `unordered_map<X3DNode*, size_t>`
  that tracks the current keyValue index for each enrolled sequencer node across
  ticks, so `next`/`previous` step from the last fraction-set position rather
  than resetting to zero each time. Every valid input still updates this private
  index even when its candidate output is later suppressed by admission.

- `attachEventUtilities` (in `X3DSceneBridge.hpp`) — the production caller.
  Must be called after `ctx.buildSceneGraph(scene)` and `ctx.buildFrom(scene)`
  so route resolution and the scene node index are in place before attachment.
  The `x3d sim` command wires it through
  `x3d::sim::attachFullRuntime(scene, ctx)` (`tools/x3d-cli/sim_runtime.hpp`).

## Conformance

See the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml) for conformance status.

## Related

- [Architecture](../architecture.md)
- [Event Cascade](event-cascade.md)
- [Routes](routes.md)
- [Execution Context](execution-context.md)
