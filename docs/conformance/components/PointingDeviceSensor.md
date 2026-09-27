# PointingDeviceSensor — conformance

_Generated. Levels 1 · 4 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| CylinderSensor | 1 | ✓ | — | ◑ | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-1, DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| PlaneSensor | 1 | ✓ | — | ◑ | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| SphereSensor | 1 | ✓ | — | ◑ | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, DS-2 | X3DChildNode, X3DDragSensorNode, X3DPointingDeviceSensorNode, X3DSensorNode |
| TouchSensor | 1 | ✓ | — | ◑ | AUD-PDS-1, AUD-PDS-2, AUD-PDS-3, CONF-CRITIC-3 | X3DChildNode, X3DPointingDeviceSensorNode, X3DSensorNode, X3DTouchSensorNode |

## Findings

- **AUD-PDS-1** [major/OPEN] — §20.2.1: Pointing sensors tied for lowest do not activate together; only the first sibling sensor gets the event.
  - Spec: tied sensors are 'activated simultaneously and independently'. PointingSensorSystem keeps a single over_/active_. Probe: 'audit tied lowest pointing sensors all receive the event'.
- **AUD-PDS-2** [major/OPEN] — §20.4.2: Disabling an active sensor deactivates it only on the next pointer update.
  - Spec: on enabled FALSE while active the sensor 'outputs an isActive FALSE event'. The update returns early when the pointer revision is unchanged. Incomplete fix of DS-2. Probe: 'audit disabling an active PlaneSensor deactivates without pointer motion'.
- **AUD-PDS-3** [major/OPEN] — §10.4.3, 20.2.3: Geometry in an unchosen Switch branch is still pickable and can occlude visible geometry.
  - PickSystem enumerates every Switch child; picking should follow the traversed (rendered) branch as the extractor does. Probe: 'audit pointing sensors ignore geometry in an unchosen Switch branch'.
- **DS-1** [major/FIXED `5eef411`] — §20.4.1: CylinderSensor disk mode tracks the Y=0 plane, not the activation-hit Y.
- **DS-2** [minor/FIXED `8652034`] — §20.4.x: A grabbed drag sensor disabled mid-drag stops tracking on the next evaluation.
- **CONF-CRITIC-3** [low/CLOSED] — §20.4.4: touchTime condition-3 (still-over-at-release pick re-resolution) verified — implemented and tested.
  - Was BACKLOG CONF-CRITIC. Verified 2026-07-17: PointingSensorSystem.hpp re-resolves the release pick (stillOver = pick.hit && resolve(pick) == active_) and gates touchTime on `pressWasOver_ && stillOver` (§20.4.4); covered by pointing_sensor_test.cpp test_isActive_touchTime, which asserts touchTime fires when released over the geometry and does NOT fire when released off it. Both present since the initial commit — the finding was carried over from BACKLOG without confirming the behavior already existed.

