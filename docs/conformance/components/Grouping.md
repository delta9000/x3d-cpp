# Grouping — conformance

_Generated. Levels 1,2,3 · 4 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Group | 1 | ✓ | — | — | GRP-ADDCHILDREN, JSON-STATEMENT-PLACEMENT, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| StaticGroup | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| Switch | 2 | ✓ | — | — | AUD-LGT-2, AUD-PDS-3, GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SENSOR-SWITCH, SW-DELTA-1 | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| Transform | 1 | ✓ | — | — | EXTRACT-VISIBILITY-HINT-DELTA, GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, READ-NUMERIC-LEXEMES, REQ-UNIT, ROUTE-IO-ALIAS, VALIDATE-ROUTES, VALIDATE-VALUE-SYNTAX | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **SW-DELTA-1** [major/FIXED] — §10.3.10: Switch.whichChoice change was invisible to the incremental delta() channel — a runtime change of whichChoice swapped the active child for full-snapshot consumers (cpuraster) but NOT for incremental ones (the OpenGL PoC), which stayed on the old child.
  - scene_extractor_incremental_test.cpp: Switch selection changes replace the extracted child set.
- **SENSOR-SWITCH** [major/CLOSED] — §22.4, 22.4.3: Environmental sensors in non-selected Switch children / inactive LOD levels are still ticked (active) instead of treated as removed from the transformation hierarchy.
  - view_dependent_test.cpp: Sensors under unselected Switch and LOD branches remain inactive.
- **READ-NUMERIC-LEXEMES** [major/FIXED] — §ISO/IEC 19776-1 5.5, 5.6, 5.8; ISO/IEC 19776-2 5.5, 5.6, 5.8, A.4: Numeric field readers accept leading plus signs and signed hexadecimal integers.
  - FieldValueIO accepts optional leading plus signs for floats, doubles, and integers, and 0x/0X integer prefixes with either sign. XML and Classic reader tests in codec_conformance_test.cpp cover scalar and vector fields, MFInt32, and signed integer boundaries. Malformed and out-of-range values retain the zero fallback. ISO/IEC 19776-5 5.8 specifies decimal JSON integers; JSON numeric tokenization is unchanged.
- **GRP-ADDCHILDREN** [major/FIXED] — §10.2.1: addChildren / removeChildren events were accepted but never changed children.
  - cascade_alias_audit_test.cpp: Runtime grouping handlers apply addChildren and removeChildren events.
- **JSON-STATEMENT-PLACEMENT** [major/FIXED] — §ISO/IEC 19776-5 Annex A: JSON statement wrappers in child arrays retain ROUTE, IMPORT, and EXPORT records.
  - ROUTE wrappers inside ProtoBody child arrays and nested body nodes belong to the enclosing prototype, while scene routes remain in the scene. Test: reader_audit_test.cpp, JSON statement wrappers in children retain their execution scope.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **EXTRACT-VISIBILITY-HINT-DELTA** [minor/FIXED]: Optional RenderItem origin-distance visibility hints stayed stale after view, limit and placement changes.
  - scene_extractor_visibility_delta_test.cpp: Changed per-placement visibility hints use remove-before-add replacement while unchanged hints preserve meshes.

