# Interpolation — conformance

_Generated. Levels 1,2,3,4,5 · 13 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ColorInterpolator | 2 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator | 1 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator2D | 3 | ✓ | — | ✓ | INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| EaseInEaseOut | 4 | ✓ | — | ✓ | INTERP-01, ROUTE-IO-ALIAS | X3DChildNode |
| NormalInterpolator | 2 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| OrientationInterpolator | 1 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator | 1 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator2D | 3 | ✓ | — | ✓ | INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| ScalarInterpolator | 1 | ✓ | — | ✓ | AUD-INTERP-1, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator2D | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SplineScalarInterpolator | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| SquadOrientationInterpolator | 5 | ✓ | — | ✓ | INTERP-01, INTERP-02, INTERP-03, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |

## Findings

- **INTERP-01** [critical/CLOSED `07c31ca`] — §19.2.4, 19.4.10-13: Spline/Squad/EaseInEaseOut interpolators had no System; set_fraction was a no-op.
  - New SplineInterpolation.hpp (Hermite + Squad + ease) + SplineInterpolatorSystem.hpp.
- **INTERP-02** [major/CLOSED `07c31ca`] — §19.3.1: Empty key must emit no events; added a live empty-key guard to all interpolator Systems.
- **PIV-1** [minor/CLOSED `07c31ca`] — §—: registerInterpolatorSystems had no production caller; added attachInterpolators scene-walk wiring + makeInterpolatorSystems factory.
- **AUD-INTERP-1** [minor/CLOSED] — §19.3.1: value_changed read before any input returns the field default instead of keyValue[0].
  - The hand-written linear, spline, and Squad attach paths initialize value_changed from the first authored keyValue or row without posting an event. Regression: interpolator_initial_value_readback.
- **INTERP-03** [low/CLOSED] — §19.4.13: SquadOrientationInterpolator.normalizeVelocity is ignored by quaternion spline interpolation.
  - Closed: the system passes normalizeVelocity into Squad interpolation, which scales quaternion-space angular tangents to total key-path length when TRUE. The FALSE path preserves the previous computation. Covered by interpolator_conformance_test's normalized Squad tangent assertion.

