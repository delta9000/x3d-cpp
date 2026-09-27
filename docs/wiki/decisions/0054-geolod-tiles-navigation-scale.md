---
title: "ADR-0054: GeoLOD Tiles and GeoViewpoint Navigation Scale"
summary: GeoLOD URL tiles use the runtime Inline resolver and attach/detach lifecycle; GeoViewpoint scales avatar size and visibility from elevation.
tags: [adr, geospatial, inline, navigation]
updated: 2026-09-27
related:
  - 0051-runtime-inline-expansion.md
  - ../subsystems/geospatial.md
---

# ADR-0054: GeoLOD Tiles and GeoViewpoint Navigation Scale

## Status

Accepted

## Decision

GeoLOD tile requests use the resolver supplied to `attachStandardRuntime`.
The root URL is requested when `rootNode` is empty. Child URLs are requested
inside range; the displayed set changes only after every specified child has
loaded. The root remains displayed while requests are pending. Child subtrees
detach and release outside range. The `children` output is the selected set,
which the extractor walks once. There is no range hysteresis in §25.3.4.

For GeoViewpoint, §25.3.11 gives a formula only for speed (elevation / 10,
times `speedFactor`); for `avatarSize` and `visibilityLimit` it asks for "an
appropriate value". Both are multiplied by `max(1, elevation / 10)`: they grow
with altitude but never shrink below the authored values, because
`avatarSize` also sets the collision distance and the near clip, and a
near-zero avatar close to the ground would break both. A `visibilityLimit` of 0
means unlimited (§23.4.4) and stays unlimited; only an authored finite limit
scales. The authored NavigationInfo values are kept as the base, so repeated
ticks do not compound and binding another viewpoint restores them. Without a
NavigationInfo nothing is culled by distance.
