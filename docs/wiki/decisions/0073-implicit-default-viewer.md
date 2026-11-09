---
title: "ADR-0073: Unbound viewer head tracking"
summary: Preserve the unbound world origin while making head tracking effective.
tags: [adr, viewpoint, host]
updated: 2026-10-18
related:
  - ../subsystems/execution-context.md
  - ../subsystems/system-viewdependent.md
---

# ADR-0073: Unbound viewer head tracking

## Status

Proposed, revised after the maintainer's build. The origin-preserving revision
and updated regressions await a rebuild.

## Problem

Without a bound Viewpoint, `viewMatrix()` returns identity before consulting
the stored head pose. Thus `setHeadPose` has no effect on camera position,
extraction, or viewer-dependent behavior. CAVEOS inserts a synthetic Viewpoint
before session construction to avoid this case.

## Decision

Keep the existing unbound camera-to-world base at the world origin with identity
orientation. Compose relative head translation and rotation after that base,
as for a Viewpoint explicitly placed at the origin. Identity head tracking
therefore preserves the previous identity view. An absolute `setViewerWorldPose`
override still wins; clearing it resumes the latest stored head pose.

Create no node. `boundViewpoint()` stays null, roots and serialization are
unchanged, and `boundViewpointWorldPose()` stays identity to indicate the
absence of an authored pose. Per-viewpoint navigation offsets require a bound
node; this change does not install navigation for the unbound viewer. A later
authored bind uses the usual path.

This makes input effective immediately, including before low-level scene
wiring. A warning alone would leave head tracking ineffective on valid scenes
without an authored viewpoint.

## Compatibility and host migration

The original proposal used the generated Viewpoint default at `(0, 0, 10)`.
The maintainer's build exposed sound attenuation and GeoLOD regressions in
existing scenes without Viewpoints. Those fixtures correctly encode the
runtime's previous origin-based camera/listener contract. Moving every such
scene is unnecessary to make head tracking effective, so preserve that contract.
Do not claim an unbound viewer is a synthetic default X3D Viewpoint. The
deviation from the spec default position is recorded as conformance finding
`BIND-UNBOUND-ORIGIN` (`docs/conformance/findings.yaml`).

Hosts that supply no head tracking keep their existing framing, listener
position, and LOD selection. Hosts that explicitly supply head tracking now
get the requested movement/rotation instead of ignored input. Bound-viewpoint
behavior and absolute overrides stay unchanged.

CAVEOS can remove its synthetic-node fallback at
`caveos/plugins/x3d/x3d_scene.cpp:274-279` in the plain-main host and
`:229-234` in the current round-two host if the node only worked around ignored
tracking. Its chosen `(0, 1.7, 0)` pose remains host presentation policy.
Retain that policy through an explicit node or `setViewerWorldPose` if needed;
the SDK no longer requires a bound node to apply tracking.

## Verification

`Unbound viewer: origin is preserved and head tracking is effective`, registered
in `x3d_extract_tests` through `default_viewer_test.cpp`, checks identity before
input and compares tracked results to an explicit origin Viewpoint. It covers
translation/rotation, extractor output, override/clear behavior, later binding,
and a context before wiring. A second case checks tracking changes LOD selection
from the origin. The pre-existing unbound override-clear expectation remains
identity; sound and GeoLOD expectations are unchanged.

The maintainer reported 74/77 suites passing before this revision. The built
sound, focused GeoLOD, and focused rejecting-resolver cases reproduce the
reported failures. The binaries predate these edits, so no passing result is
claimed for this revision.
