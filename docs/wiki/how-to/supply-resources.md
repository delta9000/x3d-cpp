---
title: Supply resources through the seams
summary: Wire texture, asset, font and script backends into a session and control file access at parse time.
tags: [how-to, seams, embedding]
updated: 2026-11-15
related:
  - ../reference/seam-matrix.md
  - ../explanation/seams.md
  - ../reference/native-host-contract.md
---

# Supply resources through the seams

The runtime asks the host for bytes, pixels and glyph metrics through
callbacks. This guide shows how to supply each one. Why they are callbacks is
in [Seams, ports and backends](../explanation/seams.md); the list of seams and
their state is the [seam matrix](../reference/seam-matrix.md).

## Pick the seam

| You want the scene to | Supply | Default without it |
|---|---|---|
| Show images | `sdk::TextureResolver` (decoded RGBA for a URL) | Always fails; textured surfaces use the white fallback |
| Load bytes (textures, movies, Inline, EXTERNPROTO) | `sdk::AssetResolver` | No bytes |
| Lay out `Text` with a real font | `sdk::FontMetrics` | `makeMonospaceStub()`: fixed-pitch cells, advance 0.6 em |
| Run `Script` nodes | `sdk::ScriptEngine`, wrapped in `sdk::ScriptSystem` | Scripts do not run |
| Use another geospatial projection | `SessionOptions::geoProjection` | Built-in ellipsoid conversion |

## Decode textures

A texture resolver takes a URL and returns pixels, `Pending` or a failure.
This one generates a checkerboard for `proc:` URLs and declines everything
else, so a real decoder can be chained behind it:

```cpp
sdk::TextureResolver proc = [](const std::string &url) {
  if (url.rfind("proc:", 0) != 0)
    return sdk::TexturePixelResult::makeFailed();
  sdk::TexturePixels px;
  px.width = px.height = 2;                       // RGBA8, origin bottom-left
  px.rgba = { 255,0,255,255,  0,0,0,255,          // bottom row
              0,0,0,255,  255,0,255,255 };        // top row
  return sdk::TexturePixelResult::makeReady(std::move(px));
};

sdk::SessionOptions options;
options.textureResolver = proc;
auto session = sdk::RuntimeSession::create(std::move(doc), options);
```

`<ImageTexture url="proc:checker"/>` now samples the generated pixels.

## Resolve URLs against the scene's location

The plain callbacks receive only the authored URL. To resolve relative URLs,
use the variants that also receive the requesting node's base:

```cpp
sdk::SessionOptions options;
options.baseUrl = sceneDirectory;
options.textureResolverWithBase = [](const std::string &url,
                                     const std::string &baseUrl) {
  return hostDecode(url, baseUrl);   // host joins, confines, fetches, decodes
};
options.assetResolverWithBase = [](const std::string &url, sdk::AssetKind kind,
                                   const std::string &baseUrl) {
  return hostFetch(url, kind, baseUrl);
};
```

- These take precedence over `textureResolver` and `assetResolver`.
- The URL stays as authored. The base comes from the originating node,
  including Inline and PROTO origins, and falls back to `options.baseUrl` only
  when the node has no recorded base.
- They are experimental; see
  [ADR-0063](../explanation/decisions/0063-resolver-base-urls.md).

## Load slowly without blocking

Callbacks run on the thread that called `tick` or `delta`. For slow IO, start
your own background load and return `Pending`. The extractor asks again on
later ticks and reports completion through `updatedMaterial`,
`updatedGeometry` or `added`. Parse-time requests (Inline and EXTERNPROTO)
can't be pending: they must answer `Ready` or `Failed`.

## Use a real font

```cpp
sdk::MeshBuildOptions opts;
opts.fontMetrics = sdk::makeMonospaceStub();   // or your own FontMetrics
sdk::SceneExtractor ex(ctx, doc.scene, opts);
```

The SDK does the text layout; your callback supplies per-glyph advance and,
optionally, atlas UVs or outlines. See
[Font metrics](../reference/subsystems/system-font-metrics.md).

## Run scripts

The bundled Duktape engine is the in-tree target `x3d_ecmascript_backend`; a
consumer of the installed package implements `sdk::ScriptEngine` instead.

1. Bind the context to the scene's author fields, build it, and attach the
   standard runtime.
2. Wrap the engine in a `sdk::ScriptSystem`. Pass an `AssetResolver` as the
   fourth argument if scripts load external urls; set the scene factory, and
   call `setResourceLimits` if you need other quotas, before enrolling any
   script.
3. Enroll every Script node with `attach`, then register the system with
   `ctx.addScriptSystem`. It runs ahead of the other systems and delivers
   routed events to script handlers itself.

```cpp
#include "x3d/sdk.hpp"
#include "EcmaScriptBackend.hpp"  // in-tree target x3d_ecmascript_backend

sdk::X3DExecutionContext ctx(doc.scene.authorFields);
ctx.buildSceneGraph(doc.scene);
ctx.buildFrom(doc.scene);
x3d::runtime::attachStandardRuntime(doc.scene, ctx);

auto engine = std::make_shared<x3d::runtime::EcmaScriptBackend>();
auto scripts = std::make_shared<sdk::ScriptSystem>(engine, "my-host", "1.0");
scripts->setSceneFactory(x3d::codec::makeStringSceneFactory());

sdk::forEachNode(doc.scene, [&](sdk::X3DNode *node) {
  if (node->nodeTypeName() == "Script") scripts->attach(node, ctx);
});
ctx.addScriptSystem(scripts);

ctx.tick(0.0);
```

With a Script whose `initialize()` sets an `outputOnly` SFVec3f to `1 2 3` and
a ROUTE to a Transform's `set_translation`, reading the translation after the
first tick prints:

```text
translation: 1 2 3
```

`scripts->saiFor(script)->log()` returns what the script printed. Call budget,
heap and quota defaults are in
[Script and SAI runtime](../reference/subsystems/system-script-sai.md#limits);
what script code can call is in the [Script ECMAScript API](../reference/script-api.md).

## Control file access at parse time

`parseFile` and the default resolvers of `parseDocument` read local files for
Inline and EXTERNPROTO. To decide every read yourself, parse from memory and
pass both resolvers explicitly. To forbid access, pass resolvers that refuse;
an empty callback is not a deny policy. See
[recursive asset loader policy](../reference/subsystems/asset-loader-policy.md).

## Build the optional backends

Ready-made backends for decoding, fonts, fetching and scripts are behind build
options; see [Build and install the SDK](build-and-install.md#turn-on-optional-backends)
and the per-seam pages linked from the [seam matrix](../reference/seam-matrix.md).

## Related

- [Native host contract](../reference/native-host-contract.md)
