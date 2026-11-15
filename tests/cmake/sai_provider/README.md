# Native/reference SAI provider harness

Builds the native provider (`adapters/sai_provider`) and the SAI SDK's
reference backend, runs the shared provider oracle against both, compares
their reports, and repeats the run against a relocated installed prefix.

```sh
bash scripts/verify_sai_provider.sh sai /empty/work/directory
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_provider.sh sai /another/empty/directory
```

Or `mise run sai-provider`. Requirements: C++20, CMake 3.24+ and Ninja. Nothing
is fetched.

The executables and what each checks are listed in
[Test suites and gates](../../../docs/wiki/sai/reference/test-suites.md#provider-harness).

`native_authoring_fixture.hpp` is a test-only wrapper over the final service.
It is never installed and is not an application API.
