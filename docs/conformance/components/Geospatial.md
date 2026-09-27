# Geospatial — conformance

_Generated. Levels 1,2 · 11 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| GeoCoordinate | 1 | ✓ | — | — | GEO-2, GEO-AUDIT-3, GEO-GEOID-DEFAULT, GEOSYSTEM, ROUTE-IO-ALIAS | X3DCoordinateNode, X3DGeometricPropertyNode |
| GeoElevationGrid | 1 | ✓ | ✓ | — | EXT-001, EXT-003, GEO-2, GEO-AUDIT-1, GEO-AUDIT-3, GEO-GEOID-DEFAULT, ROUTE-IO-ALIAS | X3DGeometryNode |
| GeoLOD | 1 | ✓ | — | — | GEO-AUDIT-3, GEO-GEOID-DEFAULT, GEOLOD-1, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| GeoLocation | 1 | ✓ | — | — | GEO-AUDIT-3, GEO-GEOID-DEFAULT, GEOSYSTEM, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| GeoMetadata | 1 | ✓ | — | — | GEO-AUDIT-2, ROUTE-IO-ALIAS | X3DChildNode, X3DInfoNode, X3DUrlObject |
| GeoOrigin | 1 | ✓ | — | — | GEO-AUDIT-3, GEO-GEOID-DEFAULT, ROUTE-IO-ALIAS |  |
| GeoPositionInterpolator | 1 | ✓ | — | ✓ | CONF-GEO, GEO-AUDIT-3, GEO-GEOID-DEFAULT, INTERP-02, PIV-1, ROUTE-IO-ALIAS | X3DChildNode, X3DInterpolatorNode |
| GeoProximitySensor | 2 | ✓ | — | ✓ | CONF-GEO, ENV-02, ENV-03, GEO-AUDIT-3, GEO-AUDIT-5, GEO-AUDIT-6, GEO-GEOID-DEFAULT, GEOSYSTEM, ROUTE-IO-ALIAS | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| GeoTouchSensor | 1 | ✓ | — | ✓ | GEO-AUDIT-3, GEO-GEOID-DEFAULT, ROUTE-IO-ALIAS, TSN-1, TSN-2 | X3DChildNode, X3DPointingDeviceSensorNode, X3DSensorNode, X3DTouchSensorNode |
| GeoTransform | 2 | ✓ | — | — | GEO-AUDIT-3, GEO-GEOID-DEFAULT, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| GeoViewpoint | 1 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-03, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, GEO-1, GEO-3, GEO-AUDIT-3, GEO-AUDIT-4, GEO-AUDIT-5, GEO-GEOID-DEFAULT, GEOSYSTEM, NAV-FLY-ROLL, ROUTE-IO-ALIAS | X3DBindableNode, X3DChildNode, X3DViewpointNode |

## Findings

- **BIND-01** [critical/CLOSED `e3235ee`] — §23.2.3: Navigation writes back into authored position/orientation — corrupts authored values, breaks retainUserOffsets and ROUTE/Script readers (CAVE-critical).
  - CONF-VIEWNAV — needs a user-offset-state design (authored pose vs accumulated offset) before fixing BIND-01..08 as one cluster.
- **BIND-02** [critical/CLOSED `95d1107`] — §23.3.1: Viewpoint.navigationInfo field ignored — bound viewpoint never dispatches set_bind to its NavigationInfo.
  - CONF-VIEWNAV cluster.
- **TSN-1** [critical/CLOSED] — §25.3.9: GeoTouchSensor is never resolved in the pointing-device cycle (PointingSensorSystem hard-checks nodeTypeName()=="TouchSensor") — receives no pointer events.
  - Fixed: PointingSensorSystem resolves GeoTouchSensor through the TouchSensor hover/grab/release path.
- **TSN-2** [critical/CLOSED] — §25.3.9: hitGeoCoord_changed (geodetic intersection via geoSystem/geoOrigin) has no implementation.
  - Fixed: hitGeoCoord_changed converts the world hit through GeoNodes::fromWorld, alongside TouchSensor outputs; covered by a box pick regression.
- **GEO-AUDIT-5** [critical/CLOSED] — §22.4.1, 25.3.8: GeoProximitySensor read GeoViewpoint.centerOfRotation as SFVec3f and crashed in debug.
  - ViewDependentSystem reads SFVec3d and converts it with geo::toWorld before parent and sensor transforms. Regression: GeoProximitySensor reports GeoViewpoint geographic centerOfRotation.
- **INTERP-02** [major/CLOSED `07c31ca`] — §19.3.1: Empty key must emit no events; added a live empty-key guard to all interpolator Systems.
- **BIND-03** [major/CLOSED `e3235ee`] — §23.3.1: dynamic_cast<Viewpoint*> in NavigationSystem disables navigation for non-Viewpoint viewpoints.
  - CONF-VIEWNAV cluster.
