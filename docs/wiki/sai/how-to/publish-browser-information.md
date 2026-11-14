---
title: Publish browser information from a SAI host
summary: Give a reference-kernel browser its identity, metrics, rendering properties and a dynamic option handler.
tags: [sai, how-to, kernel, browser]
updated: 2026-11-14
related:
  - ../reference/browser-services.md
---

# Publish browser information from a SAI host

Clients ask a browser for its name, speed, frame rate and rendering
properties. The kernel does not measure any of that; your host supplies it.
Types, validation and error codes are in
[Browser services](../reference/browser-services.md).

## Set the identity

Identity is fixed at construction:

```cpp
#include <x3d/sai/experimental/browser_services.hpp>

sai::browser browser{std::move(registry),
                     sai::browser_configuration{.name = "My Host",
                                                .version = "1.2"}};
```

Leave a member unset to report it as absent.

## Publish speed and frame rate

Publish against the browser's *current* scene context:

```cpp
auto published = browser.publish_metrics(
    scene, sai::browser_metrics{.current_speed = 1.5f,
                                .current_frame_rate = 60.f});
```

Values must be finite and nonnegative. After you install a different world,
clients read zero until you publish for that world.

## Publish rendering properties

Publish the whole map each time. A single bad entry rejects the whole update
and keeps the previous map.

```cpp
sai::browser_property_map properties{
    {"Shading", std::string{"Phong"}},
    {"MaxTextureSize", std::string{"4096x4096"}},
    {"TextureUnits", std::int32_t{16}},
    {"AntiAliased", true},
    {"ColorDepth", std::int32_t{24}},
    {"TextureMemory", 512.f},
};
auto published = browser.publish_rendering_properties(std::move(properties));
```

Use the exact types shown: `std::int32_t`, not `int` of another width, and
`float`, not `double`. You may add your own property names.

## Handle dynamic options

Without a handler, every valid `set_browser_option` request succeeds with
`false` (unsupported). Install one to apply options:

```cpp
browser.set_browser_option_handler(
    [&](const std::string &name,
        const sai::browser_property_value &value) -> sai::result<bool> {
      if (name == "Shading")
        return renderer.set_shading(std::get<std::string>(value));
      return false;   // valid option, not supported here
    });
```

- Return `true` only after the real effect succeeded.
- Return `false` when the option is unavailable or refused.
- Avoid partial effects on failure. The kernel cannot roll back your code.

The handler only sees names and values that already passed validation.

## Remove the handler

Install an empty handler. Do this before teardown if the handler captures an
owning copy of the browser, or the two will keep each other alive.

## Set the description

```cpp
browser.set_description("Kelp forest");
```

This stores a string. If your host has a window title, read it back with
`browser.description()` and apply it yourself.
