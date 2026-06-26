# Picking — conformance

_Generated. Levels 1,2,3 · 5 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| LinePickSensor | 1 | ✓ | — | ✗ | CONF-PICKSENSOR | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| PickableGroup | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DPickableObject |
| PointPickSensor | 1 | ✓ | — | ✗ | CONF-PICKSENSOR | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| PrimitivePickSensor | 2 | ✓ | — | ✗ | CONF-PICKSENSOR | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| VolumePickSensor | 3 | ✓ | — | ✗ | CONF-PICKSENSOR | X3DChildNode, X3DPickSensorNode, X3DSensorNode |

## Findings

- **CONF-PICKSENSOR** [minor/DEFERRED] — §38: Pick-sensor nodes (Line/Point/Primitive/Volume) don't run the pick engine against pickingGeometry/pickTarget.
  - Pick engine ships (M2D-2); the sensor nodes await a consumer (Picking-component kickoff).

