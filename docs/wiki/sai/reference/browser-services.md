---
title: SAI browser services
summary: Reference for browser identity, metrics, description, rendering properties, capabilities and dynamic options in the reference kernel.
tags: [sai, reference, kernel, browser]
updated: 2026-11-14
related:
  - ../how-to/publish-browser-information.md
---

# SAI browser services

Header: `<x3d/sai/experimental/browser_services.hpp>`. This is a reference
kernel extension providing an owning API for a bounded set of browser
services. Task guide:
[Publish browser information from a host](../how-to/publish-browser-information.md).

It adds no renderer, navigation engine, codec, transport, browser-reference
disposal, internal Script phase or profile conformance. All these services are
**partial** in the [service register](service-register.md).

## Types

```cpp
struct browser_configuration {
  std::optional<std::string> name;
  std::optional<std::string> version;
};
struct browser_metrics {
  float current_speed = 0.f;
  float current_frame_rate = 0.f;
};
using browser_property_value =
    std::variant<bool, std::int32_t, float, std::string>;
using browser_property_map =
    std::map<std::string, browser_property_value, std::less<>>;
using browser_option_handler =
    std::function<result<bool>(const std::string &,
                               const browser_property_value &)>;
```

## Construction

| Constructor | Identity |
|---|---|
| `browser(type_registry)` | Name and version absent |
| `browser(type_registry, browser_configuration)` | Owns immutable identity values |

An explicitly supplied empty string is distinct from absent identity. Browser
copies share the same service state.

## Client services

```cpp
result<std::optional<std::string>> browser::get_name() const;
result<std::optional<std::string>> browser::get_version() const;
result<float> browser::get_current_speed() const;
result<float> browser::get_current_frame_rate() const;
result<std::string> browser::description() const;
result<void> browser::set_description(std::string);
result<browser_property_map> browser::get_rendering_properties() const;
result<browser_property_map> browser::get_browser_properties() const;
result<bool> browser::set_browser_option(std::string, browser_property_value);
```

Strings, optional strings and maps own their data and survive later updates or
browser destruction. Changing a returned map affects that copy only.

## Host services

```cpp
result<void> browser::publish_metrics(const execution_context &, browser_metrics);
result<void> browser::publish_rendering_properties(browser_property_map);
result<void> browser::set_browser_option_handler(browser_option_handler);
```

The handler setter also accepts a move-only callable.

A successful publication reports supplied facts. It does not mean this library
implements the reported hardware or runtime behaviour.

## Common rules

- Every result-returning service rejects an empty or moved-from wrapper with
  `stale_handle` and its own operation name before touching service state.
  Older raw host-extension methods such as `current_scene()` keep their
  existing preconditions.
- State reads and writes are mutex-serialized.
- These operations are independent of scene edits, event batches and update
  gates. They do not realize nodes, increment revisions, emit graph or field
  notifications, invalidate open edits or enqueue submissions.
- There is no client-browser `dispose` in this slice.

## Description

- Writes are unbuffered and generate no events (abstract §6.3.15).
- The value is browser-owned state that a containing host may use for a title.
  The kernel has no window system; readback does not assert that an OS title
  bar changed.
- Description survives world replacement.

`description()` is an SDK readback extension. The source has `setDescription`;
a browser description getter has not been located in the published binding.
Node fields named `getDescription` are unrelated.

## Metrics

| Rule | Behaviour |
|---|---|
| Units | Average navigation speed for the active layer in the bound viewpoint's base speed units; frame rate in frames per second |
| Context | Must be the current context of this exact browser. Foreign, retired and not-yet-installed contexts return `invalid_context`, even if generation numbers coincide |
| World replacement | Publication is atomic with respect to it. After a different world is installed, reads return zero until its host publishes |
| Validity | Finite and nonnegative (SDK input policy). An invalid sample returns `invalid_value` and does not replace the last valid one |
| Atomicity | The pair publishes atomically. Separate scalar queries are separate reads and can see different publications under concurrent updates |

Samples do not infer speed from a NavigationInfo field, frame rate from event
timestamps, or per-layer navigation behaviour.

Absent name and version, and zero for unsupported speed and frame rate, follow
the published C++ binding (§7.3.3 supplies zero explicitly; the abstract speed
clause specifies no unsupported fallback).

## Rendering properties

The report starts empty. Absence means no supported report is available. It
survives world replacement. Maps publish as complete snapshots, so readers
never see a mixture of two reports.

Reserved Table 6.5 properties are validated on publication:

