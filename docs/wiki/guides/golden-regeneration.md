---
title: Golden Regeneration
summary: When and how to regenerate golden files, validate byte-identical output, and keep the golden gate green.
tags: [guide, golden, regeneration, testing]
updated: 2026-06-20
related:
  - ../decisions/0005-golden-files-in-git.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/generator.md
  - ../guides/gate-system.md
---

# Golden Regeneration

The golden tree is the committed copy of every generated C++ binding file in
`generated_cpp_bindings/`. This guide explains what the tree is, how the gate
works, what drift means, and the exact steps to regenerate and accept intentional
changes.

For the full gate inventory (golden + conformance + CLI regression + docs), see
[Gate System](gate-system.md).

---

## What the golden tree is

`generated_cpp_bindings/` holds **343 `.hpp` + 340 `.cpp`** files — 683 total.
Every file is the output of the Python generator pipeline
(`src/x3d_cpp_gen/generator.py`, driven by `mise run gen`) from the X3D 4.0
Unified Object Model XML. No file in this directory is written by hand.

Key headers you will recognize:

| File | What it contains |
|---|---|
| `X3DNode.hpp` / `.cpp` | Base class for every instantiable node; virtual `fields()`, `accept()`, `validateRanges()` |
| `X3Dtypes.hpp` | All `SFVec3f`, `SFColor`, `MFNode`, etc. type aliases |
| `X3Denums.hpp` | All bounded `enum class` vocabularies with `to_string`/`from_string` |
| `X3DReflection.hpp` | `FieldInfo`, `FieldTable`, `RangeDiagnostic`, `NodeVisitor` |
| `X3DNodeFactory.hpp` / `.cpp` | `create(typeName)` name-to-constructor registry |
| `X3DInterfaceRegistry.hpp` / `.cpp` | `nodeImplements()` / `interfacesOf()` type-membership queries |
| `<NodeName>.hpp` / `.cpp` | One pair per concrete node (260 nodes); `.cpp` holds the reflection thunks and `fields()` FieldTable |

The `.cpp` files exist because of the C1 decl/def split (2026-06-16): the heavy
`std::function` reflection thunks are compiled **once** into the `x3d_cpp_nodes`
static library, reducing cold build time from ~1296 s to ~39 s (~33×) cumulatively
(C1 at `-j4` reached ~76 s / ~17×; a subsequent `-j` raise, commit `719a6dd`, dropped
it further to ~39 s / ~33× once per-compile peak RSS fell to ~0.86 GB). The golden
gate covers both `.hpp` and `.cpp`. See
[ADR-0006](../decisions/0006-compiled-static-lib.md) for the full two-step record.

See [Generated Bindings](../subsystems/generated-bindings.md) and
[ADR-0005: Golden Files Committed to Git](../decisions/0005-golden-files-in-git.md)
for the full design rationale and file layout.

---

## How `mise run golden` works

The gate is implemented in `scripts/check_golden.sh` and invoked by:

```bash
mise run golden
```

What it does:

1. Runs `uv run x3d-cpp-gen --out <tmp> --no-test` to regenerate the full tree
   into a throwaway temp directory. (`--no-test` omits the smoke `test.cpp` binary,
   which is gitignored and not golden.)
2. Walks every `*.hpp` and `*.cpp` in the committed `generated_cpp_bindings/` and
   checks that the regenerated copy exists and is **byte-for-byte identical**.
3. Walks every `*.hpp` and `*.cpp` in the regenerated tree and checks that each
   exists in the committed golden (catches newly added output files that were not
   committed).
4. On any drift: prints a report naming every drifted/missing/extra file with a
   short unified diff snippet, then exits non-zero.
5. On clean: prints `Golden tree OK: generated *.hpp/*.cpp match generated_cpp_bindings/ byte-for-byte.` and exits 0.

There is **no sha256** step in the gate — comparison is a plain `diff -q` per file.
The gate is also run by `uv run pytest` via `tests/test_golden_tree.py` (the pytest
twin of the shell gate, which also covers both directions and is byte-exact).

**clang-format dependency.** The generator runs `clang-format` as part of emission.
Byte equality holds only when the same `clang-format` version that produced the
committed golden is installed. If `clang-format` is absent, the shell gate and
pytest skip rather than fail (the gate is conditionally disabled, not suppressed).
A version mismatch will produce formatting drift — install the matching version.

---

## What golden drift means

If `mise run golden` exits non-zero, the report will say one of three things:

| Report line | Meaning |
|---|---|
| `DRIFT: <file>` + unified diff | File exists in both trees but content differs |
| `MISSING in regen: <file>` | File is committed but the regenerator no longer emits it |
| `EXTRA (not committed): <file>` | Regenerator emits a new file that has not been committed |

Drift is always either **intentional** (you changed a template or emitter and need
to commit the new output) or **accidental** (something changed the regeneration
environment, e.g. a `clang-format` version mismatch or a non-deterministic emitter
path).

