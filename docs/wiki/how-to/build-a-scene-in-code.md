---
title: Build a scene in code
summary: Construct an X3D document with SceneBuilder and write it in any encoding.
tags: [how-to, authoring]
updated: 2026-11-14
related:
  - ../reference/subsystems/asset-import.md
  - ../reference/subsystems/codecs-writers.md
---

# Build a scene in code

`SceneBuilder` builds an `X3DDocument` with checks at the call site. It needs
only the slim authoring target: no parser, simulation or extraction.

## 1. Link the authoring target

```cmake
find_package(x3d_cpp CONFIG REQUIRED)
target_link_libraries(my_tool PRIVATE x3d_cpp::authoring)
```

```cpp
#include "x3d/authoring.hpp"
```

A host that already links `x3d_cpp::sdk` gets the same class as
`sdk::SceneBuilder`.

## 2. Add nodes

`add` puts each child in the field its type belongs in and throws
`std::invalid_argument` if the parent has no such field.

```cpp
x3d::authoring::SceneBuilder scene;
auto spin = scene.add<x3d::nodes::Transform>().def("Spin");
spin.add<x3d::nodes::Shape>().add<x3d::nodes::Sphere>().set("radius", "2");
```

- `add<T>()` takes a C++ node type; `add("TypeName")` takes an X3D type name.
- `set(field, "text")` parses X3D attribute text; `set(field, value)` stores a
  typed value; `with(fn)` hands you the node for its typed setters.
- `def(name)` names the node.

## 3. Add ROUTEs

```cpp
auto clock = scene.add<x3d::nodes::TimeSensor>().set("loop", "true");
auto turn = scene.add("OrientationInterpolator")
                .set("key", "0 1").set("keyValue", "0 1 0 0  0 1 0 3.14");
scene.route(clock, "fraction_changed", turn, "set_fraction");
scene.route(turn, "value_changed", spin, "rotation");
```

`route` rejects an unknown field, a wrong direction and mismatched field types.
An endpoint without a DEF gets a generated name.

## 4. Write the document

```cpp
std::cout << x3d::authoring::XmlWriter{}.writeDocument(scene.document());
```

`JsonWriter` and `VrmlWriter` have the same call. See
[Codec writers](../reference/subsystems/codecs-writers.md).

## What the builder does not check

Whether a child's node type is legal in its slot, and value ranges. Every call
and what makes it throw is in the
[SceneBuilder reference](../reference/subsystems/asset-import.md#scenebuilder).
A complete program is `examples/embed_minimal/authoring.cpp`.
