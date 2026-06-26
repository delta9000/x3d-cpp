# Navigation — conformance

_Generated. Levels 1,2,3 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Billboard | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Collision | 2 | ✓ | — | ◑ | COL-1, COL-2, COL-3, CONF-NAV-COLLISION | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DSensorNode |
| LOD | 2 | ✓ | — | — | LOD-1 | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| NavigationInfo | 1 | ✓ | — | ✓ | BIND-05, BIND-06 | X3DBindableNode, X3DChildNode |
| OrthoViewpoint | 3 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-03, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, NAV-FLY-ROLL | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| Viewpoint | 1 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, NAV-FLY-ROLL, NAV-LOOKAT-SCALE | X3DBindableNode, X3DChildNode, X3DViewpointNode |
| ViewpointGroup | 3 | ✓ | — | — | — | X3DChildNode |

## Findings

- **COL-1** [critical/DEFERRED] — §23.4.2: Collision isActive/collideTime never fire — no avatar-volume collision detection.
  - Blocked on the collision subsystem (volume sweep). See CONF-NAV-COLLISION.
- **CONF-NAV-COLLISION** [major/DEFERRED] — §23.2.4, 23.3.2: WALK mode + Collision/CollisionSensor (avatar-volume collision) not implemented.
  - Needs a volume-sweep collision subsystem (post-v1). FLY ships collision-free.
- **COL-3** [major/DEFERRED] — §23.4.2: Collision.enabled=FALSE must propagate through the descendant subtree to gate collision (overriding nested enabled=TRUE).
  - Only observable once the collision subsystem exists. See CONF-NAV-COLLISION.
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

