# EnvironmentalSensor — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ProximitySensor | 1 | ✓ | — | ◑ | ENV-03, ENV-04, ENV-06, ENV-07, ENV-08 | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| TransformSensor | 3 | ✓ | — | ✓ | ENV-01 | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| VisibilitySensor | 2 | ✓ | — | ◑ | ENV-05, ENV-06, ENV-07, ENV-09 | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |

## Findings

- **ENV-06** [major/OPEN] — §22.4.1, 22.4.3: Dynamic removal of an active sensor doesn't fire isActive FALSE/exitTime (no detach).
  - Needs a System detach() hook (shared with BIND-06).
- **ENV-03** [minor/OPEN] — §22.4.1: centerOfRotation_changed never emitted.
- **ENV-05** [minor/OPEN] — §22.4.3: Cone (not frustum) test → false isActive=FALSE in wide-aspect periphery.
- **ENV-08** [minor/OPEN] — §22.4.1: enter/exitTime use tick now, not the interpolated boundary-crossing time.
- **ENV-09** [minor/OPEN] — §22.4.3: Visibility radius uses local (unscaled) size, ignoring ancestor scale.
- **ENV-01** [critical/CLOSED] — §22.4.2: TransformSensor has no System — node inert (no isActive/position/orientation_changed).
- **ENV-07** [major/CLOSED `2b84a99`] — §22.4.1: enabled FALSE→TRUE with the viewer already inside doesn't re-fire isActive/enterTime.
- **ENV-04** [minor/CLOSED `2b84a99`] — §22.4.1: position/orientation_changed fire every tick even when the viewer is still (no change-gate).

