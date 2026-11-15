---
title: Author and inspect with the SAI reference kernel
summary: Build a scene with atomic edits, discover the vocabulary, read through snapshots and preflight a commit.
tags: [sai, how-to, kernel]
updated: 2026-11-14
related:
  - ../reference/kernel.md
---

# Author and inspect with the SAI reference kernel

The reference kernel offers atomic edits and immutable snapshots on top of the
generated catalog. It is a reference-only extension, so code written against it
does not run on the native backend. Exact behaviour is in
[Reference kernel](../reference/kernel.md).

Link `x3d::sai_reference_metadata`. A complete program is
`sai/examples/generated_author_inspect.cpp`.

## Create a browser and scene

```cpp
#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"
#include "x3d/sai/experimental/bindings/Shape.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"

namespace sai = x3d::sai::experimental;
namespace bindings = sai::bindings;

auto registry =
    sai::generated_type_registry_for<bindings::Shape, bindings::Transform>();
if (!registry) return EXIT_FAILURE;

sai::browser browser{std::move(registry).value()};
auto scene = browser.current_scene();
```

## Discover the vocabulary

To build a palette or inspector before any node exists:

```cpp
const auto choices = scene.snapshot().node_types();
for (const auto &type : choices) {
  if (type.abstract)
    continue;
  // Display type.name and choose field editors from type.fields.
}
```

Pass a chosen `type.name` to `edit.create_node(type.name)` and check the
result.

## Make an atomic edit

```cpp
auto edit = scene.edit();
auto transform = edit.create<bindings::Transform>();
auto shape = edit.create<bindings::Shape>();
if (!transform || !shape) return EXIT_FAILURE;

const auto translation = transform->field(bindings::Transform::translation);
const auto children = transform->field(bindings::Transform::children);
const std::array child_nodes{*shape};

if (!edit.set(translation, sai::vec3f{1, 2, 3}) ||
    !edit.set(children, child_nodes) ||
    !edit.append_root(*transform) ||
    !edit.commit())
  return EXIT_FAILURE;
```

Nothing is visible until `commit()` succeeds, and a failed commit publishes
nothing.

## Configure initializeOnly fields later

A node that is committed but not yet attached stays in setup. Create it in one
edit, then set its initializeOnly fields in a later edit before you attach it.
Attaching a node (as a child, a field value or a root) ends its setup.

## Set scene metadata

```cpp
auto edit = scene.edit();
if (!edit.set_metadata("title", "A world")) return EXIT_FAILURE;
if (!edit.set_metadata("obsolete", std::nullopt)) return EXIT_FAILURE;
auto committed = edit.commit();
```

## Read through a snapshot

```cpp
const auto snapshot = scene.snapshot();

auto value = snapshot.read(translation);              // owning copy
auto kids  = snapshot.read_nodes(children);            // usable handles
auto typed = snapshot.as<bindings::Shape>(kids->front());
auto view  = snapshot.read_view(translation);          // pins storage, no copy
```

Use `read_view` for large values you only inspect. Release it when you are
done, because it keeps its whole revision alive.

## Assign NULL to a node field

```cpp
edit.set(geometry, std::nullopt);
```

Pass node handles, never raw IDs. IDs you get from `snapshot.read` on a node
field are inspection data only.

## Preflight before committing

```cpp
if (auto ok = edit.validate(); !ok) {
  // ok.error() names the node and field that close a containment cycle.
  // The edit is intact: fix it and validate again.
}
```

`validate()` runs the same checks as `commit()` without consuming the edit. It
reserves nothing, so a concurrent change can still make the later commit fail.

## Recover from a failure

| What failed | Do |
|---|---|
| `validate()` or a commit-time graph error | Correct the edit and try again |
| A mutation call returned an error | The edit is poisoned; start a fresh `scene.edit()` |
