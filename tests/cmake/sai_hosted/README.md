# Opt-in hosted runtime proof

The ordinary x3d-cpp build has no SAI dependency. This separate harness consumes
an explicitly supplied x3d-sai checkout or installed companion package.

Run the complete embedded-source and relocated-install proof:

```sh
X3D_CPP_SHARED_NODES=OFF CMAKE_BUILD_PARALLEL_LEVEL=3 \
  scripts/verify_sai_hosted.sh /path/to/x3d-sai /tmp/empty-static-proof
X3D_CPP_SHARED_NODES=ON CMAKE_BUILD_PARALLEL_LEVEL=3 \
  scripts/verify_sai_hosted.sh /path/to/x3d-sai /tmp/empty-shared-proof
```

An explicit work directory must be empty; otherwise a private temporary directory
is created. The script never fetches x3d-sai. Jobs default to two and are capped at
three. `X3D_CPP_SHARED_NODES` selects shared or static native node libraries; the
hosted adapter and portable/reference libraries are static in both configurations.
Both modes are supported by the harness; a successful run proves only the mode
actually selected, not the other mode.

The native, reference, and parity executables compile the same backend-neutral
fixture. A separate native evidence executable reuses the companion's test source
for real storage, world transforms, render deltas, activation/failure lifetime and
callback destruction. Only the parity and reference executables link the SAI
reference kernel or metadata. Exact native link commands and symbol lists are
saved and rejected if they contain reference implementation dependencies.

The script installs current headers and archives into one prefix, checks exported
CMake files for source/build paths, moves that prefix, copies the consumer and
native evidence source outside the repository, then configures with package
registries disabled and reruns all four tests. Installed consumer includes and
libraries must come from the relocated SDK, not the source/build tree.

For an already prepared installed prefix, configure this directory with
`SAI_HOSTED_INSTALLED=ON` and `CMAKE_PREFIX_PATH=/path/to/prefix`. To include all six
native evidence cases, copy the companion's `hosted_native_test.cpp` alongside the
consumer as `native_evidence.cpp`, or set `SAI_HOSTED_NATIVE_EVIDENCE_SOURCE` to its
path. The complete script performs that staging automatically.

This proves the documented bounded hosted slice, not full profile/ISO conformance,
resources, PROTO, Script, geospatial behavior, or CAVEOS integration. See
`adapters/sai_hosted/README.md` and x3d-sai's `docs/hosted-runtime.md`.
