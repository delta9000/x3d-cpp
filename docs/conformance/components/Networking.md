# Networking — conformance

_Generated. Levels 2,3 · 3 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Anchor | 2 | ✓ | — | — | AUD-NET-3, GRP-ADDCHILDREN, IACC-4, META-EXTENDED-ATTRIBUTES-WRITE, NSN-11, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DUrlObject |
| Inline | 2 | ✓ | — | — | ASSET-LOADER-TRANSITIVE-POLICY, AUD-NET-1, AUD-NET-2, IMPORT-EXPORT-WIRE, JSON-STATEMENT-PLACEMENT, META-EXTENDED-ATTRIBUTES-WRITE, NATIVE-AUTHOR-FIELD-OWNERSHIP, NSN-12, PROTO-BODY-IMPORT-MODEL, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, ROUTE-IO-ALIAS, VRML-IMPORT-EXPORT-WRITE | X3DBoundedObject, X3DChildNode, X3DUrlObject |
| LoadSensor | 3 | ✓ | — | ? | AUD-NET-3, META-EXTENDED-ATTRIBUTES-WRITE, NSN-1, NSN-11, NSN-12, NSN-2, NSN-3, NSN-4, NSN-5, NSN-6, NSN-7, NSN-9, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-2** [critical/CLOSED `9bb71c2`] — §9.4.3: isActive (TRUE on load start; FALSE on all-done/timeout) not emitted.
- **NSN-3** [critical/CLOSED `9bb71c2`] — §9.4.3: isLoaded (TRUE when all children load; FALSE on any failure/timeout) not emitted.
- **NSN-4** [critical/CLOSED `9bb71c2`] — §9.4.3: loadTime (now, on successful completion only) not emitted.
- **NSN-5** [critical/CLOSED `9bb71c2`] — §9.4.3: progress events (advancing to 1.0 on full load) not emitted.
- **NSN-6** [critical/CLOSED `597e5a0`] — §9.4.3: timeOut deadline tracking (emit isLoaded=FALSE/isActive=FALSE on expiry) unimplemented.
- **NSN-9** [critical/CLOSED `9bb71c2`] — §9.4.3: Already-resolved children at scene-build must emit the immediate isLoaded/loadTime/progress burst.
- **ASSET-LOADER-TRANSITIVE-POLICY** [critical/FIXED] — §4.4.4.2; 9.2 (host I/O policy and source-URL integration): Nested Inline and EXTERNPROTO parsing preserves the supplied AssetResolver policy and declaration-source bases without local-file fallback.
  - asset_loader_policy_test.cpp, asset_proto_resolver_test.cpp: Paired synchronous loaders preserve resolver policy and source bases throughout recursive loading.
- **NATIVE-AUTHOR-FIELD-OWNERSHIP** [critical/FIXED] — §4.4.4; 9.2; 29.2 (native ownership and host integration): Scene and declaration owners replace process-global author-field storage and clone factory configuration.
  - scene_author_field_ownership_test.cpp: Fresh scenes own separate author stores while deliberate graph copies and retained children preserve their intended entry identities.
- **NSN-7** [major/CLOSED `632d8a2`] — §9.4.3: watched child url/load change must reset LoadSensor state and re-evaluate.
- **AUD-NET-1** [major/CLOSED] — §9.2.5: IMPORT resolves an Inline DEF that the Inline never EXPORTed.
  - wireInlineImports now requires an explicit child EXPORT whose public name matches importedDEF; a private DEF no longer becomes a parent alias. Regression: import_requires_explicit_export.
- **AUD-NET-2** [major/CLOSED] — §9.4.2: An Inline cannot load (or unload/replace) at runtime: load/url events after parse do nothing.
  - events_misc_test.cpp: Runtime Inline unload and URL replacement detach old content before loading new content.
- **REQ-FTP** [major/FIXED] — §B.6 Table B.4; F.6 Table F.4; 9.2.1: The optional curl backend supplies separate HTTP/HTTPS and FTP adapters; the CPU reference host routes these alongside confined local files.
  - interchange_test.cpp, interchange_test.cpp: The CPU reference host fetches FTP resources through its configured resolver.
