# Networking — conformance

_Generated. Levels 2,3 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Anchor | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DUrlObject |
| Inline | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DUrlObject |
| LoadSensor | 3 | ✓ | — | ✗ | NSN-1, NSN-2, NSN-3, NSN-4, NSN-5, NSN-6, NSN-7, NSN-9 | X3DChildNode, X3DNetworkSensorNode, X3DSensorNode |

## Findings

- **NSN-2** [critical/DEFERRED] — §9.4.3: isActive (TRUE on load start; FALSE on all-done/timeout) not emitted.
- **NSN-3** [critical/DEFERRED] — §9.4.3: isLoaded (TRUE when all children load; FALSE on any failure/timeout) not emitted.
- **NSN-4** [critical/DEFERRED] — §9.4.3: loadTime (now, on successful completion only) not emitted.
- **NSN-5** [critical/DEFERRED] — §9.4.3: progress events (advancing to 1.0 on full load) not emitted.
- **NSN-6** [critical/DEFERRED] — §9.4.3: timeOut deadline tracking (emit isLoaded=FALSE/isActive=FALSE on expiry) unimplemented.
- **NSN-9** [critical/DEFERRED] — §9.4.3: Already-resolved children at scene-build must emit the immediate isLoaded/loadTime/progress burst.
- **NSN-7** [major/DEFERRED] — §9.4.3: watched child url/load change must reset LoadSensor state and re-evaluate.
- **NSN-1** [minor/DEFERRED] — §9.4.3: LoadSensor not wired as an active System observing child URL-object load state per tick.
  - Unblocked by the asset-resolver/IO seam (shipped 2026-06-23; see ADR-0023 + docs/wiki/seam-status.md). Drives NSN-2..9.

