---
title: H-Anim Motion
summary: Discrete HAnimMotion playback through joint fields and the event context.
tags: [subsystem, hanim, animation]
updated: 2026-09-27
related:
  - ../decisions/0055-hanim-skinning-descriptor.md
  - ../subsystems/execution-context.md
---

# H-Anim Motion

`HAnimMotionSystem` plays each motion referenced by `HAnimHumanoid.motions`. `attachStandardRuntime` registers it. Both the humanoid's `motionsEnabled` entry and the motion's `enabled` field permit playback; absent entries default to enabled. The system resolves channel joint names against the humanoid's joints, including those in its skeleton hierarchy.

The `channels` string contains a count and that many transform names for each name in `joints`. Commas and spaces separate tokens. `IGNORED` consumes its group's values. `channelsEnabled` indexes the flattened channels; absent entries default to enabled. `values` is indexed by frame, then group, then channel. Rotations in the listed order are composed into one axis-angle joint rotation. Position values set the named translation components. The system posts joint changes through the event context so ROUTEs and dirty tracking receive them before transform propagation. This follows [ISO/IEC 19774-2 draft 2.1 §6.3–6.4](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19774/ISO-IEC19774-2/ISO-IEC19774-2v2.1/ISO-IEC19774-2v2.1-WD/MotionDataAnimation/MotionNodes.html); the published 2.0 motion-node page was unavailable during implementation.

Captured Euler channel angles use degrees. Joint axis-angle rotations use radians, as described by [published ISO/IEC 19774-2 §5.2.3–5.2.4](https://www.web3d.org/documents/specifications/19774-2/V2.0/MotionDataAnimation/AnimationUsingInterpolators.html). Quaternion multiplication preserves the channel order when several axes are present.

`frameDuration` sets the interval, `frameIncrement` sets direction and stride, and zero pauses automatic advance. `frameIndex` is clamped to the available frames. `startFrame` and `endFrame` bound playback; zero `endFrame` selects the last frame. A true `next` or `previous` steps once and wraps at the bounds. Automatic playback stops at the boundary unless `loop` is true. The motion emits `frameCount`, `cycleTime` at activation and each wrap, and cumulative `elapsedTime`. These control rules follow draft 2.1 §6.3, used because the corresponding published 2.0 page timed out.

The implementation and synthetic two-joint regression tests are in `runtime/hanim/HAnimMotionSystem.hpp` and `runtime/events/tests/hanim_motion_test.cpp`. Set `X3D_ARCHIVE_DIR` to the archive's `HumanoidAnimation` directory to run the optional KoreanCharacterMotionAnnexD01Jin smoke case. The system runs during `X3DExecutionContext::tick`, before ROUTE drainage and transform propagation, following [ADR-0055](../decisions/0055-hanim-skinning-descriptor.md).
