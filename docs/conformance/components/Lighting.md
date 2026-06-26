# Lighting — conformance

_Generated. Levels 1,2 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| DirectionalLight | 1 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |
| PointLight | 2 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |
| SpotLight | 2 | ✓ | — | — | LGT-1 | X3DChildNode, X3DLightNode |

## Findings

- **LGT-1** [minor/OPEN] — §17.3.1: shadows / shadowIntensity absent from LightDesc — extraction can't drive shadows.
  - §17.3.1 defines shadows (SFBool) + shadowIntensity (SFFloat) on every light. LightDesc (runtime/extract/RenderItem.hpp:473-491) has neither member; LightSystem::makeLight (LightSystem.hpp:120-156) reads neither. May tie into main's ADR-0028 shadow seam — check before wiring. (sweep 2026-06-25)

