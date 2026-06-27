---
title: "ADR-0034: Environmental Sensor Activation Under Switch/LOD"
summary: "An environmental sensor reachable only through a non-selected Switch child or inactive LOD level is treated as removed from the transformation hierarchy (deactivated); Script and time-dependent nodes are not gated."
tags: [adr, sensors, switch, lod, traversal, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../subsystems/system-viewdependent.md
  - ../../conformance/findings.yaml
---

# ADR-0034: Environmental Sensor Activation Under Switch/LOD

## Status

Proposed — 2026-06-25. Conformance finding `SENSOR-SWITCH`.

## Context

§22.4/§22.4.3 define environmental-sensor activation via being "inserted into / removed from the
transformation hierarchy", but never say whether a non-selected `Switch` child or inactive `LOD` level
counts as "in" it. §10.4.3 (`grouping.md:222`) pulls the other way for *event routing*: a TimeSensor
in a dead branch keeps ticking. Implementations split — Xj3D/Contact cull such sensors; Vivaty does
not. Open since 2009 (Parisi/Puk).

x3d-cpp is currently branch-blind (Vivaty-like) for sensors while branch-aware for rendering — a
contradiction inside one engine. Enrolment is a flat full-graph walk (`X3DSceneBridge.hpp:305`
`forEachNode` recurses *all* Switch children / LOD levels → `:338` `attach`), and
`ViewDependentSystem::update` (`ViewDependentSystem.hpp:63-92`) ticks every enrolled sensor
unconditionally, so a ProximitySensor in a hidden Switch child still fires. But the render path culls
(`SceneExtractor.hpp:570-593` recurses only `children[whichChoice]` / the selected LOD level).

## Decision

Define an **active transformation path**: a path from a scene root to the node that, at every `Switch`
ancestor, enters `children[whichChoice]` (none if `whichChoice < 0` or out of range), and at every
`LOD` ancestor, enters the selected level. A node is **in the active transformation hierarchy** iff at
least one active path reaches it (union over DEF/USE, consistent with the existing union-of-boxes rule).

- **ProximitySensor / VisibilitySensor / TransformSensor** tick only while in the active hierarchy.
  - active→inactive edge (branch deselected / LOD level changes away): treated as *removed* — if
    active, force `isActive=FALSE` and emit `exitTime` once (reuse `deactivateIfActive`,
    `ViewDependentSystem.hpp:101-107`), then suppress events.
  - inactive→active edge (branch reselected): treated as *inserted* — re-evaluate this tick; emit
    `isActive=TRUE` + `enterTime` if the condition holds; reset the change-gate state.
- **Script and time-dependent nodes are NOT gated** — they keep initializing, running, and routing
  events in a dead branch, per §10.4.3.

This is the line that reconciles §10.4.3 (event routing) with §22 (viewer-relative activation, which
requires an instantiated coordinate system a non-traversed branch does not have).

## Consequences

**Positive:**
- Removes the render-vs-sensor asymmetry; matches Xj3D/Contact and the natural reading of "removed
  from the transformation hierarchy".
- Deterministic `isActive`/`enterTime`/`exitTime` edges on branch selection changes.

**Trade-offs:**
- Sensor enrolment must become reachability-aware (gate on the visibility-aware walk, or have
  `worldTransform` report active-path membership) — more bookkeeping per tick.
- Multiply-instanced sensors with mixed active/inactive paths use strict union (the chosen convention).

## X3D 4.1 alignment

Unresolved upstream (open since 2009; never landed). The engine adopts the Xj3D/Contact reading ahead
of the spec; an upstream erratum (a §22.2 clause cross-referencing §10.4.3/§23.4.3) is owed.