- **BIND-04** [major/CLOSED `2af9570`] — §23.3.1: retainUserOffsets never tracked (follows from BIND-01).
  - CONF-VIEWNAV cluster.
- **BIND-05** [major/CLOSED `95d1107`] — §23.4.4: Viewpoint-bind transition (transitionType/Time, transitionComplete) only fired for LOOKAT, not on set_bind.
  - CONF-VIEWNAV cluster.
- **BIND-06** [major/CLOSED `95d1107`] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.
- **BIND-07** [major/CLOSED `2af9570`] — §23.3.1: jump=FALSE not honored on bind.
  - CONF-VIEWNAV cluster.
- **BIND-08** [major/CLOSED `2af9570`] — §23.3.1: Per-viewpoint stored relative transform on push-down not captured/restored.
  - CONF-VIEWNAV cluster.
- **ENV-02** [major/CLOSED] — §25.3.8: GeoProximitySensor has no System and no geoCoord_changed.
  - Fixed: ViewDependentSystem evaluates the tangent box at geoCenter through active paths, emits the ProximitySensor edge and pose outputs, and pairs position_changed with geoCoord_changed via fromWorld. Includes DEF/USE union and disable behavior. Covered by view_dependent_test.
- **GEOLOD-1** [major/CLOSED] — §25.3.4: GeoLOD selects and displays URL tiles by range.
  - Closed 2026-09-27: InlineRuntimeSystem resolves rootUrl when rootNode is empty, waits for all specified child URLs, switches children and level_changed with the displayed set, and unloads child tiles on exit. The extractor walks only that set. Covered by GeoLOD URL tiles wait, switch, unload, and recurse.
- **GEO-1** [major/CLOSED] — §25.3.11: GeoViewpoint.position silently reads as zero (SFVec3d/SFVec3f type mismatch).
  - Closed 2026-09-27: GeoViewpoint.position converts through geo::tangentFrameOf in X3DExecutionContext and NavigationSystem; orientation is relative to local east/up/south. centerOfRotation converts with geo::toWorld, and navigation speed tracks elevation/10 times speedFactor. Regressions: geospatial transforms and viewpoint, GeoViewpoint navigation speed. Earlier SFVec3d type mismatch coverage remains.
- **GEO-AUDIT-1** [major/CLOSED] — §25.3.2: GeoElevationGrid added geoGridOrigin altitude to absolute height samples.
  - gridCoordinate now uses each scaled height as the absolute elevation; mesh and bounds share that conversion. Regression: GeoElevationGrid heights are absolute elevations regardless of geoGridOrigin altitude.
- **GEO-AUDIT-2** [major/CLOSED] — §25.3.5: GeoMetadata.data references were rendered as scene content.
  - SceneExtractor skips GeoMetadata.data and nonvisual metadata fields, including MetadataSet.value. Regression: GeoMetadata data references do not create render items.
- **GEO-AUDIT-4** [major/CLOSED] — §23.4.4, 25.3.11: LOOKAT wrote a Cartesian SFVec3f to GeoViewpoint.centerOfRotation.
  - NavigationSystem converts the picked pivot with geo::fromWorld and writes SFVec3d. Regression: GeoViewpoint LOOKAT updates geographic centerOfRotation. This completes the GeoViewpoint centerOfRotation path after GEO-1.
- **GEO-AUDIT-6** [major/CLOSED] — §25.3.8: GeoProximitySensor.geoCoord_changed dropped the sensor ancestor transform.
  - Reviewed, convention kept (ADR-0053): geographic coordinates are placed in the node's local frame, with ancestor transforms applied on top as for any geometry, so geoCoord_changed converts the viewer position in the sensor's frame (it agrees with GeoCoordinate content displaced by the same Transform). The audit's world-frame reading was not adopted; GeoTouchSensor.hitGeoCoord_changed was aligned to the same local frame. Tests: GeoProximitySensor geoCoord is in the sensor's local geo frame; test_geo_touch_under_transform.
- **ENV-03** [minor/CLOSED] — §22.4.1: centerOfRotation_changed never emitted.
  - ProximitySensor and GeoProximitySensor emit centerOfRotation_changed (bound Viewpoint's centerOfRotation in the sensor's frame) under LOOKAT, change-gated.
- **PIV-1** [minor/CLOSED `07c31ca`] — §—: registerInterpolatorSystems had no production caller; added attachInterpolators scene-walk wiring + makeInterpolatorSystems factory.
- **CONF-GEO** [minor/CLOSED] — §25: Geospatial behavioral nodes have no System (geo-coordinate projection prerequisite missing).
  - Fixed: GeoPositionInterpolator interpolates authored coordinates in geoSystem before toWorld, while GeoProximitySensor uses the tangent frame and fromWorld. GeoTouchSensor is tracked by TSN-1/2.
