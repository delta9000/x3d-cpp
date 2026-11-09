# Interpolation — conformance

_Generated. Levels 1,2,3,4,5 · 13 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ColorInterpolator | 2 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator | 1 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator2D | 3 | ✓ | — | ? | INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| EaseInEaseOut | 4 | ✓ | — | ? | INTERP-01, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| NormalInterpolator | 2 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| OrientationInterpolator | 1 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator | 1 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator2D | 3 | ✓ | — | ? | INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| ScalarInterpolator | 1 | ✓ | — | ? | AUD-INTERP-1, INTERP-02, JSON-STATEMENT-PLACEMENT, LIFE-STRUCTURAL-DETACH, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator | 4 | ✓ | — | ? | INTERP-01, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator2D | 4 | ✓ | — | ? | INTERP-01, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplineScalarInterpolator | 4 | ✓ | — | ? | INTERP-01, INTERP-02, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SquadOrientationInterpolator | 5 | ✓ | — | ? | INTERP-01, INTERP-02, INTERP-03, META-EXTENDED-ATTRIBUTES-WRITE, PIV-1, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |

## Findings

- **INTERP-01** [critical/CLOSED] — §19.2.4, 19.4.10-13: Spline/Squad/EaseInEaseOut interpolators had no System; set_fraction was a no-op.
  - New SplineInterpolation.hpp (Hermite + Squad + ease) + SplineInterpolatorSystem.hpp.
- **LIFE-STRUCTURAL-DETACH** [major/FIXED] — §ISO/IEC 19775-1 §4.4.3, §10.2.1: Structural field writes detach unreachable systems before releasing nodes and attach newly reachable subtrees.
  - Shared nodes remain attached through their other parents. Cascade and direct writes retain replaced node values through lifecycle reconciliation, using the same subtree collector as Inline retirement. System detach waits for active cascade callbacks to return; retained nodes keep their author fields for reattachment. Tests: x3d_structural_lifetime and runtime_callback_retirement_test.cpp structural Script cases.
- **INTERP-02** [major/CLOSED] — §19.3.1: Empty key must emit no events; added a live empty-key guard to all interpolator Systems.
- **JSON-STATEMENT-PLACEMENT** [major/FIXED] — §ISO/IEC 19776-5 Annex A: JSON statement wrappers in child arrays retain ROUTE, IMPORT, and EXPORT records.
  - ROUTE wrappers inside ProtoBody child arrays and nested body nodes belong to the enclosing prototype, while scene routes remain in the scene. Test: reader_audit_test.cpp, JSON statement wrappers in children retain their execution scope.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **PIV-1** [minor/CLOSED] — §—: attachInterpolators enrolls scene nodes using the makeInterpolatorSystems factory; the unused add-only registration helper was removed.
- **AUD-INTERP-1** [minor/CLOSED] — §19.3.1: value_changed read before any input returns the field default instead of keyValue[0].
  - The hand-written linear, spline, and Squad attach paths initialize value_changed from the first authored keyValue or row without posting an event. Regression: interpolator_initial_value_readback.
- **INTERP-03** [low/CLOSED] — §19.4.13: SquadOrientationInterpolator.normalizeVelocity is ignored by quaternion spline interpolation.
  - Closed: the system passes normalizeVelocity into Squad interpolation, which scales quaternion-space angular tangents to total key-path length when TRUE. The FALSE path preserves the previous computation. Covered by interpolator_conformance_test's normalized Squad tangent assertion.

