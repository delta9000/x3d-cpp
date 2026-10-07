---
title: "ADR-0058: Context-Owned Immutable Geospatial Projection"
summary: Retain one constructor-fixed projection backend per execution context and pass it explicitly through every geographic rendering and behavior path, preventing independent worlds from changing one another's conversions or backend lifetime.
tags: [adr, geospatial, ownership, runtime]
updated: 2026-10-07
related:
  - 0053-geo-projection-seam.md
  - 0057-scene-owned-author-fields.md
  - ../subsystems/geospatial.md
---

# ADR-0058: Context-Owned Immutable Geospatial Projection

## Status

Accepted ownership decision; supersedes the process-wide selector in ADR-0053.
The early implementation checkpoint passes focused ownership tests. Full native
regression, package and exact-head CI results are tracked in the draft PR.

## Problem

The previous mutable process-wide selector was consulted whenever a conversion
omitted its backend argument. A world retained no projection owner. Replacing
the selector could therefore combine old cached transforms with new camera,
mesh, picking and event conversions, and release a backend still logically used
by that world. The selector was also unsynchronized across independent threads.

A before-fix probe against the published SDK established the defect: creating
world B doubled world A's live transform/camera/mesh/interpolator results while
A's cached transform stayed unchanged, and A's weak backend expired. A control
without B retained the backend and consistent values. The deliberately different
test backends are ownership oracles, not geodesy-conformance implementations.

## Decision

`X3DExecutionContext` retains a constructor-fixed
`shared_ptr<const geo::GeoProjection>` before constructing dependent systems.
`SessionOptions::geoProjection` supplies that owner to `RuntimeSession`. Null
resolves once to the stable immutable built-in owner. There is no replacement
method, process-wide mutable selector or independently configurable mesh backend.
The mathematical `GeoProjection` virtual interface is unchanged.

World code passes `ctx.geoProjection()` explicitly. Pure GeoFrame/GeoNodes,
transform, geometry/bounds, mesh, light/fog and HAnim helper calls require a
projection reference, making an omitted propagation a compile error. Standalone
system constructors may select the built-in; standalone mathematical helpers
can explicitly pass `geo::builtinProjection()`.

TransformSystem and PickSystem retain fixed owners where needed; BoundsSystem
uses its supplied TransformSystem's projection. SceneExtractor always uses the
context owner, including cached rebuilds. Navigation, sensors, interpolators,
Inline/GeoLOD adoption, camera and collision paths use the same choice. Retained
node handlers preserve their existing weak context/system guards and do not
capture a strong world or projection owner.

Because selection cannot change during a world's lifetime, no projection-switch
cache invalidation protocol is needed. To use another projection, construct a
new context/session. A backend remains alive until the last legitimate owning
world/system/caller reference is released.

## Migration

```cpp
x3d::runtime::SessionOptions options;
options.geoProjection = projection;
auto world = x3d::runtime::RuntimeSession::create(
    std::move(document), std::move(options));

// Or select ownership before a low-level context builds the scene:
x3d::runtime::X3DExecutionContext context(scene.authorFields, projection);
context.buildSceneGraph(scene);
auto mesh = x3d::runtime::extract::buildLocalMesh(
    geometry, context.geoProjection(), meshOptions);
```

Calls to `geo::projection()` and `geo::setProjection()` must migrate. Shared
helper calls now pass the projection explicitly. Concrete context/system layout
and function signatures change, so rebuild runtime consumers; the mathematical
virtual interface alone remains stable. Existing untouched default-context
author-field binding behavior is preserved.

## Evidence and limits

The new eight-case ownership regression exercises interleaved worlds, backend
retention/destruction, full/delta and dirty geometry, transforms/bounds/picking,
camera, initial and callback interpolation, geographic sensors/navigation,
delayed Inline/GeoLOD enrollment, HAnim walks, escaped callback retirement and
independent owner threads. It passes 1,204 assertions at this checkpoint. Existing
test migrations preserve their assertions and numerical expectations.

Operations within one mutable world remain serialized. Custom backends must
provide stable conversion behavior; implementations shared across threads must
synchronize their internal caches. `const` ownership does not prevent arbitrary
external mutation of a custom implementation. Shared mutable native graphs,
concurrent world destruction and existing unsupported sensor modes remain
outside this isolation guarantee. No I/O or backend dependency is enabled by
default. The portable SAI provider's advertised four-node slice is unchanged;
this work is not a claim of complete Geospatial, SAI or CAVEOS conformance.