- **BIND-09** [minor/CLOSED] — §23.3.1: Pop (unbind/delete) does not apply the §23.3.1 r6.3 un-jump (next viewpoint keeps its stored relative transform); ViewpointBindSystem treats a pop like a fresh jump bind.
  - Needs push-vs-pop signaling from BindingSystem to distinguish rule 5.1 (reset) from 6.3 (restore stored offset). Per-node offset persists; only the reset-on-rebind path differs. CAVE doesn't exercise viewpoint stacks.
- **GEO-3** [minor/CLOSED] — §25.3.11: GeoViewpoint scales navigation avatarSize and visibilityLimit with elevation.
  - Closed: NavigationSystem multiplies the bound NavigationInfo avatarSize, and a finite visibilityLimit, by max(1, elevation / 10) while a GeoViewpoint is bound (ADR-0054); an unlimited limit (0) stays unlimited and the authored values are restored when another viewpoint binds. Tests: GeoViewpoint scales avatar and visibility with elevation; GeoViewpoint keeps an unlimited visibilityLimit unlimited.
- **GEO-2** [minor/CLOSED] — §25.3.1: Geo double-precision geometry reads dropped silently (MFVec3d/SFDouble/MFDouble as float).
  - Closed 2026-09-27: GeoCoordinate MFVec3d points now convert in one list through geo::toWorld for mesh and bounds; CoordinateDouble remains Cartesian. GeoElevationGrid lattice, elevation, authored tangent normals, and bounds convert through GeoNodes. Earlier type mismatch coverage remains. Regressions: mesh_builder_b5_test.cpp and geometry_bounds_test.cpp.
- **GEOSYSTEM** [minor/CLOSED] — §25.2.3: geoSystem stored unchecked; non-conforming token N (83 Squaw*.x3d) kept silently with no conformance warning.
  - Per ADR-0003 keep the value; add a geoSystem token validator (SRF GD|GDC|GC|GCC|UTM|WM + UTM Z<n>/optional S/ellipsoid/ordering grammar) emitting RangeDiagnostic-style warnings, never rejecting. N is undefined (only S exists; northern is the UTM default) — tolerate as a no-op northern alias + warning (Mantis 938). UOM cannot enforce (additionalEnumerationValuesAllowed = true). Also add the missing WM (Web Mercator) enumeration to geoSystemSpatialReferenceFrameValues. Site GeoCoordinate.hpp:111 setGeoSystemUnchecked. 4.1 - unresolved (4.1 UOM still lacks WM, enum Closed 2026-09-26: runtime/X3DRangeValidate.hpp validates frame, zone and option tokens without mutation; range_warnings_test covers GD, UTM Z10 S, N and garbage. The validator accepts WM (Web Mercator). The vendored X3D UOM is left as published (it still lacks WM), so the generated enumeration does not list it; the open enum stores it regardless.
- **GEO-AUDIT-3** [minor/CLOSED] — §25.2.3, 25.2.4: geoSystem validation rejected valid ellipsoid and ordering tokens and omitted nodes.
  - Validation now runs on every geoSystem field and reuses parseGeoSystem and ellipsoidByCode; it accepts AA, ordering tokens, GDC/GCC, and WGS84 while retaining warnings for unknown tokens, N, and bad UTM zones. Regression: geoSystem validation accepts spec tokens on every geospatial node. This completes GEOSYSTEM.
- **GEO-GEOID-DEFAULT** [minor/CLOSED] — §25.2.3: Default backend has no geoid model, so WGS84 heights are converted as ellipsoidal.
  - Closed 2026-09-27: runtime/io/tinygeoid/TinygeoidGeoid.hpp (vendored tinygeoid, MIT, header-only, no dependencies) gives the built-in backend a WGS84 geoid from a .tng grid (e.g. EGM2008 2.5′ via tng_pack); the optional PROJ backend takes a PROJ grid. The grid data (~150 MB for EGM2008) is supplied by the application, so a build without one still converts WGS84 heights as ellipsoidal, as documented in geospatial.md. Tests: geoid: tinygeoid supplies the WGS84 geoid...; geoid: a .tng file loads...
- **NAV-FLY-ROLL** [low/CLOSED] — §23.4.4: FLY accumulates orientation incrementally (yaw-about-world-up + pitch-about-local-right), so a long mixed drag can introduce gradual horizon roll.
  - Pre-existing (unchanged by the offset model). Re-level to world-up each step (decompose to yaw/pitch) if a consumer needs roll-free fly.

