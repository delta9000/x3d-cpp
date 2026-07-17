# x3d-cpp-gen

Generate spec-faithful C++ node bindings from the X3D Unified Object Model (UOM).

`x3d-cpp-gen` is the code generator for
[x3d-cpp](https://github.com/delta9000/x3d-cpp), a headless, renderer-agnostic
X3D domain-runtime SDK. This package is **only the generator** — installing it
does not install the C++ runtime.

## What it generates

A tree of C++20 sources under `x3d/core/` and `x3d/nodes/`:

- `x3d/core/` — the vocabulary: field value types, enums, and reflection support.
- `x3d/nodes/` — one `.hpp`/`.cpp` pair per X3D node, in namespace `x3d::nodes`,
  plus a node factory and an interface registry.

Node and field declarations, types and defaults are derived from the UOM.
Generation substantially reduces structural drift — it does not prove runtime
semantics or eliminate UOM errata. Behavioral conformance is not this package's
concern; it is tested in the runtime repository.

## Install

Not yet published to PyPI — install from a checkout of the
[x3d-cpp](https://github.com/delta9000/x3d-cpp) repository:

```bash
git clone https://github.com/delta9000/x3d-cpp
cd x3d-cpp
uv tool install .        # or: pip install .
```

## Use

```bash
x3d-cpp-gen --out generated_cpp_bindings
```

The X3D 4.0 UOM ships inside the package, so no input file is required. Point it
at another UOM revision with `--spec`:

```bash
x3d-cpp-gen --spec X3dUnifiedObjectModel-4.1.xml --out out/
```

## Supported UOM inputs

Any `X3dUnifiedObjectModel-*.xml` revision. The spec version is auto-detected
from the root element's `version` attribute; override it with `--spec-version`.
The packaged default is 4.0.

If the UOM contains a field whose type the generator doesn't support, generation
**fails closed** rather than silently emitting a node with fields missing. Pass
`--allow-unsupported-fields` to opt out (the unsupported fields are skipped).

## Verification behavior

The generator compiles and runs a smoke test over its own output, and **fails
closed**: it will not report success for a test that never ran.

| Option | Behavior |
|---|---|
| *(default)* | Generates, then compiles and runs the smoke test. **Fails** if the compiler is missing or the generated main is absent. |
| `--no-test` | Skips the smoke test and succeeds. The one explicit opt-out. |
| `--compiler` | C++ compiler for the smoke test (env `CXX`, default `g++`). An empty value is an error, not a skip. |
| `--clang-format` | Formatter executable (env `CLANG_FORMAT`); empty disables formatting. |

## Formatting

Output is formatted with `clang-format` when it is on your PATH. The style comes
from the repository's `.clang-format`, which is **not** shipped inside the
installed package: running the generator from a checkout picks it up
automatically, while running it from an installed wheel elsewhere falls back to
clang-format's built-in **LLVM** style.

That distinction only matters if you need output byte-identical to the x3d-cpp
repository's committed tree — there, formatting is part of a byte-exact golden
contract and the formatter version is pinned exactly. For generating bindings for
your own use, either style is fine.

## Relation to x3d-cpp

The [x3d-cpp](https://github.com/delta9000/x3d-cpp) repository commits this
generator's output and gates it byte-for-byte, so the generated tree is a
reviewed artifact rather than a build-time surprise. Use this package directly if
you want to generate bindings from a different UOM revision, or to inspect what
the runtime's node layer is derived from.

## License

MIT — see `LICENSE`. The bundled UOM data carries its own attribution; see
`NOTICE`.
