---
title: Rigid-body physics
summary: "X3D §37 rigid-body dynamics via a engine-agnostic seam + a flag-gated Jolt example backend; sim traces it."
tags: [subsystem, physics, seam, jolt]
updated: 2026-06-21
related:
  - ../../explanation/architecture.md
  - ../../explanation/decisions/0019-physics-seam.md
  - ../../explanation/decisions/0001-ext-firewall.md
  - cli-suite.md
---

# Rigid-body physics

`PhysicsSystem` maps X3D §37 nodes to `PhysicsBackend`, steps the backend on
host ticks, and posts pose changes through the event cascade. The interface
contains runtime values and opaque handles, without engine-library types.

## Key files

The implementation is split at the engine boundary.

| Source | Role |
|---|---|
| `runtime/physics/PhysicsBackend.hpp` | World, body, shape, constraint, and solver contracts |
| `runtime/physics/PhysicsSystem.hpp` | Node enrollment, parameter mapping, stepping, and pose events |
| `runtime/physics/ContactReporter.hpp` | CollisionSensor output from resolved contacts |
| `runtime/physics/jolt/JoltBackend.cpp` | Jolt implementation |
| `cmake/x3d/backends.cmake` | Optional backend build |

## Backend configuration

Enable `X3D_CPP_BUILD_PHYSICS` to fetch and build Jolt. Jolt dependencies stay
private to `x3d_physics_jolt`; the default SDK build does not fetch it.
`tools/x3d-cli/sim_runtime.hpp` attaches the Jolt-backed system when linked.

Jolt defaults to a single-threaded job system. Passing a nonzero worker count
selects a thread pool; deterministic comparisons require a fixed count.
World capacity is 1024 bodies with bounded contact storage, and one collision
substep is used per update. Fast bodies can tunnel.

## Node mapping

`PhysicsSystem::attach` creates a backend world per RigidBodyCollection and
maps supported CollidableShape geometry to shape descriptors. Bodies with
unsupported geometry are excluded and counted by `droppedBodyCount()`.
Collection and body fields configure mass, pose, velocities, gravity, forces,
damping, and sleeping through backend parameters.

Ball, single-axis hinge, and slider joints map to constraint descriptors.
A missing body endpoint binds the other endpoint to the world. Jolt hinge
limits reverse sign to match the authored rotation convention.

Solver fields cross the seam only when different from their X3D defaults,
leaving backend defaults intact otherwise. See `PhysicsSystem::attach` for
the field-to-parameter mapping and the
[findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml) for remaining §37 gaps.

## Events and contacts

After stepping, the system posts RigidBody position and orientation changes.
Authored ROUTEs can drive visible transforms from those values.
`ContactReporter` resolves engine contacts to native nodes and emits
CollisionSensor contacts, intersections, and isActive. Reports are read-only;
contacts are not fed back to the solver (§37.4.5).

The engine boundary is recorded in
[ADR-0019](../../explanation/decisions/0019-physics-seam.md).

## Related

- [Architecture](../../explanation/architecture.md)
- [ADR-0001: External-Code Firewall](../../explanation/decisions/0001-ext-firewall.md)
- [CLI suite](cli-suite.md)
