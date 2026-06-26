# Core — conformance

_Generated. Levels 1,2 · 9 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| MetadataBoolean | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataDouble | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataFloat | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataInteger | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataSet | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataString | 1 | ✓ | — | — | — | X3DMetadataObject |
| ProtoInstance | 2 | ✓ | — | — | PROTO-IS-001 | X3DChildNode, X3DPrototypeInstance |
| WorldInfo | 1 | ✓ | — | — | — | X3DChildNode, X3DInfoNode |
| X3DStatement | 1 | ✓ | — | — | — |  |

## Findings

- **PROTO-IS-001** [major/FIXED] — §4.4.4: Nested ProtoInstance `MFNode` overrides did not propagate through inner `IS/connect`; inner proto defaulted its own field value.

