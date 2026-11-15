---
title: Animate your first SAI scene
summary: Tutorial that builds a routed scene on the reference backend, sends one input through it and observes the result.
tags: [sai, tutorial]
updated: 2026-11-14
related:
  - native-host.md
---

# Animate your first SAI scene

In this tutorial you write a small program that builds a scene with a box,
routes an interpolator into the box's position, sends one input, and watches
the box move. Along the way you meet every stage of the SAI provider contract:
setup, activation, queued input, the host pump and notifications.

You need a C++20 compiler, CMake 3.24 or newer, Ninja, and an x3d-cpp checkout.
The first build compiles the SDK and takes a few minutes.

## 1. Create the project

Make an empty directory next to your x3d-cpp checkout and add a
`CMakeLists.txt`. Adjust the path if your checkout is somewhere else:

```cmake
cmake_minimum_required(VERSION 3.24)
project(sai_first_scene LANGUAGES CXX)

add_subdirectory(../x3d-cpp/sai sai EXCLUDE_FROM_ALL)

add_executable(first_scene main.cpp)
target_link_libraries(first_scene PRIVATE x3d::sai_reference_provider)
```

## 2. Start a service

Create `main.cpp`:

```cpp
#include <x3d/sai/experimental/bindings/PositionInterpolator.hpp>
#include <x3d/sai/experimental/bindings/Shape.hpp>
#include <x3d/sai/experimental/bindings/Transform.hpp>
#include <x3d/sai/experimental/reference_provider.hpp>

#include <iostream>
#include <stdexcept>
#include <utility>

namespace sai = x3d::sai::experimental;
namespace provider = sai::provider;
namespace bindings = sai::bindings;

// Unwrap a result or stop with its error.
template <class T> T must(sai::result<T> result) {
  if (!result)
    throw std::runtime_error(result.error().operation + ": " +
                             result.error().message);
  return std::move(*result);
}
void must(sai::result<void> result) {
  if (!result)
    throw std::runtime_error(result.error().operation + ": " +
                             result.error().message);
}

void print(const char *label, const sai::vec3f &v) {
  std::cout << label << ": " << v.x << ' ' << v.y << ' ' << v.z << '\n';
}

void run(provider::service &service) {
  std::cout << "backend: " << service.supported().backend_name << '\n';
}

int main() {
  try {
    auto service = must(sai::make_reference_provider());
    run(*service);
    must(service->close());
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
```

Every SAI call returns a `result`. The `must` helper keeps the tutorial short
by turning a failure into an exception.

Build and run:

```sh
cmake -S . -B build -G Ninja
cmake --build build --parallel 3
./build/first_scene
```

```text
backend: reference-kernel
```

Notice that `run` takes a `provider::service &`. Nothing in it will know which
backend it is talking to.

## 3. Build a scene

Add this to the end of `run`:

```cpp
  const auto scene = must(service.create_scene());
  const auto context = must(service.root_context(scene));
  const auto mover = must(service.create_node(context, "Transform"));
  const auto shape = must(service.create_node(context, "Shape"));
  const auto box = must(service.create_node(context, "Box"));
  must(service.author(shape, bindings::Shape::geometry,
                      provider::node_value{box}));
  must(service.author(mover, bindings::Transform::children,
                      provider::node_values{shape}));
  must(service.append_root(context, mover));
  print("at rest", must(service.read(mover, bindings::Transform::translation)));
```

You created three nodes, put the box in the shape, put the shape in the
transform, and made the transform a root of the scene.
`bindings::Transform::translation` is a generated key: it carries the field's
type, so `read` hands you back a `vec3f` directly.

Build and run again:

```text
backend: reference-kernel
at rest: 0 0 0
```

## 4. Add an interpolator and a ROUTE

Add:

```cpp
  const auto path = must(service.create_node(context, "PositionInterpolator"));
  must(service.author(path, bindings::PositionInterpolator::key,
                      sai::float_list{0, 1}));
  must(service.author(path, bindings::PositionInterpolator::keyValue,
                      sai::vec3f_list{{0, 0, 0}, {10, 0, 0}}));
  must(service.append_root(context, path));
  const auto translation =
      must(service.get_field(mover, bindings::Transform::translation));
  must(service.add_route(
      context,
      must(service.get_field(path,
                             bindings::PositionInterpolator::value_changed)),
      translation));
```

The interpolator maps a fraction from 0 to 1 onto a position from the origin
to 10 on the x axis. The ROUTE sends whatever it outputs into the transform's
`translation`.

The program prints the same two lines. The scene is still in *setup*: nothing
runs yet.

## 5. Activate and send an input

Add:

```cpp
  const auto active = must(service.activate(scene));
  const auto client = must(service.connect(active));
  const auto receipt = must(service.enqueue(
      client, path, bindings::PositionInterpolator::set_fraction, 0.5f));
  print("after enqueue",
        must(service.read(mover, bindings::Transform::translation)));
```

Run it:

```text
backend: reference-kernel
at rest: 0 0 0
after enqueue: 0 0 0
```

The box has not moved. `enqueue` succeeded, but that only means the input was
*accepted*. The service does no work until the host tells it to.

## 6. Pump a turn

Add:

```cpp
  const auto turn = must(service.pump(active, {1.0}));
  if (turn.error)
    throw std::runtime_error(turn.error->message);
  print("after pump",
        must(service.read(mover, bindings::Transform::translation)));
  std::cout << "processed: " << std::boolalpha
            << (must(receipt.status()).state ==
                provider::request_state::processed)
            << '\n';
```

Run it:

```text
backend: reference-kernel
at rest: 0 0 0
after enqueue: 0 0 0
after pump: 5 0 0
processed: true
```

`pump` evaluated your queued input at host time 1.0. The interpolator turned
0.5 into `5 0 0`, and the ROUTE delivered it. The receipt you kept from
`enqueue` now reports `processed`.

## 7. Observe the change

Insert this **before** the `pump` call from step 6:

```cpp
  auto subscription = must(service.observe(
      active, translation, [](const provider::notification &notice) {
        print("notified", std::get<sai::vec3f>(notice.value));
      }));
```

And add this after the `processed` line:

```cpp
  std::cout << "draining\n";
  must(service.dispatch_notifications());
```

Run it:

```text
backend: reference-kernel
at rest: 0 0 0
after enqueue: 0 0 0
after pump: 5 0 0
processed: true
draining
notified: 5 0 0
```

Look at the order. The callback did not run during `pump`. It ran only when
you called `dispatch_notifications`. Your host chooses when application
callbacks execute.

## 8. Shut down

Finish `run` with:

```cpp
  subscription.cancel();
  must(service.retire(scene));
```

`main` already closes the service. The output is unchanged, and the program
now releases everything in order.

## What you built

You wrote a program that:

- built a scene graph through one service
- wired behaviour with a ROUTE during setup
- activated the scene and queued an input as a client
- drove evaluation with an explicit pump
- received a notification on your own schedule

The finished example in the repository, `sai/examples/provider_scene.cpp`, is
the same program with extra checks. Run it with `mise run sai-hello`.

## Where next

- [Run a scene on the native runtime](native-host.md): the same contract over
  the real x3d-cpp runtime
- [Run the host loop](../how-to/run-the-host-loop.md): update gates,
  backpressure and failure handling
- [Architecture](../explanation/architecture.md): why the host drives time
