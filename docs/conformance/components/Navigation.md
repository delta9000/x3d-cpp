# Navigation — conformance

_Generated. Levels 1,2,3 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Billboard | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Collision | 2 | ✓ | — | ✓ | COL-1, COL-2, COL-3, CONF-NAV-COLLISION | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DSensorNode |
| LOD | 2 | ✓ | — | — | LOD-1, LOD-DELTA-1, SENSOR-SWITCH | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| NavigationInfo | 1 | ✓ | — | ✓ | BIND-05, BIND-06 | X3DBindableNode, X3DChildNode |
| OrthoViewpoint | 3 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-03, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, FOV-TYPE, NAV-FLY-ROLL | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| Viewpoint | 1 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, NAV-FLY-ROLL, NAV-LOOKAT-SCALE | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| ViewpointGroup | 3 | ✓ | — | — | — | X3DChildNode |

## Findings

- **BIND-01** [critical/CLOSED `e3235ee`] — §23.2.3: Navigation writes back into authored position/orientation — corrupts authored values, breaks retainUserOffsets and ROUTE/Script readers (CAVE-critical).
  - CONF-VIEWNAV — needs a user-offset-state design (authored pose vs accumulated offset) before fixing BIND-01..08 as one cluster.
- **BIND-02** [critical/CLOSED `95d1107`] — §23.3.1: Viewpoint.navigationInfo field ignored — bound viewpoint never dispatches set_bind to its NavigationInfo.
  - CONF-VIEWNAV cluster.
- **COL-1** [critical/CLOSED] — §23.4.2: Collision isActive/collideTime never fire — no avatar-volume collision detection.
  - NavigationSystem fires isActive TRUE + collideTime on the enclosing enabled Collision nodes when avatar contact begins (FLY/WALK moves blocked avatarSize[0] short of geometry), keeps it while resting against the geometry, and isActive FALSE when it ends (collision_test case 1).
- **BIND-03** [major/CLOSED `e3235ee`] — §23.3.1: dynamic_cast<Viewpoint*> in NavigationSystem disables navigation for non-Viewpoint viewpoints.
  - CONF-VIEWNAV cluster.
- **BIND-04** [major/CLOSED `2af9570`] — §23.3.1: retainUserOffsets never tracked (follows from BIND-01).
  - CONF-VIEWNAV cluster.
- **BIND-05** [major/CLOSED `95d1107`] — §23.4.4: Viewpoint-bind transition (transitionType/Time, transitionComplete) only fired for LOOKAT, not on set_bind.
  - CONF-VIEWNAV cluster.
- **BIND-06** [major/CLOSED `95d1107`] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.
- **BIND-07** [major/CLOSED `2af9570`] — §23.3.1: jump=FALSE not honored on bind.
  - CONF-VIEWNAV cluster.
- **BIND-08** [major/CLOSED `2af9570`] — §23.3.1: Per-viewpoint stored relative transform on push-down not captured/restored.
  - CONF-VIEWNAV cluster.
- **CONF-NAV-COLLISION** [major/CLOSED] — §23.2.4, 23.3.2: WALK mode + Collision/CollisionSensor (avatar-volume collision) not implemented.
  - Closed for navigation: NavigationSystem implements WALK (horizontal motion, gravity, terrain following at avatarSize[1], step height avatarSize[2]) and collision-checks FLY/WALK moves via PickSystem::castCollidable (collision_test). The rigid-body CollisionSensor (§37) is a physics concern tracked under CONF-RBP*, not avatar collision.
- **COL-2** [major/CLOSED `2b84a99`] — §23.4.2: Collision.proxy geometry must NOT be emitted as a visible render item (collision-only geometry).
  - Extraction fix — skip the proxy field in SceneExtractor; independent of the collision subsystem.
- **COL-3** [major/CLOSED] — §23.4.2: Collision.enabled=FALSE must propagate through the descendant subtree to gate collision (overriding nested enabled=TRUE).
  - PickSystem::castCollidable skips the whole subtree of a disabled Collision, nested enabled Collision nodes included (collision_test case 2).
