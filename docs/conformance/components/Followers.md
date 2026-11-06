# Followers — conformance

_Generated. Levels 1 · 14 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ColorChaser | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| ColorDamper | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| CoordinateChaser | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, FOL-9, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| CoordinateDamper | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, FOL-9, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| OrientationChaser | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| OrientationDamper | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| PositionChaser | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| PositionChaser2D | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| PositionDamper | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| PositionDamper2D | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| ScalarChaser | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| ScalarDamper | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |
| TexCoordChaser2D | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-5, FOL-9, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChaserNode, X3DChildNode, X3DFollowerNode |
| TexCoordDamper2D | 1 | ✓ | — | ? | FOL-1, FOL-2, FOL-3, FOL-4, FOL-6, FOL-7, FOL-9, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDamperNode, X3DFollowerNode |

## Findings

- **FOL-1** [critical/FIXED] — §39.3.1, 39.3.2, 39.3.3: No follower System — chasers (FIR) / dampers (IIR) never advance value_changed per tick. Whole family inert.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **FOL-2** [critical/FIXED] — §39.3.3: set_destination must drive isActive TRUE + begin per-tick value_changed emission toward the destination — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **FOL-3** [critical/FIXED] — §39.3.3: set_value must abort the in-progress transition and emit value_changed with the received value same-step — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **FOL-4** [critical/FIXED] — §ISO/IEC 19775-1 39.3.3: Equal initial follower values emit one value_changed event on initialization.
  - DamperSystem and ChaserSystem publish initialValue once when it equals initialDestination and remain inactive. Unequal initial values start a transition with isActive TRUE before animation output. The equal-value and unequal-value initialization cases in follower_conformance_test.cpp verify node readback, cascade event counts, and activation ordering for both systems.
- **FOL-6** [critical/FIXED] — §39.3.2: Damper IIR cascade (order chained filters, o_n = d_n + (o_{n-1}-d_n)·e^(-ΔT/τ)) unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **FOL-7** [critical/FIXED] — §39.3.2: Damper tolerance-based settle (isActive→FALSE when output within tolerance of destination) unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **FOL-9** [major/FIXED] — §39.4.3, 39.4.4: MF (array) followers must transition element-wise and converge per-element — unimplemented.
  - Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (cosine-response FIR taps); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser sums overlapping destination deltas, composing SFRotation taps with quaternion slerp.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **FOL-5** [minor/CLOSED] — §39.3.1: Chaser FIR superposition of overlapping set_destination transitions.
  - Closed: ChaserSystem applies the §39.3.1 cosine response to timestamped destination-delta taps, summing scalar/vector/color values and composing rotation deltas with quaternion slerp. Covered by follower_conformance_test's scalar response, overlapping scalar, and OrientationChaser assertions.