Accidental drift from a `clang-format` version mismatch produces formatting-only
diffs across every file simultaneously — a recognizable pattern. If you see that,
fix the formatter version, not the templates.

---

## When to regenerate

Regenerate the golden tree whenever you make an intentional change to:

- A Jinja2 template (`src/x3d_cpp_gen/templates/class_template.hpp.jinja`,
  `class_template.cpp.jinja`)
- An emitter module (`src/x3d_cpp_gen/emit/`, `src/x3d_cpp_gen/backends/`)
- The generator orchestrator (`src/x3d_cpp_gen/generator.py`) or parser
  (`src/x3d_cpp_gen/parser.py`) in ways that affect emitted output
- The UOM data file (`src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml`)

Do **not** regenerate to fix accidental drift caused by environment differences.
Fix the environment first, confirm the gate is clean, then proceed.

---

## How to regenerate and accept drift

```bash
# 1. Regenerate into the committed golden tree (overwrites in place)
uv run x3d-cpp-gen --out generated_cpp_bindings

# 2. Review what changed
git diff generated_cpp_bindings/

# 3. Stage all changed + any newly added files
git add generated_cpp_bindings/

# 4. Commit the generator change together with the regenerated output.
#    Stage the generator/template change too (if not already done).
git add src/x3d_cpp_gen/
git commit -m "feat(codegen): <describe the change>"

# 5. Verify the gate passes on the committed tree
mise run golden
```

The commit should include **both** the generator/template change and the regenerated
`generated_cpp_bindings/` output. This is the reviewable artifact: the diff in
`generated_cpp_bindings/` shows exactly what the emitter change produced across all
nodes, and can be bisected later with `git log generated_cpp_bindings/`.

The `--no-test` flag is already applied by `scripts/check_golden.sh` internally,
but when you run `uv run x3d-cpp-gen --out generated_cpp_bindings` directly
(without `--no-test`), the generator also writes a smoke `test.cpp` and its compiled
binary, both of which are gitignored. They do not appear in the diff and need not be
committed.

---

## The cli/canon gate baselines

The [Gate System](gate-system.md) also maintains two baseline TSVs that protect the
CLI differential harness:

- `tools/x3d-cli/goldens/cli-gate-baseline.tsv`
- `tools/x3d-cli/goldens/canon-gate-baseline.tsv`

These are not related to the generator golden tree. They record the expected
validate/convert/canonicalize behavior of the `x3d` CLI over the curated corpus
subset. To refresh them after intentionally changing CLI behavior:

```bash
mise run cli-gate-baseline
# Then review + commit the updated TSVs
git add tools/x3d-cli/goldens/cli-gate-baseline.tsv
git add tools/x3d-cli/goldens/canon-gate-baseline.tsv
git commit -m "chore(gate): accept new baseline divergences — <reason>"
```

See [Gate System](gate-system.md) for the full cli/canon gate workflow.

---

## Quick reference

| Situation | Command |
|---|---|
| Check whether the committed tree matches the generator | `mise run golden` |
| Regenerate the committed tree after a template/emitter change | `uv run x3d-cpp-gen --out generated_cpp_bindings` |
| Run the Python twin of the golden gate | `uv run pytest tests/test_golden_tree.py` |
| Run all gates together (includes golden) | `mise run ci` |
| Refresh CLI/canon baselines after accepting new behavior | `mise run cli-gate-baseline` |

---

## Gotchas

**clang-format version.** The gate is byte-exact, so the formatter version must
match. If you upgrade `clang-format`, you must regenerate and commit a new golden.
The gate skips (not fails) when `clang-format` is absent.

**Do not retry PCH.** A shared precompiled header was measured at ~0% build-time
improvement on this codebase (the bottleneck is template instantiation and codegen
of the `std::function` thunks, not parsing). The C1 decl/def split addresses the
root cause. See `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`.

**`X3DNode.hpp` and its siblings are generator-emitted.** Do not edit them by hand.
Any hand edit will be overwritten the next time someone runs `mise run gen`, and the
golden gate will detect the mismatch immediately. If you need to change the base
class behavior, change the template or emitter instead.

**Large diffs are normal after emitter changes.** A change that touches every node
(e.g., adding a field to `X3DNode`, changing the reflection thunk signature) will
produce a diff across all 683 files simultaneously. This is expected and reviewable:
the diff is uniform and each file's change is the same structural edit applied by
the emitter.

**Generator determinism is enforced.** The emitter uses `sorted()` throughout
(`src/x3d_cpp_gen/emit/factory.py`, `src/x3d_cpp_gen/generator.py`) rather than
iterating raw `dict`/`set` collections, so repeated runs on the same input must
produce byte-identical output. If you add emitter code that iterates a `set` or
unsorted `dict` and produces output, the gate will report spurious drift on CI even
when no logic changed. Always sort collections before emitting.
