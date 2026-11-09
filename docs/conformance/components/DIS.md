# DIS — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| DISEntityManager | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| DISEntityTypeMapping | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, ROUTE-IO-ALIAS | X3DChildNode, X3DInfoNode, X3DUrlObject |
| EspduTransform | 1 | ✓ | — | ? | GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, NSN-10, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DNetworkSensorNode, X3DSensorNode |
| ReceiverPdu | 1 | ✓ | — | ? | META-EXTENDED-ATTRIBUTES-WRITE, NSN-10, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |
| SignalPdu | 1 | ✓ | — | ? | META-EXTENDED-ATTRIBUTES-WRITE, NSN-10, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |
| TransmitterPdu | 1 | ✓ | — | ? | META-EXTENDED-ATTRIBUTES-WRITE, NSN-10, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-10** [major/DEFERRED] — §28.2.3, 28.3: DIS PDU nodes react to networkMode (standAlone/networkReader/networkWriter) + emit isActive/dead-reckoning — no DIS transport runtime.
  - Blocked on a DIS/networking transport seam (post-v1).
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.

