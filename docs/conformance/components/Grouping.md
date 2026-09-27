# Grouping — conformance

_Generated. Levels 1,2,3 · 4 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Group | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| StaticGroup | 3 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| Switch | 2 | ✓ | — | — | AUD-LGT-2, AUD-PDS-3, SENSOR-SWITCH, SW-DELTA-1 | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Transform | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **SW-DELTA-1** [major/FIXED] — §10.3.10: Switch.whichChoice change was invisible to the incremental delta() channel — a runtime change of whichChoice swapped the active child for full-snapshot consumers (cpuraster) but NOT for incremental ones (the OpenGL PoC), which stayed on the old child.
  - Root cause: X3DExecutionContext::classifyDirty mapped a whichChoice field change to DirtyField, but a Switch is in neither geomDeps_ nor materialDeps_, so SceneExtractor::delta() ignored it (no subtree re-walk). The extractor already reads whichChoice on a full walk — only the incremental channel was affected (the prior 'Switch audited clean' note covered fullSnapshot only). Fix: classifyDirty maps whichChoice -> DirtyChildren, so delta() re-walks the Switch subtree and emits removed(old child)+added(new child). Regression: scene_extractor_t8_test.cpp case 5 (flip 0->1->-1). Surfaced via a Switch+IntegerSequencer demo rendered through the OpenGL PoC's incremental delta() path.
- **SENSOR-SWITCH** [major/CLOSED] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Implemented per ADR-0034. Each tick ViewDependentSystem::update computes active-path reachability from ctx.sceneRoots() (Switch descends only children[whichChoice]; LOD only the level the extractor would choose), and deactivates any attached sensor not reached (isActive=FALSE + exitTime once, then suppressed). A reselected branch re-evaluates and re-fires enter. Script and time-dependent nodes are not gated (per 10.4.3). No detach hook was needed; the gate lives in update().

