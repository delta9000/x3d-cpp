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

The session snippets assume you have linked `x3d_cpp::sdk`, included
`x3d/sdk.hpp`, and declared `namespace sdk = x3d::sdk;`. They use `doc` for an
already parsed document. The font and script recipes below are complete
programs with their own build instructions.

## Pick the seam

| You want the scene to | Supply | Default without it |
|---|---|---|
| Show images | `sdk::TextureResolver` (decoded RGBA for a URL) | Always fails; textured surfaces use the white fallback |
| Resolve resources watched by `LoadSensor` | `SessionOptions::assetResolver` | No bytes |
| Load Inline and EXTERNPROTO while parsing | Both `parseDocument` resolver callbacks | Local-file resolution |
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
your own background load and return `Pending`. Consume `delta()` after each
tick to retry pending texture and geometry resources and receive completion
through `updatedMaterial`, `updatedGeometry` or `added`. `LoadSensor` retries
its byte requests during ticks. Parse-time requests (Inline and EXTERNPROTO)
can't be pending: they must answer `Ready` or `Failed`.

## Prepare the backend example

The font and script recipes need a C++20 compiler, CMake 3.21 or later, Ninja,
and an x3d-cpp checkout. The bundled stb font adapter and Duktape script engine
are source-tree targets. Create an empty directory outside the checkout and
save this as `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(resource_host LANGUAGES CXX)

add_subdirectory("${X3D_CPP_SOURCE_DIR}" x3d-cpp EXCLUDE_FROM_ALL)
add_executable(resource_host main.cpp)
target_link_libraries(resource_host PRIVATE
  x3d_cpp::sdk x3d_stbtt x3d_ecmascript_backend)
```

The installed package exports the callback interfaces; these two backend
targets are available when building from source. An installed-package host can
instead supply its own `FontMetrics` or `ScriptEngine` implementation.

Choose [Use a real font](#use-a-real-font) or [Run scripts](#run-scripts), then
save that recipe's `main.cpp` beside `CMakeLists.txt` before configuring or
building. Each recipe works independently. The first build compiles the SDK
and takes a few minutes.

## Use a real font

First [prepare the backend example](#prepare-the-backend-example). Save the
following program as `main.cpp`. It maps the scene's `SANS` family to a font
file and passes its metrics through `SessionOptions::meshOptions`:

```cpp
#include "x3d/sdk.hpp"
#include "StbttFontMetrics.hpp"
#include <iostream>

namespace sdk = x3d::sdk;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: resource_host /path/to/font.ttf\n";
    return 1;
  }
  auto metrics = x3d::runtime::io::stbtt::makeStbttFontMetrics(
      {{"SANS", argv[1]}});
  if (!metrics({"SANS", "PLAIN", 'A'}).ready()) {
    std::cerr << "font or glyph unavailable\n";
    return 1;
  }

  sdk::SessionOptions options;
  options.meshOptions.fontMetrics = metrics;
  auto doc = sdk::parseDocument(R"(
    <X3D profile="Immersive" version="4.0"><Scene><Shape>
      <Text string='"Wi"'><FontStyle family='"SANS"' style="PLAIN"/></Text>
    </Shape></Scene></X3D>)");
  auto session = sdk::RuntimeSession::create(std::move(doc), options);
  auto snapshot = session->fullSnapshot();
  std::cout << "glyph ready; placements=" << snapshot.added.size() << '\n';
}
```

From your example directory, configure, build and run with the checkout's
bundled font. Replace `/path/to/x3d-cpp` with your checkout path in both places:

```sh
cmake -S . -B build -G Ninja \
  -DX3D_CPP_SOURCE_DIR=/path/to/x3d-cpp \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_ENABLE_DEV_TOOLING=OFF \
  -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_SCRIPT=ON
cmake --build build --target resource_host --parallel 2
./build/resource_host /path/to/x3d-cpp/third_party/fonts/LiberationSans-Regular.ttf
```

```text
glyph ready; placements=1
```

The adapter supplies real glyph advances and boxes for PLAIN faces. It does
not supply a glyph atlas; to draw rasterized glyphs, wire an atlas as described
in [Text extraction](../reference/subsystems/extract-text.md#optional-glyph-atlas).
The SDK performs layout using the supplied metrics. See
[Font metrics](../reference/subsystems/system-font-metrics.md).

## Run scripts

First [prepare the backend example](#prepare-the-backend-example). Save the
following program as `main.cpp`, replacing it if you already tried the font
recipe. It uses the bundled Duktape backend, registers a script system, and
explicitly enrolls the scene's Script nodes before the first tick:

```cpp
#include "x3d/sdk.hpp"
#include "EcmaScriptBackend.hpp"
#include <iostream>
#include <memory>

namespace sdk = x3d::sdk;

int main() {
  auto doc = sdk::parseDocument(R"(
    <X3D profile="Immersive" version="4.0"><Scene>
      <Script DEF="Reporter"><![CDATA[
        function initialize() { Browser.print("script initialized\n"); }
        function prepareEvents() { Browser.print("script tick\n"); }
      ]]></Script>
    </Scene></X3D>)");
  auto session = sdk::RuntimeSession::create(std::move(doc));
  auto engine = std::make_shared<x3d::runtime::EcmaScriptBackend>();
  auto scripts = std::make_shared<sdk::ScriptSystem>(engine, "resource-host", "1");
  auto &ctx = session->context();
  ctx.addScriptSystem(scripts);
  sdk::forEachNode(session->scene(), [&](sdk::X3DNode *node) {
    scripts->attach(node, ctx);
  });
  auto reporter = session->scene().resolve("Reporter");
  auto *script = dynamic_cast<x3d::nodes::Script *>(reporter.get());
  if (!script || scripts->handleFor(script) == sdk::kInvalidScriptHandle) {
    std::cerr << "script failed to load\n";
    return 1;
  }
  session->tick(0.0);
  std::cout << scripts->saiFor(script)->log();
}
```

From your example directory, configure, build and run. Replace
`/path/to/x3d-cpp` with your checkout path:

```sh
cmake -S . -B build -G Ninja \
  -DX3D_CPP_SOURCE_DIR=/path/to/x3d-cpp \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_ENABLE_DEV_TOOLING=OFF \
  -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_SCRIPT=ON
cmake --build build --target resource_host --parallel 2
./build/resource_host
```

```text
script initialized
script tick
```

`initialize` runs during enrollment; `prepareEvents` runs when the host ticks.
`Browser.print` accumulates text in the script's SAI log, which the host prints
here. For external script URLs, pass an `AssetResolver` as the fourth
`ScriptSystem` constructor argument. Call `setSceneFactory` (for
`Browser.createX3DFromString`) and `setResourceLimits` before enrolling any
script. Call budget, heap and quota defaults are in
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
