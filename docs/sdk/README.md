# x3d-cpp SDK — embedder quick-start (v1)

A headless, renderer-agnostic X3D domain runtime. You load an X3D document, tick
a runtime, and pull renderer-ready descriptors out the other side. The SDK does
no windowing, no GPU work, and no file/network/font IO — those are the embedder's
job, wired through small callback **seams** (ports; defined under
[The seams](#the-seams-embedder-supplied-io) below).

One include, one namespace:

```cpp
#include "x3d/sdk.hpp"
namespace sdk = x3d::sdk;
```

One CMake target:

```cmake
find_package(x3d_cpp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE x3d_cpp::sdk)
```

Build a third-party project against an installed SDK by putting the install
prefix on `CMAKE_PREFIX_PATH`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
```

The smallest complete downstream project lives in
[`examples/embed_minimal/`](../../examples/embed_minimal/). It includes only
`x3d/sdk.hpp`, links only `x3d_cpp::sdk`, parses a built-in X3D scene, builds
the runtime context, and extracts one render snapshot.

## The three layers

### 1. Load

```cpp
sdk::X3DDocument doc = sdk::parseFile("scene.x3dv");          // sniffs encoding + gzip
// or, from memory:
sdk::X3DDocument doc = sdk::parseDocument(text, sdk::Encoding::XML);
```

`parseFile` handles all four encodings (XML, ClassicVRML, VRML97, JSON), gzip
input, versions 3.0–4.1, lenient reads, and PROTO/EXTERNPROTO expansion. The
returned `X3DDocument` carries `version`, `profile`, `head`, `scene`, and two
diagnostic channels: `rangeWarnings` (out-of-range authored values the lenient
read kept) and `protoWarnings` (PROTO expansion issues). The document never fails
to load over one bad field.

You can serialize back out with `XmlWriter`/`JsonWriter`/`VrmlWriter`
(`writeDocument(doc) -> std::string`).

### 2. Runtime (tick)

```cpp
sdk::X3DExecutionContext ctx;
ctx.buildSceneGraph(doc.scene);   // index transforms, binding stacks, pick tree
ctx.buildFrom(doc.scene);         // resolve DEF-named ROUTEs + IS redirects

const auto t0 = std::chrono::steady_clock::now();   // monotonic clock start

// once per frame — `now` is seconds since start (any monotonic clock):
double now = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
ctx.setPointer(worldRay);
ctx.setPointerButton(down);
ctx.tick(now);                    // advance clock, run systems, drain cascade
```

`tick(now)` advances the clock, updates every registered `System`, drains the
ROUTE event cascade, and propagates dirty transforms + bounds. After a tick the
pull surface is current: `worldTransform(node)`, `worldBounds(node)`,
`viewMatrix()`, `cameraWorldPosition()`, `pick(ray)`, and the bound-node
accessors (`boundViewpoint()`, `boundBackground()`, `boundNavigationInfo()`,
`boundFog()`).

The animation set (TimeSensor, the full interpolator family, the ROUTE cascade)
and the view-dependent sensors (Proximity/Visibility/LOD/Billboard) are built in.
You add your own behavior by subclassing `sdk::System` (`attach` + `update`) and
registering it with `ctx.addSystem(sys)`. From inside `update`, write fields with
`ctx.writeField(node, "field", value)` — it is dirty-aware so the extractor sees
the change. (See `examples/03_attach_behavior_tick.cpp`.)

### 3. Extract (render feed)

```cpp
sdk::SceneExtractor ex(ctx, doc.scene);
sdk::RenderDelta f0 = ex.fullSnapshot();          // upload everything once
for (auto id : f0.added) {
    const sdk::RenderItem &ri = ex.item(id);      // ri.mesh, ri.material, ri.worldTransform
}

const auto t0 = std::chrono::steady_clock::now();
while (running) {
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();  // seconds since start
    ctx.tick(now);
    sdk::RenderDelta d = ex.delta();              // incremental: what changed this tick
    // d.updatedTransform / d.updatedGeometry / d.added / d.removed
}
```

`fullSnapshot()` returns every item in `added` and initializes the reverse
indices. `delta()` returns only what changed since the last tick. Per-frame
descriptors: `camera()`, `lights()` / `snapshotLights()` + `lightsOf(id)`,
`background()`, `sceneWorldBounds()`. `skippedGeometryCounts()` reports geometry
types the extractor does not yet emit.

`MeshData` shading contract for the consumer: if `topology != Triangles` **or**
`!hasNormals`, bind an unlit program and skip back-face culling.

## The tick model (threading)

The SDK is single-threaded and synchronous. One `X3DExecutionContext` and one
`SceneExtractor` per loaded document, driven from one thread:

1. push inputs (`setPointer*`, `setKey`) since the last tick,
2. `ctx.tick(now)` once,
3. `ex.delta()` **at most once** per tick (asserted — the dirty set is cleared by
   the next tick).

There is no internal threading and no hidden IO on the tick path. If your asset
or font seams are async, return `Pending` and retry on a later frame.

## The seams (embedder-supplied IO)

A **seam is a port** — in the ports-and-adapters (hexagonal) sense, a.k.a. a
Service Provider Interface (SPI): the IO-free core defines a frozen interface, and
the embedder supplies the *adapter* (here called a *backend*) that does the actual
IO. Every seam is proven backend-agnostic by a second independent implementation
plus a swap-test, so the contract — not any one backend — is what's frozen.

The core is IO-free apart from the parse path's own local-file reads
(`parseFile`). Decoding and rasterization live in the embedder, supplied as
`std::function` callbacks. Each seam carries its own stability marker:
**[STABLE]** seams are frozen pre-v2 (a breaking change is a major bump),
**[EXPERIMENTAL]** seams are usable but may gain fields. See the
[seam status matrix](../wiki/seam-status.md) for the current per-seam state —
do not assume a blanket answer.

| Seam | Type | What you supply |
|---|---|---|
| Asset | `sdk::AssetResolver` | bytes for a url (`AssetKind` Texture/Movie/Inline/ExternProto). Render-time may return `Pending`; parse-time (Inline/ExternProto) must be sync `Ready`/`Failed`. |
| Texture | `sdk::TextureResolver` | decoded RGBA pixels for a url (`width`, `height`, bytes). The SDK threads the result onto `TextureRef`. |
| Font | `sdk::FontMetrics` | per-glyph advance + optional atlas UV / outline. The SDK does all Text layout. Defaults to `makeMonospaceStub()`. |
| Geo | `sdk::GeoProjection` | geographic coordinate → local Cartesian, supplied via `MeshBuildOptions::geoProjection`. Empty = flat-fallback (unanchored). |
| Script | `sdk::ScriptEngine` | a language backend (e.g. ECMAScript). Wrap in `sdk::ScriptSystem`, register with `ctx.addScriptSystem(ss)`. `SaiContext` is the backend↔runtime channel. |

### Implementing a seam

A seam impl is just a value you hand the SDK: a `std::function` you assign
(Asset/Texture/Font/Geo) or, for Script, an abstract class you subclass. Two
minimal ports:

**1 — wire a shipped canonical.** The one seam impl that ships ready-to-use in the
installed package is the monospace `FontMetrics` stub (also the default). Opting
in explicitly is one line — it lets `Text` lay out as fixed-width glyph cells with
no font backend:

```cpp
sdk::MeshBuildOptions opts;
opts.fontMetrics = sdk::makeMonospaceStub();      // shipped canonical FontMetrics port
sdk::SceneExtractor ex(ctx, doc.scene, opts);
```

**2 — write a contrived one (no IO).** A reduced version of cpu_raster's `proc:`
texture seam: it handles only `proc:` urls by *generating* pixels — no file, no
decoder — and fails everything else so a real decoder can be chained behind it.
This is the `TextureResolver` port reduced to its essence:

```cpp
sdk::TextureResolver proc = [](const std::string &url) {
  if (url.rfind("proc:", 0) != 0)                 // not a proc: url -> not ours
    return sdk::TexturePixelResult::makeFailed();
  sdk::TexturePixels px;
  px.width = px.height = 2;
  px.rgba = { 255,0,255,255,  0,0,0,255,          // bottom row: magenta, black   (RGBA8,
              0,0,0,255,  255,0,255,255 };         // top row:    black, magenta    origin bottom-left)
  return sdk::TexturePixelResult::makeReady(std::move(px));
};
sdk::SceneExtractor ex(ctx, doc.scene, /*meshOptions=*/{}, proc);
// <ImageTexture url="proc:checker"/> now samples the generated checkerboard.
```

The shipped defaults aren't inert: `makeMonospaceStub` produces real fixed-pitch
Text layout (advanceEm = 0.6), and the geo seam flat-falls-back to a real planar
(geographically unanchored) grid — basic but functional. Only the Texture and
Asset resolvers default to no-ops (`makeNullTextureResolver` always fails → white
fallback). What's *not* in the installed package is the rich IO — FreeType/stb
fonts, stb/wuffs texture decode, QuickJS/Duktape scripts, miniaudio audio. Those
live in the source tree as reference adapters; surfacing them to installed
embedders (one labeled file per seam, built via `find_package`) is the queued
reference-consumer work.

## Capability matrix

See [v1-capabilities.md](v1-capabilities.md) for what works in v1 and what is
deferred to post-v1 (with reasons).

## Examples

Runnable, headless, built + run as ctests (`X3D_CPP_BUILD_EXAMPLES=ON`):

- `examples/01_load_validate_convert.cpp` — load, report conformance, convert to all encodings.
- `examples/02_extract_render_feed.cpp` — extract render items + camera + lights (the renderer-feed path).
- `examples/03_attach_behavior_tick.cpp` — register a custom `System` and drive ticks.
- `examples/embed_minimal/` — installed-package smoke project for embedders using `find_package(x3d_cpp)`.

`examples/poc_renderer/` is a full out-of-SDK OpenGL consumer (off by default,
`X3D_CPP_BUILD_POC=ON`).
