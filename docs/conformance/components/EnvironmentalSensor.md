# EnvironmentalSensor — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ProximitySensor | 1 | ✓ | — | ✓ | ENV-03, ENV-04, ENV-06, ENV-07, ENV-08, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| TransformSensor | 3 | ✓ | — | ◑ | ENV-01, SENSOR-SWITCH, TRANSFORMSENSOR-SCALE | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| VisibilitySensor | 2 | ✓ | — | ✓ | ENV-05, ENV-06, ENV-07, ENV-09, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |

## Findings

- **TRANSFORMSENSOR-SCALE** [major/OPEN] — §22.4.5: TransformSensor extracts orientation_changed from a scale-bearing relative matrix — a uniform 3x scale collapses the reported angle to 0; non-uniform scale can emit a NaN SFRotation into the route graph.
  - ViewDependentSystem.hpp:278 `rotationFromMatrix(swInv * tw)`; rotationFromMatrix (Mat4.hpp:128) is documented "assumes upper-left 3x3 is pure rotation (no scale)" but the relative matrix folds in both hierarchies' scale. Probe: uniform-scale(3x)+90deg-about-X -> angle=0.000 (true 1.571). Fix: strip scale (normalize the 3x3 columns / polar or TRS decompose) before rotationFromMatrix. (numeric probe.)
- **ENV-01** [critical/CLOSED] — §22.4.2: TransformSensor has no System — node inert (no isActive/position/orientation_changed).
- **ENV-06** [major/CLOSED] — §22.4.1, 22.4.3: Dynamic removal of an active sensor doesn't fire isActive FALSE/exitTime (no detach).
  - update() computes active-path reachability from scene roots (Switch/LOD aware, ADR-0034) and deactivates unreachable sensors via the existing deactivateIfActive path.
- **ENV-07** [major/CLOSED `2b84a99`] — §22.4.1: enabled FALSE→TRUE with the viewer already inside doesn't re-fire isActive/enterTime.
- **SENSOR-SWITCH** [major/CLOSED] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Implemented per ADR-0034. Each tick ViewDependentSystem::update computes active-path reachability from ctx.sceneRoots() (Switch descends only children[whichChoice]; LOD only the level the extractor would choose), and deactivates any attached sensor not reached (isActive=FALSE + exitTime once, then suppressed). A reselected branch re-evaluates and re-fires enter. Script and time-dependent nodes are not gated (per 10.4.3). No detach hook was needed; the gate lives in update().
- **ENV-03** [minor/CLOSED] — §22.4.1: centerOfRotation_changed never emitted.
  - ProximitySensor now emits centerOfRotation_changed (bound Viewpoint's centerOfRotation in the sensor's frame), change-gated; GeoProximitySensor remains deferred (CONF-GEO).
- **ENV-04** [minor/CLOSED `2b84a99`] — §22.4.1: position/orientation_changed fire every tick even when the viewer is still (no change-gate).
- **ENV-05** [minor/CLOSED] — §22.4.3: Cone (not frustum) test → false isActive=FALSE in wide-aspect periphery.
  - Six view-frustum planes from Viewpoint.fieldOfView (smaller angle) + ViewVolume aspect; per-axis so wide aspect widens only the horizontal half-angle.
- **ENV-08** [minor/CLOSED] — §22.4.1: enter/exitTime use tick now, not the interpolated boundary-crossing time.
  - ProximitySensor interpolates enter/exitTime along the viewer's straight-line motion between ticks (box crossing); VisibilitySensor keeps tick now (no geometric boundary).
- **ENV-09** [minor/CLOSED] — §22.4.3: Visibility radius uses local (unscaled) size, ignoring ancestor scale.
  - The sensor box is transformed to world by worldTransformAny (full ancestor matrix, scale included) before the frustum test.

