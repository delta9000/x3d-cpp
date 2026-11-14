---
title: Drive the context and extractor directly
summary: Use X3DExecutionContext and SceneExtractor without RuntimeSession when you need control over what is attached.
tags: [how-to, execution, extraction, embedding]
updated: 2026-11-14
related:
  - ../reference/subsystems/execution-context.md
  - ../reference/subsystems/extract.md
---

# Drive the context and extractor directly

`RuntimeSession` wires a document, a context and an extractor for you. Use the
three objects yourself when you want to choose which behaviors are attached,
or when you need only part of the pipeline (for example ticking with no
extraction).

## 1. Load

```cpp
sdk::X3DDocument doc = sdk::parseFile("scene.x3dv");   // sniffs encoding and gzip
// or from memory:
sdk::X3DDocument doc2 = sdk::parseDocument(text, sdk::Encoding::XML);
```

Keep `doc` alive for as long as the context and the extractor exist; they
borrow its scene.

## 2. Build the context

```cpp
sdk::X3DExecutionContext ctx;
ctx.buildSceneGraph(doc.scene);                       // transforms, binding stacks, pick tree
const auto routes = ctx.buildFrom(doc.scene);         // check routes.rejected
x3d::runtime::attachStandardRuntime(doc.scene, ctx);  // built-in behaviors
```

The build calls alone attach no behaviors. Without `attachStandardRuntime`,
TimeSensors, interpolators and the view-dependent sensors do not run. Leave it
out only if you add the Systems you want yourself.

## 3. Tick

```cpp
ctx.setPointer(worldRay);
ctx.setPointerButton(down);
ctx.tick(now);   // seconds on any monotonic clock
```

After the tick, read `ctx.worldTransform(node)`, `ctx.worldBounds(node)`,
`ctx.viewMatrix()`, `ctx.pick(ray)` and the bound-node accessors.

## 4. Extract

```cpp
sdk::SceneExtractor ex(ctx, doc.scene);
sdk::RenderDelta f0 = ex.fullSnapshot();
for (auto id : f0.added) {
  const sdk::RenderItem &ri = ex.item(id);   // ri.mesh, ri.material, ri.worldTransform
}
// per frame, after ctx.tick(now):
sdk::RenderDelta d = ex.delta();
```

Per-frame descriptors come from `ex.camera()`, `ex.snapshotLights()` with
`ex.lightsOf(id)`, `ex.background()` and `ex.sceneWorldBounds()`.

## 5. Serialize, if you need to

```cpp
std::string xml = sdk::XmlWriter{}.writeDocument(doc);
```

`JsonWriter` and `VrmlWriter` work the same way.

## Complete programs

| File | Shows |
|---|---|
| `examples/01_load_validate_convert.cpp` | Load, report diagnostics, convert |
| `examples/02_extract_render_feed.cpp` | Context, snapshot, items, camera, lights, delta |
| `examples/03_attach_behavior_tick.cpp` | A custom System and ticks |

The contracts are in
[Execution context](../reference/subsystems/execution-context.md) and
[Extraction](../reference/subsystems/extract.md).
