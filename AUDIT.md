# §25 Geospatial audit

Follow-up (2026-09-27): GEO-AUDIT-1 through GEO-AUDIT-6 were fixed after this audit. The evidence below records the pre-fix failures; the renamed regression tests now pass. GEO-AUDIT-7 remains the deliberate ADR-0053 fallback and is tracked as open finding GEO-GEOID-DEFAULT.

Scope: X3D 4.0 §25's eleven nodes, the projection/frame code, and the related §22/§23 behavior. I compared the implementation with the [Web3D §25 text](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html), [§23 Navigation](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html), [§22 Environmental sensors](https://www.web3d.org/specifications/ISO-IEC19775-1v4-WD2.Web3D.pdf), ADR-0053, and `docs/conformance/findings.yaml`. The provided 2023 IS §25 URL timed out through the available web reader; the linked 2022 DIS is the available §25 source. Spec excerpts below are deliberately short; each clause link gives the complete sentence and context.

The added `audit ...` tests are intentionally failing repros. Configuration and full build succeeded in `/tmp/x3d-geo-audit-build` after the workspace volume ran out of space. `ctest --test-dir /tmp/x3d-geo-audit-build --output-on-failure -j5` ran 55 tests: 51 passed and the four suites containing these repros failed. `ctest --preset dev` found no tests because its workspace `build` directory had to be moved to `/tmp`.

## Gaps

### GEO-AUDIT-1 — GeoElevationGrid adds the grid origin's elevation to absolute heights

