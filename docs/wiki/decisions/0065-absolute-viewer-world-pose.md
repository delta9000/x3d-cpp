---
title: "ADR-0065: Absolute viewer world pose"
summary: Let a host own the effective camera while retaining authored viewpoint queries and relative navigation state.
tags: [adr, viewpoint, host]
updated: 2026-10-18
related:
  - ../subsystems/system-viewpointbind.md
  - ../guides/native-host.md
---

# ADR-0063: Absolute viewer world pose

Proposed. CAVEOS owns navigation and head tracking in world space. Its trial
integration cancels the SDK's relative camera frame and decomposes the result
into `setHeadPose(position, orientation)` every frame. Alignment also recovers
the authored viewpoint by removing the navigation offset from the effective
camera. Both operations belong at the SDK boundary.

Add these methods to `X3DExecutionContext`, also available through `x3d::sdk`:

```cpp
void setViewerWorldPose(const Mat4 &pose);
void clearViewerWorldPose();
Mat4 boundViewpointWorldPose() const;
```

`setViewerWorldPose` stores a camera-to-world matrix in X3D world coordinates.
The caller supplies a finite, invertible rigid translation/rotation matrix;
scale, shear, projection, and invalid matrices are outside the contract. The
matrix uses the existing column-major `Mat4` convention. The override takes
effect immediately, works without a bound viewpoint, and persists until
explicitly cleared or the context is destroyed. Set it before `tick` so the
view-dependent systems observe the same pose as extraction.

While overridden, `viewMatrix()` returns its inverse. Camera position/up,
ProximitySensor, LOD, Billboard, VisibilitySensor, picking, and extraction read
that effective pose through the existing camera queries. Projection parameters
still come from the bound viewpoint (or existing defaults), and aspect still
comes from the host's view-volume input. This adds no per-eye or per-layer
camera state.

`setHeadPose` continues storing relative tracking; it neither changes nor
clears the override. Navigation offsets and viewpoint binding continue to
operate on the relative camera independently of the host camera, including
jump/retain rules and transitions. Clearing resumes the then-current bound
viewpoint, offset, and stored head pose; it does not restore a saved camera or
copy the absolute pose into a viewpoint. Hosts that own navigation need not
install SDK navigation.

`boundViewpointWorldPose()` returns ancestor world frame times authored
position/orientation, including the existing GeoViewpoint tangent-frame
conversion through the context's projection. It excludes navigation offsets,
relative tracking, and the override. With no bound viewpoint it returns
identity; use `boundViewpoint()` to distinguish that case. It keeps the existing
first-path semantics for shared viewpoints. Authored fields remain unchanged.

Existing callers retain their behavior until they opt into the override. This
is an additive source API; the additional context member changes object layout
and consumers must rebuild. The focused cases in `viewer_world_pose_test.cpp`
are registered in `x3d_geometry_scene_tests`; they have not been run as part of
this proposal.
