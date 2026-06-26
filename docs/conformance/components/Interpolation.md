# Interpolation — conformance

_Generated. Levels 1,2,3,4,5 · 13 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ColorInterpolator | 2 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator | 1 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| CoordinateInterpolator2D | 3 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| EaseInEaseOut | 4 | ✓ | — | ✓ | INTERP-01 | X3DChildNode |
| NormalInterpolator | 2 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| OrientationInterpolator | 1 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator | 1 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| PositionInterpolator2D | 3 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| ScalarInterpolator | 1 | ✓ | — | ✓ | INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| SplinePositionInterpolator2D | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| SplineScalarInterpolator | 4 | ✓ | — | ✓ | INTERP-01, INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| SquadOrientationInterpolator | 5 | ✓ | — | ◑ | INTERP-01, INTERP-02, INTERP-03, PIV-1 | X3DChildNode, X3DInterpolatorNode |

## Findings

- **INTERP-03** [low/OPEN] — §19.4.13: SquadOrientationInterpolator.normalizeVelocity is read by nothing.
  - SplineInterpolatorSystem.hpp:49-56 calls squadOrientation(key,keyValue,fraction); squadOrientation (SplineInterpolation.hpp:199-201) has no normalizeVelocity parameter and the field (binding getNormalizeVelocity, SquadOrientationInterpolator.hpp:94) is never read. §19.4.13/§19.2.3 default FALSE. (sweep 2026-06-25)
- **INTERP-01** [critical/CLOSED `07c31ca`] — §19.2.4, 19.4.10-13: Spline/Squad/EaseInEaseOut interpolators had no System; set_fraction was a no-op.
  - New SplineInterpolation.hpp (Hermite + Squad + ease) + SplineInterpolatorSystem.hpp.
- **INTERP-02** [major/CLOSED `07c31ca`] — §19.3.1: Empty key must emit no events; added a live empty-key guard to all interpolator Systems.
- **PIV-1** [minor/CLOSED `07c31ca`] — §—: registerInterpolatorSystems had no production caller; added attachInterpolators scene-walk wiring + makeInterpolatorSystems factory.

