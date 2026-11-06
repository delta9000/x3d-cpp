# HAnim — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| HAnimDisplacer | 1 | ✓ | — | — | DELTA-SKIN-BINDING, HAN-2, HAN-AUDIT-2, HANIM-DISP, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometricPropertyNode |
| HAnimHumanoid | 1 | ✓ | — | — | DELTA-SHARED-HUMANOID, DELTA-SITE-POSE, DELTA-SKIN-BINDING, HAN-1, HAN-AUDIT-1, HAN-AUDIT-4, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimJoint | 1 | ✓ | — | — | DELTA-SITE-POSE, DELTA-SKIN-BINDING, HAN-1, HAN-AUDIT-1, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimMotion | 2 | ✓ | — | — | HAN-3, HAN-AUDIT-4, META-EXTENDED-ATTRIBUTES-WRITE, NATIVE-CALLBACK-RETIREMENT, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| HAnimSegment | 1 | ✓ | — | — | GRP-ADDCHILDREN, HAN-AUDIT-2, HANIM-DISP, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| HAnimSite | 1 | ✓ | — | — | DELTA-SITE-POSE, GRP-ADDCHILDREN, HAN-AUDIT-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **NATIVE-CALLBACK-RETIREMENT** [critical/FIXED] — §4.4.3 (execution-context integration lifetime): Native runtime callbacks retained by nodes are revoked before their activation or behavior state is destroyed.
  - runtime_callback_retirement_test.cpp: Native callback leases retire before owner state and reject retained callbacks after retirement.
- **HAN-1** [major/FIXED] — §26.3.2, 26.3.3: HAnim skins deform with the skeleton (ADR-0055).
  - hanim_skin_test.cpp, scene_extractor_hanim_test.cpp: Skin bindings retain uncapped normalized influences and extraction separates binding from pose changes.
- **HAN-2** [major/FIXED] — §26.3.1: HAnimDisplacer morphs Joint skins and Segment meshes with live weights.
  - hanim_skin_test.cpp, scene_extractor_hanim_test.cpp: Joint weights update skin poses while Segment displacement rebuilds the affected mesh.
- **HAN-3** [major/FIXED] — §26.3.4: HAnimMotion plays frame-major channels through the event context.
  - HAnimMotionSystem drives referenced motions, honours enable gates, frame controls and channel masks, and emits frameCount/cycleTime/elapsedTime. Synthetic regression tests cover two joints, Euler order, stepping, looping, gating and ignored groups. (2026-09-27)
- **HAN-AUDIT-1** [major/FIXED] — §26.3.2; ISO/IEC 19774-1 6.2: jointBinding* values were treated as absolute; a child Joint's bind matrix omitted its parents'.
  - hanim_skin_test.cpp: Joint binding matrices compose each joint's binding TRS through its ancestors.
- **HAN-AUDIT-2** [major/FIXED] — §26.3.1, 26.3.5; ISO/IEC 19774-1 6.6: One geometry under two Segments sharing a Coordinate got one Segment's displacement, and delta() disagreed with fullSnapshot().
  - scene_extractor_hanim_test.cpp: Geometry shared by Segments receives displacement per placement's Segment.
- **HAN-AUDIT-3** [major/FIXED] — §26.3.6; ISO/IEC 19774-1 6.5: HAnimSite's center/rotation/scale/scaleOrientation/translation were ignored.
  - TransformSystem::isTransform omitted HAnimSite, so Site children (attachments, and cameras under Sites) were placed in the parent's frame. HAnimSite is now a transform-bearing node for propagation, dirty classification and extraction. Test: scene_extractor_hanim_test "HAnimSite transforms its child geometry". (2026-09-27)
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **DELTA-SHARED-HUMANOID** [major/FIXED] — §ISO/IEC 19775-1 26.3.2: Scalar humanoid edits preserve every shared render placement.
- **DELTA-SKIN-BINDING** [major/FIXED] — §ISO/IEC 19775-1 26.3.1, 26.3.2, 26.3.3: Joint binding-frame and displacer payload edits invalidate cached skin bindings.
- **DELTA-SITE-POSE** [major/FIXED] — §ISO/IEC 19775-1 26.3.2, 26.3.3, 26.3.6: Motion of skeletal frames above joints publishes dependent skin-pose updates.
- **HAN-AUDIT-4** [minor/FIXED] — §26.3.2; ISO/IEC 19774-2 6.4: A Motion assigned to HAnimHumanoid.motions after load never played.
  - HAnimMotionSystem captured the motions list only at attach. It now re-syncs each tick when the [in,out] motions list changes, keeping the playback state of motions still referenced. Test: hanim_motion_test "HAnimHumanoid motions accepts a newly assigned Motion". (2026-09-27)
- **HANIM-DISP** [minor/FIXED] — §26.3.1, 26.3.5: Segment displacers deform the extracted Segment mesh; the authored Coordinate is unchanged.
  - ADR-0055 resolves the policy: displacement is applied to the mesh built for the Segment's coord, never written back into Coordinate.point, so no point_changed event is emitted (superseding the ADR-0032 mutation policy). The extractor passes the nearest enclosing HAnimSegment to the mesh builder. PickSystem builds meshes without a Segment and so picks against the undisplaced points. (2026-09-27)

