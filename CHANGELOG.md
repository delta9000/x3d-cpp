# Changelog

Notable changes to x3d-cpp. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning is [SemVer](https://semver.org) with the 0.x caveats in
[CONTRIBUTING.md](CONTRIBUTING.md#api-stability-at-0x).

## [Unreleased]

### Fixed

- The installed CMake package no longer declares `ARCH_INDEPENDENT` and now uses
  `SameMinorVersion`. It ships compiled shared libraries, so `ARCH_INDEPENDENT`
  wrongly deleted CMake's pointer-size guard — a consumer with a different
  pointer size silently accepted a mismatched ABI. `SameMajorVersion` was also
  wrong at 0.x, where the major is `0`: installed 0.1.0 satisfied a request for
  0.0.9, making every 0.y look interchangeable.
- Shared library `SOVERSION` now tracks `major.minor` instead of `major`, which
  was `0` for every 0.x — the loader would have accepted a 0.2 runtime for a 0.1
  consumer that `find_package` had just refused.
- The generator no longer reports success when the smoke test never ran (missing
  compiler, empty `--compiler`, missing generated main). `--no-test` remains the
  one explicit skip.
- Removed the `--namespace` generator option, which was parsed and then ignored.
- The docs site pointed at the wrong repository owner and name.
- Corrected two false public claims: the SDK's "no file IO" (it exports
  `parseFile`, which opens a file) and "spec-correct by construction" (generation
  from the UOM constrains declarations, not runtime semantics).

### Changed

- Adding x3d-cpp via `add_subdirectory()` no longer writes the ccache launcher
  into the shared cache or defines a global Ninja job pool, so it no longer
  reaches the consumer's own targets. Gated behind `X3D_CPP_ENABLE_DEV_TOOLING`,
  off for subprojects. (`-Wall -Wextra` is unaffected: it is directory-scoped and
  never reached consumers.)
- **CMake floor raised to 3.21.** `PROJECT_IS_TOP_LEVEL` was already in use while
  the floor said 3.20, where it silently evaluates empty and
  `X3D_CPP_BUILD_TESTS` / `X3D_CPP_BUILD_EXAMPLES` defaulted OFF.
- The golden formatter is pinned exactly (clang-format 22.1.8) with a committed
  `.clang-format`; the golden gate now fails on a version mismatch instead of
  skipping when the formatter is absent.
- Project identity settled: product/repository `x3d-cpp`, Python generator
  package and command `x3d-cpp-gen`, CMake project `x3d_cpp`. The
  `urn:x3d-cpp-gen:ext:ExternalGeometry` wire-format constant is unchanged.
- The Python package publishes its own README instead of the C++ runtime's, and
  ships `LICENSE` + `NOTICE` in the sdist.

### Added

- `CONTRIBUTING.md`, `CHANGELOG.md`, and a PR template.
- A plain `cmake` build-and-install path in the README.
- Contract tests for the package version file, dev-tooling isolation, and the
  installed imported-target set.
