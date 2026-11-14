---
title: Declare SAI scene units and metadata
summary: Record UNIT declarations and scene META entries during setup and read them back later.
tags: [sai, how-to, provider, units]
updated: 2026-11-14
related:
  - ../reference/scene-information.md
  - ../explanation/canonical-units.md
---

# Declare SAI scene units and metadata

Both are setup operations. Exact rules are in
[Scene units and metadata](../reference/scene-information.md).

## Check the capabilities

```cpp
const auto caps = service.supported();
const bool units_ok = caps.supports(provider::service_id::scene_units);
const bool meta_ok  = caps.supports(provider::service_id::scene_metadata);
```

## Declare a unit

Do this **before creating any node**, detached nodes included:

```cpp
auto scene = service.create_scene();
auto declared = service.declare_unit(
    *scene, sai::unit_declaration{.category = "length",
                                  .name = "centimetre",
                                  .conversion_factor = 0.01});
```

- One declaration per category: `angle`, `force`, `length`, `mass`.
- The name must be nonempty with no whitespace; the factor finite and positive.
- A rejected call changes nothing, and you can try again with a valid one.

!!! note "Values stay in SI"
    A declaration records the document's units. It does not rescale anything
    you pass to or read from the provider. Keep authoring in metres, radians,
    newtons and kilograms. See [Canonical units](../explanation/canonical-units.md).

## Read the effective units

```cpp
auto context = service.root_context(*scene);
auto units = service.units(*context);
// Always four entries, in angle, force, length, mass order.
```

Undeclared categories report the SI default with factor 1.

## Set, replace and remove metadata

```cpp
service.set_metadata(*scene, "title", "A world");        // append
service.set_metadata(*scene, "title", "A better world"); // replace, same position
service.set_metadata(*scene, "obsolete", std::nullopt);  // remove
```

## Read metadata

```cpp
auto title = service.metadata(*scene, "title");
if (title && *title) { /* **title is the value */ }

auto all = service.metadata(*scene);   // entries in insertion order
```

`metadata(scene, key)` returns an optional string inside the result, so a
missing key and an empty value are distinguishable.

## After activation

You can still read units and metadata in active and faulted scenes. You cannot
change them: there is no live `setMetaData` in this slice.
