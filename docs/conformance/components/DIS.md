# DIS — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| DISEntityManager | 2 | ✓ | — | — | — | X3DChildNode |
| DISEntityTypeMapping | 2 | ✓ | — | — | — | X3DChildNode, X3DInfoNode, X3DUrlObject |
| EspduTransform | 1 | ✓ | — | ✗ | NSN-10 | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DNetworkSensorNode, X3DSensorNode |
| ReceiverPdu | 1 | ✓ | — | ✗ | NSN-10 | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |
| SignalPdu | 1 | ✓ | — | ✗ | NSN-10 | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |
| TransmitterPdu | 1 | ✓ | — | ✗ | NSN-10 | X3DBoundedObject, X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-10** [major/DEFERRED] — §28.2.3, 28.3: DIS PDU nodes react to networkMode (standAlone/networkReader/networkWriter) + emit isActive/dead-reckoning — no DIS transport runtime.
  - Blocked on a DIS/networking transport seam (post-v1).

