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
| `runtime/math/GeoFrame.hpp` | SDK-side conversions shared by every backend: authored coordinate ↔ geodetic ↔ geocentric (axis order, degrees, geoid heights, Web Mercator), the local east/up/south basis, `OriginFrame` (GeoOrigin, `rotateYUp`), `tangentFrame`; and the process-wide backend (`projection()` / `setProjection()`). |
| `runtime/scene/GeoNodes.hpp` | Node glue: `systemOf`, `originOf`, `toWorld` (one point or a list), `fromWorld`, `tangentFrameOf` — reads `geoSystem`, `geoOrigin`, `geoCoords`, `rotateYUp` by reflection. |
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
- **Geoid:** the `"WGS84"` option adds the backend's geoid undulation; the
  built-in backend has none unless the application supplies a function, in
  which case geoid-relative heights are converted to ellipsoidal heights.

## Status

The conversion core and optional PROJ backend ship. The `x3d_proj_geo_swap`
test compares their geocentric and UTM results; set `X3D_GEOID_GRID` to a
local GTX or GTG grid path to exercise geoid lookup. Node wiring (geometry,
transforms, camera, interpolator and sensors) remains tracked in
`docs/conformance/findings.yaml` (CONF-GEO, ENV-02, TSN-1, TSN-2).
