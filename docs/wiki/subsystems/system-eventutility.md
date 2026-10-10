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
`Script`. Before this system was introduced, every one of these nodes was
behaviorally inert: their `inputOnly` handlers were the empty default, so any
ROUTE into them was silently dropped. The system was introduced as part of the
conformance campaign wave-3 fix cycle (commit `47c0714`), closing 16 findings
(TRIG-1/2/3/5/6, SEQ-1..5/7/8, EUF-1/2/4/5).

## Purpose

The subsystem gives behavioral life to seven X3D node types that the generated
bindings define but whose handlers were never wired to a `System`:

| Node | Spec clause | Behavior |
|---|---|---|
| `BooleanTrigger` | §30.4.4 | Any `set_triggerTime` → emits `triggerTrue=TRUE` |
| `IntegerTrigger` | §30.4.6 | `set_boolean=TRUE` → emits `triggerValue=integerKey`; FALSE ignored. Writing `integerKey` (even to the same value) also emits `triggerValue` with that value (TRIG-4), via a field-write listener |
| `TimeTrigger` | §30.4.7 | Any `set_boolean` (value irrelevant) → emits `triggerTime=now` |
| `BooleanFilter` | §30.4.1 | Routes `inputTrue`/`inputFalse` by value; always emits `inputNegate` |
| `BooleanToggle` | §30.4.3 | `set_boolean=TRUE` flips `toggle`; FALSE is a no-op |
| `BooleanSequencer` | §30.4.2 | Stepwise `value_changed` (SFBool) on `set_fraction`; `next`/`previous` step with wrap |
| `IntegerSequencer` | §30.4.5 | Stepwise `value_changed` (SFInt32) on `set_fraction`; `next`/`previous` step with wrap |

All seven are event-driven: they register input handlers in `attach()` and
leave `update()` a no-op. The six nodes with outputOnly fields use
`ctx.postOutputEvent(...)`; admission precedes source storage, observers and
ROUTEs. BooleanToggle retains legacy inputOutput delivery and is excluded from
this admission slice.

The sequencer stepwise-selection rule (`sequencerStepIndex`) deserves explicit
notation: it is the largest index `i` such that `key[i] <= t`, boundary-clamped
at both ends. On duplicate key values, the lowest index wins (SEQ-7: "first
definition wins") — implemented by walking back while `key[i] == t && key[i-1]
== key[i]`. The final-key clamp also walks back over equal final keys for fractions
at or beyond that key (AUD-SEQ-1). This is NOT linear interpolation.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/EventUtilitySystem.hpp` | All seven System classes + `sequencerStepIndex`; the entire subsystem is header-only |
| `runtime/events/X3DSceneBridge.hpp` | `attachEventUtilities(scene, ctx)` — production wiring; iterates every node in the scene, calls `attach` on each system, then registers all seven with `ctx.addSystem` |
| `runtime/events/X3DSystem.hpp` | `System` base class (`attach` / `update`) that all seven classes extend |
| `runtime/events/X3DExecutionContext.hpp` | `postOutputEvent` / `postEvent` + `addSystem` + `tick` — the cascade/context seams the systems call |
| `generated_cpp_bindings/x3d/nodes/BooleanFilter.hpp` | `setOnSet_booleanHandler` / `emitInput*` accessors |
| `generated_cpp_bindings/x3d/nodes/BooleanToggle.hpp` | `setOnSet_booleanHandler` / `getToggle` |
| `generated_cpp_bindings/x3d/nodes/BooleanTrigger.hpp` | `setOnSet_triggerTimeHandler` / `emitTriggerTrue` |
| `generated_cpp_bindings/x3d/nodes/IntegerTrigger.hpp` | `setOnSet_booleanHandler` / `getIntegerKey` / `emitTriggerValue` |
| `generated_cpp_bindings/x3d/nodes/TimeTrigger.hpp` | `setOnSet_booleanHandler` / `emitTriggerTime` |
| `generated_cpp_bindings/x3d/nodes/BooleanSequencer.hpp` | `setOnSet_fractionHandler` / `setOnNextHandler` / `setOnPreviousHandler` / `getKey` / `getKeyValue` |
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
class BooleanTriggerSystem  : public System { ... };
class IntegerTriggerSystem  : public System { ... };
class TimeTriggerSystem     : public System { ... };
class BooleanFilterSystem   : public System { ... };
class BooleanToggleSystem   : public System { ... };

template <typename NodeT, typename ValueT>
class SequencerSystem       : public System { ... };

// Step-index helper (exposed inline; also used by the sequencer systems):
std::size_t sequencerStepIndex(const MFFloat &key, float t);

} // namespace x3d::runtime

