# X3D Behavioral Conformance — campaign methodology

> **Per-finding status now lives in the generated conformance view, not here.**
> The source of truth for behavioral findings is
> [`docs/conformance/findings.yaml`](../../conformance/findings.yaml); the
> zoom-out is [`docs/conformance/INDEX.md`](../../conformance/INDEX.md). This file
> retains the campaign *methodology* (how we audit and verify). Edit findings in
> the YAML and run `mise run conformance` — do not re-table findings here.

The #2 behavioral conformance campaign proves the runtime **behaves** per ISO/IEC
19775-1 prose (the corpus run only proves no-crash + round-trip). It targets the
~14 behavioral interfaces (sensors, time-dependent, interpolators, followers,
bindable, grouping active-child, script, triggers, sequencers).

## Cadence (user-chosen, 2026-06-18)

Session-scoped phases: **ONE audit fanout (3–4 interfaces) + ONE fix cycle per
session.** Keeps the map and the closures in sync. The audit workflow is expensive
(calibration ≈ 1.9M subagent tokens / 55 agents for 3 interfaces), so a full sweep
does not fit alongside fixes in one session.

## Audit workflow

`docs/superpowers/specs/2026-06-18-conformance-audit-workflow-design.md` —
registry-partitioned audit → **3-skeptic adversarial verify (≥2/3 to survive)** →
prioritize (severity × corpus-prevalence × 1/effort) → completeness critic. Parse
`args` as array-or-string; re-ingest `code_rag` before a run.

**Tuning tweak (full-run):** in synthesis, cross-check rejected findings against the
critic's `unchecked_behaviors`; if the critic re-raises a rejected finding, resurrect
it for one re-review rather than discarding. (Calibration: the verifier rejected
`pauseTime_changed` (TDN-1, 1/3) while confirming the identical-mandate
`resumeTime_changed` (TDN-2); the critic independently re-flagged TDN-1 — and it was
real.)

**Process choice:** prefer DIRECT TDD over an SDD fan-out for coupled single-file /
single-cluster fixes (cheaper); use subagent-driven dev for independent multi-file
work. Each high-tier cluster: `writing-plans` → fix → golden/ctest/corpus-gated.

## Waves completed

Per-finding status, severity, wave, and commit live in `findings.yaml`; fix history
is in git + the `wave:`/`commit:` fields.

- **Calibration** — X3DTimeDependentNode / X3DDragSensorNode / X3DScriptNode.
- **Wave-2 fanout** — X3DInterpolatorNode / X3DBindableNode / X3DEnvironmentalSensorNode.
- **Wave-3 fanout** (2026-06-19, run `wf_fa4d2200-6fe`, 89 agents) — X3DTriggerNode /
  X3DSequencerNode / event-utility filters / X3DFollowerNode. **28 audited → 25
  adversarially confirmed** (rejected 0–1/3: SEQ-6, EUF-3, FOL-8). Headline: every
  Trigger/Sequencer/BooleanFilter/BooleanToggle/Follower node is behaviorally INERT
  (no System wired → ROUTEs silently dropped). TRIG-*/SEQ-*/EUF-* are `open` (common,
  small clean-add Systems — the next fix cycle); FOL-* are `deferred` (§39 advanced,
  low prevalence, no consumer). One `verify:FOL-4:repro` skeptic died on an API error
  (2/2 surviving votes still confirmed). Findings TRIG-1…6, SEQ-1…8, EUF-1/2/4/5,
  FOL-1…9 in `findings.yaml`.
- **Wave-3 fix cycle** (2026-06-19, commit `47c0714`) — the EventUtilities cluster:
  `runtime/events/EventUtilitySystem.hpp` (BooleanTrigger/IntegerTrigger/TimeTrigger,
  Boolean/IntegerSequencer stepwise + next/previous, BooleanFilter/BooleanToggle) +
  `attachEventUtilities` production wiring. **16 findings CLOSED** (TRIG-1/2/3/5/6,
  SEQ-1…5/7/8, EUF-1/2/4/5); direct-TDD, regression `x3d_event_utility`, ctest
  129/129, golden byte-identical. EventUtilities now reads 6✓/1◑. **TRIG-4** left
  open (integerKey-write side effects need §30.4.6 re-review); Followers (FOL-*)
  remain deferred.

- **Wave-4 fanout** (2026-06-19, run `wf_be659767-7d1`, 83 agents) — X3DTouchSensorNode
  / X3DKeyDeviceSensorNode / X3DNetworkSensorNode / X3DGroupingNode active-child.
  **26 audited → 25 confirmed** (rejected NSN-8 1/3). TouchSensor + Switch audited
  clean; GeoTouchSensor (TSN-1/2), Collision-volume (COL-1/3), LoadSensor (NSN-1..9),
  DIS PDUs (NSN-10) deferred on their seams; KeyDeviceSensor (KDS-1..10), Collision
  proxy-render (COL-2), and LOD clamp (LOD-1) `open`. Findings in `findings.yaml`.

