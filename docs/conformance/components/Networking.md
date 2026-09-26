# Networking — conformance

_Generated. Levels 2,3 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Anchor | 2 | ✓ | — | — | NSN-11 | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DUrlObject |
| Inline | 2 | ✓ | — | — | IMPORT-EXPORT-WIRE, NSN-12 | X3DBoundedObject, X3DChildNode, X3DUrlObject |
| LoadSensor | 3 | ✓ | — | ✓ | NSN-1, NSN-11, NSN-12, NSN-2, NSN-3, NSN-4, NSN-5, NSN-6, NSN-7, NSN-9 | X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-2** [critical/CLOSED `9bb71c2`] — §9.4.3: isActive (TRUE on load start; FALSE on all-done/timeout) not emitted.
- **NSN-3** [critical/CLOSED `9bb71c2`] — §9.4.3: isLoaded (TRUE when all children load; FALSE on any failure/timeout) not emitted.
- **NSN-4** [critical/CLOSED `9bb71c2`] — §9.4.3: loadTime (now, on successful completion only) not emitted.
- **NSN-5** [critical/CLOSED `9bb71c2`] — §9.4.3: progress events (advancing to 1.0 on full load) not emitted.
- **NSN-6** [critical/CLOSED `597e5a0`] — §9.4.3: timeOut deadline tracking (emit isLoaded=FALSE/isActive=FALSE on expiry) unimplemented.
- **NSN-9** [critical/CLOSED `9bb71c2`] — §9.4.3: Already-resolved children at scene-build must emit the immediate isLoaded/loadTime/progress burst.
- **NSN-7** [major/CLOSED `632d8a2`] — §9.4.3: watched child url/load change must reset LoadSensor state and re-evaluate.
- **NSN-1** [minor/CLOSED `9bb71c2`] — §9.4.3: LoadSensor not wired as an active System observing child URL-object load state per tick.
  - Closed by LoadSensorSystem (runtime/events/LoadSensorSystem.hpp): a time-driven System over the AssetResolver seam (ADR-0023, ADR-0046). Wired by attachStandardRuntime/attachFullRuntime. See docs/wiki/subsystems/system-loadsensor.md. Drives NSN-2..9.
- **NSN-11** [minor/CLOSED] — §9.4.3, 9.4.1: Spec-literal Anchor children cases (b) replacement-world / (c) separate-window are not the SDK default; the headless default policy treats "#Name" as loaded iff a Viewpoint DEF exists and other Anchor urls as resolver load-request-acknowledged.
  - AnchorSystem now activates Anchors on click: '#Name' binds that viewpoint, any other url goes to the embedder's AnchorHandler with url + parameter, which implements the replacement-world / separate-window cases (events_misc_test).
- **NSN-12** [minor/CLOSED] — §9.4.3: LoadSensor's per-child readiness check does not recurse into a watched Inline's own nested sub-Inlines.
  - An Inline counts as loaded only when every nested sub-Inline it asks to load has loaded; an un-expanded Inline left in an expanded child's content makes that child Failed (LoadSensor ruling R8, events_misc_test).
- **IMPORT-EXPORT-WIRE** [minor/CLOSED] — §9.4.2; 4.4.6: IMPORT/EXPORT statements parse and round-trip but are never wired to routing — a ROUTE to an imported name hits the unresolved-endpoint drop.
  - Closed by wireInlineImports (runtime/InlineExpand.hpp), called from parseDocument after expandInlines: each Import{inlineDEF, importedDEF, AS} resolves the imported name against the named Inline's retained child scene (its <EXPORT AS> alias first, else a child DEF) and registers the local alias in scene.defs before re-running resolveRoutes, so a ROUTE to/from the AS name binds. Regression: x3d_parse_tests core_diagnostics_test (import_export_wire_route_to_imported_as_name).

