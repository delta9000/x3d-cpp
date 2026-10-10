# EventUtilities — conformance

_Generated. Levels 1 · 7 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BooleanFilter | 1 | ✓ | — | ? | EUF-1, EUF-4, EUF-5, NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS | X3DChildNode |
| BooleanSequencer | 1 | ✓ | — | ? | AUD-SEQ-1, IACC-3, NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS, SEQ-1, SEQ-2, SEQ-3, SEQ-4, SEQ-5, SEQ-7, SEQ-8 | X3DChildNode, X3DSequencerNode |
| BooleanToggle | 1 | ✓ | — | ? | EUF-2, EUF-5, IACC-2, NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS | X3DChildNode |
| BooleanTrigger | 1 | ✓ | — | ? | NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS, TRIG-1, TRIG-6 | X3DChildNode, X3DTriggerNode |
| IntegerSequencer | 1 | ✓ | — | ? | AUD-SEQ-1, IACC-3, NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS, SEQ-1, SEQ-2, SEQ-3, SEQ-4, SEQ-5, SEQ-7, SEQ-8 | X3DChildNode, X3DSequencerNode |
| IntegerTrigger | 1 | ✓ | — | ? | NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS, TRIG-2, TRIG-4, TRIG-6 | X3DChildNode, X3DTriggerNode |
| TimeTrigger | 1 | ✓ | — | ? | NATIVE-CALLBACK-RETIREMENT, ROUTE-IO-ALIAS, TRIG-3, TRIG-5, TRIG-6 | X3DChildNode, X3DTriggerNode |

## Findings

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
- **NATIVE-CALLBACK-RETIREMENT** [critical/FIXED] — §4.4.3 (execution-context integration lifetime): Native runtime callbacks retained by nodes are revoked before their activation or behavior state is destroyed.
  - Weak per-context and per-system callback leases cover bindable handlers, interpolators (including spline, NURBS and geospatial), followers, event utilities, HAnimMotion and context-owned timing/key/trigger hooks. RuntimeSession retires before extractor teardown; context destruction also covers partial-construction unwinding. Standalone BindingSystem and standard callback-owning systems retire in their destructor bodies before member captures can invoke a retained node. Custom most-derived systems must call the protected early-retirement hook before destroying members. No raw-node traversal or global lifetime registry is used, so retirement cannot clear a replacement's handlers. Runtime/extract/tests/runtime_callback_retirement_test.cpp pins retained storage, valid behavior, replacement, failed construction, independent owner lifetimes and reentrant retirement rejection. Native access stays serial: the host defers destruction until no call is in flight; reentrant destruction fails closed. Context, BindingSystem and System owners are noncopyable/nonmovable; transfer owning pointers or build fresh owners. Custom unguarded callbacks and GeoFrame configuration isolation are outside this fix; author-field ownership is tracked separately below. Process-wide pick/local-matrix diagnostic counters use relaxed atomics so distinct owner-thread worlds do not race on metrics; runtime_diagnostic_counters_test.cpp verifies the combined totals. These metrics are not per-world semantic state. This does not establish full browser/SAI conformance.
- **TRIG-4** [major/CLOSED] — §30.4.6: IntegerTrigger integerKey inputOutput write does not emit integerKey_changed / triggerValue_changed.
  - Writing integerKey (even to the same value) now also emits triggerValue with that value, per the §30.4.6 text; integerKey_changed already came from the inputOutput fan-out (events_misc_test).
- **SEQ-7** [major/CLOSED `47c0714`] — §30.2.4: Duplicate-key tie-break (lowest index wins) + steady-fraction re-emit semantics unimplemented.
- **SEQ-8** [major/CLOSED `47c0714`] — §30.3.1: Internal fraction/index state (seed from key[0], updated by next/previous) not maintained.
- **EUF-4** [major/CLOSED `47c0714`] — §30.4.1: BooleanFilter must emit exactly one of inputTrue/inputFalse per event (not both) plus inputNegate — selection logic absent.
- **EUF-5** [major/CLOSED `47c0714`] — §30.4.1, 30.4.3: No production wiring for the event-filter/toggle nodes (no attach for BooleanFilter/BooleanToggle).
- **TRIG-5** [minor/CLOSED `47c0714`] — §30.4.7: TimeTrigger must fire on FALSE as well as TRUE (boolean value ignored) — relevant once TRIG-3 is implemented.
- **AUD-SEQ-1** [minor/CLOSED] — §30.2.4: With a duplicated final key, the sequencer returns the last keyValue instead of the first.
  - The final-key clamp walks backward over equal final keys, including for fractions beyond the key range. Regression: integer_sequencer_duplicate_last_key_uses_first_value.
- **IACC-2** [minor/FIXED] — §30.4.3: Two TRUE set_boolean events in one cascade both computed the toggle from the same stored value.
  - Each TRUE now flips the stored state at once and schedules one re-read publication per cascade, so N TRUE inputs flip N times and storage, observers and ROUTEs agree on one toggle_changed with the final value. FALSE stays a no-op and set_toggle is ordinary inputOutput delivery. If set_toggle and a later TRUE both arrive in one cascade, state and observers follow both while the outgoing ROUTE keeps the legacy one-delivery-per-timestamp cap shared by every inputOutput field. Tests: event_utility_output_admission_test (BooleanToggle cases) and interactive_profile_test's routed click state machine.
- **IACC-3** [minor/FIXED] — §30.2.4: Sequencers emitted value_changed for every set_fraction, not once per key interval.
  - SequencerSystem remembers the key interval of the last fraction that emitted and stays silent while later fractions remain in it. next/previous and key/keyValue edits forget that interval, so the following fraction emits even in the same interval. Per-cascade output admission is unchanged: a fraction suppressed by admission still selects its interval. Tests: event_utility_output_admission_test (`sequencer emits one value_changed per key interval across timestamps`) and interactive_profile_test's TimeSensor-driven sequencers.

