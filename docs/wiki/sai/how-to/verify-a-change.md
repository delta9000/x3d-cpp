---
title: Verify a change to the SAI SDK
summary: Which gate to run for which kind of SAI change, and how to run the harnesses by hand.
tags: [sai, how-to, testing]
updated: 2026-11-14
related:
  - ../reference/test-suites.md
---

# Verify a change to the SAI SDK

Run from the x3d-cpp root. What each suite covers is in
[Test suites and gates](../reference/test-suites.md).

## Pick the gate

| You changed | Run |
|---|---|
| Anything under `sai/` | `mise run sai-ci` |
| `adapters/sai_provider/`, `examples/sai_host/`, `tests/cmake/sai_provider/` | `mise run sai-provider` |
| Runtime headers the adapter includes (anything that moves or renames under `runtime/` or `include/x3d/`) | `mise run sai-provider` |
| Generated header ownership, exported include paths, or the SAI/runtime boundary | `mise run sai-coexistence`, in both linkage modes |
| The runtime generator front end (`src/x3d_cpp_gen/`) | `mise run sai-ci` as well as the runtime gates |
| A register YAML | See [Update the service register](update-the-service-register.md) |
| SAI wiki pages | `mise run docs-build` |

`mise run ci` includes `sai-ci`. `mise run ci-all` adds `sai-provider`.

## Try the example

```sh
mise run sai-hello
```

Builds and runs `sai/examples/provider_scene.cpp` only.

## Run the SDK suite

```sh
mise run sai-ci
```

To iterate faster, run the parts:

```sh
mise run sai-build
mise run sai-ctest
```

Set `X3D_SAI_BUILD_DIR` to use a build directory other than `sai/build`.

## Run the provider harness

```sh
mise run sai-provider
X3D_CPP_SHARED_NODES=OFF mise run sai-provider
```

To keep the build tree somewhere specific, pass the SAI source and an empty
work directory:

```sh
bash scripts/verify_sai_provider.sh sai /empty/work/directory
```

Builds and logs stay in the printed work directory.

## Run the coexistence check

```sh
bash scripts/verify_sai_coexistence.sh
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_coexistence.sh
```

The second line exercises static runtime symbols.

## After regenerating bindings

```sh
mise run sai-gen
mise run sai-gen-determinism
```

Commit the regenerated `sai/generated_cpp_bindings` with the generator change.

## Adding an adapter test

Keep tests under `adapters/sai_provider/tests/` as plain `main()` programs. The
harness builds them in an installed-package consumer that has no doctest.
