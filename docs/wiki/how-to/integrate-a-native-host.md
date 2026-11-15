---
title: Integrate a native host
summary: Wire a RuntimeSession into an application frame loop with input, a camera and scene acceptance checks.
tags: [how-to, embedding]
updated: 2026-11-14
related:
  - ../reference/native-host-contract.md
  - supply-resources.md
  - ../tutorials/first-scene.md
---

# Integrate a native host

This guide wires the runtime into an application that already has a window, a
renderer and an event loop. It assumes you have linked `x3d_cpp::sdk`
([Build and install the SDK](build-and-install.md)) and know the three calls
from [the first tutorial](../tutorials/first-scene.md). The rules behind each
step are in the [native host contract](../reference/native-host-contract.md).

## 1. Create the session

```cpp
sdk::SessionOptions options;
options.interactive = true; // pointer navigation; leave false if you own navigation
auto session = sdk::RuntimeSession::create(sdk::parseFile(path), options);
```

If your host owns navigation but still wants TouchSensor and drag sensors, set
`options.pointingSensors` instead of `interactive`. Wire resource callbacks
here too; see [Supply resources through the seams](supply-resources.md).

## 2. Check the scene before you accept it

Loading reports problems instead of throwing. Before you show the scene, check:

| Check | Where |
|---|---|
| Range, PROTO and Inline warnings | `session->document().rangeWarnings`, `.protoWarnings`, `.inlineWarnings` |
| Rejected ROUTEs | `session->routes().rejected` |
| Silently skipped ROUTE endpoints | Compare `session->routes().routesAdded` with the count you expect |
| Geometry the extractor does not emit | `session->extractor().skippedGeometryCounts()` |
| A partial snapshot | `session->extractor().budgetExceeded()` after `fullSnapshot()`; reject the snapshot if true |

## 3. Upload the first snapshot

```cpp
sdk::RenderDelta first = session->fullSnapshot();
for (sdk::RenderItemId id : first.added) {
  const sdk::RenderItem &item = session->extractor().item(id);
  // Upload *item.mesh, item.material and item.worldTransform.
}
```

Copy what you need from `item` now. The reference is valid only until the next
mutation or extraction.

## 4. Run the frame loop

Do these in order, on one thread, every frame:

```cpp
session->context().setPointer(worldRay);      // 1. input since the last tick
session->context().setPointerButton(down);
session->tick(now);                           // 2. advance; now is in seconds
sdk::RenderDelta d = session->delta();        // 3. take the delta once
for (auto id : d.removed) { /* destroy */ }   //    removals first
for (auto id : d.added)   { /* create */ }
// then d.updatedTransform, d.updatedGeometry, d.updatedMaterial,
// d.updatedSkinPose and the camera, light, background and fog flags
// 4. draw
```

If you run simulation substeps without presenting, take the delta after each
one. If you miss one, call `fullSnapshot()` and replace your placement set.

## 5. Drive the camera

Choose one:

- **Let the scene drive.** Read `viewMatrix()` after the tick. Pass head
  tracking with `setHeadPose`; it composes after the bound Viewpoint and the
  navigation offset.
- **Own the camera.** Pass your rigid camera-to-world matrix before the tick:

    ```cpp
    session->context().setViewerWorldPose(worldFromHead);
    session->tick(now);
    ```

    Check the return value: a non-finite or non-rigid matrix is refused and the
    previous pose is kept. Call `clearViewerWorldPose()` to hand control back.

## 6. Change the scene from the host

Queue your edits and apply them with `context.writeField` from a `System`; see
[Add a behavior System](add-a-behavior-system.md). To replace the whole scene,
build a new session and swap it at a frame boundary.

## 7. Retire GPU resources safely

Removed placements can still be in flight on the GPU. Defer destruction with
your own fences. Clear any state indexed by `RenderItemId` when you unload a
session, because the IDs are local to it.

## See it working

`examples/embed_minimal/native_host.cpp` is a renderer-free host that follows
these steps. To build and run it against an installed SDK:

```sh
cmake -S examples/embed_minimal -B build-host -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build-host --target x3d_embed_native_host
./build-host/x3d_embed_native_host
```

## Related

- [Measure tick phases](measure-tick-phases.md)
- [Seam matrix](../reference/seam-matrix.md)
