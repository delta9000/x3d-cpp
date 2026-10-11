# Modern C++ X3D SAI proposal

One experimental provider-owned contract covers setup authoring and host-driven
execution. Applications use `<x3d/sai/experimental/provider.hpp>` and
`x3d::sai::experimental::provider`; the selected backend owns the actual scene.
The frontend has no reference-kernel or x3d-cpp scene dependency.

The reference and native implementations share handles, values, errors, gates,
receipts and event-delivery rules. They currently advertise a bounded node/service
slice, not complete ISO SAI, browser, component/profile or CAVEOS support. This is
a modern replacement proposal for review, not a source-compatible implementation
of every published C++ interface or a stable binary ABI.

## Build and consume

Requires C++20 and CMake 3.24 or newer:

```sh
cmake -S . -B build-sdk -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdk --parallel 1
cmake --install build-sdk --prefix /path/to/sdk
```

For the reference backend:

```cmake
find_package(x3d_sai CONFIG REQUIRED)
add_executable(application main.cpp)
target_link_libraries(application PRIVATE x3d::sai_reference_provider)
```

`make_reference_provider(resource_limits = {})` returns an owning
`provider::service`. A native application instead uses the optional
[x3d-cpp companion](https://github.com/delta9000/x3d-cpp/tree/main/adapters/sai_provider),
`find_package(x3d_cpp_sai CONFIG REQUIRED)`, and `x3d_cpp::sai_provider`.
Its factory is `native::make_service(resource_limits = {})`; subsequent ordinary
scene, field and event operations use the same provider contract.
The native-only `make_presented_scene()` factory additionally binds one ordinary
scene to an owned mesh/transform feed. Its
[headless host example](https://github.com/delta9000/x3d-cpp/tree/main/examples/sai_host)
replays explicit local input and retains immutable frames through shutdown; it
does not add a renderer or event loop to the portable service.

Public targets:

- `x3d::sai`: the compiled provider-neutral frontend
- `x3d::sai_provider`: an alias of that same target, not another interface
- `x3d::sai_reference_provider`: the reference implementation
- `x3d::sai_provider_fixture`: the shared behavioral test oracle
- `x3d::sai_reference_kernel` and `x3d::sai_reference_metadata`: explicit
  reference-only snapshot/editor/runtime extensions

Embedding with `add_subdirectory` uses the same targets. Installed packages are
relocatable and carry C++20/thread requirements. The ordinary x3d-cpp SDK remains
independent: only its explicit companion depends on this package. Existing
experimental offline/hosted adapter APIs and targets have been consolidated,
not retained as additional application interfaces.

## Start with the contract

[Portable contract and operation matrix](docs/portable-contract.md) describes
identity, setup/live access, gates, failure effects, callbacks and resource
budgets. [Runtime details](docs/hosted-runtime.md) and the
[coverage migration ledger](docs/provider-fixture-coverage-ledger.md) explain
which old assertions were retained or deliberately replaced.

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

Normal register checks validate consistency; the strict convergence gate
remains intentionally failing while required semantics remain incomplete.

## Development

The generator consumes a vendored SHA-pinned Web3D UOM. Python/generation are not
needed by installed C++ applications. Run `mise run ci`, or its documented
constituent UOM, generation, register, Python, CMake and CTest commands.
Cross-repository native evidence is separately reproduced with
`x3d-cpp/scripts/verify_sai_provider.sh`, in static and shared native modes.

This repository was split from x3d-cpp on 2026-07-19; pre-split history is retained
there as `archive/sai-semantic-kernel`. Earlier provenance commits are not all
buildable. Both repositories remain independently buildable packages.
