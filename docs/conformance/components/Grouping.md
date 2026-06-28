# Grouping — conformance

_Generated. Levels 1,2,3 · 4 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Group | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| StaticGroup | 3 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| Switch | 2 | ✓ | — | — | SENSOR-SWITCH, SW-DELTA-1 | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Transform | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **SENSOR-SWITCH** [major/OPEN] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Today branch-blind - sensors enrolled by the full-graph walk (X3DSceneBridge.hpp:338) and ticked unconditionally (ViewDependentSystem.hpp:63-92) while rendering culls inactive branches (SceneExtractor.hpp:570-593). Policy (ADR-0034) - a sensor is in the ACTIVE transformation hierarchy iff some root->node path takes children[whichChoice] at every Switch and the selected level at every LOD (union over DEF/USE); tick only while active (active->inactive forces isActive=FALSE + exitTime once then suppress; inactive->active re-evaluates + enterTime). Script and time-dependent nodes are NOT gated (per 10.4.3). 4.1 - unresolved (open since 2009); engine adopts the Xj3D/Contact reading ahead of spec.
- **SW-DELTA-1** [major/FIXED] — §10.3.10: Switch.whichChoice change was invisible to the incremental delta() channel — a runtime change of whichChoice swapped the active child for full-snapshot consumers (cpuraster) but NOT for incremental ones (the OpenGL PoC), which stayed on the old child.
  - Root cause: X3DExecutionContext::classifyDirty mapped a whichChoice field change to DirtyField, but a Switch is in neither geomDeps_ nor materialDeps_, so SceneExtractor::delta() ignored it (no subtree re-walk). The extractor already reads whichChoice on a full walk — only the incremental channel was affected (the prior 'Switch audited clean' note covered fullSnapshot only). Fix: classifyDirty maps whichChoice -> DirtyChildren, so delta() re-walks the Switch subtree and emits removed(old child)+added(new child). Regression: scene_extractor_t8_test.cpp case 5 (flip 0->1->-1). Surfaced via a Switch+IntegerSequencer demo rendered through the OpenGL PoC's incremental delta() path.

