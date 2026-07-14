# Shared generated-node runtime design

## Context

The generated node definitions are compiled once into `x3d_cpp_nodes`, but the
library is currently static. The generated factory references the complete node
registry, so normal consumers pull nearly the whole archive rather than a small
dead-stripped subset.

That topology is especially costly under ASan/UBSan. A cold Debug sanitizer
build produced a roughly 500 MB instrumented node archive and 45 executables of
roughly 400--650 MB each. Minimal `-g1` debug information reduced the complete
tree only from approximately 24 GB to 22 GB: object files occupy 2.7 GB, while
duplicated linked executables account for almost all remaining storage.

An attempted unity build of the five grouped behavior suites was rejected. All
19 generated unity batches failed because independent test files intentionally
reuse file-local helper names. Rewriting tests to accommodate a build trick
would be invasive and would not solve production or downstream duplication.

## Decision

Build `x3d_cpp_nodes` as a shared library by default in every configuration.
Provide an `X3D_CPP_SHARED_NODES=OFF` option for embedders that explicitly need
a self-contained static artifact.

The target name, aliases, headers, transitive CMake linkage, and generated API
remain unchanged. Installation exports the appropriate archive, shared-library,
and runtime artifacts. Shared builds use the project version as their library
version and major version as the ABI soname. Windows shared builds request
CMake's automatic symbol export because generated declarations do not carry a
platform export macro.

Remove `generated_cpp_bindings/x3d/nodes/test.cpp` from the production node
source list. Its `main()` was inert only because no static-archive consumer
requested that object. A shared library includes every object and must never
ship a test entry point. Existing generator and compile-contract gates provide
the intended test coverage without compiling that file into the runtime.

Keep sanitizer builds at Debug with ASan, UBSan, frame pointers, recovery
disabled, and level-one debug information. The shared topology removes the
large repeated instrumented text; `-g1` separately avoids debug records that CI
does not inspect.

## Compatibility

The default installed package now has a runtime shared-library dependency. This
is appropriate for the project SDK and avoids duplicating the complete node
registry in every executable. Consumers requiring a single-file/static
deployment can configure `-DX3D_CPP_SHARED_NODES=OFF`; that mode remains covered
by configure, build, and installed-consumer tests.

The project is version 0.1.0 and does not claim a stable binary ABI. A soname is
still installed so future incompatible releases can coexist correctly.

## Validation

- CMake shape tests prove shared-by-default and static opt-out artifacts.
- Compile commands prove `nodes/test.cpp` is absent from the production target.
- Normal Werror behavior and compile-contract suites pass.
- Installed-consumer smoke tests build and execute against both linkage modes.
- All 45 behavior tests pass under ASan/UBSan.
- A cold sanitizer build is measured against the 22--24 GB baseline, with a
  target live-tree size below 10 GB.
