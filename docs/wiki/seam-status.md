---
title: Seam matrix
summary: Interfaces, available implementations, and contract tests.
tags: [seam, reference]
updated: 2026-11-05
related: []
---

# Seam matrix

This matrix lists implementations and tests in the source tree. Optional tests
require their backend flags and `X3D_CPP_BUILD_TESTS`. Contract and parity tests
cover their fixtures, not every X3D obligation. Conformance status belongs in
`docs/conformance/findings.yaml`; subsystem pages own behavior details.

| Seam | Interface policy | Implementations | Contract or comparison tests |
|---|---|---|---|
| ScriptEngine | Stable abstract backend contract | Duktape; optional QuickJS | `x3d_quickjs_swap` |
| AssetResolver | Stable callback | Confined local files; optional curl HTTP/FTP | `x3d_fileresolver_test`, `x3d_assetresolver_swap` |
| PhysicsBackend | Experimental | Optional Jolt | `x3d_physics_jolt`, `x3d_physics_system` |
| AudioBackend | Stable abstract backend contract | BuiltinDsp | `x3d_sound_system`, `x3d_sound_immersive` and focused DSP tests |
| FontMetrics | Stable callback | Monospace stub; optional stb_truetype metrics and atlas | `x3d_text_tests`, `x3d_text_atlas_tests` |
| TextureResolver | Stable callback | Optional stb_image and Wuffs | `x3d_texture_tests` |
| GeoProjection | Frozen mathematical interface; context-owned | Built-in ellipsoid conversion; optional PROJ | `x3d_math_tests`, `x3d_proj_geo_swap` |
| MovieDecoder | Stable callback | Optional pl_mpeg MPEG-1 | `x3d_movie_tests` |
| RenderDelta | Experimental in-process feed | CPU, OpenGL, SVG, and native-host examples | `x3d_native_host_contract` and example suites |

## ScriptEngine

The differential suite runs authored scripts through Duktape and QuickJS and
compares observable traces. It does not establish every scripting or SAI
requirement. See [ADR-0022](decisions/0022-scriptengine-second-backend-swap-test.md)
and [Script runtime](subsystems/system-script-sai.md).

## AssetResolver

File and curl resolvers return bytes through one callback. The curl contract
test uses an in-process HTTP server. See
[ADR-0023](decisions/0023-assetresolver-second-backend-swap-test.md) and
[Asset IO](subsystems/system-asset-io.md).

## PhysicsBackend

Jolt is enabled with `X3D_CPP_BUILD_PHYSICS`. The core interface and optional
backend are described in [ADR-0019](decisions/0019-physics-seam.md) and
[Physics](subsystems/physics.md).

## AudioBackend

BuiltinDsp supplies the dependency-free implementation. Sound tests check graph
behavior and samples. See [ADR-0020](decisions/0020-sound-seam.md), the
[backend amendment](decisions/0026-audiobackend-second-backend-swap-test.md),
and [Sound](subsystems/sound.md).

## FontMetrics

The default stub supplies advances; stb_truetype supplies metrics and a glyph
atlas when configured. See
[ADR-0025](decisions/0025-fontmetrics-second-backend-swap-test.md) and
[Font metrics](subsystems/system-font-metrics.md).

## TextureResolver

The comparison suite checks decoded pixels from stb_image and Wuffs on shared
fixtures. Format routing and lifecycle are described in
[ADR-0024](decisions/0024-textureresolver-second-backend-swap-test.md) and
[Texture decode](subsystems/system-texture-decode.md).

## GeoProjection

PROJ tests compare numeric conversions with explicit tolerances. Each context
retains one immutable projection. See
[ADR-0053](decisions/0053-geo-projection-seam.md),
[ADR-0058](decisions/0058-context-owned-geo-projection.md), and
[Geospatial](subsystems/geospatial.md).

## MovieDecoder

The pl_mpeg tests check decoder lifecycle and resolver composition. See
[ADR-0041](decisions/0041-moviedecoder-seam-royalty-free-defaults.md).

## RenderDelta

The native-host example checks a CPU mirror of the in-process feed, including
an installed and relocated consumer. It does not validate a Vulkan or CAVEOS
renderer. See [ADR-0015](decisions/0015-extraction-pull-per-path.md) and
[Native host integration](guides/native-host.md).
