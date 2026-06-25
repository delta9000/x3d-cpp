# Grouping — conformance

_Generated. Levels 1,2,3 · 4 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Group | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| StaticGroup | 3 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| Switch | 2 | ✓ | — | — | SENSOR-SWITCH | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Transform | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **SENSOR-SWITCH** [major/OPEN] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Today branch-blind - sensors enrolled by the full-graph walk (X3DSceneBridge.hpp:338) and ticked unconditionally (ViewDependentSystem.hpp:63-92) while rendering culls inactive branches (SceneExtractor.hpp:570-593). Policy (ADR-0034) - a sensor is in the ACTIVE transformation hierarchy iff some root->node path takes children[whichChoice] at every Switch and the selected level at every LOD (union over DEF/USE); tick only while active (active->inactive forces isActive=FALSE + exitTime once then suppress; inactive->active re-evaluates + enterTime). Script and time-dependent nodes are NOT gated (per 10.4.3). 4.1 - unresolved (open since 2009); engine adopts the Xj3D/Contact reading ahead of spec.