- **SENSOR-SWITCH** [major/CLOSED] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Implemented per ADR-0034. Each tick ViewDependentSystem::update computes active-path reachability from ctx.sceneRoots() (Switch descends only children[whichChoice]; LOD only the level the extractor would choose), and deactivates any attached sensor not reached (isActive=FALSE + exitTime once, then suppressed). A reselected branch re-evaluates and re-fires enter. Script and time-dependent nodes are not gated (per 10.4.3). No detach hook was needed; the gate lives in update().
- **LOD-1** [minor/CLOSED `2b84a99`] — §23.4.3: When children.size() < range.size()+1, level_changed must report the index of the child actually rendered (clamp), not the raw range bin.
- **LOD-DELTA-1** [minor/FIXED] — §23.4.3: LOD active-level changes were invisible to the incremental delta() channel — the rendered level is computed from the camera, not a settable field, so it never reaches classifyDirty. Incremental consumers (the OpenGL PoC) stayed on the stale level while full-snapshot consumers (cpuraster, which re-extracts every frame) swapped.
  - View-dependent sibling of SW-DELTA-1 (settable-field active-child). Fix: ViewDependentSystem calls X3DExecutionContext::markActiveChildChanged(lod) when the announced level flips, so delta() re-walks the LOD subtree and swaps the active child. Regression: scene_extractor_t8_test.cpp case 6 (move Viewpoint d=10 -> d=1 -> swap). RESIDUAL (deferred): ViewDependentSystem's per-node level uses worldTransform(node) = the FIRST-PATH (identity-if-unknown) transform (the M2C-1 per-node-event / per-path-render split), so an LOD under an ANIMATED PARENT TRANSFORM or a multi-path USE is not detected and stays stale in delta() — the accurate level is computed per-path only in the extractor walk. Same per-path view-dependent gap as Billboard orientation (deferred, classifyDirty M2c/M2d note). Found by differential testing: poc --animate (incremental delta) vs cpuraster --animate (full snapshot).
- **BIND-09** [minor/CLOSED] — §23.3.1: Pop (unbind/delete) does not apply the §23.3.1 r6.3 un-jump (next viewpoint keeps its stored relative transform); ViewpointBindSystem treats a pop like a fresh jump bind.
  - Needs push-vs-pop signaling from BindingSystem to distinguish rule 5.1 (reset) from 6.3 (restore stored offset). Per-node offset persists; only the reset-on-rebind path differs. CAVE doesn't exercise viewpoint stacks.
- **FOV-TYPE** [minor/CLOSED] — §23.4.5, 42.4.2: fieldOfView is the same 4-tuple but typed MFFloat (OrthoViewpoint) vs SFVec4f (TextureProjectorParallel); OrthoViewpoint arity/ordering unvalidated.
  - Do NOT retype MFFloat->SFVec4f (breaks ClassicVRML brackets vs the sfvec4fValue grammar; Mantis 1398/1468). Policy (ADR-0030) - keep MFFloat storage; add size==4 normalization (FOV_TUPLE_ARITY warning) + min<max ordering check (FOV_EXTENT_ORDER) applied to BOTH nodes; add a non-breaking SFVec4f convenience accessor. Sites OrthoViewpoint.hpp:111/177, TextureProjectorParallel.hpp:77. Closed 2026-09-26: runtime/X3DRangeValidate.hpp emits FOV_TUPLE_ARITY and FOV_EXTENT_ORDER for both nodes and exposes orthoFieldOfView4; runtime/parse/tests/range_warnings_test.cpp covers arities 3/5 and ordering. 4.1 - validated; the 4.1 UOM KEEPS OrthoViewpoint.fieldOfView MFFloat (committee declined the retype for the same reason), so validate-don't-retype is correct.
- **NAV-LOOKAT-SCALE** [low/CLOSED] — §23.4.4: LOOKAT framing distance mixes a world-space radius with a local-frame eye placement, so a non-uniformly-scaled ancestor Transform mis-sizes the framed object.
  - Pre-existing; compute the framing distance in the same (local) frame as the placement, or scale by the ancestor factor.
- **NAV-FLY-ROLL** [low/CLOSED] — §23.4.4: FLY accumulates orientation incrementally (yaw-about-world-up + pitch-about-local-right), so a long mixed drag can introduce gradual horizon roll.
  - Pre-existing (unchanged by the offset model). Re-level to world-up each step (decompose to yaw/pitch) if a consumer needs roll-free fly.

