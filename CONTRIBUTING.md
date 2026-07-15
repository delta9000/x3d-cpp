# Contributing to x3d-cpp

## Build and run the fast gate

x3d-cpp is a normal CMake package — see [Build and install](README.md#build-and-install):

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

Contributors additionally use [mise](https://mise.jdx.dev) as a task runner:

```bash
mise run build   # configure + build + ctest (dev preset)
mise run ci      # tests, golden, conformance, coverage, build, cli-gate
```

`mise run ci` mirrors the required CI gates (the workflow runs them as separate
jobs, not as this aggregate). Get it green before opening a PR.

Two things `mise run ci` does **not** cover, so run them by hand when relevant:

```bash
mise run validate-examples   # the out-of-SDK renderer consumers (cpu_raster + poc_renderer)
mise run docs-drift working  # which docs your change may have staled
```

`mise run docs-drift` needs `X3D_QDRANT_URL` set; without it, it falls back to
`localhost:6333` and fails.

## API stability at 0.x

x3d-cpp is pre-1.0. The promise is deliberately narrow:

- **Patch releases (0.1.0 → 0.1.1) are *intended* to preserve API and ABI.** The
  installed CMake package declares `SameMinorVersion` and the shared libraries
  carry `SOVERSION 0.1`, so a consumer that found 0.1.0 accepts 0.1.1 and is
  rejected by 0.2.0 at both configure time and load time. This is intent backed
  by convention, **not** a proof: there is no ABI checker in CI yet. If you
  depend on it, pin exactly.
- **Minor releases (0.1.x → 0.2.0) may break both API and ABI.**
- Symbols in `include/x3d/sdk.hpp` marked `[STABLE]` are frozen pre-v2; those
  marked `[EXPERIMENTAL]` may gain fields. Per-seam state lives in
  [`docs/wiki/seam-status.md`](docs/wiki/seam-status.md) — there is no blanket
  answer.

Installed CMake target names are also not one promise. `x3d_cpp::sdk`,
`x3d_cpp::authoring` and `x3d_cpp::x3d_cpp` are documented entry points;
`x3d_cpp::headers`, `x3d_cpp::nodes`, `x3d_cpp::authoring_runtime` and
`x3d_cpp::runtime` exist so the export set resolves and may be restructured.

## Reporting a bug with a scene

Attach a **minimal** X3D reproduction: the smallest scene that still shows the
problem, with textures and Inlines removed unless they *are* the bug. State the
encoding (XML / ClassicVRML / VRML97 / JSON), the X3D version, and what you
expected the parser, the tick, or the extraction to do.

Note the two version axes — the node layer is generated from the X3D **4.0**
UOM, while the parser reads **3.0–4.1** encodings. A 4.1 scene parses, but
4.1-only nodes the 4.0 UOM does not define are not in the generated node layer.

The SDK parses untrusted input, so parser crashes, unbounded memory growth,
hangs, and unexpected file access are **security-relevant**. Private
vulnerability reporting is not yet enabled on this repository; until it is,
please contact the maintainer directly rather than opening a public issue.

## PR checklist

See [`.github/PULL_REQUEST_TEMPLATE.md`](.github/PULL_REQUEST_TEMPLATE.md). In
short: a conformance claim you can point at, a test that fails without your
change (or an honest N/A), and the docs updated in the same diff.

Docs are part of the diff — the living docs have drifted from the code before,
and `mise run docs-drift` exists because of it.

## How work is tracked

Tasks come from the [GitHub Project](https://github.com/users/delta9000/projects/2)
(`scripts/pick-card.sh --list`). The card→issue→branch→PR→docs→Done chain and the
Definition of Ready/Done are in
[`docs/contributor/card-to-done-workflow.md`](docs/contributor/card-to-done-workflow.md).
