# Browser information and host configuration

## Source and scope

This is a modern owning API for a bounded set of browser services. Where the
published C++ binding describes browser properties both as string arrays and as
primitive property arrays, this SDK uses one owning map (see below). It does not add a renderer, navigation engine, codec,
transport, browser-reference disposal, internal Script phases or profile
conformance.

The source comparison uses:

- Abstract SAI [§§6.3.2–5](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#getName),
  [§6.3.15](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#setDescription),
  [§§6.3.21–22](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#getRenderingProperties)
  and [§6.3.26](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#setBrowserOption)
- The designated published C++ binding's
  [§§7.3.1–4](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getName)
  and [§§7.3.20–21](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getRenderingProperties)
- Architecture [§9.2.4 and Table 9.2](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#BrowserOptions)
  for exact browser-option names and values

Name/version absence and zero unsupported speed/frame rate follow the designated
C++ source. The abstract speed clause describes the active layer's navigation
measurement without specifying an unsupported fallback; C++ §7.3.3 explicitly
supplies zero. `description()` is a native owning readback extension: the source
has `setDescription`, but the browser description getter has not been located
in the designated binding. Node fields named `getDescription` are unrelated.

These services are **partial** in the conformance register. Their concrete
kernel behavior is covered by executable tests; the missing browser lifecycle,
layer/navigation and real host facilities remain visible gaps. A successful
host-publication call reports supplied facts, not that this library implements
the reported hardware or runtime behavior.

## Public values and API

`<x3d/sai/experimental/browser_services.hpp>` provides:

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

The existing `browser(type_registry)` constructor remains available and supplies
absent name/version. The overload `browser(type_registry, browser_configuration)`
owns immutable identity values. An explicitly supplied empty string remains
distinct from absent identity. Browser copies share the same service state.

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

Strings, optional strings and maps own their returned data and survive later
updates or browser destruction. Changing a returned map affects that copy only.
This replaces the published immutable-array intent with ordinary independent
C++ value ownership; no borrowed arrays, cloning protocol or unchecked object
casts are exposed.

Host extensions publish actual measurements/properties and install behavior:

```cpp
result<void> browser::publish_metrics(const execution_context &, browser_metrics);
result<void> browser::publish_rendering_properties(browser_property_map);
result<void> browser::set_browser_option_handler(browser_option_handler);
```

The handler setter also accepts a move-only callable. The setter owns its
captures with the same shared-callable mechanism as the kernel's other callback
services.

## Immediate state and lifetime

Description writes are unbuffered and generate no events, as specified by
abstract §6.3.15. The value is browser-owned description state that a containing
host may use for a title or other UI; this kernel has no window system. Description
readback does not assert that an OS title bar was updated.

All metadata operations are independent of scene edits, field-event batches and
external-session update gates. They do not realize nodes, increment scene
revisions, emit graph/field notifications, invalidate open edits, or enqueue
external-session submissions. An option handler can independently perform
explicit scene edits through ordinary APIs; those application effects are not
implicit metadata mutations. Description and rendering properties survive world
replacement.

Host metrics publication is atomic with respect to world replacement. The
supplied execution context must be the current context of this exact browser;
foreign, retired and not-yet-installed contexts return `invalid_context` even if
their generation numbers coincide. Measurements are associated with that world
generation. After installing a different world, reads return zero until its
host publishes measurements. Samples do not infer speed from a NavigationInfo
field, frame rate from event timestamps, or per-layer navigation behavior.

The host reports average navigation speed for the active layer in the bound
viewpoint's base speed units and frame display rate in frames per second.
Finite nonnegative values are the SDK's input policy; invalid samples
return `invalid_value` without replacing the last valid sample. The pair is
published atomically; separate scalar queries remain separate reads and can
observe different publications under concurrent updates.

State reads/writes are mutex-serialized. Property maps are published as complete
snapshots, so readers cannot observe a mixture of two rendering reports.
Browser wrappers retain ordinary kernel ownership; this slice does not add
client-browser `dispose` or its required post-disposal errors. Every new
result-returning service rejects an empty/moved-from wrapper with `stale_handle`
and its own operation name before touching service state. This does not change
the preconditions of older raw host-extension methods such as `current_scene()`.

## Rendering properties and capabilities

The rendering report starts empty. Absence means no supported report is
available. Host publication validates the reserved Table 6.5 properties:

- `Shading` and `MaxTextureSize` are strings
- `TextureUnits` and `ColorDepth` are exact `int32_t` values
- `AntiAliased` is a boolean (its capitalization differs from the option)
- `TextureMemory` is an exact `float`

`MaxTextureSize` requires two positive decimal pixel counts separated by `x`.
The numeric count/memory properties must be nonnegative, and memory must be
finite. Positivity, finite-number validation and rejection diagnostics are
explicit reference-provider policies. An incorrect reserved type
produces `type_mismatch`; malformed values produce `invalid_value`. An empty
property name produces `invalid_name`. The complete update is rejected on any
error and the previous map is retained.

The rendering `Shading` description gives examples rather than a closed list;
host-specific shading names are therefore preserved. Nonempty implementation
property names and any of the four value alternatives are accepted. They are
informative metadata and do not create new services.

`get_browser_properties()` returns all eight reserved Table 6.6 capabilities as
false: `ABSTRACT_NODES`, `CONCRETE_NODES`, `EXTERNAL_INTERACTIONS`,
`PROTOTYPE_CREATE`, `DOM_IMPORT`, `XML_ENCODING`, `CLASSIC_VRML_ENCODING` and
`COMPRESSED_BINARY_ENCODING`. The bounded external-session implementation and
generated vocabulary do not establish the full referenced facilities or an ISO
SAI conformance level. There is no setter that can turn these claims on without
implementing them. Publishing rendering measurements or installing an option
handler does not alter this map or `capability_set`.

## Dynamic options and application code

Names and values are exact and case-sensitive:

- Booleans: `Antialiased`, `Dashboard`, `EnableInlineViewpoints`, `MotionBlur`,
  `SplashScreen`
- `PrimitiveQuality` and `TextureQuality`: strings `Low`, `Medium`, `High`
- `QualityWhenMoving`: those quality strings plus `Same`
- `Shading`: strings `Wireframe`, `Flat`, `Gouraud`, `Phong`

Unknown names return `invalid_name`, wrong variant alternatives return
`type_mismatch`, and strings outside the corresponding choices return
`invalid_value`. The implementation does not coerce integer booleans, normalize
case, trim strings, accept vendor options, or conflate `AntiAliased` with
`Antialiased`. These structured-error choices are SDK policy; abstract
§6.3.26 does not prescribe this complete validation error vocabulary.

A valid request with no installed host handler returns a successful `false`,
the source's unsupported-dynamic-option result. No map is changed to pretend
the option was applied. An installed handler must return `true` only after the
real requested effect succeeds, or `false` if unavailable/refused. The host owns
its side effects and must avoid partial effects on failure: the kernel cannot
roll back arbitrary application code. Handler-returned errors are preserved.
Thrown standard/nonstandard exceptions become structured `callback_failed`
errors, and a later request can run normally.

Application code runs outside every browser, scene and service lock. Reads,
description changes, measurements, handler replacement and other ordinary
browser operations remain usable within the handler. Valid concurrent/reentrant
option requests return `operation_in_progress` instead of deadlocking or
entering the same host handler concurrently.

Replacement creates the new owned callable before locking and releases the
previous callable after unlocking. An in-flight invocation retains its original
handler even if it clears or replaces the installed one. The busy flag is
cleared before that retained callable is destroyed; its destructor may reenter
ordinary services. Calls pin service state, so destroying the initiating browser
wrapper from a handler does not invalidate the active operation. As with other
owned callbacks, capturing an owning browser copy can create a cycle; clear the
handler when its ownership is no longer needed.

Moving the initiating wrapper during an active callback is also safe; its
destination retains ordinary services and subsequent calls on the moved-from
wrapper fail. None of these guarantees permits a new call into an object whose
lifetime has ended or while its destructor is running, or concurrent mutation of
the same C++ wrapper object. Concurrent service calls on stable wrappers/copies
operate on the synchronized shared browser state.

## Profile/component discovery remains open

No profile list or component-support catalog is derived from node descriptors.
Abstract §§6.3.6–9 require real support and at least one supported profile and
component. Returning an empty collection as successful complete discovery would
not satisfy those requirements, and returning a familiar profile name without
implementing it would be a false claim.

A future host-advertised catalog can own component name/level declarations and
profile component requirements, validate exact names and requested levels,
reject duplicates and inconsistent dependencies, and distinguish absent host
advertisement from an actual supported list. Such a catalog must be supplied
by a host that implements and can substantiate the advertised behavior.
Cross-checking descriptor presence alone cannot prove runtime support. It
should therefore remain explicitly host-advertised metadata until backed by
profile/component behavioral conformance evidence; it must not silently promote
this offline kernel into a conforming browser.

## Executable evidence

`tests/browser_services_test.cpp` covers optional identity ownership, aliasing,
description persistence, finite sample validation, cross-world rejection,
detached property ownership, conservative capability truth, complete-map
failure atomicity, every standard option name/choice, wrong names/types/values,
unavailable/refused/applied/error outcomes, thrown callbacks, move-only captures,
callback reentry, unlocked resource destruction, wrapper destruction during
an active call, concurrent snapshot publication and option exclusion, and no
scene revision, graph event, field event or update-gate effects.
