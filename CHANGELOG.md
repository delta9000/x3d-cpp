# Changelog

Notable changes to x3d-cpp. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning is [SemVer](https://semver.org) with the 0.x caveats in
[CONTRIBUTING.md](CONTRIBUTING.md#api-stability-at-0x).

## [Unreleased]

### Added

- **`operator==`/`!=` on the generated value structs** (`SFVec3f`, `SFColor`,
  `SFRotation`, the matrix types, `SFImage`, …) — C++20 defaulted, exact
  member-wise comparison. Equality is vocabulary, not math: no epsilon, and
  arithmetic stays out of `x3d::core` per ADR-0012.

### Changed

- **`MFNode` getters return `const MFNode&` instead of a copy** (e.g.
  `X3DGroupingNode::getChildren()`). Reading a grouping node's children no
  longer copies the vector and bumps every child's refcount. Callers that
  mutated the returned temporary (a silent no-op before) now fail to compile;
  take an explicit copy if you need one. All other getters keep the by-value
  contract.

- The binding generator now **fails closed when the UOM contains a field type
  it doesn't support**, listing the affected fields instead of silently
  shrinking the generated API; `--allow-unsupported-fields` is the explicit
  opt-out.
- **`x3d::sdk::RuntimeSession`** — the recommended entry point. Owns the
  document, context, and extractor, and does the wiring you can silently forget:
  `buildSceneGraph` (skip it and no Viewpoint binds, so the camera is identity),
  `buildFrom` (skip it and authored ROUTEs never fire), and
  `attachStandardRuntime` (skip it and the ROUTEs resolve but nothing drives
  them, so the scene renders one static frame forever). `SessionOptions` names
  what it turns on. The low-level path stays public and reachable via
  `context()` / `extractor()` / `scene()`.
- `X3DExecutionContext::tickGeneration()` — a monotonic count of completed
  advances, independent of the simulation clock.
- A supported-platform table in the README, and the two X3D version axes stated
  where a reader meets them rather than only in `CONTRIBUTING.md`.
- `CONTRIBUTING.md`, `CHANGELOG.md`, and a PR template.
- A plain `cmake` build-and-install path in the README.
- Contract tests for the package version file, dev-tooling isolation, and the
  installed imported-target set.

### Fixed

- **Extraction no longer duplicates per placement.** `RenderItem` held `MeshData`
  by value and `buildLocalMesh()` ran once per *placement*, so N `USE`s of one
  DEF'd geometry re-tessellated identical content N times and retained N copies —
  to hand the consumer N descriptors it would immediately dedupe using `GeomId`,
  the key the SDK had already computed. Measured on 200 `USE`s of one
  19,602-triangle `IndexedFaceSet`: **702 MiB → 3.3 MiB** RSS and **1085 ms →
  8.7 ms**. Decoded textures had the same shape (200 resolver calls and 200 MiB
  for one image) and are now memoized by URL. See ADR-0045.
- **`SceneExtractor::delta()`'s contract is now total.** It asserted
  `ctx.now() != lastDeltaNow_`, which failed a paused / fixed-timestep / replay
  consumer for doing nothing wrong (a clock may legitimately repeat), and
  asserts compile out under `NDEBUG` — so a release build behaved differently
  from the debug build it was tested against. The guard keys on
  `tickGeneration()`; `delta()` before any `fullSnapshot()` returns the snapshot,
  and a second `delta()` with no intervening `tick()` returns an empty delta.
- **`ctx.writeField` reports instead of guessing.** It returned `void`, so a null
  node, a typo'd field name, and an unwritable field were one silent nothing; a
  wrong-typed `std::any` was worse, escaping as an uncaught `std::bad_any_cast`
  from inside the generated setter. It now returns a `[[nodiscard]]`
  `FieldWriteResult`. (Note: `outputOnly` fields *are* writable — their thunk
  routes to the field's emitter.)
- Documentation that contradicted the code: the SDK header and wiki called the
  product by the generator's name (`x3d-cpp-gen`); the wiki still claimed the
  generator emits "spec-correct" bindings after the README correctly narrowed
  that; and both the wiki and the façade's own banner marked all five embedder
  seams `[EXPERIMENTAL]` while four are `[STABLE]` with second-backend swap-test
  proofs. Example 03 claimed façade-only use while including a core header.
- The unqualified "versions 3.0–4.1" claim. The parser reads 3.0–4.1; the
  generated node model targets 4.0, so the **six** 4.1-only node types
  (`EnvironmentLight`, `FontLibrary`, `HAnimPose`, `InlineGeometry`,
  `RenderedTexture`, `Tangent`) are absent. Previously stated only in
  `CONTRIBUTING.md`, and under-counted at one node in the capability matrix.
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
