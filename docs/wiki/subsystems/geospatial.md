---
title: Geospatial
summary: §25 Geospatial coordinates — GD/UTM/GC/WM on the 23 §25 ellipsoids, the WGS84 geoid option, and GeoOrigin frames — converted through the GeoProjection seam (first-party default backend, optional PROJ backend) into the X3D world.
tags: [subsystem, geospatial, seam, math]
updated: 2026-09-27
related:
  - ../decisions/0053-geo-projection-seam.md
  - ../architecture.md
---

# Geospatial

## Purpose

X3D Geospatial nodes (§25) author positions in a spatial reference frame named
by their `geoSystem` field rather than in X3D Cartesian space. This subsystem
converts those coordinates into earth-fixed geocentric metres and then into the
X3D world of the node — relative to its GeoOrigin, if it has one — so that
geometry, transforms, the camera and sensors all see ordinary local
coordinates. The design is [ADR-0053](../decisions/0053-geo-projection-seam.md).

## Key files

| File | Role |
|---|---|
| `runtime/math/GeoProjection.hpp` | The seam: `Ellipsoid` + the §25.2.3 Table 25.3 ellipsoid codes, `GeoSystem` + `parseGeoSystem` (frame, ellipsoid, `WGS84` geoid option, axis order, UTM zone/hemisphere), and the `GeoProjection` backend interface (geodetic ↔ geocentric, geodetic ↔ UTM, optional geoid undulation). |
| `runtime/math/GeoBuiltinProjection.hpp` | The default backend: closed-form geodetic → geocentric, iterative inverse, Krüger 6th-order UTM (Karney 2011). No dependencies, no IO; accepts an optional geoid function. |
| `runtime/io/proj/ProjGeoProjection.{hpp,cpp}` | Optional PROJ backend (`X3D_CPP_BUILD_PROJ`): ellipsoid cart/UTM conversions and a configured vertical geoid grid. |
| `runtime/io/proj/tests/proj_geo_swap_test.cpp` | Grid swap-test against the built-in backend on six ellipsoids, including polar and antimeridian cases; checked-in small geoid fixture and optional external grid case. |
| `runtime/math/GeoFrame.hpp` | SDK-side conversions shared by every backend: authored coordinate ↔ geodetic ↔ geocentric (axis order, degrees, geoid heights, Web Mercator), the local east/up/south basis, `OriginFrame` (GeoOrigin, `rotateYUp`), `tangentFrame`; and the immutable built-in default. Every shared conversion helper requires an explicit projection reference. |
| `runtime/scene/GeoNodes.hpp` | Node glue: `systemOf`, `originOf`, `toWorld` (one point or a list), `fromWorld`, `tangentFrameOf` — reads `geoSystem`, `geoOrigin`, `geoCoords`, `rotateYUp` by reflection. |
| `runtime/scene/GeometryBounds.hpp` and `runtime/extract/MeshBuilder.cpp` | Convert GeoCoordinate lists and GeoElevationGrid lattices through the node helpers for bounds and meshes; grid heights are absolute elevations, so `geoGridOrigin.z` is not added. |
| `runtime/scene/TransformSystem.hpp` | GeoLocation tangent placement and GeoTransform tangent-frame TRS, shared by scene walks. |
| `runtime/events/X3DExecutionContext.hpp` and `runtime/events/NavigationSystem.hpp` | Geographic GeoViewpoint camera pose, center of rotation (including LOOKAT updates), and elevation-based speed. |
| `runtime/events/GeoPositionInterpolatorSystem.hpp` | Interpolates in the authored geoSystem, then projects the result to world coordinates. |
| `runtime/scene/ViewDependentSystem.hpp` and `runtime/events/InlineRuntimeSystem.hpp` | GeoProximitySensor's tangent box and geographic viewer output (in the sensor's local geo frame), GeoViewpoint centre-of-rotation output; GeoLOD range selection, URL tile loading, and children events. |
| `runtime/events/PointingSensorSystem.hpp` | GeoTouchSensor pick events and geographic hit coordinates. |
| `runtime/io/tinygeoid/TinygeoidGeoid.hpp` | Optional WGS84 geoid for the built-in backend from a tinygeoid `.tng` grid (vendored tinygeoid, MIT): `makeGeoidFunction(grid)`, or `makeBuiltinWithGeoid(path)` to set in `SessionOptions::geoProjection`. Lives under `runtime/io/` because loading reads a file; the application supplies the grid (e.g. EGM2008 2.5′ converted with tinygeoid's `tng_pack`). |
| `runtime/math/tests/geo_projection_test.cpp` | Reference values from PROJ 9.8 on WGS84, Clarke 1866, Airy and International ellipsoids; UTM north/south/zone edges; Web Mercator; parsing; the geoid hook; GeoOrigin frames; node glue. |

## Conventions

- **Units:** GD latitude and longitude in degrees (see the ADR for the
  "angle base units" wording), everything else in metres.
- **Axis order:** GD defaults to latitude first (`longitude_first` swaps), UTM
  to northing first (`easting_first` swaps).
- **Local frames:** +X east, +Y up (the ellipsoid normal), −Z north (§25.3.3).
- **GeoOrigin:** world = geocentric − origin, rotated so the origin's up is +Y
  when `rotateYUp` is TRUE. Without a GeoOrigin the world is geocentric.
- **Precision:** double until the point is relative to its GeoOrigin, then
  float.
- **No datum shifts:** each ellipsoid's geometry maps into the one earth-fixed
  frame; GC and WM are WGS84.
- **Geoid:** the `"WGS84"` option adds the backend's geoid undulation. The
  built-in backend has none by itself; `runtime/io/tinygeoid` supplies one from
  a tinygeoid `.tng` grid, and the PROJ backend from a PROJ grid. Without
  either, heights stay ellipsoidal.

## Status

The conversion core ships and is tested against PROJ, and every Geospatial
node converts through it:

- **Geometry and bounds:** GeoCoordinate points; GeoElevationGrid lattices
  (east/longitude columns, north/latitude rows, absolute `height × yScale`,
  authored normals rotated from the tangent frame). GeoMetadata.data and
  metadata references are not rendered.
- **Transforms:** GeoLocation (tangent frame at `geoCoords`) and GeoTransform
  (TRS inside the `geoCenter` tangent frame).
- **Camera:** GeoViewpoint position, orientation relative to the tangent frame,
  geographic centre of rotation (including LOOKAT), and navigation speed
  `elevation / 10 × speedFactor`. Avatar size and a finite visibility limit are
  multiplied by `max(1, elevation / 10)` (never below the authored values); an
  unlimited visibility limit stays unlimited (ADR-0054).
- **Behaviour:** GeoPositionInterpolator interpolates in its geoSystem;
  GeoProximitySensor (tangent box at `geoCenter`, `geoCoord_changed`);
  GeoTouchSensor (`hitGeoCoord_changed`); GeoLOD loads rootUrl when rootNode is
  empty, waits for all specified child URLs before switching, and unloads
  children outside range. The children output and extractor use the displayed set.
- **Optional PROJ backend** (`-DX3D_CPP_BUILD_PROJ=ON`) with the swap-test
  `x3d_proj_geo_swap`: worst differences against the built-in backend are
  0 m geocentric, 3 nm UTM, 3e-11° latitude and 3.7 µm height (PROJ's
  non-iterated inverse).

Set `X3D_GEOID_GRID` to a local GTX or GTG grid to exercise the PROJ backend's
geoid lookup in the swap-test; without it that case is skipped.


## World ownership and migration

[ADR-0058](../decisions/0058-context-owned-geo-projection.md) makes projection
selection immutable for each execution context. Set `SessionOptions::geoProjection`
before `RuntimeSession::create`, or construct a low-level context with
`X3DExecutionContext(projection)`. Null selects the shared
immutable built-in backend. The world retains the supplied backend after caller
references and option objects disappear.

All world conversions use `ctx.geoProjection()`: transforms, geometry and bounds,
picking/collision, camera/navigation, sensors, geospatial interpolation, delayed
Inline/GeoLOD enrollment, lighting/fog and HAnim ancestor transforms. The extractor
has no separate projection override. No runtime replacement is permitted, so
caches cannot silently mix projections. Shared helper functions now require the
projection explicitly; standalone calls may pass `geo::builtinProjection()`.

This is a source/layout migration; rebuild concrete runtime consumers. The
GeoProjection mathematical virtual interface is unchanged. Independent worlds
may run on separate owner threads; operations within one mutable world remain
serialized. A shared custom backend must provide stable conversion behavior and
synchronize its own mutable caches. Sharing mutable native node graphs between
active worlds is not an isolation guarantee. This ownership proof does not add
Geospatial capabilities to the portable four-node SAI provider.
