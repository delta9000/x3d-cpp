# x3d-cpp SAI SDK

A modern C++20 Scene Access Interface for X3D, built around one provider-owned
contract for setup authoring and host-driven execution. Applications use
`<x3d/sai/experimental/provider.hpp>` and `x3d::sai::experimental::provider`;
the selected backend owns the actual scene. The frontend has no reference-kernel
or x3d-cpp scene dependency.

Two backends implement the contract today: the reference backend in this
directory and the native backend in
[`adapters/sai_provider`](../adapters/sai_provider/), which drives a real
x3d-cpp `RuntimeSession`. They share handles, values, errors, gates, receipts
and event-delivery rules, and the same behavioural oracle checks both. Each
backend advertises the node types and services it supports, so check
`service.supported()` rather than assuming the whole ISO SAI. The namespace is
`experimental` because the API can still change between 0.x releases, and it
makes no binary ABI promise.

## Relationship to the ISO C++ binding

[ISO/IEC 19777-4](https://www.iso.org/standard/83753.html) is the published C++
language binding for the SAI. Its interfaces keep the shape shared with the
other SAI language bindings, notably Java (ISO/IEC 19777-2): accessor pairs
such as `getUserData`/`setUserData`, an `X3DField` class hierarchy, and a
`CObject` user-data type. That parallel structure is a deliberate choice: it
lets one abstract service description map the same way into every language.

This SDK explores a different trade-off. It maps the same abstract services
(ISO/IEC 19775-2) onto idiomatic C++20: owning values, `result<T>` errors,
distinct strong types, and handles whose authority the provider checks. It is
simply another way of doing it, shared to help the X3D ecosystem. It doesn't
replace the standard and isn't source-compatible with 19777-4. The
[service register](docs/conformance/SAI-BASELINE.md) cites the ISO clause behind
each service, so anyone can compare the two.

Where a topic page notes that our reading of the published text differs, or
that the text leaves a policy open, it records the choice this SDK made. It is
not a defect claim.

## Build and consume

Requires C++20 and CMake 3.24 or newer:

```sh
cmake -S . -B build-sdk -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdk --parallel 1
cmake --install build-sdk --prefix /path/to/sdk
```

Run the ordinary provider example from the x3d-cpp checkout with `mise run sai-hello`.
It creates a Transform containing a Shape and Box, routes a PositionInterpolator
into the transform, pumps one input, and drains its notification. The task builds
only the example and its dependencies, using one compile job by default.
[The example](examples/provider_scene.cpp) keeps application operations in a
function accepting `provider::service &`; backend selection happens in `main`.

For an installed reference backend, configure your application with
`-DCMAKE_PREFIX_PATH=/path/to/sdk` and use:

```cmake
find_package(x3d_sai CONFIG REQUIRED)
add_executable(application main.cpp)
target_link_libraries(application PRIVATE x3d::sai_reference_provider)
```

`make_reference_provider(resource_limits = {})` returns an owning
`provider::service`. A native application instead uses the
[native provider](../adapters/sai_provider/),
`find_package(x3d_cpp_sai CONFIG REQUIRED)`, and `x3d_cpp::sai_provider`.
Its header is `<x3d/sai_provider.hpp>` and its factory is
`x3d::sai::experimental::native::make_service(resource_limits = {})`. Subsequent
scene, field and event operations use the same provider contract.
The native-only `make_presented_scene()` factory additionally binds one ordinary
scene to an owned mesh/transform feed. Its
[headless host example](../examples/sai_host/)
replays explicit local input and retains immutable frames through shutdown; it
does not add a renderer or event loop to the portable service.

Public targets:

- `x3d::sai`: the compiled provider-neutral frontend
- `x3d::sai_provider`: an alias of that same target, not another interface
- `x3d::sai_reference_provider`: the reference implementation
- `x3d::sai_provider_fixture`: the shared behavioral test oracle
- `x3d::sai_reference_kernel` and `x3d::sai_reference_metadata`: explicit
  reference-only snapshot/editor/runtime extensions

To consume a source checkout, replace `find_package` with
`add_subdirectory(path/to/x3d-cpp/sai sai EXCLUDE_FROM_ALL)` and keep the same
link target. CMake then builds the SDK dependencies selected by your
application. Installed packages are relocatable and carry C++20/thread
requirements. The ordinary x3d-cpp runtime build does not add this directory;
only the native provider depends on it. The earlier offline and hosted adapter
APIs were consolidated into this one contract.

## Start with the contract

[Portable contract and operation matrix](docs/portable-contract.md) describes
identity, setup/live access, gates, failure effects, callbacks and resource
budgets. [Runtime details](docs/hosted-runtime.md) and the
[coverage migration ledger](docs/provider-fixture-coverage-ledger.md) explain
which earlier assertions were retained or deliberately replaced.

A client creates a scene/context, creates supported nodes, authors fields and
routes, then activates the same scene. The same node/field handles remain valid.
Live input is enqueued through a client, processed only by an explicit host pump,
and observed through a separate notification drain. An owning receipt records
submission outcome even after the service closes; accepted does not mean applied.
No renderer, window, hidden clock, resource access or event loop is installed.

Inspect backend capabilities and each field's supported flag. Node component
membership tags are schema facts, not complete component/profile support claims.
The current backends support general construction within their advertised slice;
there is no required test-only scene bootstrap in the application interface.

## Reference implementation and standards

The [reference-kernel guide](docs/reference-kernel-guide.md) describes optional
atomic editors, immutable snapshots, strict-time kernel sessions and stateful
runtime checkpoints. These are not requirements imposed on another backend.
The generated field keys and exact field-kind vocabulary remain shared.

The [service register](docs/conformance/SAI-BASELINE.md) tracks each ISO SAI
service as implemented, partial or planned, with the tests that back it. Normal
register checks validate consistency and run in CI. `mise run sai-conformance`
is the opt-in convergence report: it lists every service and falsification test
still to finish and exits non-zero until none remain.

## Development

The generator consumes a vendored SHA-pinned Web3D UOM. Python/generation are not
needed by installed C++ applications. From the x3d-cpp checkout run
`mise run sai-ci` (UOM pin, generator determinism, registers, generator pytest,
CMake build and CTest); `mise run ci` includes it. `mise run sai-provider`
checks the native provider against this directory in source and relocated
installed modes, and `mise run ci-all` includes that proof.

This SDK lived in a separate `x3d-sai` repository from 2026-07-19 to 2026-11-11
and was folded back into x3d-cpp with its history
([ADR-0076](../docs/wiki/decisions/0076-fold-sai-into-x3d-cpp.md)). Earlier
provenance commits are not all buildable. `sai/` remains its own CMake project
and uv package, so it still builds and installs on its own.