// Production wiring — the normal call site (in X3DSceneBridge.hpp):
void attachEventUtilities(Scene &scene, X3DExecutionContext &ctx);
```

`attachEventUtilities` constructs one instance of each system, walks every node
in the scene calling `sys->attach(n, ctx)` (each system guards with a
`dynamic_cast` and returns immediately on a type miss), then calls
`ctx.addSystem(std::move(sys))` so the context retains ownership.

### Seam points

- **`System::attach(node, ctx)`** — the only entry point each system needs;
  wires the node's `inputOnly` handler closure. No `update` override is needed
  (all seven nodes are event-driven). The `System` base is in
  `runtime/events/X3DSystem.hpp`.

- **`ctx.postOutputEvent(node, field, value)`** — outputOnly producers queue a
  candidate without pre-writing its backing field. The first admitted output per
  field in a logical cascade reaches storage, observers and ROUTEs. All incoming
  inputOnly occurrences still run their handlers. This is the existing native
  selection policy, not a uniquely prescribed ordering for simultaneous events.
  `postEvent` remains external ingress and BooleanToggle's legacy inputOutput path.

- **`ctx.now()`** — `TimeTriggerSystem` reads this to stamp `triggerTime`. The
  clock is advanced by `ctx.tick(now)` before systems run, so `now()` is the
  current-tick timestamp.

- **`setOnSet_<field>Handler(fn)`** — the generated binding API that the systems
  call in `attach`. Each `inputOnly` field on the generated node class has a
  corresponding `setOnXxxHandler` registration method; the cascade delivers into
  the corresponding `onXxx` dispatch method, which calls the stored functor.

- **`SequencerSystem::index_`** — a per-system `unordered_map<X3DNode*, size_t>`
  that tracks the current keyValue index for each enrolled sequencer node across
  ticks, so `next`/`previous` step from the last fraction-set position rather
  than resetting to zero each time. Every valid input still updates this private
  index even when its candidate output is later suppressed by admission.

- **`attachEventUtilities` (in `X3DSceneBridge.hpp`)** — the production caller.
  Must be called after `ctx.buildSceneGraph(scene)` and `ctx.buildFrom(scene)`
  so route resolution and the scene node index are in place before attachment.
  The `x3d sim` command wires it through
  `x3d::sim::attachFullRuntime(scene, ctx)` (`tools/x3d-cli/sim_runtime.hpp`).

## How it is tested

- **`ctest --preset dev -R x3d_events_tests`** (doctest case: `event_utility_test`) — the primary behavioral
  conformance test (`runtime/events/tests/event_utility_test.cpp`). Covers all
  seven node types and 16 conformance IDs:
  - TRIG-1/2: `BooleanTrigger` emits `triggerTrue=TRUE`; `IntegerTrigger`
    emits `triggerValue=integerKey` on TRUE and ignores FALSE.
  - TRIG-3/5: `TimeTrigger` fires on both TRUE and FALSE values of `set_boolean`.
  - EUF-1/4: `BooleanFilter` emits exactly one of `inputTrue`/`inputFalse` (by
    value) plus always `inputNegate`; the mutually-exclusive selection logic is
    confirmed.
  - EUF-2: `BooleanToggle` flips on TRUE and no-ops on FALSE.
  - SEQ-1..5/8: `IntegerSequencer` and `BooleanSequencer` stepwise selection
    across boundary and mid-range fractions; `next`/`previous` step with
    wrap-around.
  - SEQ-7: duplicate key values → lowest index wins (confirmed with a
    four-key fixture).
  - AUD-SEQ-1: repeated final keys select their first value at and beyond the
    final fraction.
  - Production wiring (`attachEventUtilities`) is smoke-tested end-to-end
    via a `BooleanToggle` and `IntegerSequencer` exercised through the cascade.

- **Golden regression** — the wave-3 fix was verified byte-identical against the
  committed golden files; ctest advanced to 129/129 at ship. The golden gate
  continues to guard against inadvertent output changes.

- **`x3d sim` behavioral correctness tests** (`tools/x3d-cli/goldens/`) —
  `attachFullRuntime` wires `attachEventUtilities`, so the CLI sim golden traces
  implicitly exercise the event-utility systems on real corpus fixtures.

- `event_utility_output_admission_test.cpp` uses production registration and
  real aliased ROUTE sinks for all six scoped types and eight output fields.
  It covers equal/distinct input occurrences,
  independent BooleanFilter branch/negation admission, nested drains and loops,
  both IntegerTrigger producer paths, valid sequencer transitions despite
  suppressed output, FALSE no-ops where required, wraparound and same-cascade
  repair. TimeTrigger still fires for FALSE. Equal numeric host-time behavior is
  labeled current logical-tick compatibility, not ISO timestamp conformance.

## Open findings

This outputOnly migration does not complete the component:

- BooleanToggle's `toggle` is inputOutput. Its deferred toggle/reset state and
  output-side admission need a separate contract; two queued TRUE inputs can
  currently compute from the same old stored value.
- [§30.2.4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#SequencingSingleFieldEvents)
  requires sequencer suppression within a key interval. The current fraction
  handler still emits across later ticks in the same interval. That distinct
  interval-state gap is not fixed or treated as normative by the admission tests.
- IntegerTrigger's key-write listener is implemented and participates in checked
  `triggerValue` admission. Its `integerKey` inputOutput writes/notifications
  retain their existing behavior; this slice does not generalize inputOutput
  output-side admission or routed fan-in.
- Each outer tick/process still opens a new logical timestamp, even for equal
  numeric host times. See [Event Cascade](event-cascade.md) for this limitation.
- Followers and stateful time producers require their own storage/state review,
  especially sites that mutate output storage before queueing. They are not
  safely migrated by a mechanical queue-call replacement.

The older wave-3 finding labels below are historical audit records. This focused
proof raises no portable SAI capability, complete component/profile claim or
service-register status.

## Related specs and ADRs

- [Architecture](../architecture.md) — the overall runtime layer map in which
  this subsystem sits (behavior/event systems tier).
- [Event Cascade](event-cascade.md) — the `X3DEventCascade` that `postEvent`
  and `tick` drive; the Event Utility handlers post into it.
- [Routes](routes.md) — the ROUTE graph that the cascade fans events along
  after an Event Utility node emits.
- [Execution Context](execution-context.md) — `X3DExecutionContext`: owns the
  cascade, the `addSystem` registry, `postEvent`, and the `now()` clock.
- Spec: `docs/superpowers/specs/2026-06-18-conformance-audit-workflow-design.md`
  — the audit workflow that produced the wave-3 findings (TRIG-*/SEQ-*/EUF-*).
- Conformance findings source of truth: `docs/conformance/findings.yaml`
  (historical wave-3 block, IDs TRIG-1..6, SEQ-1..8, EUF-1/2/4/5).