- **Clause:** [§25.3.2 GeoElevationGrid](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html#GeoElevationGrid). Exact excerpt: “represent elevation above the ellipsoid or the geoid, as appropriate.” The clause says `geoGridOrigin` gives the geographic southwest corner and `height` values give elevations above the ellipsoid/geoid. Thus a height of 10 is already 10 metres above the reference surface, even when the corner's third coordinate is 100.
- **Code:** [`GeoNodes.hpp:93`](runtime/scene/GeoNodes.hpp#L93) adds `o.z + elevation` in every frame; [`MeshBuilder.cpp:1455`](runtime/extract/MeshBuilder.cpp#L1455) passes each scaled height to it, and [`GeometryBounds.hpp:385`](runtime/scene/GeometryBounds.hpp#L385) repeats it for bounds.
- **Evidence:** PROVEN, `GeoElevationGrid heights are absolute elevations regardless of geoGridOrigin altitude`: `CHECK( 110 == Approx( 10 ) )` fails for `geoGridOrigin=(0,0,100)` and four heights of 10.
- **Severity/confidence:** major / medium. The prose does not explicitly say to discard the corner elevation, but describing `height` itself as elevation above the reference surface rules out adding 100 to it.

### GEO-AUDIT-2 — GeoMetadata.data renders referenced nodes

- **Clause:** [§25.3.5 GeoMetadata](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-WD3/Part01/components/geospatial.html#GeoMetadata). Exact excerpt: “The nodes in the data field are not rendered”.
- **Code:** [`SceneExtractor.hpp:778`](runtime/extract/SceneExtractor.hpp#L778) descends every readable node-valued field, including `GeoMetadata.data`, and [`SceneExtractor.hpp:787`](runtime/extract/SceneExtractor.hpp#L787) walks its Shape as visible content.
- **Evidence:** PROVEN, `GeoMetadata data references do not create render items`: `CHECK( ex.fullSnapshot().added.empty() )` fails (`false`) when the only Shape is referenced by `data`.
- **Severity/confidence:** major / high.

### GEO-AUDIT-3 — valid geoSystem tokens receive conformance warnings (closed GEOSYSTEM incomplete)

- **Clause:** [§25.2.3–25.2.4](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html#Specifyingaspatialreferenceframe). Exact excerpt: “Optional arguments may appear in any order.” §25.2.4 explicitly permits `longitude_first`; Table 25.3 includes `AA` (Airy 1830). The matching `latitude_first`, `northing_first`, and `easting_first` tokens are also specified.
- **Code:** [`GeoProjection.hpp:92`](runtime/math/GeoProjection.hpp#L92) parses the ordering tokens and [`GeoProjection.hpp:49`](runtime/math/GeoProjection.hpp#L49) contains `AA`, but [`X3DRangeValidate.hpp:78`](runtime/X3DRangeValidate.hpp#L78) omits `AA` and [`X3DRangeValidate.hpp:98`](runtime/X3DRangeValidate.hpp#L98) treats all ordering tokens as unsupported. The validator is also restricted to four of the geoSystem-bearing nodes at [`X3DRangeValidate.hpp:67`](runtime/X3DRangeValidate.hpp#L67). This is an incomplete fix for the **closed `GEOSYSTEM` finding**.
- **Evidence:** PROVEN, `geoSystem validation accepts spec tokens on every geospatial node`: `CHECK( collectRangeWarnings(*geo).empty() )` fails for `{"GD","AA","longitude_first"}`.
- **Severity/confidence:** minor / high. Correct geometry can still be projected, but conforming scenes receive false diagnostics; invalid tokens on the other geospatial nodes get no corresponding diagnostic.

### GEO-AUDIT-4 — GeoViewpoint LOOKAT does not set its geographic centerOfRotation

- **Clause:** [§23.4.4 NavigationInfo](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#NavigationInfo) and [§25.3.11 GeoViewpoint](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html#GeoViewpoint). Exact excerpt from §23: “Sets the center of rotation in the currently bound Viewpoint node to the approximate centre of the selected object.” GeoViewpoint's `centerOfRotation` is `SFVec3d` in geographic coordinates.
- **Code:** [`NavigationSystem.hpp:584`](runtime/events/NavigationSystem.hpp#L584) writes a Cartesian `SFVec3f` to the `SFVec3d` field and discards the failed write result. It also does not convert the picked point to the GeoViewpoint's `geoSystem`/`geoOrigin`. This is an incomplete GeoViewpoint integration following the **closed `GEO-1` finding**, whose note specifically claims `centerOfRotation` conversion.
- **Evidence:** PROVEN, `GeoViewpoint LOOKAT updates geographic centerOfRotation`: after a successful box click, `CHECK( 2 == Approx( 0 ) )` fails; the initial geographic pivot remains unchanged.
- **Severity/confidence:** major / high. The camera transition still runs, but subsequent EXAMINE uses the stale pivot.

### GEO-AUDIT-5 — GeoProximitySensor crashes while reporting a GeoViewpoint pivot

- **Clause:** [§25.3.8 GeoProximitySensor](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html#GeoProximitySensor) declares `centerOfRotation_changed`; [§22.4.1 ProximitySensor](https://www.web3d.org/specifications/ISO-IEC19775-1v4-WD2.Web3D.pdf) defines its LOOKAT behavior. Exact excerpt from §22: “centerOfRotation_changed events are only generated when the currently bound NavigationInfo node includes LOOKAT navigation.”
- **Code:** [`ViewDependentSystem.hpp:452`](runtime/scene/ViewDependentSystem.hpp#L452) requests `SFVec3f` from a GeoViewpoint `centerOfRotation` field stored as `SFVec3d`. In this debug build, [`GeometryBounds.hpp:56`](runtime/scene/GeometryBounds.hpp#L56) asserts on that mismatch. A release build would take the fallback zero pivot instead of the authored geographic position.
- **Evidence:** PROVEN, `GeoProximitySensor reports GeoViewpoint geographic centerOfRotation`: `SIGABRT`, `getField: field present but stored type != requested type`, while the viewer is inside the box with LOOKAT bound.
- **Severity/confidence:** critical / high in debug; major / high in release.

### GEO-AUDIT-6 — GeoProximitySensor geographic position loses an ancestor Transform

- **Clause:** [§25.3.8 GeoProximitySensor](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-DIS/Part01/components/geospatial.html#GeoProximitySensor). Exact excerpt: “the geospatial coordinates of the viewer's position”. The output is the viewer's geographic position, not the parent-local origin of a moved sensor box.
- **Code:** [`ViewDependentSystem.hpp:436`](runtime/scene/ViewDependentSystem.hpp#L436) applies the inverse parent matrix to the viewer's world position before [`GeoNodes.hpp:113`](runtime/scene/GeoNodes.hpp#L113) interprets it in the sensor's GeoOrigin frame.
- **Evidence:** PROVEN, `GeoProximitySensor geoCoord preserves ancestor Transform placement`: `CHECK( 0 == Approx( 5 ) )` after round-tripping the reported coordinate to the sensor's world frame. The viewer is 5 metres east and inside a sensor translated 5 metres east.
- **Severity/confidence:** major / medium. The clause calls the output geographic; the related `position_changed` output is sensor-local, so the two prose descriptions leave some room for interpretation of their relationship under a parent Transform.

### GEO-AUDIT-7 — default WGS84 geoid mode silently uses ellipsoid heights

- **Clause:** [§25.2.3](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4-WD3/Part01/components/geospatial.html#Specifyingaspatialreferenceframe). Exact excerpt: “all elevations will be relative to the WGS84 geoid”.
- **Code:** [`GeoBuiltinProjection.hpp:14`](runtime/math/GeoBuiltinProjection.hpp#L14) documents the missing default geoid; [`GeoFrame.hpp:75`](runtime/math/GeoFrame.hpp#L75) leaves elevation unchanged when `geoidUndulation` returns false. This converts a geoid-relative height as an ellipsoidal height, without signaling an unavailable conversion. ADR-0053 explicitly accepts that fallback.
- **Evidence:** **UNPROVEN** against an independent geoid height. The existing `geo: the WGS84 geoid option adds the undulation when a model is present` test confirms the default returns unchanged height 100, but this audit did not pin an authoritative WGS84 geoid undulation at a coordinate to make a numeric failing repro.
- **Severity/confidence:** major / medium. A supplied geoid callback or optional PROJ backend can provide correct values; the shipped default cannot.

## Coverage and conformant paths checked

- **GeoCoordinate:** `point` conversion, `geoSystem` axis order, GeoOrigin and geometry/bounds paths agree in the existing tests; see GEO-AUDIT-3 for diagnostics.
- **GeoElevationGrid:** row/column spacing, `yScale`, generated texture coordinates, authored colors/normals, `colorPerVertex`/`normalPerVertex`, `ccw`, crease smoothing and bounds pass through the shared height-grid path; see GEO-AUDIT-1 for elevation.
- **GeoLOD:** geographic center/range and `children`/`level_changed` selection reviewed; URL tile loading is already OPEN as GEOLOD-1 and is omitted here.
- **GeoLocation:** local east/up/south placement and dynamic `geoCoords` update are covered by existing navigation tests; nested inside GeoTransform remains sensitive to parent transforms, but the spec's absolute-location warning does not justify a separate finding without a decisive case.
- **GeoMetadata:** `url`, `summary`, and `data` fields are stored and readable; the rendered `data` reference is GEO-AUDIT-2. The spec calls `url` a link to external metadata, not a required scene-graph loader.
- **GeoOrigin:** own `geoSystem`, `geoCoords`, and `rotateYUp` enter `OriginFrame`; no-origin conversion stays geocentric. The spec discourages mixing distinct GeoOrigins, so I did not classify the resulting composition ambiguity as a gap.
- **GeoPositionInterpolator:** key-span clamping and interpolation occur in authored `geoSystem` coordinates before `value_changed` projection; both outputs and GeoOrigin are exercised by existing tests.
- **GeoProximitySensor:** tangent-oriented box, `enabled`, enter/exit, pose outputs and change gating reviewed; GEO-AUDIT-5/6 cover the two output defects.
- **GeoTouchSensor:** shared TouchSensor hover/active/touch path emits all standard hit outputs and converts `hitGeoCoord_changed` using the sensor's own `geoSystem`/GeoOrigin. Existing box-pick test covers this path; no separate defect was established for different origins.
- **GeoTransform:** conjugates TRS about the `geoCenter` tangent frame for GeoCoordinate geometry; existing transform test covers rotation and translation. A nested GeoLocation is absolute by §25.3.3 and is not an established additional failure here.
- **GeoViewpoint:** geographic pose, orientation, generic bind stack, jump/retain offsets, field of view, and elevation-scaled FLY speed paths were reviewed; GEO-AUDIT-4/5 cover missed geo-specific interactions. The known OPEN GEO-3 scaling item is omitted.
- **Projection seam:** GD/GDC, GC/GCC, UTM zone/hemisphere, WM, the 23 ellipsoids, and `WGS84` token parsing are present; GEO-AUDIT-3/7 describe the validator and default-geoid exceptions. No-origin world coordinates are geocentric by design.

**Count:** 6 PROVEN, 1 UNPROVEN.
