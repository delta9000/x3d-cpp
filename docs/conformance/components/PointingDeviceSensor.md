# PointingDeviceSensor — conformance

_Generated. Levels 1 · 4 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| CylinderSensor | 1 | ✓ | — | ✓ | DS-1, DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| PlaneSensor | 1 | ✓ | — | ✓ | DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| SphereSensor | 1 | ✓ | — | ✓ | DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| TouchSensor | 1 | ✓ | — | ◑ | CONF-CRITIC-3 | X3DChildNode, X3DPointingDeviceSensorNode, X3DSensorNode, X3DTouchSensorNode |

## Findings

- **CONF-CRITIC-3** [low/OPEN] — §20.4.4: touchTime condition-3 (still-over-at-release pick re-resolution) not verified.
  - Was BACKLOG CONF-CRITIC. Confirm touchTime fires only when the release pick still resolves over the sensor's geometry.
- **DS-1** [major/FIXED `5eef411`] — §20.4.1: CylinderSensor disk mode tracks the Y=0 plane, not the activation-hit Y.
- **DS-2** [minor/FIXED `8652034`] — §20.4.x: A grabbed drag sensor disabled mid-drag stops tracking on the next evaluation.

