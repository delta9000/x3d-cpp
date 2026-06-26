# Time — conformance

_Generated. Levels 1 · 1 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| TimeSensor | 1 | ✓ | — | ◑ | CONF-CRITIC-1, CONF-TDN1V, TDN-1, TDN-2, TDN-3, TDN-4, TDN-6, TDN-7, TDN-8 | X3DChildNode, X3DSensorNode, X3DTimeDependentNode |

## Findings

- **CONF-TDN1V** [low/OPEN] — §8.2.4.4: pauseTime_changed/resumeTime_changed emit the field-echo value, not strict simulation-now.
  - Open decision (was BACKLOG CONF-TDN1V). X3DTimeDependentSystem.hpp emits the field value (~=transition instant); a strict reading would pass `now`. 1-line flip if a consumer needs it.
- **CONF-CRITIC-1** [low/OPEN] — §8.2.4.3: Same-tick stop->start restart of a TimeSensor is unverified (completed run does not auto-restart).
  - Was BACKLOG CONF-CRITIC (critic unchecked_behaviors). Verify the same-timestamp stop-then-start case in X3DTimeDependentSystem.
- **TDN-1** [major/FIXED `e92042e`] — §8.2.4.4: pauseTime_changed emitted at the pause edge.
- **TDN-2** [major/FIXED `e92042e`] — §8.2.4.4: resumeTime_changed emitted at the resume edge.
- **TDN-3** [major/FIXED `c7d2c21`] — §8.2.4.3: loop=FALSE finishes the current cycle instead of deactivating next tick.
- **TDN-4** [major/FIXED `c7d2c21`] — §8.2.4.3, 8.4.1: set_startTime ignored while active (activation snapshot).
- **TDN-6** [minor/FIXED `775c3ff`] — §8.2.4.4: Resume guard uses strict resumeTime > pauseTime.
- **TDN-7** [minor/FIXED `3b842d0`] — §8.4.1: Final time emitted at the exact cycle boundary; no auto-restart after completion.
- **TDN-8** [minor/FIXED `e92042e`] — §8.4.1, 8.2.4.4: fraction_changed continues from its paused value (paused-aware elapsed clock), not wall-clock now.
  - Spec self-contradiction (Mantis 1106) - 8.4.1's wall-clock fraction formula vs 8.2.4.4 "continues from its value when paused". x3d-cpp already conforms - X3DTimeDependentSystem.hpp:226 sinceStart = now - timeBase, :321 timeBase += (resumeTime - pauseTime). Corollary of TDN-1/2/6. Follow-ups - retire the naive legacy seed TimeSensorBehavior.hpp:39; add a fraction-continuity regression test. 4.1 - partial upstream (added the 8.2.4.4 requirement, not the 8.4.1 formula); engine already implements the fix.

