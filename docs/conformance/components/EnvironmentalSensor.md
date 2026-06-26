# EnvironmentalSensor — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ProximitySensor | 1 | ✓ | — | ◑ | ENV-03, ENV-04, ENV-06, ENV-07, ENV-08, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| TransformSensor | 3 | ✓ | — | ◑ | ENV-01, SENSOR-SWITCH, TRANSFORMSENSOR-SCALE | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| VisibilitySensor | 2 | ✓ | — | ◑ | ENV-05, ENV-06, ENV-07, ENV-09, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |

## Findings

- **ENV-06** [major/OPEN] — §22.4.1, 22.4.3: Dynamic removal of an active sensor doesn't fire isActive FALSE/exitTime (no detach).
  - Needs a System detach() hook (shared with BIND-06).
- **SENSOR-SWITCH** [major/OPEN] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Today branch-blind - sensors enrolled by the full-graph walk (X3DSceneBridge.hpp:338) and ticked unconditionally (ViewDependentSystem.hpp:63-92) while rendering culls inactive branches (SceneExtractor.hpp:570-593). Policy (ADR-0034) - a sensor is in the ACTIVE transformation hierarchy iff some root->node path takes children[whichChoice] at every Switch and the selected level at every LOD (union over DEF/USE); tick only while active (active->inactive forces isActive=FALSE + exitTime once then suppress; inactive->active re-evaluates + enterTime). Script and time-dependent nodes are NOT gated (per 10.4.3). 4.1 - unresolved (open since 2009); engine adopts the Xj3D/Contact reading ahead of spec.
- **TRANSFORMSENSOR-SCALE** [major/OPEN] — §22.4.5: TransformSensor extracts orientation_changed from a scale-bearing relative matrix — a uniform 3x scale collapses the reported angle to 0; non-uniform scale can emit a NaN SFRotation into the route graph.
  - ViewDependentSystem.hpp:278 `rotationFromMatrix(swInv * tw)`; rotationFromMatrix (Mat4.hpp:128) is documented "assumes upper-left 3x3 is pure rotation (no scale)" but the relative matrix folds in both hierarchies' scale. Probe: uniform-scale(3x)+90deg-about-X -> angle=0.000 (true 1.571). Fix: strip scale (normalize the 3x3 columns / polar or TRS decompose) before rotationFromMatrix. (numeric probe.)
- **ENV-03** [minor/OPEN] — §22.4.1: centerOfRotation_changed never emitted.
- **ENV-05** [minor/OPEN] — §22.4.3: Cone (not frustum) test → false isActive=FALSE in wide-aspect periphery.
- **ENV-08** [minor/OPEN] — §22.4.1: enter/exitTime use tick now, not the interpolated boundary-crossing time.
- **ENV-09** [minor/OPEN] — §22.4.3: Visibility radius uses local (unscaled) size, ignoring ancestor scale.
- **ENV-01** [critical/CLOSED] — §22.4.2: TransformSensor has no System — node inert (no isActive/position/orientation_changed).
- **ENV-07** [major/CLOSED `2b84a99`] — §22.4.1: enabled FALSE→TRUE with the viewer already inside doesn't re-fire isActive/enterTime.
- **ENV-04** [minor/CLOSED `2b84a99`] — §22.4.1: position/orientation_changed fire every tick even when the viewer is still (no change-gate).

