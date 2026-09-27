---
title: "ADR-0053: Geospatial Coordinates Through a GeoProjection Seam With a First-Party Default"
summary: X3D Geospatial coordinates (GD, UTM, GC, WM on the 23 §25 ellipsoids, optional WGS84 geoid heights) are converted to earth-fixed geocentric metres and then into each node's GeoOrigin frame. The ellipsoid mathematics sits behind a GeoProjection seam whose default backend is first-party closed-form math (no dependencies, no IO); a PROJ backend is optional and doubles as the reference in a swap-test. Axis order, units, Web Mercator and the local frames are SDK-side and identical for every backend.
tags: [adr, geospatial, seam, math]
updated: 2026-09-27
related:
  - ../subsystems/geospatial.md
  - 0040-nurbs-tessellation-first-party.md
  - 0003-throw-on-range.md
---

# ADR-0053: Geospatial Coordinates Through a GeoProjection Seam With a First-Party Default

## Status

Accepted

## Context

Every Geospatial node (§25) authors coordinates in a spatial reference frame
named by its `geoSystem` field: geodetic (GD: latitude, longitude, elevation),
UTM (northing, easting, elevation), geocentric (GC, metres) or Web Mercator
(WM). The frame can name one of 23 ellipsoids, and the "WGS84" option makes
elevations relative to the WGS84 geoid rather than the ellipsoid. §25.2.3:
"Internally, an X3D browser will transform all geographic coordinates into
earth-fixed geocentric coordinates." A GeoOrigin then gives the world origin,
optionally rotated so the local up is +Y (§25.3.6).

Until now the only hook was `MeshBuildOptions::geoProjection` (B5), a
per-extraction callback for GeoElevationGrid that was empty by default, so the
grid fell back to a flat planar lattice. Nothing else converted: GeoCoordinate points and GeoViewpoint
positions were read as plain Cartesian vectors (so a latitude became metres),
GeoElevationGrid fell back to a flat planar grid, and the behavioural nodes
(GeoProximitySensor, GeoTouchSensor, GeoPositionInterpolator) were deferred on
this prerequisite (CONF-GEO, ENV-02, TSN-1, TSN-2).

Options considered:

1. **PROJ (or GDAL, which uses PROJ) as the only implementation.** Correct and
   battle-tested, and it would also supply geoid grids. But PROJ reads
   `proj.db` (SQLite) and grid files at run time, which the IO-free SDK core
   cannot do, and GDAL adds roughly a hundred transitive dependencies for
   transforms it delegates to PROJ anyway. The default build, the footprint
   gate and the MSVC cross-build would all carry it.
2. **First-party math only.** No dependency, but nothing independent checks it.
3. **A seam with a first-party default and PROJ as a second backend.** The
   repo's usual genericity proof (two backends plus a swap-test) then checks
   the first-party math against PROJ for free.

## Decision

Option 3.

**The seam** (`runtime/math/GeoProjection.hpp`) carries only ellipsoid
mathematics: geodetic ↔ geocentric, geodetic ↔ UTM (zone, hemisphere), and an
optional geoid undulation. Radians and metres; UTM values include the false
easting and southern false northing. Everything else is SDK-side
(`runtime/math/GeoFrame.hpp`, `runtime/scene/GeoNodes.hpp`) so all backends
share it: `geoSystem` parsing, axis order (`latitude_first`/`longitude_first`,
`northing_first`/`easting_first`), units, Web Mercator (EPSG:3857, a sphere of
radius a), the local east/up/south basis, the GeoOrigin frame and the node
field reads.

**The default backend** (`runtime/math/GeoBuiltinProjection.hpp`) is
first-party and header-only: the closed-form geodetic → geocentric formula,
an iterative inverse using the pole-stable height form, and Krüger's
transverse Mercator series to sixth order (Karney 2011) for UTM. It has no
geoid of its own but accepts a geoid function, so an application can supply a
grid (for example EGM2008 through tinygeoid) without writing a backend.
Reference values generated with PROJ 9.8 are pinned in
`geo_projection_test`: geocentric within 10 µm on four ellipsoids, UTM within
0.1 mm and inverse latitude/longitude within 1e-9°.

**PROJ** is an optional backend under `runtime/io/proj/` (flag-gated, like the
other IO backends), with a swap-test that runs the same conversions through
both. It also provides the geoid when the application points it at a grid.

**Interpretation choices:**

- **GD latitude/longitude are degrees.** §25.2.3 says "angle base units" and
  then "the following assumes an angle base unit of degrees", while X3D's
  default angle base unit is radians. Geospatial content and browsers use
  degrees (the corpus's GeoLocation/GeoOrigin/GeoViewpoint values, e.g.
  `41.5 -71.5 0`), and no geospatial scene in the corpus redefines the angle
  unit, so degrees it is.
- **No datum transformation.** §25.2.3 names ellipsoids only. A GD coordinate
  on Clarke 1866 is converted with that ellipsoid's geometry into the one
  earth-fixed frame; GC and WM are always WGS84.
- **The "WGS84" geoid option without a geoid model** keeps ellipsoidal
  heights. The geoSystem validator (GEOSYSTEM) already reports tokens; the
  geoid's absence is a backend capability, not a scene error.
- **The backend is process-wide** (`geo::projection()` / `geo::setProjection()`),
  defaulting to the first-party one. Transform, bounds and mesh code reach
  conversions through static paths that take no context, and the conversion
  is pure, so a per-context injection would thread a pointer through every
  one of them for no behavioural gain. An application that wants PROJ sets it
  once before building scenes; the setter is not synchronised.
- **Precision (§25.2.5).** Geocentric values are ~6.4e6 m, so conversions stay
  in double until a point is made relative to its GeoOrigin; only then is it
  narrowed to the float vectors the runtime uses. A node without a GeoOrigin
  lives in raw geocentric floats, as the spec defines, at about 0.5 m
  resolution.

## Consequences

- The B5 `MeshBuildOptions::geoProjection` / `GeoSystemDesc` callback is
  removed from the SDK façade. It covered only GeoElevationGrid, only inside
  mesh extraction, and had no default; the process-wide backend replaces it
  for every Geospatial consumer.

- The Geospatial nodes can now be wired: GeoCoordinate and GeoElevationGrid
  geometry and bounds, GeoLocation and GeoTransform as transforms, the
  GeoViewpoint camera frame, GeoPositionInterpolator, GeoProximitySensor,
  GeoTouchSensor and GeoLOD. Each uses `GeoNodes.hpp`, so they convert
  identically.
- Adding a frame the spec gains later touches the SDK side only if it is not
  ellipsoid mathematics; a new ellipsoid is one table row.
- The PROJ backend is optional, so the default build stays dependency-free;
  the swap-test is the check that the first-party math stays right.
