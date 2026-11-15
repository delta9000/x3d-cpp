---
title: Present frames from the native SAI feed
summary: Create a presented scene, pull immutable frames after each pump and upload only what changed.
tags: [sai, how-to, native, presentation]
updated: 2026-11-14
related:
  - ../reference/presentation-feed.md
---

# Present frames from the native SAI feed

The presentation feed gives a renderer immutable frames of meshes, materials
and world transforms from a native scene. Exact semantics are in
[Presentation feed](../reference/presentation-feed.md).

## Steps

1. Create the service and scene through the presentation factory:

    ```cpp
    #include <x3d/sai_presentation.hpp>

    namespace native = x3d::sai::experimental::native;

    auto presented = native::make_presented_scene();
    if (!presented) { /* inspect presented.error() */ }
    auto &service = *presented->service;
    ```

2. Build and activate `presented->scene` with the ordinary service calls:
   `create_node`, `author`, `add_route`, `activate`.

3. Pull the first frame and upload everything:

    ```cpp
    auto previous = presented->feed.snapshot(service);       // shared_ptr<const frame>
    auto initial = native::changes_between(nullptr, **previous);
    for (auto key : initial->added)
      upload(initial->items.at(key));
    ```

4. After each pump, pull the latest frame and diff it:

    ```cpp
    service.pump(active, {now});
    auto current = presented->feed.snapshot(service);
    auto delta = native::changes_between(previous->get(), **current);
    ```

5. Apply the delta:

    ```cpp
    for (auto key : delta->removed)
      discard(key);
    for (auto key : delta->added)
      upload(delta->items.at(key));
    for (auto key : delta->updated_geometry)
      upload(delta->items.at(key));
    for (auto key : delta->updated_material)
      upload(delta->items.at(key));
    for (auto key : delta->updated_transform)
      move(key, delta->items.at(key).world_transform);
    previous = current;
    ```

    A key can appear in more than one update bucket. `delta->items` holds its
    complete current payload once.

## Skip frames safely

You do not have to present every tick. Pump as often as you like, call
`snapshot` when you are ready to draw, and diff against the last frame you
actually presented. Skipped ticks are handled by `changes_between`.

## Keep frames after shutdown

A frame owns everything it references. You can keep rendering one after the
service is closed and destroyed.

## Things to expect

- `snapshot` fails during setup, on the wrong thread and after retirement or
  close.
- `updated_material` can fire for a material whose value did not change.
- Materials are the extractor's unlit-white fallback in the current node
  slice.
- Camera, shading and rasterization are yours to supply.

A complete working host is `examples/sai_host`; see
[Headless host example](../reference/host-example.md).
