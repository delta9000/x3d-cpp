# EnvironmentalSensor — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ProximitySensor | 1 | ✓ | — | ✓ | AUD-ENV-2, AUD-ENV-3, AUD-ENV-4, AUD-ENV-5, ENV-03, ENV-04, ENV-06, ENV-07, ENV-08, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| TransformSensor | 3 | ✓ | — | ✓ | ENV-01, SENSOR-SWITCH, TRANSFORMSENSOR-SCALE | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| VisibilitySensor | 2 | ✓ | — | ✓ | AUD-ENV-2, AUD-ENV-3, ENV-05, ENV-06, ENV-07, ENV-09, SENSOR-SWITCH | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |

## Findings

- **ENV-01** [critical/CLOSED] — §22.4.2: TransformSensor has no System — node inert (no isActive/position/orientation_changed).
- **ENV-06** [major/CLOSED] — §22.4.1, 22.4.3: Dynamic removal of an active sensor doesn't fire isActive FALSE/exitTime (no detach).
  - update() computes active-path reachability from scene roots (Switch/LOD aware, ADR-0034) and deactivates unreachable sensors via the existing deactivateIfActive path.
- **ENV-07** [major/CLOSED `2b84a99`] — §22.4.1: enabled FALSE→TRUE with the viewer already inside doesn't re-fire isActive/enterTime.
- **SENSOR-SWITCH** [major/CLOSED] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Implemented per ADR-0034. Each tick ViewDependentSystem::update computes active-path reachability from ctx.sceneRoots() (Switch descends only children[whichChoice]; LOD only the level the extractor would choose), and deactivates any attached sensor not reached (isActive=FALSE + exitTime once, then suppressed). A reselected branch re-evaluates and re-fires enter. Script and time-dependent nodes are not gated (per 10.4.3). No detach hook was needed; the gate lives in update().
- **TRANSFORMSENSOR-SCALE** [major/CLOSED] — §22.4.5: TransformSensor extracts orientation_changed from a scale-bearing relative matrix — a uniform 3x scale collapses the reported angle to 0; non-uniform scale can emit a NaN SFRotation into the route graph.
  - TransformSensor now orthonormalizes the relative matrix basis before extracting orientation; degenerate zero-scale bases retain the last valid rotation (identity initially). Covered by testTransformSensorScaleRotation in view_dependent_test.cpp (uniform and non-uniform scale, plus zero scale).
- **AUD-ENV-3** [major/CLOSED] — §22.4.1, 22.4.3: DEF/USE sensor instances are evaluated through one transform path, not the union of all instances.
  - ProximitySensor and VisibilitySensor now evaluate the union of per-path boxes, limited to active paths by ADR-0034. Covered by proximity_sensor_uses_union_of_active_instances and visibility_sensor_uses_union_of_active_instances.
- **ENV-03** [minor/CLOSED] — §22.4.1: centerOfRotation_changed never emitted.
  - ProximitySensor and GeoProximitySensor emit centerOfRotation_changed (bound Viewpoint's centerOfRotation in the sensor's frame) under LOOKAT, change-gated.
- **ENV-04** [minor/CLOSED `2b84a99`] — §22.4.1: position/orientation_changed fire every tick even when the viewer is still (no change-gate).
- **ENV-05** [minor/CLOSED] — §22.4.3: Cone (not frustum) test → false isActive=FALSE in wide-aspect periphery.
  - Six view-frustum planes from Viewpoint.fieldOfView (smaller angle) + ViewVolume aspect; per-axis so wide aspect widens only the horizontal half-angle.
- **ENV-08** [minor/CLOSED] — §22.4.1: enter/exitTime use tick now, not the interpolated boundary-crossing time.
  - ProximitySensor interpolates enter/exitTime along the viewer's straight-line motion between ticks (box crossing); VisibilitySensor keeps tick now (no geometric boundary).
- **ENV-09** [minor/CLOSED] — §22.4.3: Visibility radius uses local (unscaled) size, ignoring ancestor scale.
  - The sensor box is transformed to world by worldTransformAny (full ancestor matrix, scale included) before the frustum test.
- **AUD-ENV-2** [minor/CLOSED] — §22.4.1: Disabling an active sensor emits exitTime.
  - Disabling an active ProximitySensor or VisibilitySensor now emits isActive FALSE for routed state (following §20.4.2 pointing-sensor precedent) but no exitTime. Covered by disabling_proximity_sensor_sends_no_exit_time and disabling_visibility_sensor_sends_no_exit_time.
- **AUD-ENV-4** [minor/CLOSED] — §22.4.1: No position_changed/orientation_changed at exit time.
  - ProximitySensor now emits changed position and orientation at the exit instant. Covered by proximity_sensor_reports_position_on_exit.
- **AUD-ENV-5** [minor/CLOSED] — §22.4.1: centerOfRotation_changed fires without LOOKAT in the bound NavigationInfo.
  - ProximitySensor now emits centerOfRotation_changed only while the bound NavigationInfo includes LOOKAT. Covered by proximity_center_of_rotation_requires_lookat and testProximityCenterOfRotationChanged.

