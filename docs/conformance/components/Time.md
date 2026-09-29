# Time — conformance

_Generated. Levels 1 · 1 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| TimeSensor | 1 | ✓ | — | ? | AUD-TIME-1, AUD-TIME-2, AUD-TIME-3, AUD-TIME-4, AUD-TIME-5, CONF-CRITIC-1, CONF-TDN1V, ROUTE-IO-ALIAS, TDN-1, TDN-2, TDN-3, TDN-4, TDN-6, TDN-7, TDN-8, TIME-ORIGIN-1 | X3DChildNode, X3DSensorNode, X3DTimeDependentNode |

## Findings

- **TDN-1** [major/FIXED `e92042e`] — §8.2.4.4: pauseTime_changed emitted at the pause edge.
- **TDN-2** [major/FIXED `e92042e`] — §8.2.4.4: resumeTime_changed emitted at the resume edge.
- **TDN-3** [major/FIXED `c7d2c21`] — §8.2.4.3: loop=FALSE finishes the current cycle instead of deactivating next tick.
- **TDN-4** [major/FIXED `c7d2c21`] — §8.2.4.3, 8.4.1: set_startTime ignored while active (activation snapshot).
- **AUD-TIME-1** [major/CLOSED] — §8.2.4.4: A late resume shifts the clock by resumeTime-pauseTime instead of the actual paused span, so fraction_changed jumps on resume.
  - X3DTimeDependentSystem now shifts the elapsed and cycle bases by the actual tick-to-tick paused span. Regression: timesensor_late_resume_keeps_fraction_frozen.
- **AUD-TIME-3** [major/CLOSED] — §8.2.4.3: set_startTime (and set_stopTime <= startTime) to an active node still overwrite the field and emit *_changed.
  - The shared X3DTimeDependentSystem registers a cascade/direct-write input filter for all attached time-dependent nodes; it rejects active startTime and stopTime <= active startTime before field writes or route fan-out, while allowing ordered stop-then-start restart. Regressions: timesensor_ignores_start_and_invalid_stop_while_active, time_dependent_nodes_ignore_active_timing_inputs.
- **AUD-TIME-5** [major/CLOSED] — §8.4.1: Changing cycleInterval during an active loop makes fraction_changed jump instead of continuing smoothly at the new rate.
  - The cycle phase uses a separate base rebased from the last evaluated tick, leaving elapsedTime independent; a new interval below elapsed cycle duration completes that cycle. Regressions: timesensor_cycle_interval_change_keeps_fraction_continuous, timesensor_shorter_interval_completes_current_cycle.
- **TDN-6** [minor/FIXED `775c3ff`] — §8.2.4.4: Resume guard uses strict resumeTime > pauseTime.
- **TDN-7** [minor/FIXED `3b842d0`] — §8.4.1: Final time emitted at the exact cycle boundary; no auto-restart after completion.
- **TDN-8** [minor/FIXED `e92042e`] — §8.4.1, 8.2.4.4: fraction_changed continues from its paused value (paused-aware elapsed clock), not wall-clock now.
  - Spec self-contradiction (Mantis 1106) - 8.4.1's wall-clock fraction formula vs 8.2.4.4 "continues from its value when paused". x3d-cpp already conforms - X3DTimeDependentSystem.hpp:226 sinceStart = now - timeBase, :321 timeBase += (resumeTime - pauseTime). Corollary of TDN-1/2/6. Follow-ups - retire the naive legacy seed TimeSensorBehavior.hpp:39; add a fraction-continuity regression test. 4.1 - partial upstream (added the 8.2.4.4 requirement, not the 8.4.1 formula); engine already implements the fix.
- **TIME-ORIGIN-1** [minor/CLOSED] — §8.2.1: By design: the time origin is the consumer's choice (the `now` fed to tick()), not baked into the SDK. App-relative feeding yields Castle's timeOriginAtLoad behaviour; strict epoch feeding yields literal SFTime semantics.
  - X3D SFTime is normatively seconds since the Unix epoch (8.2.1), so a strict reading makes startTime=0 mean 1970. The SDK never reads a wall clock; every time-dependent system reads only the `now` passed to X3DExecutionContext::tick(now), so the embedder picks the origin. Both bundled renderers feed app-relative time (OpenGL PoC: glfwGetTime(); cpuraster --animate: frame/fps), starting ~0 at load — so startTime=0 behaves as "at load", matching Castle Game Engine's NavigationInfo.timeOriginAtLoad extension by default (https://castle-engine.io/x3d_time_origin_considered_uncomfortable). A consumer needing literal spec semantics feeds epoch `now`. CONSEQUENCE / nuance: a file authoring an absolute startTime phases differently under app-relative vs epoch feeding (for startTime<=now loops this only shifts phase, not whether they animate). This is a deliberate contract, documented in docs/wiki/subsystems/system-time.md ("Time origin"). No code change — recorded so the matrix does not later flag it as an unhandled epoch gap.
- **AUD-TIME-2** [minor/CLOSED] — §8.4.1: A stop detected after stopTime emits the final values evaluated at the tick instead of at stopTime.
  - A scheduled stop evaluates final outputs at stopTime; a set_stopTime received after its requested time still evaluates at the receiving tick. Regression: timesensor_stop_uses_stop_time_for_final_output.
- **CONF-TDN1V** [low/CLOSED] — §8.2.4.4: pauseTime_changed/resumeTime_changed emit the field-echo value, not strict simulation-now.
  - pauseTime_changed / resumeTime_changed now carry the simulation time the pause/resume was recognised, per §8.2.4.4 (events_misc_test).
- **CONF-CRITIC-1** [low/CLOSED] — §8.2.4.3: Same-tick stop->start restart of a TimeSensor is unverified (completed run does not auto-restart).
  - A set_stopTime / set_startTime pair at the same instant restarts an active node in place (stays active, no isActive FALSE/TRUE pair, new run from that instant); before, it restarted one tick later with an isActive toggle (events_misc_test).
- **AUD-TIME-4** [low/CLOSED] — §8.2.2, 8.2.4.4: A negative absolute pauseTime never pauses (the code also requires pauseTime > 0).
  - The shared pause/resume guards no longer require positive absolute times. Regression: timesensor_accepts_negative_absolute_pause_time.

