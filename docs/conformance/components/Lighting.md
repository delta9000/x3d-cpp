# Lighting — conformance

_Generated. Levels 1,2 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| DirectionalLight | 1 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |
| PointLight | 2 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |
| SpotLight | 2 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |

## Findings

- **LGT-1** [minor/FIXED] — §17.3.1: shadows / shadowIntensity now extracted onto LightDesc.
  - §17.3.1 defines shadows (SFBool, default FALSE) + shadowIntensity (SFFloat, default 1, [0,1]) on every light. FIXED: LightDesc (runtime/extract/RenderItem.hpp) gained bool shadows + float shadowIntensity members; LightSystem::makeLight reads both in the shared X3DLightNode block via getField, so all three light types carry them. Consumers can now honor authored shadow-casting + darkness (a downstream renderer chooses the technique). (sweep 2026-06-25; resolved 2026-07-02)
- **ENVLIGHT-ORPHAN** [low/CLOSED] — §17.5 Table 17.6: EnvironmentLight orphaned in 4.0 Table 17.6 only; removed from the 4.0 body and X3DUOM. Engine correctly omits it for 4.0.
  - For 4.0 the engine is correct (no EnvironmentLight in the 4.0 UOM); ensure conformance tooling derives expected nodes from the X3DUOM, not prose tables, so this stale row is not mis-flagged. 4.1 - FLIPS to an implementation task. The committee RESTORED EnvironmentLight as a full IBL node in 4.1 (fields ambientIntensity, color, diffuseCoefficients, diffuseTexture, specularTexture, global, intensity, on, origin, rotation, shadowIntensity, shadows). When x3d-cpp targets 4.1, regenerate from the 4.1 UOM and implement it (Holger Seelig already had an X_ITE impl the 4.0 removal stranded). Tracked here for the 4.1 bump.

