# Networking — conformance

_Generated. Levels 2,3 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Anchor | 2 | ✓ | — | — | NSN-11 | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DUrlObject |
| Inline | 2 | ✓ | — | — | IMPORT-EXPORT-WIRE | X3DBoundedObject, X3DChildNode, X3DUrlObject |
| LoadSensor | 3 | ✓ | — | ◑ | NSN-1, NSN-11, NSN-2, NSN-3, NSN-4, NSN-5, NSN-6, NSN-7, NSN-9 | X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-11** [minor/DEFERRED] — §9.4.3, 9.4.1: Spec-literal Anchor children cases (b) replacement-world / (c) separate-window are not the SDK default; the headless default policy treats "#Name" as loaded iff a Viewpoint DEF exists and other Anchor urls as resolver load-request-acknowledged.
  - By design — the runtime is headless. Headed embedders express cases (b)/(c) via LoadSensorSystem::setChildLoadPolicy + a Pending-returning resolver (ADR-0046). See docs/wiki/subsystems/system-loadsensor.md.
- **IMPORT-EXPORT-WIRE** [minor/OPEN] — §9.4.2; 4.4.6: IMPORT/EXPORT statements parse and round-trip but are never wired to routing — a ROUTE to an imported name hits the unresolved-endpoint drop.
  - scene.imports/exports are consumed only by the codecs; no event/routing code references them. Inline DEF isolation (InlineExpand.hpp:176) is correct, but the spec's sanctioned cross-Inline escape hatch is inert. Fix: at scene-bridge build, resolve each Import{inlineDEF, importedDEF, AS} against the named Inline's exported DEF and register the alias before resolveRoutes. X_ITE supports this. (event-model review.)
- **NSN-2** [critical/CLOSED `9bb71c2`] — §9.4.3: isActive (TRUE on load start; FALSE on all-done/timeout) not emitted.
- **NSN-3** [critical/CLOSED `9bb71c2`] — §9.4.3: isLoaded (TRUE when all children load; FALSE on any failure/timeout) not emitted.
- **NSN-4** [critical/CLOSED `9bb71c2`] — §9.4.3: loadTime (now, on successful completion only) not emitted.
- **NSN-5** [critical/CLOSED `9bb71c2`] — §9.4.3: progress events (advancing to 1.0 on full load) not emitted.
- **NSN-6** [critical/CLOSED `597e5a0`] — §9.4.3: timeOut deadline tracking (emit isLoaded=FALSE/isActive=FALSE on expiry) unimplemented.
- **NSN-9** [critical/CLOSED `9bb71c2`] — §9.4.3: Already-resolved children at scene-build must emit the immediate isLoaded/loadTime/progress burst.
- **NSN-7** [major/CLOSED `632d8a2`] — §9.4.3: watched child url/load change must reset LoadSensor state and re-evaluate.
- **NSN-1** [minor/CLOSED `9bb71c2`] — §9.4.3: LoadSensor not wired as an active System observing child URL-object load state per tick.
  - Closed by LoadSensorSystem (runtime/events/LoadSensorSystem.hpp): a time-driven System over the AssetResolver seam (ADR-0023, ADR-0046). Wired by attachStandardRuntime/attachFullRuntime. See docs/wiki/subsystems/system-loadsensor.md. Drives NSN-2..9.

