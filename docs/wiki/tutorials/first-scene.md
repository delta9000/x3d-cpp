---
title: Load, run and extract your first scene
summary: Build a small host that loads an animated scene, ticks it and reads the render feed.
tags: [tutorial, embedding, extraction]
updated: 2026-11-14
related:
  - ../how-to/build-and-install.md
  - ../how-to/integrate-a-native-host.md
  - ../reference/native-host-contract.md
---

# Load, run and extract your first scene

In this tutorial you write a 40-line host program. It loads a scene with a
spinning box, advances the scene clock, and prints what a renderer would
receive on each frame. By the end you will have used the three calls every
host is built on: `fullSnapshot()`, `tick(now)` and `delta()`.

You need a C++20 compiler, CMake 3.21 or later, Ninja, and an x3d-cpp checkout.

## 1. Install the SDK

From the x3d-cpp checkout:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_BUILD_EXAMPLES=OFF
cmake --build build --parallel 2
cmake --install build --prefix "$PWD/install"
```

The build takes a few minutes. When it finishes, `install/` holds the headers,
the libraries and the CMake package.

## 2. Create the project

Make an empty directory outside the checkout and add `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(first_scene LANGUAGES CXX)

find_package(x3d_cpp CONFIG REQUIRED)

add_executable(first_scene main.cpp)
target_link_libraries(first_scene PRIVATE x3d_cpp::sdk)
```

## 3. Write the scene

Save this as `spin.x3d` in the same directory. A `TimeSensor` drives an
`OrientationInterpolator`, which turns the box once every four seconds.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Transform DEF="Spin">
      <Shape>
        <Appearance><Material diffuseColor="0.8 0.2 0.2"/></Appearance>
        <Box size="2 1 1"/>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
    <OrientationInterpolator DEF="Turn" key="0 0.5 1"
        keyValue="0 1 0 0  0 1 0 3.14159  0 1 0 6.28318"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Turn" toField="set_fraction"/>
    <ROUTE fromNode="Turn" fromField="value_changed" toNode="Spin" toField="rotation"/>
  </Scene>
</X3D>
```

## 4. Load the scene

Create `main.cpp`:

```cpp
#include "x3d/sdk.hpp"

#include <iomanip>
#include <iostream>

namespace sdk = x3d::sdk;

int main() {
  sdk::X3DDocument doc = sdk::parseFile("spin.x3d");
  std::cout << "loaded: version " << doc.version << ", "
            << doc.rangeWarnings.size() << " range warnings\n";
}
```

Build and run it:

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
./build/first_scene
```

```text
loaded: version 4.0, 0 range warnings
```

`parseFile` detected the XML encoding and returned a document. The document
loads even when a field is out of range; such values are reported in
`rangeWarnings`, and this scene has none.

## 5. Create a session and take the first snapshot

Add this before the closing brace of `main`:

```cpp
  auto session = sdk::RuntimeSession::create(std::move(doc));

  sdk::RenderDelta first = session->fullSnapshot();
  for (sdk::RenderItemId id : first.added) {
    const sdk::RenderItem &item = session->extractor().item(id);
    std::cout << "item " << id << ": " << item.mesh->positions.size()
              << " vertices, " << item.mesh->indices.size() << " indices\n";
  }
```

Build and run again:

```text
loaded: version 4.0, 0 range warnings
item 0: 36 vertices, 36 indices
```

The session owns the document, the execution context and the extractor.
`fullSnapshot()` lists every render item in `added`. The scene has one `Shape`,
so there is one item: the box, as 12 triangles. A renderer would upload
`item.mesh`, `item.material` and `item.worldTransform` here.

## 6. Tick the scene and read the deltas

Add the frame loop after the snapshot:

```cpp
  std::cout << std::fixed << std::setprecision(2);
  for (int frame = 0; frame <= 4; ++frame) {
    const double now = frame * 0.5;
    session->tick(now);
    sdk::RenderDelta d = session->delta();
    std::cout << "t=" << now << "  added=" << d.added.size()
              << "  updatedTransform=" << d.updatedTransform.size();
    for (sdk::RenderItemId id : d.updatedTransform) {
      const sdk::Mat4 &world = session->extractor().item(id).worldTransform;
      const auto x = world.transformDirection(x3d::core::SFVec3f{1, 0, 0});
      std::cout << "  local +X -> (" << x.x << ", " << x.y << ", " << x.z << ")";
    }
    std::cout << "\n";
  }
```

Build and run:

```text
loaded: version 4.0, 0 range warnings
item 0: 36 vertices, 36 indices
t=0.00  added=0  updatedTransform=1  local +X -> (1.00, 0.00, 0.00)
t=0.50  added=0  updatedTransform=1  local +X -> (0.71, 0.00, -0.71)
t=1.00  added=0  updatedTransform=1  local +X -> (0.00, 0.00, -1.00)
t=1.50  added=0  updatedTransform=1  local +X -> (-0.71, 0.00, -0.71)
t=2.00  added=0  updatedTransform=1  local +X -> (-1.00, 0.00, -0.00)
```

Each `tick` advanced the clock by half a second. The TimeSensor fired, the
ROUTEs carried its fraction through the interpolator into the Transform, and
`delta()` reported the one placement whose world transform changed. Nothing
was added or removed, and the mesh was never sent again. After two seconds the
box has made half a turn, so its local +X axis points along world -X.

## What you built

You have a host that:

- loads a scene with `parseFile`
- owns it through a `RuntimeSession`
- uploads everything once from `fullSnapshot()`
- advances time with `tick(now)` and applies only the changes from `delta()`

A real host replaces the `std::cout` lines with renderer calls and the fixed
loop with its frame loop.

## Next steps

- [Integrate a native host](../how-to/integrate-a-native-host.md) covers input,
  cameras and the checks to make before accepting a scene.
- [Build and install the SDK](../how-to/build-and-install.md) covers build
  options and optional backends.
- [Native host contract](../reference/native-host-contract.md) has the exact
  rules for deltas, lifetimes and resources.
- [Tour the x3d command line](cli-tour.md) runs the same scene without code.