- **Wave-4 fix cycle** (2026-06-19, commit `85b90b0`) — the KeyDeviceSensor cluster:
  extended the `KeyState` seam with a discrete key-event queue (serves both nav and
  the sensors) + `runtime/events/KeyDeviceSensorSystem.hpp` (KeySensor keyPress/Release/
  actionKey/modifiers/isActive; StringSensor enteredText/finalText/deletion/isActive) +
  `attachKeyDeviceSensors`. **9 findings CLOSED** (KDS-1/2/3/4/5/7/8/9/10); direct-TDD,
  regression `x3d_key_device_sensor`, ctest 130/130, golden byte-identical.
  KeyDeviceSensor now reads 2◑. **KDS-6** (exclusive-focus arbitration) left open.

- **Small-standalone fix batch** (2026-06-19, commit `2b84a99`) — four isolated
  open findings: COL-2 (Collision.proxy not rendered — extractor), LOD-1
  (level_changed clamped to rendered child), ENV-04 (ProximitySensor pose events
  change-gated), ENV-07 (re-enable re-fires enter). Direct-TDD (regressions
  `x3d_scene_extractor_col2` + 3 in `x3d_view_dependent`), ctest 131/131, golden
  byte-identical. Findings register now **45 closed / 45 open**. Still-open small
  items (need seams/design): ENV-03 (centerOfRotation producer), ENV-05 (6-plane
  frustum seam), ENV-06 (System detach hook), ENV-08 (interpolated boundary time),
  ENV-09 (scaled visibility radius), KDS-6 (focus arbitration).

- **CONF-VIEWNAV cluster CLOSED** (2026-06-19, branch `conf-viewnav`, design
  `2026-06-19-conf-viewnav-user-offset-design.md`) — the user-offset model, 3 phases:
  Phase 1 `e3235ee` (offset/head state + effective-view recompute + NavigationSystem→
  offset, BIND-01/03), Phase 2 `2af9570` (ViewpointBindSystem jump/retain/stored-xform,
  BIND-04/07/08), Phase 3 `95d1107` (navigationInfo dispatch / bind transitions /
  delete-detach, BIND-02/05/06). All 8 BIND findings closed. Authored pose stays
  pristine (CAVE-syncable); navigation + head-pose compose as process-local offsets.
  Regressions `x3d_viewpoint_offset` + `x3d_viewpoint_bind`; ctest 133/133, golden
  byte-identical. Viewpoint/OrthoViewpoint now read conformant.

## Remaining audit surface

**The behavioral-interface audit surface is now CLOSED** (calibration + waves 2/3/4
covered all ~14 behavioral interfaces). Deferred-behavioral interfaces (PickSensor §38,
Sound\* §16/17, RigidJoint/NBody §37, Particle\* §40, Layer/Layout §35/36) stay
known-unimplemented — audit only if a "deferred-confirm" tier is wanted.

## Extraction-semantic axis

The "do we EXTRACT per spec" wave — distinct from the behavioral axis.

- **Extraction wave** (2026-06-19, run `wf_6fec64e4-858`, 68 agents) — geometry
  tessellation+normals / Material-PBR / TextureCoordinate+transform / Text layout.
  **21 audited → 15 confirmed** (rejected 0-1/3: TXT-3/6/7/8/14/15). Folded into the
  EXTRACTION-SEMANTIC section of `findings.yaml`: MAT-001 (critical — Appearance
  material=NULL gives grey Phong instead of unlit white), EXT-001/002 (ElevationGrid
  authored color/normal dropped; fan/strip per-face indexing per-triangle), MAT-002/
  003/004/005 (shininess/ambient texture slots, occlusionStrength, normalScale),
  TXF-1 (TextureTransform order), TXT-1/2/4/5 (justify minor-axis + family fallback).
  Deferred (seams): TXF-2 (generator render-time UVs), TXF-3 (MultiTexture/Matrix
  transform), MAT-006 (backMaterial two-sided).

- **Critic next-extraction surface** (unaudited, for a follow-up): `Appearance.backMaterial`
  two-sided lighting; `FillProperties`/`LineProperties`/`PointProperties` (no extraction
  path); deprecated `TwoSidedMaterial` dispatch; **`MultiTextureCoordinate` silently
  yields zero texcoords** (buildAttrs reads a nonexistent `point` field — likely a real
  bug, verify); ElevationGrid per-quad color/normal ordering; IndexedFaceSet normalIndex
  `-1` separator alignment; per-channel MultiTextureCoordinate binding. Deferred-behavioral interfaces (PickSensor, Sound\*, RigidJoint/NBody,
Particle\*, Layer/Layout) stay known-unimplemented — see the `deferred` findings in
the view; audit only if a "deferred-confirm" tier is wanted.

## Next-wave behaviors surfaced by the completeness critic (not yet findings)

Same-tick stop-then-start restart (§8.2.4.3); `inputOutput` echo `*_changed` events
(§4.4.2.2); AudioClip `duration_changed`/pitch; MovieTexture `speed`/reverse;
Script external URL loading; Script `initialize()` timestamp back-dating; TouchSensor
`touchTime` condition 3 (still-over-at-release pick re-resolution); ProximitySensor
initial-activation-at-load burst; bindable simultaneous-timestamp rule (§23.3.1 r8);
Normal/Color interpolator normalization + hue shorter-arc; GeoViewpoint
elevation-scaled speed; Viewpoint.viewAll. These seed the next audit wave.
