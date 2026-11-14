# x3d-cpp

A C++20 headless X3D runtime SDK. Parse XML, ClassicVRML, VRML97, or JSON,
advance scene behavior, and extract meshes, materials, lights, and cameras for
an application renderer. The node bindings are generated from the X3D 4.0
Unified Object Model. Behavioral conformance is tracked in the
[findings register](docs/conformance/findings.yaml).

The [documentation](docs/wiki/index.md) is one tree of tutorials, how-to
guides, reference and explanation. Start with the
[first-scene tutorial](docs/wiki/tutorials/first-scene.md), or serve the tree
locally with `mise run docs`.

## Gallery — real X3D, rendered headless

The [CPU rasterizer example](examples/cpu_raster/) produces these images.

| | |
|---|---|
| ![Textured lion bust](docs/images/gallery/lion-head.png) | ![PhysicalMaterial spheres in a skybox](docs/images/gallery/skybox-pbr.png) |
| [CC0 lion model](https://polyhaven.com/a/lion_head) | PhysicalMaterial and Background |
| ![Metallic-roughness grid](docs/images/gallery/pbr-grid.png) | ![RGB three-point lighting](docs/images/gallery/rgb-lighting.png) |

### Animated — interpolators over time

The CPU example steps the scene clock with `--animate`; ffmpeg encodes the frames.

![Kelp Forest flythrough](docs/images/gallery/anim-kelp.gif)

The [Kelp Forest example](examples/cpu_raster/assets/kelp_flythrough/) uses the
NPS/MOVES exhibit, fetched separately with attribution. See the
[animation demos](docs/wiki/reference/examples/cpu-raster.md#animation-demos) for reproduction.

## Supported platforms & versions

The [CI workflow](.github/workflows/ci.yml) defines compiler and platform checks.
The CMake package requires C++20 and CMake 3.21 or later.

Readers accept X3D 3.0–4.1 version declarations; the generated node model is 4.0.
Loading a newer version declaration does not add its node types. The XML reader
keeps unknown elements as opaque content, so XML to XML conversion preserves them. See the
[parse readers](docs/wiki/reference/subsystems/parse-readers.md) for version handling.

## Quick start

Requires a C++20 compiler, CMake 3.21 or later, and Ninja.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_BUILD_EXAMPLES=OFF
cmake --build build --parallel 2
cmake --install build --prefix "$PWD/install"
```

Try the command-line tool:

```bash
build/x3d validate scene.x3d
build/x3d convert scene.x3dv -o scene.x3d -f xml
build/x3d sim scene.x3d --ticks 120
build/x3d extract scene.x3d -o scene.glb
```

Or embed the SDK:

```cmake
find_package(x3d_cpp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE x3d_cpp::sdk)
```

```cpp
#include "x3d/sdk.hpp"
namespace sdk = x3d::sdk;

auto session = sdk::RuntimeSession::create(sdk::parseFile("scene.x3d"));
auto initial = session->fullSnapshot();   // upload every item once
// each frame:
session->tick(now);                       // seconds on a monotonic clock
auto changes = session->delta();          // apply only what changed
```

## Where to go next

| You want to | Read |
|---|---|
| Build a first host step by step | [Load, run and extract your first scene](docs/wiki/tutorials/first-scene.md) |
| Try the tool without code | [Tour the x3d command line](docs/wiki/tutorials/cli-tour.md) |
| Build, install and link | [Build and install the SDK](docs/wiki/how-to/build-and-install.md) |
| Integrate with a renderer and event loop | [Integrate a native host](docs/wiki/how-to/integrate-a-native-host.md) |
| Build scenes in code | [Build a scene in code](docs/wiki/how-to/build-a-scene-in-code.md) |
| See what is covered | [SDK capabilities](docs/wiki/reference/capabilities.md) |
| Render headless, convert assets, run the examples | [Examples](docs/wiki/reference/examples/index.md) |
| Drive a scene through the Scene Access Interface | [SAI SDK](docs/wiki/sai/index.md) |
| Regenerate the bindings | [Generator](docs/wiki/reference/subsystems/generator.md) and [Regenerate the golden bindings](docs/wiki/how-to/regenerate-goldens.md) |
| Contribute | [CONTRIBUTING.md](CONTRIBUTING.md) |

## License

[MIT](LICENSE) © 2026 Ben Sandbrook. Third-party components and model sources
are credited in [NOTICE](NOTICE).