| Property | Type | Extra rule |
|---|---|---|
| `Shading` | string | Examples, not a closed list; host-specific names preserved |
| `MaxTextureSize` | string | Two positive decimal pixel counts separated by `x` |
| `TextureUnits` | exact `int32_t` | Nonnegative |
| `ColorDepth` | exact `int32_t` | Nonnegative |
| `AntiAliased` | bool | Capitalization differs from the option name |
| `TextureMemory` | exact `float` | Nonnegative and finite |

| Error | Cause |
|---|---|
| `type_mismatch` | Wrong type for a reserved property |
| `invalid_value` | Malformed value |
| `invalid_name` | Empty property name |

Any error rejects the complete update and keeps the previous map. Nonempty
implementation property names with any of the four value alternatives are
accepted as informative metadata. Positivity, finite-number validation and the
rejection diagnostics are reference-provider policies.

## Browser properties (capabilities)

`get_browser_properties()` returns all eight reserved Table 6.6 capabilities as
false:

`ABSTRACT_NODES`, `CONCRETE_NODES`, `EXTERNAL_INTERACTIONS`,
`PROTOTYPE_CREATE`, `DOM_IMPORT`, `XML_ENCODING`, `CLASSIC_VRML_ENCODING`,
`COMPRESSED_BINARY_ENCODING`.

There is no setter that can turn these on without implementing them.
Publishing rendering measurements or installing an option handler does not
alter this map or `capability_set`.

## Dynamic options

Names and values are exact and case-sensitive.

| Option | Values |
|---|---|
| `Antialiased`, `Dashboard`, `EnableInlineViewpoints`, `MotionBlur`, `SplashScreen` | bool |
| `PrimitiveQuality`, `TextureQuality` | `Low`, `Medium`, `High` |
| `QualityWhenMoving` | `Low`, `Medium`, `High`, `Same` |
| `Shading` | `Wireframe`, `Flat`, `Gouraud`, `Phong` |

| Request | Result |
|---|---|
| Unknown name | `invalid_name` |
| Wrong variant alternative | `type_mismatch` |
| String outside the choices | `invalid_value` |
| Valid, no handler installed | Successful `false` (the source's unsupported-dynamic-option result) |
| Valid, handler returns `true` | The option was applied |
| Valid, handler returns `false` | Unavailable or refused |
| Handler returns an error | Preserved |
| Handler throws | Structured `callback_failed`; a later request runs normally |
| Concurrent or reentrant valid request | `operation_in_progress` |

The implementation does not coerce integer booleans, normalize case, trim
strings, accept vendor options, or conflate `AntiAliased` with `Antialiased`.
These structured-error choices are SDK policy; abstract §6.3.26 does not
prescribe a validation error vocabulary.

No map is changed to pretend an option was applied.

## Handler lifetime and reentrancy

- Application code runs outside every browser, scene and service lock. Reads,
  description changes, measurements, handler replacement and other ordinary
  browser operations stay usable inside the handler.
- Replacement creates the new callable before locking and releases the
  previous one after unlocking.
- An in-flight invocation keeps its original handler even if it clears or
  replaces the installed one. The busy flag is cleared before that retained
  callable is destroyed, so its destructor may reenter ordinary services.
- Calls pin service state. Destroying or moving the initiating wrapper from a
  handler does not invalidate the active operation; later calls on a moved-from
  wrapper fail.
- Capturing an owning browser copy in the handler can create a cycle. Clear
  the handler when its ownership is no longer needed.

None of this permits a call into an object whose lifetime has ended or whose
destructor is running, or concurrent mutation of the same C++ wrapper object.

## Sources

- Abstract SAI
  [§§6.3.2–5](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#getName),
  [§6.3.15](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#setDescription),
  [§§6.3.21–22](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#getRenderingProperties)
  and [§6.3.26](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#setBrowserOption)
- Published C++ binding
  [§§7.3.1–4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getName)
  and [§§7.3.20–21](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getRenderingProperties)
- Architecture
  [§9.2.4 and Table 9.2](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#BrowserOptions)
  for exact browser-option names and values

The published binding describes browser properties both as string arrays and
as primitive property arrays. This SDK uses one owning map with ordinary
independent C++ value ownership: no borrowed arrays, cloning protocol or
unchecked object casts.

Profile and component discovery is not provided; see
[Scope and limits](../explanation/scope-and-limits.md#profile-and-component-discovery).

## Evidence

`sai/tests/browser_services_test.cpp` (CTest `sai_browser_services`) covers
optional identity ownership, aliasing, description persistence, finite sample
validation, cross-world rejection, detached property ownership, conservative
capability truth, complete-map failure atomicity, every standard option name
and choice, wrong names, types and values, unavailable, refused, applied and
error outcomes, thrown callbacks, move-only captures, callback reentry,
unlocked resource destruction, wrapper destruction during an active call,
concurrent snapshot publication and option exclusion, and the absence of scene
revision, graph event, field event and update-gate effects.
