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
| ProtoInstance | 2 | ✓ | — | — | PROTO-IS-001, PROTO-SHADOW | X3DChildNode, X3DPrototypeInstance |
| WorldInfo | 1 | ✓ | — | — | — | X3DChildNode, X3DInfoNode |
| X3DStatement | 1 | ✓ | — | — | — |  |

## Findings

- **PROTO-SHADOW** [major/OPEN] — §4.4.4: PROTO reusing a built-in node name silently shadows the built-in (proto table resolved before factory; no collision check at registration) - a security hazard per Mantis 1492.
  - Policy (ADR-0033) - reject (quarantine + ProtoWarning BuiltinShadow) any ProtoDeclare/ ExternProtoDeclare whose name is in X3DNodeFactory::registry(); the built-in keeps the name. Default lenient (warn, scene loads); strict/conformance mode treats it as fatal. Sites ClassicVrmlReader.hpp:298/797, JsonReader.hpp:622/419. Consistent with the self-reference precaution (concepts.md:956) and the ADR-0001 ext-firewall. 4.1 - ALIGNED; the Mantis 1492 4.1 direction is built-in-takes-precedence (Seelig/X_ITE) flagged as an error/security consideration (Brutzman) — exactly this policy.
- **PROTO-IS-001** [major/FIXED] — §4.4.4: Nested ProtoInstance `MFNode` overrides did not propagate through inner `IS/connect`; inner proto defaulted its own field value.

