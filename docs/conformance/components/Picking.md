# Picking — conformance

_Generated. Levels 1,2,3 · 5 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| LinePickSensor | 1 | ✓ | — | ◑ | CONF-PICKSENSOR, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| PickableGroup | 1 | ✓ | — | — | GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DPickableObject |
| PointPickSensor | 1 | ✓ | — | ? | CONF-PICKSENSOR, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| PrimitivePickSensor | 2 | ✓ | — | ? | CONF-PICKSENSOR, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DPickSensorNode, X3DSensorNode |
| VolumePickSensor | 3 | ✓ | — | ? | CONF-PICKSENSOR, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DPickSensorNode, X3DSensorNode |

## Findings

- **CONF-PICKSENSOR** [minor/DEFERRED] — §38: Pick-sensor nodes (Line/Point/Primitive/Volume) don't run the pick engine against pickingGeometry/pickTarget.
  - Pick engine ships (M2D-2); the sensor nodes await a consumer (Picking-component kickoff).
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.

