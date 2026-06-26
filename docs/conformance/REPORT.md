# X3D Structural Conformance Report

Corpus: `$X3D_CORPUS_DIR` — 16866 `.x3d` files found.

**Validated 16717 files against their own-version oracle (Vc := Vd): 16660 structurally clean (99.66%).** Skipped (no manifest / unversioned): 133; parse errors: 16.

## Per-version scorecard

| Version | Validated | Clean | Clean % | UOM manifest hash | Nodes |
|--------:|----------:|------:|--------:|:------------------|------:|
| 3.0 | 2418 | 2381 | 98.47% | `336a5e2a31e5` | 189 |
| 3.1 | 11481 | 11478 | 99.97% | `e88789ef0233` | 222 |
| 3.2 | 868 | 867 | 99.88% | `85bfa5318b44` | 298 |
| 3.3 | 1389 | 1382 | 99.5% | `7427c924eed0` | 320 |
| 4.0 | 258 | 250 | 96.9% | `c3ce06dc5df4` | 338 |
| 4.1 | 303 | 302 | 99.67% | `5c380d6dcade` | 346 |

## Findings by code (residual)

| Code | Total | Files |
|:-----|------:|------:|
| FIELD_UNKNOWN_FOR_NODE | 95 | 12 |
| NODE_UNKNOWN_FOR_VERSION | 1715 | 34 |
| ROUTE_ACCESS_ILLEGAL | 23 | 9 |
| ROUTE_ENDPOINT_UNKNOWN | 20 | 8 |

## Top unknown nodes (extension / version-skew / malformed)

- `XvlFace` × 1413
- `XvlFaces` × 79
- `XvlShell` × 62
- `XvlShape` × 51
- `XvlEdge` × 45
- `XvlEdges` × 7
- `Tangent` × 5
- `ComposedCubeMapTexture` × 4
- `ShaderPart` × 4
- `XvlVertex` × 4
- `XvlVertices` × 3
- `SpinGroupTag` × 3
- `FitToBoxTransform` × 2
- `SmoothSurface` × 2
- `ComposedShader` × 2

## Cited UOM-errata corrections applied

- **3.0** `Viewpoint.orientation.accessType` initializeOnly → inputOutput (corrected upstream in 3.1) — X3D 3.0 UOM erratum: X3DViewpointNode.orientation declared initializeOnly but is inputOutput — the field is routable (animated viewpoints) and carries set_orientation/orientation_changed event aliases.
- **3.0** `GeoViewpoint.orientation.accessType` initializeOnly → inputOutput (corrected upstream in 3.1) — X3D 3.0 UOM erratum: X3DViewpointNode.orientation declared initializeOnly but is inputOutput — the field is routable (animated viewpoints) and carries set_orientation/orientation_changed event aliases.
