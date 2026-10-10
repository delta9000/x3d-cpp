# Unified native/reference SAI provider proof

The optional companion in `adapters/sai_provider` implements the shared final
`provider::service` through one owned native backend. The same scene/context/
node/field handles work during generic setup and after runtime activation.
This is bounded experimental evidence, not complete ISO SAI conformance.

Requirements: C++20, CMake 3.24+, Ninja and the independent x3d-sai source tree.
No repository is fetched and ordinary x3d-cpp builds gain no SAI dependency.

```sh
bash scripts/verify_sai_provider.sh /path/to/x3d-sai /empty/work/directory
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_provider.sh /path/to/x3d-sai /another/empty/directory
```

Each shared/static run builds source consumers, installs both projects and the
companion, relocates the complete prefix, then configures copied standalone
consumers with package registries disabled. Four executables run in each mode:

1. `sai_provider_reference`: common authoring + live oracle on reference backend
2. `sai_provider_native`: same oracle plus native scalar/metadata/graph evidence
3. `sai_provider_parity`: exact comparison of complete backend-blind reports
4. `sai_provider_native_runtime`: common live oracle plus six native session,
   interpolation, ROUTE, world-transform, RenderDelta and failure-effect proofs

All checks remain active under Release/NDEBUG. Native-only link commands are
checked for absence of reference-kernel and reference-metadata libraries; the
parity executable deliberately links both backends. The source-level native
header guards additionally reject accidental reference implementation includes.

The generic shared fixture creates Transform, PositionInterpolator, Shape and
Box nodes, with named roots and nested renderable geometry. There is no native
production fixture bootstrap API or separate hosted public service. Native-only
extraction inspection is an explicit extension outside the portable contract.

See `adapters/sai_provider/README.md` for capability restrictions.
`native_authoring_fixture.hpp` is a test-only convenience wrapper over the final
service, never installed and never an additional application API.
