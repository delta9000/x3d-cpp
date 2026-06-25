# Navigation — conformance

_Generated. Levels 1,2,3 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Billboard | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Collision | 2 | ✓ | — | ◑ | COL-1, COL-2, COL-3, CONF-NAV-COLLISION | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DSensorNode |
| LOD | 2 | ✓ | — | — | LOD-1, SENSOR-SWITCH | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| NavigationInfo | 1 | ✓ | — | ✓ | BIND-05, BIND-06 | X3DBindableNode, X3DChildNode |
| OrthoViewpoint | 3 | ✓ | — | ◑ | BIND-01, BIND-02, BIND-03, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, FOV-TYPE, NAV-FLY-ROLL | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| Viewpoint | 1 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, NAV-FLY-ROLL, NAV-LOOKAT-SCALE | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| ViewpointGroup | 3 | ✓ | — | — | — | X3DChildNode |

## Findings

- **COL-1** [critical/DEFERRED] — §23.4.2: Collision isActive/collideTime never fire — no avatar-volume collision detection.
  - Blocked on the collision subsystem (volume sweep). See CONF-NAV-COLLISION.
- **CONF-NAV-COLLISION** [major/DEFERRED] — §23.2.4, 23.3.2: WALK mode + Collision/CollisionSensor (avatar-volume collision) not implemented.
  - Needs a volume-sweep collision subsystem (post-v1). FLY ships collision-free.
- **COL-3** [major/DEFERRED] — §23.4.2: Collision.enabled=FALSE must propagate through the descendant subtree to gate collision (overriding nested enabled=TRUE).
  - Only observable once the collision subsystem exists. See CONF-NAV-COLLISION.
- **SENSOR-SWITCH** [major/OPEN] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - Today branch-blind - sensors enrolled by the full-graph walk (X3DSceneBridge.hpp:338) and ticked unconditionally (ViewDependentSystem.hpp:63-92) while rendering culls inactive branches (SceneExtractor.hpp:570-593). Policy (ADR-0034) - a sensor is in the ACTIVE transformation hierarchy iff some root->node path takes children[whichChoice] at every Switch and the selected level at every LOD (union over DEF/USE); tick only while active (active->inactive forces isActive=FALSE + exitTime once then suppress; inactive->active re-evaluates + enterTime). Script and time-dependent nodes are NOT gated (per 10.4.3). 4.1 - unresolved (open since 2009); engine adopts the Xj3D/Contact reading ahead of spec.
- **FOV-TYPE** [minor/OPEN] — §23.4.5, 42.4.2: fieldOfView is the same 4-tuple but typed MFFloat (OrthoViewpoint) vs SFVec4f (TextureProjectorParallel); OrthoViewpoint arity/ordering unvalidated.
  - Do NOT retype MFFloat->SFVec4f (breaks ClassicVRML brackets vs the sfvec4fValue grammar; Mantis 1398/1468). Policy (ADR-0030) - keep MFFloat storage; add size==4 normalization (FOV_TUPLE_ARITY warning) + min<max ordering check (FOV_EXTENT_ORDER) applied to BOTH nodes; add a non-breaking SFVec4f convenience accessor. Sites OrthoViewpoint.hpp:111/177, TextureProjectorParallel.hpp:77. 4.1 - validated; the 4.1 UOM KEEPS OrthoViewpoint.fieldOfView MFFloat (committee declined the retype for the same reason), so validate-don't-retype is correct.
- **BIND-01** [critical/CLOSED `e3235ee`] — §23.2.3: Navigation writes back into authored position/orientation — corrupts authored values, breaks retainUserOffsets and ROUTE/Script readers (CAVE-critical).
  - CONF-VIEWNAV — needs a user-offset-state design (authored pose vs accumulated offset) before fixing BIND-01..08 as one cluster.
- **BIND-02** [critical/CLOSED `95d1107`] — §23.3.1: Viewpoint.navigationInfo field ignored — bound viewpoint never dispatches set_bind to its NavigationInfo.
  - CONF-VIEWNAV cluster.
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
- **COL-2** [major/CLOSED `2b84a99`] — §23.4.2: Collision.proxy geometry must NOT be emitted as a visible render item (collision-only geometry).
  - Extraction fix — skip the proxy field in SceneExtractor; independent of the collision subsystem.
- **LOD-1** [minor/CLOSED `2b84a99`] — §23.4.3: When children.size() < range.size()+1, level_changed must report the index of the child actually rendered (clamp), not the raw range bin.
- **BIND-09** [minor/CLOSED] — §23.3.1: Pop (unbind/delete) does not apply the §23.3.1 r6.3 un-jump (next viewpoint keeps its stored relative transform); ViewpointBindSystem treats a pop like a fresh jump bind.
  - Needs push-vs-pop signaling from BindingSystem to distinguish rule 5.1 (reset) from 6.3 (restore stored offset). Per-node offset persists; only the reset-on-rebind path differs. CAVE doesn't exercise viewpoint stacks.
- **NAV-LOOKAT-SCALE** [low/CLOSED] — §23.4.4: LOOKAT framing distance mixes a world-space radius with a local-frame eye placement, so a non-uniformly-scaled ancestor Transform mis-sizes the framed object.
  - Pre-existing; compute the framing distance in the same (local) frame as the placement, or scale by the ancestor factor.
- **NAV-FLY-ROLL** [low/CLOSED] — §23.4.4: FLY accumulates orientation incrementally (yaw-about-world-up + pitch-about-local-right), so a long mixed drag can introduce gradual horizon roll.
  - Pre-existing (unchanged by the offset model). Re-level to world-up each step (decompose to yaw/pitch) if a consumer needs roll-free fly.

