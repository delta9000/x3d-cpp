# Geospatial — conformance

_Generated. Levels 1,2 · 11 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| GeoCoordinate | 1 | ✓ | — | — | GEO-2 | X3DCoordinateNode, X3DGeometricPropertyNode |
| GeoElevationGrid | 1 | ✓ | ✓ | — | EXT-001, EXT-003, GEO-2 | X3DGeometryNode |
| GeoLOD | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| GeoLocation | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| GeoMetadata | 1 | ✓ | — | — | — | X3DChildNode, X3DInfoNode, X3DUrlObject |
| GeoOrigin | 1 | ✓ | — | — | — |  |
| GeoPositionInterpolator | 1 | ✓ | — | ◑ | CONF-GEO, INTERP-02, PIV-1 | X3DChildNode, X3DInterpolatorNode |
| GeoProximitySensor | 2 | ✓ | — | ✗ | CONF-GEO, ENV-02, ENV-03 | X3DChildNode, X3DEnvironmentalSensorNode, X3DSensorNode |
| GeoTouchSensor | 1 | ✓ | — | ✗ | TSN-1, TSN-2 | X3DChildNode, X3DPointingDeviceSensorNode, X3DSensorNode, X3DTouchSensorNode |
| GeoTransform | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| GeoViewpoint | 1 | ✓ | — | ✓ | BIND-01, BIND-02, BIND-03, BIND-04, BIND-05, BIND-06, BIND-07, BIND-08, BIND-09, GEO-1, NAV-FLY-ROLL | X3DBindableNode, X3DChildNode, X3DViewpointNode |

## Findings

- **TSN-1** [critical/DEFERRED] — §25.3.9: GeoTouchSensor is never resolved in the pointing-device cycle (PointingSensorSystem hard-checks nodeTypeName()=="TouchSensor") — receives no pointer events.
  - Resolution is a small add, but the node is only useful with TSN-2; deferred with the Geospatial seam (CONF-GEO).
- **TSN-2** [critical/DEFERRED] — §25.3.9: hitGeoCoord_changed (geodetic intersection via geoSystem/geoOrigin) has no implementation.
  - Blocked on the GeoProjection seam (ECEF→geodetic). See CONF-GEO.
- **ENV-02** [major/DEFERRED] — §25.3.8: GeoProximitySensor has no System and no geoCoord_changed.
  - Blocked on geo-projection (Geospatial deferred). See CONF-GEO.
- **ENV-03** [minor/OPEN] — §22.4.1: centerOfRotation_changed never emitted.
- **CONF-GEO** [minor/DEFERRED] — §25: Geospatial behavioral nodes have no System (geo-coordinate projection prerequisite missing).
  - Blocked on the GeoProjection seam (geoSystem/geoOrigin → local Cartesian). Drives ENV-02; GeoTouchSensor = TSN-1/2.
- **BIND-01** [critical/CLOSED `e3235ee`] — §23.2.3: Navigation writes back into authored position/orientation — corrupts authored values, breaks retainUserOffsets and ROUTE/Script readers (CAVE-critical).
  - CONF-VIEWNAV — needs a user-offset-state design (authored pose vs accumulated offset) before fixing BIND-01..08 as one cluster.
- **BIND-02** [critical/CLOSED `95d1107`] — §23.3.1: Viewpoint.navigationInfo field ignored — bound viewpoint never dispatches set_bind to its NavigationInfo.
  - CONF-VIEWNAV cluster.
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
- **GEO-1** [major/FIXED] — §25.3.11: GeoViewpoint.position silently reads as zero (SFVec3d/SFVec3f type mismatch).
  - Was: position read via getField<SFVec3f> on an SFVec3d field → camera pinned to origin. Fixed by geombounds::getVec3fLenient at ALL bound-viewpoint read sites — X3DExecutionContext::viewMatrix AND NavigationSystem::poseOf (position) + cor (centerOfRotation), the latter found by the systematic getField-type audit. getField now asserts on such mismatches in debug. Tested in getfield_typecheck_test.cpp + navigation_geoviewpoint_examine. Residue tracked as GitHub issues: #34 (OrthoViewpoint.fieldOfView MFFloat), #35 (write-side centerOfRotation persist). (sweep 2026-06-25, fixed same day)
- **PIV-1** [minor/CLOSED `07c31ca`] — §—: registerInterpolatorSystems had no production caller; added attachInterpolators scene-walk wiring + makeInterpolatorSystems factory.
- **BIND-09** [minor/CLOSED] — §23.3.1: Pop (unbind/delete) does not apply the §23.3.1 r6.3 un-jump (next viewpoint keeps its stored relative transform); ViewpointBindSystem treats a pop like a fresh jump bind.
  - Needs push-vs-pop signaling from BindingSystem to distinguish rule 5.1 (reset) from 6.3 (restore stored offset). Per-node offset persists; only the reset-on-rebind path differs. CAVE doesn't exercise viewpoint stacks.
- **GEO-2** [minor/FIXED] — §25.3.1: Geo double-precision geometry reads dropped silently (MFVec3d/SFDouble/MFDouble as float).
  - Was: coord points read via getField<vector<SFVec3f>> on GeoCoordinate/CoordinateDouble MFVec3d 'point' → empty mesh; GeoElevationGrid SFDouble spacing + MFDouble height read as float in the bounds path. Fixed across MeshBuilder extract AND GeometryBounds (pointsBounds + GeoElevationGrid handler) via geombounds::getPointsLenient + new getFloatLenient/getFloatsLenient. Sibling bounds/grid sites found by the systematic getField-type audit. Tested in getfield_typecheck_test.cpp + geometry_bounds_test (GeoCoordinate IFS + GeoElevationGrid). (sweep 2026-06-25, fixed same day)
- **NAV-FLY-ROLL** [low/CLOSED] — §23.4.4: FLY accumulates orientation incrementally (yaw-about-world-up + pitch-about-local-right), so a long mixed drag can introduce gradual horizon roll.
  - Pre-existing (unchanged by the offset model). Re-level to world-up each step (decompose to yaw/pitch) if a consumer needs roll-free fly.

