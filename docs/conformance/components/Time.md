# Time — conformance

_Generated. Levels 1 · 1 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| TimeSensor | 1 | ✓ | — | ? | AUD-TIME-1, AUD-TIME-2, AUD-TIME-3, AUD-TIME-4, AUD-TIME-5, CONF-CRITIC-1, CONF-TDN1V, JSON-STATEMENT-PLACEMENT, LIFE-STRUCTURAL-DETACH, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, READ-NUMERIC-LEXEMES, ROUTE-IO-ALIAS, TDN-1, TDN-2, TDN-3, TDN-4, TDN-6, TDN-7, TDN-8, TIME-ORIGIN-1 | X3DChildNode, X3DSensorNode, X3DTimeDependentNode |

## Findings

- **LIFE-STRUCTURAL-DETACH** [major/FIXED] — §ISO/IEC 19775-1 §4.4.3, §10.2.1: Structural field writes detach unreachable systems before releasing nodes and attach newly reachable subtrees.
  - Shared nodes remain attached through their other parents. Cascade and direct writes retain replaced node values through lifecycle reconciliation, using the same subtree collector as Inline retirement. System detach waits for active cascade callbacks to return; retained nodes keep their author fields for reattachment. Tests: x3d_structural_lifetime and runtime_callback_retirement_test.cpp structural Script cases.
- **TDN-1** [major/FIXED] — §8.2.4.4: pauseTime_changed emitted at the pause edge.
- **TDN-2** [major/FIXED] — §8.2.4.4: resumeTime_changed emitted at the resume edge.
- **TDN-3** [major/FIXED] — §8.2.4.3: loop=FALSE finishes the current cycle instead of deactivating next tick.
- **TDN-4** [major/FIXED] — §8.2.4.3, 8.4.1: set_startTime ignored while active (activation snapshot).
- **READ-NUMERIC-LEXEMES** [major/FIXED] — §ISO/IEC 19776-1 5.5, 5.6, 5.8; ISO/IEC 19776-2 5.5, 5.6, 5.8, A.4: Numeric field readers accept leading plus signs and signed hexadecimal integers.
  - FieldValueIO accepts optional leading plus signs for floats, doubles, and integers, and 0x/0X integer prefixes with either sign. XML and Classic reader tests in codec_conformance_test.cpp cover scalar and vector fields, MFInt32, and signed integer boundaries. Malformed and out-of-range values retain the zero fallback. ISO/IEC 19776-5 5.8 specifies decimal JSON integers; JSON numeric tokenization is unchanged.
- **AUD-TIME-1** [major/CLOSED] — §8.2.4.4: A late resume shifts the clock by resumeTime-pauseTime instead of the actual paused span, so fraction_changed jumps on resume.
  - X3DTimeDependentSystem now shifts the elapsed and cycle bases by the actual tick-to-tick paused span. Regression: timesensor_late_resume_keeps_fraction_frozen.
- **AUD-TIME-3** [major/CLOSED] — §8.2.4.3: set_startTime (and set_stopTime <= startTime) to an active node still overwrite the field and emit *_changed.
  - The shared X3DTimeDependentSystem registers a cascade/direct-write input filter for all attached time-dependent nodes; it rejects active startTime and stopTime <= active startTime before field writes or route fan-out, while allowing ordered stop-then-start restart. Regressions: timesensor_ignores_start_and_invalid_stop_while_active, time_dependent_nodes_ignore_active_timing_inputs.
- **AUD-TIME-5** [major/CLOSED] — §8.4.1: Changing cycleInterval during an active loop makes fraction_changed jump instead of continuing smoothly at the new rate.
  - The cycle phase uses a separate base rebased from the last evaluated tick, leaving elapsedTime independent; a new interval below elapsed cycle duration completes that cycle. Regressions: timesensor_cycle_interval_change_keeps_fraction_continuous, timesensor_shorter_interval_completes_current_cycle.
- **JSON-STATEMENT-PLACEMENT** [major/FIXED] — §ISO/IEC 19776-5 Annex A: JSON statement wrappers in child arrays retain ROUTE, IMPORT, and EXPORT records.
  - ROUTE wrappers inside ProtoBody child arrays and nested body nodes belong to the enclosing prototype, while scene routes remain in the scene. Test: reader_audit_test.cpp, JSON statement wrappers in children retain their execution scope.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **TDN-6** [minor/FIXED] — §8.2.4.4: Resume guard uses strict resumeTime > pauseTime.
- **TDN-7** [minor/FIXED] — §8.4.1: Final time emitted at the exact cycle boundary; no auto-restart after completion.
- **TDN-8** [minor/FIXED] — §8.4.1, 8.2.4.4: fraction_changed continues from its paused value (paused-aware elapsed clock), not wall-clock now.
  - timesensor_test.cpp: Resume continues active-time fraction rather than advancing through paused wall time.
- **TIME-ORIGIN-1** [minor/CLOSED] — §8.2.1: By design: the time origin is the consumer's choice (the `now` fed to tick()), not baked into the SDK. App-relative feeding yields Castle's timeOriginAtLoad behaviour; strict epoch feeding yields literal SFTime semantics.
  - timesensor_test.cpp: The host supplies one consistent time origin and the runtime uses supplied times without epoch conversion.
- **AUD-TIME-2** [minor/CLOSED] — §8.4.1: A stop detected after stopTime emits the final values evaluated at the tick instead of at stopTime.
  - A scheduled stop evaluates final outputs at stopTime; a set_stopTime received after its requested time still evaluates at the receiving tick. Regression: timesensor_stop_uses_stop_time_for_final_output.
- **CONF-TDN1V** [low/CLOSED] — §8.2.4.4: pauseTime_changed/resumeTime_changed emit the field-echo value, not strict simulation-now.
  - pauseTime_changed / resumeTime_changed now carry the simulation time the pause/resume was recognised, per §8.2.4.4 (events_misc_test).
- **CONF-CRITIC-1** [low/CLOSED] — §8.2.4.3: Same-tick stop->start restart of a TimeSensor is unverified (completed run does not auto-restart).
  - A set_stopTime / set_startTime pair at the same instant restarts an active node in place (stays active, no isActive FALSE/TRUE pair, new run from that instant); before, it restarted one tick later with an isActive toggle (events_misc_test).
- **AUD-TIME-4** [low/CLOSED] — §8.2.2, 8.2.4.4: A negative absolute pauseTime never pauses (the code also requires pauseTime > 0).
  - The shared pause/resume guards no longer require positive absolute times. Regression: timesensor_accepts_negative_absolute_pause_time.

