# PointingDeviceSensor — conformance

_Generated. Levels 1 · 4 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| CylinderSensor | 1 | ✓ | — | ? | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-1, DS-2, IACC-1, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| PlaneSensor | 1 | ✓ | — | ? | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| SphereSensor | 1 | ✓ | — | ? | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| TouchSensor | 1 | ✓ | — | ? | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, CONF-CRITIC-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DPointingDeviceSensorNode, X3DSensorNode, X3DTouchSensorNode |

## Findings

- **DS-1** [major/FIXED] — §20.4.1: CylinderSensor disk mode tracks the Y=0 plane, not the activation-hit Y.
- **AUD-PDS-1** [major/CLOSED] — §20.2.1: Pointing sensors tied for lowest do not activate together; only the first sibling sensor gets the event.
  - PointingSensorSystem resolves all tied lowest siblings and retains independent active drag state for each. Covered by 'tied lowest pointing sensors all receive the event' and 'tied PlaneSensors retain independent drag offsets'.
- **AUD-PDS-2** [major/CLOSED] — §20.4.2: Disabling an active sensor deactivates it only on the next pointer update.
  - PointingSensorSystem checks enabled before its pointer-revision gate and deactivates disabled grabbed sensors on the next tick without pointer motion. Covered by 'disabling an active PlaneSensor deactivates without pointer motion'.
- **AUD-PDS-3** [major/CLOSED] — §10.4.3, 20.2.3: Geometry in an unchosen Switch branch is still pickable and can occlude visible geometry.
  - PickSystem checks each cached placement against live Switch and LOD selection on every pick, matching the extractor's active transformation path without stale index entries. Covered by 'pointing sensors ignore geometry in an unchosen Switch branch', 'pick index follows live Switch selection', and 'pick index follows live LOD level'.
- **IACC-1** [major/FIXED] — §20.4.1: CylinderSensor rotation_changed turned the opposite way to the drag.
  - drag_math_test.cpp, interactive_profile_test.cpp: Cylinder and disk drags use the right-handed positive-Y rotation sign.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **DS-2** [minor/FIXED] — §20.4.x: A grabbed drag sensor disabled mid-drag stops tracking on the next evaluation.
- **CONF-CRITIC-3** [low/CLOSED] — §20.4.4: touchTime condition-3 (still-over-at-release pick re-resolution) verified — implemented and tested.
  - pointing_sensor_test.cpp: TouchSensor emits touchTime only when release remains over the active sensor's geometry.

