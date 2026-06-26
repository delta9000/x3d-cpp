# Followers — conformance

_Generated. Levels 1 · 14 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ColorChaser | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| ColorDamper | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| CoordinateChaser | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, FOL-9 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| CoordinateDamper | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, FOL-9 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| OrientationChaser | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| OrientationDamper | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| PositionChaser | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| PositionChaser2D | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| PositionDamper | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| PositionDamper2D | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| ScalarChaser | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| ScalarDamper | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7 | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| TexCoordChaser2D | 1 | ✓ | — | ◑ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, FOL-9 | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| TexCoordDamper2D | 1 | ✓ | — | ✓ | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, FOL-9 | X3DChildNode, X3DDamperNode, X3DFollowerNode |

## Findings

- **FOL-5** [minor/DEFERRED] — §39.3.1: Chaser FIR ring buffer (time-stamped set_destination events over the duration window) unimplemented.
  - ChaserSystem ships as a re-basing single-transition lerp (reaches destination `duration` after the last set_destination); for non-overlapping use this is spec-equivalent. The literal FIR ring-buffer / overlapping-transition superposition (multiple set_destination events within one duration window accumulating via independent taps) is not implemented — deferred, see FOL-OVL in BACKLOG.md.
- **FOL-1** [critical/FIXED] — §39.3.1, 39.3.2, 39.3.3: No follower System — chasers (FIR) / dampers (IIR) never advance value_changed per tick. Whole family inert.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-2** [critical/FIXED] — §39.3.3: set_destination must drive isActive TRUE + begin per-tick value_changed emission toward the destination — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-3** [critical/FIXED] — §39.3.3: set_value must abort the in-progress transition and emit value_changed with the received value same-step — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-4** [critical/FIXED] — §39.3.3: Initialization (initialValue/initialDestination → one value_changed; isActive seed) unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-6** [critical/FIXED] — §39.3.2: Damper IIR cascade (order chained filters, o_n = d_n + (o_{n-1}-d_n)·e^(-ΔT/τ)) unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-7** [critical/FIXED] — §39.3.2: Damper tolerance-based settle (isActive→FALSE when output within tolerance of destination) unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.
- **FOL-9** [major/FIXED] — §39.4.3, 39.4.4: MF (array) followers must transition element-wise and converge per-element — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within duration is deferred.

