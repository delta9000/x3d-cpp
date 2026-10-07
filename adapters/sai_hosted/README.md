# Experimental hosted SAI companion

`x3d_cpp::sai_hosted` is an explicit, additive companion package. It implements
`x3d::sai::experimental::hosted::service` with independently owned native
`RuntimeSession` instances. Ordinary x3d-cpp builds do not enable it or discover
x3d-sai. The companion links the portable `x3d::sai_hosted` scheduler and the
real x3d-cpp domain runtime. It does **not** link the reference semantic kernel,
SAI metadata catalog, render window, or rendering backend.

## Deliberately bounded executable slice

`SaiHostedService::create_route_fixture()` builds a native document with three
root DEFs: `Transform`, `Interpolator` (`PositionInterpolator`), and `Mirror`
(a second Transform). `Transform` contains one native Shape/Box so runtime
transform dirtiness has a real extraction consequence. Initial keys are
`[0,1]`, key values are `[(0,0,0),(10,20,30)]`, and the document contains an
actual authored `Interpolator.value_changed -> Transform.translation` ROUTE.
The common test adds an independently validated fan-out ROUTE to Mirror.

The document remains setup-only until activation; `RuntimeSession::create`
then owns it, builds the native graph, bridges authored ROUTEs, and attaches
native interpolator behavior. Field discovery comes from the native static
reflection tables. All field kinds are reported; only SFFloat, MFFloat,
SFVec3f, and MFVec3f are supported as payloads. Generated document syntax
attributes are not presented as runtime fields. Reads use actual native getters,
including outputOnly readback; there is no mirrored field-value database.

Setup authoring may use reflected setter thunks. Live external ingress always
uses `context.postEvent`, followed by exactly one `session.tick(host_time)` per
host pump, including empty pumps. The hosted slice explicitly stages key and
keyValue events before other submitted inputs, stable within each group, so a
fraction-first batch can still describe a complete resized curve. This does
not alter the native event cascade's ordering or promise general atomicity,
rollback, fan-in winners, or application-level per-write sequencing.

This hosted slice restricts admitted interpolation curves to nonempty matching
key/keyValue counts, strictly increasing finite keys, finite values, and finite
adjacent key/component differences in float arithmetic. Duplicate keys and
finite endpoints whose differences overflow float are rejected by setup
activation and whole-batch admission/revalidation. These restrictions avoid
known native/reference boundary differences without changing the native
interpolator. They are explicit proof-slice limits, not a claim about full ISO
key semantics or bitwise equality for every finite floating-point computation.

Native field-write listeners copy owning event values during the cascade.
Application callbacks run only on the portable service's explicit notification
drain. Logical turns remain distinct when host time repeats. A failure after
native entry can leave real mutations; the activation faults, queued work is
cancelled, and no retry/replay is attempted.

## Ownership and global-state boundary

One document/session owns each scene; semantic scene/context/node identity is
independent of activation epoch and native pointer address. The only pointer
registry is private and bounded to the three immutable fixture roots. The
protected diagnostics seam returns copied evidence, never a native mutable
pointer or retained scene ownership. Retirement destroys the session before
releasing its registry, relying on the runtime's guarded callback teardown.

This bounded path directly constructs built-in nodes, and neither installs nor
clears mutable process-global behavior, author-field, geospatial or resource
configuration. Native ROUTE bridge reflection currently consults the runtime's
process-wide DynamicFieldStore after static fields; the known built-in fields
resolve from static reflection, and live native delivery returns before the
author-field fallback. The native test leaves an unrelated author-field entry
live, verifies its value survives all hosted teardown, and verifies the global
entry count does not grow. This is evidence for the bounded built-in path, not
a claim that every existing native-runtime subsystem is free of globals.
The runtime's process-wide transform-work diagnostic is a relaxed atomic
counter; it can count work across independent scenes, but neither scene values
nor behavior depend on its value.

There is no general scene/node factory or full profile claim. PROTO,
EXTERNPROTO, Script, Geo nodes, Inline, external resources, network I/O,
interactive input, and renderer/window integration are outside this adapter.

## Build and tests

Install compatible `x3d_cpp` and `x3d_sai` packages, then configure this directory
with those prefixes in `CMAKE_PREFIX_PATH`. The companion installs its own
`x3d_cpp_sai_hosted` package and exported `x3d_cpp::sai_hosted` target. It is also
usable with `add_subdirectory` when the dependency targets already exist.

`X3D_CPP_SAI_HOSTED_BUILD_TESTS=ON` enables:

- `sai_hosted_native_runtime`: the unchanged portable behavioral fixture plus
  native-only evidence for actual interpolation/output storage, Transform
  storage/worldTransform, real transform-only RenderDelta, repeated-time turns,
  four payload kinds, two-scene retirement, throw-after-mutation faulting,
  partially failed activation teardown, and service destruction during deferred
  notification delivery
- `sai_hosted_cross_backend_parity`, when `x3d::sai_reference_hosted` exists:
  exact comparison of the same portable fixture's observable reports. Only
  this test executable links the reference kernel and metadata

The shared fixture checks rejection as well as success paths, request lifetime,
setup versus activation, aliases, client isolation, callback errors,
cancellation, retirement, and owner-thread affinity. Native evidence uses
copied diagnostics after the real RuntimeSession turn; it never substitutes a
mock node map or synthetic render delta.
