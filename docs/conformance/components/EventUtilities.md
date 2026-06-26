# EventUtilities — conformance

_Generated. Levels 1 · 7 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BooleanFilter | 1 | ✓ | — | ✓ | EUF-1, EUF-4, EUF-5 | X3DChildNode |
| BooleanSequencer | 1 | ✓ | — | ✓ | SEQ-1, SEQ-2, SEQ-3, SEQ-4, SEQ-5, SEQ-7, SEQ-8 | X3DChildNode, X3DSequencerNode |
| BooleanToggle | 1 | ✓ | — | ✓ | EUF-2, EUF-5 | X3DChildNode |
| BooleanTrigger | 1 | ✓ | — | ✓ | TRIG-1, TRIG-6 | X3DChildNode, X3DTriggerNode |
| IntegerSequencer | 1 | ✓ | — | ✓ | SEQ-1, SEQ-2, SEQ-3, SEQ-4, SEQ-5, SEQ-7, SEQ-8 | X3DChildNode, X3DSequencerNode |
| IntegerTrigger | 1 | ✓ | — | ◑ | TRIG-2, TRIG-4, TRIG-6 | X3DChildNode, X3DTriggerNode |
| TimeTrigger | 1 | ✓ | — | ✓ | TRIG-3, TRIG-5, TRIG-6 | X3DChildNode, X3DTriggerNode |

## Findings

- **TRIG-4** [major/OPEN] — §30.4.6: IntegerTrigger integerKey inputOutput write does not emit integerKey_changed / triggerValue_changed.
  - Deferred from the wave-3 fix — integerKey_changed already fans out via the cascade inputOutput alias (AUD-CAS); the triggerValue_changed-on-write half needs §30.4.6 spec re-review before implementing.
- **TRIG-1** [critical/CLOSED `47c0714`] — §30.4.4: BooleanTrigger never emits triggerTrue=TRUE on set_triggerTime — handler is the empty default (no System).
  - set_triggerTimeHandler is never wired; every ROUTE into set_triggerTime is dropped.
- **TRIG-2** [critical/CLOSED `47c0714`] — §30.4.6: IntegerTrigger never emits triggerValue=integerKey on set_boolean=TRUE (and never applies the TRUE-only filter) — no System.
- **TRIG-3** [critical/CLOSED `47c0714`] — §30.4.7: TimeTrigger never emits triggerTime on set_boolean (any value) — no System.
- **TRIG-6** [critical/CLOSED `47c0714`] — §30.2.3: No production wiring for trigger nodes — X3DSceneBridge has attachInterpolators/attachViewDependent but no attachTriggers.
  - Closure shape mirrors attachInterpolators — a TriggerSystem + scene-walk attach.
- **SEQ-1** [critical/CLOSED `47c0714`] — §30.2.4, 30.3.1: set_fraction never produces value_changed — no SequencerSystem; handler slots unwired.
- **SEQ-2** [critical/CLOSED `47c0714`] — §30.2.4: Stepwise selection f(t) (largest key ≤ t, boundary-clamp, NO interpolation) is unimplemented.
  - Distinct from interpolators — sequencers select, never blend. Mirror InterpolatorSystem's key lookup minus the lerp.
- **SEQ-3** [critical/CLOSED `47c0714`] — §30.3.1: next(TRUE) must advance the current index (+1, wrap) and fire value_changed — unimplemented.
- **SEQ-4** [critical/CLOSED `47c0714`] — §30.3.1: previous(TRUE) must step the current index (−1, wrap) and fire value_changed — unimplemented.
- **SEQ-5** [critical/CLOSED `47c0714`] — §30.3.1: next/previous index wrap-around (last→0, 0→last) unimplemented.
- **EUF-1** [critical/CLOSED `47c0714`] — §30.4.1: BooleanFilter routes nothing — on set_boolean it must emit inputTrue/inputFalse (by value) + always inputNegate; no System.
- **EUF-2** [critical/CLOSED `47c0714`] — §30.4.3: BooleanToggle never toggles — on set_boolean=TRUE it must flip and emit toggle_changed; FALSE is a no-op. No System.
- **SEQ-7** [major/CLOSED `47c0714`] — §30.2.4: Duplicate-key tie-break (lowest index wins) + steady-fraction re-emit semantics unimplemented.
- **SEQ-8** [major/CLOSED `47c0714`] — §30.3.1: Internal fraction/index state (seed from key[0], updated by next/previous) not maintained.
- **EUF-4** [major/CLOSED `47c0714`] — §30.4.1: BooleanFilter must emit exactly one of inputTrue/inputFalse per event (not both) plus inputNegate — selection logic absent.
- **EUF-5** [major/CLOSED `47c0714`] — §30.4.1, 30.4.3: No production wiring for the event-filter/toggle nodes (no attach for BooleanFilter/BooleanToggle).
- **TRIG-5** [minor/CLOSED `47c0714`] — §30.4.7: TimeTrigger must fire on FALSE as well as TRUE (boolean value ignored) — relevant once TRIG-3 is implemented.