- **JSON-STATEMENT-PLACEMENT** [major/FIXED] — §ISO/IEC 19776-5 Annex A: JSON statement wrappers in child arrays retain ROUTE, IMPORT, and EXPORT records.
  - ROUTE wrappers inside ProtoBody child arrays and nested body nodes belong to the enclosing prototype, while scene routes remain in the scene. Test: reader_audit_test.cpp, JSON statement wrappers in children retain their execution scope.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **VRML-IMPORT-EXPORT-WRITE** [major/FIXED] — §ISO/IEC 19776-2, A.2: Classic writer emits IMPORT and EXPORT statements with their effective aliases.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **NSN-1** [minor/CLOSED `9bb71c2`] — §9.4.3: LoadSensor not wired as an active System observing child URL-object load state per tick.
  - Closed by LoadSensorSystem (runtime/events/LoadSensorSystem.hpp): a time-driven System over the AssetResolver seam (ADR-0023, ADR-0046). Wired by attachStandardRuntime/attachFullRuntime. See docs/wiki/reference/subsystems/system-loadsensor.md. Drives NSN-2..9.
- **NSN-11** [minor/CLOSED] — §9.4.3, 9.4.1: Spec-literal Anchor children cases (b) replacement-world / (c) separate-window are not the SDK default; the headless default policy treats "#Name" as loaded iff a Viewpoint DEF exists and other Anchor urls as resolver load-request-acknowledged.
  - AnchorSystem now activates Anchors on click: '#Name' binds that viewpoint, any other url goes to the embedder's AnchorHandler with url + parameter, which implements the replacement-world / separate-window cases (events_misc_test).
- **NSN-12** [minor/CLOSED] — §9.4.3: LoadSensor's per-child readiness check does not recurse into a watched Inline's own nested sub-Inlines.
  - An Inline counts as loaded only when every nested sub-Inline it asks to load has loaded; an un-expanded Inline left in an expanded child's content makes that child Failed (LoadSensor ruling R8, events_misc_test).
- **IMPORT-EXPORT-WIRE** [minor/CLOSED] — §9.4.2; 4.4.6: IMPORT/EXPORT statements parse and round-trip but are never wired to routing — a ROUTE to an imported name hits the unresolved-endpoint drop.
  - core_diagnostics_test.cpp: Inline imports resolve explicit child exports into parent route aliases.
- **AUD-NET-3** [minor/CLOSED] — §9.4.3: LoadSensor counts an Anchor's target Viewpoint as loaded before it is bound.
  - LoadSensor now waits until the Anchor's #Viewpoint target is actually bound, and ignores Anchor.load as required by §9.4.1. Regression: events_misc_test checks unbound then bound target with load FALSE.
- **IACC-4** [minor/FIXED] — §9.4.1: A RuntimeSession host could not receive Anchor activations whose url is not an in-scene viewpoint.
  - AnchorSystem already bound "#Name" viewpoints and called an optional handler for other urls, but attachInteractive created the system without exposing it. SessionOptions::anchorHandler (and an attachInteractive parameter) now install the handler, which receives the url and parameter lists. The headless runtime still loads nothing itself. Test: interactive_profile_test Anchor case.
- **PROTO-BODY-IMPORT-MODEL** [minor/FIXED] — §ISO/IEC 19775-1 9.2.5: Prototype-local IMPORT records round-trip and body ROUTEs bind within each instance's Inline scope.
  - Deferred endpoint identities bind after Inline loading without publishing aliases in enclosing DEF tables. Explicit exports and exported PROTO interfaces are resolved with access/type checks; unload removes concrete edges, and reload rebinds them. Auxiliary ProtoBody Inline roots are included. Tests: proto_roundtrip_test.cpp and proto_import_runtime_test.cpp (two-instance isolation, bidirectional events, deferred loading, nested unload, and peer-root reload).

