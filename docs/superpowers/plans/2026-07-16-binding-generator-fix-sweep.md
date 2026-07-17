# Binding Generator Fix Sweep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the 9 highest-leverage correctness/maintainability/robustness gaps found by two independent reviews of `src/x3d_cpp_gen/` (the C++ binding generator): a broad architectural review (fable model, prioritized per explicit instruction) plus a narrower diff review of the just-merged Clang matrix-brace fix (PR #73 / issue #72).

**Architecture:** No architectural rewrite. Each task is an independently mergeable, narrowly-scoped fix inside the existing generator/descriptor/backend/template layering. The one deferred item — fully consolidating the ~6 scattered per-type fact tables into `model/types.py`'s `TypeRegistry` as the single source of truth (fable finding #1's "north star") — is intentionally OUT of this sweep; it is a large migration (would need to re-derive `generator.py`'s `FIELD_TYPE_MAPPING`/struct bodies and the test template's `defaultsEqual` overloads from one table while proving byte-identical golden output) that deserves its own dedicated plan. This sweep instead closes the specific, acute instance of that risk class that just caused a real bug (`_STRUCT_ARITY`'s hand-maintained `row_size`) and adds a coverage-completeness safety net, without attempting the full migration.

**Tech Stack:** Python 3 (generator, `uv run pytest`), Jinja2 (templates), C++20 (generated output, verified via `mise run build` / `ctest`).

## Global Constraints

- Repo root: `/home/ben/code/x3d-cpp`. Work in a fresh git worktree per `superpowers:using-git-worktrees` (branch off `main`, which already includes PR #73).
- Every task that changes generated C++ output is **codegen-authorized**: after editing `src/x3d_cpp_gen/`, run `uv run x3d-cpp-gen --out generated_cpp_bindings && uv run python scripts/gen_profile_tables.py`, then `git add` the changed generated files alongside the generator change in the SAME commit. Tasks that do NOT change emitted bytes (e.g. Task 7's dead-code deletion) must show a clean `git status` on `generated_cpp_bindings/` after regenerating — if anything changed, stop and investigate before committing.
- Run `uv run pytest` (full suite) before every commit. It includes the golden-tree gate (`tests/test_golden_tree.py`), which is the primary regression net for accidental output drift.
- For any task whose generated-output diff is nonzero, additionally run the real build: `cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"` (mirrors `mise run build`) — confirms the regenerated C++ actually compiles and passes, not just that Python tests pass.
- Follow the repo's card-to-done workflow: `docs/contributor/card-to-done-workflow.md`. Each task below becomes one PR. Branch naming: `feat/<task-slug>`. Commit messages and PR bodies must NOT contain `Claude-Session:` trailers or `claude.ai/code/session_…` URLs (tool-agnostic, per repo CLAUDE.md).
- `mise run docs-drift working` does not apply to any of these tasks (none touch `runtime/`, `tools/`, or `include/x3d/` — this is entirely inside the Python generator and its templates) — skip it, don't run it needlessly.

---

## File Structure

No new files except two: a small shared-helper module (Task 5) and its test. Everything else modifies existing files in place.

- `src/x3d_cpp_gen/emit/defaults.py` — Tasks 1, 3, 9 (matrix arity, enum defaults, chunk-helper dedup)
- `src/x3d_cpp_gen/generator.py` — Tasks 1, 5, 7 (coverage assertion, escaping, dead code)
- `src/x3d_cpp_gen/emit/descriptors.py` — Tasks 2, 4 (range diagnostics, reflection-descriptor resolution)
- `src/x3d_cpp_gen/backends/cpp_header.py` — Task 4 (delegates to the new descriptors.py function)
- `src/x3d_cpp_gen/backends/base.py` — Task 7 (delete)
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja`, `class_template.cpp.jinja` — Tasks 2, 5, 6
- `src/x3d_cpp_gen/emit/reflection.py` — Task 6 (new shared C++ helper emission)
- `src/x3d_cpp_gen/emit/naming.py` — Task 5 (new shared `cpp_str` formatter lives here, next to the existing `pascal`/`sanitize_field_name` naming helpers)
- `src/x3d_cpp_gen/parser.py`, `src/x3d_cpp_gen/cli.py`, `src/x3d_cpp_gen/model/enums.py` — Task 8
- `tests/test_defaults.py`, `tests/test_descriptors.py`, `tests/test_enums.py`, `tests/test_naming.py`, `tests/test_parser.py`, `tests/test_cli_fail_closed.py`, new `tests/test_reflection.py` — one or more per task

---

### Task 1: Derive matrix row_size from count; close the unmapped-struct-type gap

**Files:**
- Modify: `src/x3d_cpp_gen/emit/defaults.py:81-139` (`_STRUCT_ARITY`, `_struct_literal`)
- Modify: `src/x3d_cpp_gen/generator.py:68-72` (`SPECIAL_STRUCTS` — add a coverage assertion)
- Test: `tests/test_defaults.py` (extend existing matrix tests)

**Interfaces:**
- Consumes: `X3DType` enum from `model/types.py` (unchanged).
- Produces: `_STRUCT_ARITY` keeps its existing consumer contract — `default_expr_for` still does `_struct_literal(*_STRUCT_ARITY[x3d_type], d=d)` — but `row_size` is no longer a hand-typed literal for matrix rows; it derives from `count` via `math.isqrt`. `default_expr_for`'s fallback path (line 211, `return str(d).strip()`) now only triggers for genuinely-unmapped types, guarded by a generation-time assertion that fires BEFORE any of that fallback logic runs, in `generator.py`'s import path.

This closes both diff-review findings D1 (unmapped struct type silently produces invalid C++) and D2 (`row_size` redundant/unvalidated) from the same root cause: the table shouldn't carry a second hand-maintained number, and the two source-of-truth lists (`generator.py`'s `SPECIAL_STRUCTS` and `defaults.py`'s `_STRUCT_ARITY`) shouldn't be able to silently disagree on which types exist.

- [ ] **Step 1: Write the failing test for row_size derivation**

Add to `tests/test_defaults.py` (after the existing `test_sfmatrix4f_arity_padding_still_row_braced`):

```python
def test_struct_arity_row_size_matches_sqrt_of_count():
    # row_size must always be math.isqrt(count) for matrix entries (0 for
    # flat structs) -- this is what makes it impossible for the table to
    # drift the way it did before the Clang -Wmissing-braces fix (a
    # hand-typed row_size that silently disagreed with count).
    import math
    from x3d_cpp_gen.emit.defaults import _STRUCT_ARITY
    from x3d_cpp_gen.model.types import X3DType

    matrix_types = {
        X3DType.SFMatrix3f, X3DType.SFMatrix4f,
        X3DType.SFMatrix3d, X3DType.SFMatrix4d,
    }
    for x3d_type, (struct, count, floaty, row_size) in _STRUCT_ARITY.items():
        if x3d_type in matrix_types:
            assert row_size == math.isqrt(count), (
                f"{struct}: row_size={row_size} does not match "
                f"isqrt(count={count})={math.isqrt(count)}"
            )
            assert row_size * row_size == count, (
                f"{struct}: count={count} is not a perfect square of "
                f"row_size={row_size} -- a non-square matrix cannot use "
                f"this row-chunking scheme"
            )
        else:
            assert row_size == 0, f"{struct}: non-matrix type must have row_size=0"
```

- [ ] **Step 2: Run test to verify it currently passes (it's a characterization test of the existing hand-typed values, not a behavior change yet)**

Run: `cd src/x3d_cpp_gen/.. && uv run pytest tests/test_defaults.py::test_struct_arity_row_size_matches_sqrt_of_count -v`
Expected: PASS (the current hand-typed values already happen to be correct — this test guards against future drift, it doesn't fix a live bug).

- [ ] **Step 3: Derive row_size instead of hand-typing it**

Replace the `_STRUCT_ARITY` table in `src/x3d_cpp_gen/emit/defaults.py` (currently lines 96-110):

```python
import math

# ... (keep existing module docstring/imports above)

# Struct SF* types -> (struct name, component count, is-float, row size).
# Drives the brace initializer for fixed-arity vector/colour/rotation/matrix
# defaults. ``row_size`` is nonzero only for the matrix types: their struct
# wraps a genuine 2D array member (e.g. ``float matrix[4][4]``), which C++
# aggregate-init rules let you flatten into a single elided-brace list -- but
# Clang's -Wmissing-braces (promoted to -Werror) rejects the elided form even
# though GCC accepts it silently, and empirically a single extra wrapping
# brace around the flat list is *not* enough either: Clang still wants each
# row of the 2D array individually braced (verified directly against both
# clang and gcc with -Wmissing-braces -Werror, not just read about). So a
# matrix's literal chunks the flat value list into ``row_size``-sized rows,
# each explicitly braced, wrapped in one more brace for the array member
# itself: ``Struct{ {row0}, {row1}, ... }``. The plain vector/colour/rotation
# structs are flat (e.g. ``float x, y, z;``), where a single brace is exactly
# right and extra nesting would itself warn.
#
# row_size is DERIVED (math.isqrt(count)) rather than hand-typed: a
# hand-maintained row_size that could silently disagree with count is exactly
# the class of bug that produced the Clang -Wmissing-braces break this table
# was introduced to fix (a stale/wrong value with no validation). All
# supported matrix types are square, so isqrt is exact for them; the assert
# in _matrix_row_size below fails loudly if a future non-square matrix type
# is ever added here (it would need a different chunking scheme entirely).
def _matrix_row_size(count: int) -> int:
    row_size = math.isqrt(count)
    assert row_size * row_size == count, (
        f"_STRUCT_ARITY matrix entry with count={count} is not a perfect "
        f"square -- row-chunking assumes a square matrix; a non-square "
        f"matrix type needs a different scheme, not this helper."
    )
    return row_size


_STRUCT_ARITY = {
    X3DType.SFVec2f: ("SFVec2f", 2, True, 0),
    X3DType.SFVec3f: ("SFVec3f", 3, True, 0),
    X3DType.SFVec4f: ("SFVec4f", 4, True, 0),
    X3DType.SFColor: ("SFColor", 3, True, 0),
    X3DType.SFColorRGBA: ("SFColorRGBA", 4, True, 0),
    X3DType.SFVec2d: ("SFVec2d", 2, False, 0),
    X3DType.SFVec3d: ("SFVec3d", 3, False, 0),
    X3DType.SFVec4d: ("SFVec4d", 4, False, 0),
    X3DType.SFRotation: ("SFRotation", 4, True, 0),
    X3DType.SFMatrix3f: ("SFMatrix3f", 9, True, _matrix_row_size(9)),
    X3DType.SFMatrix4f: ("SFMatrix4f", 16, True, _matrix_row_size(16)),
    X3DType.SFMatrix3d: ("SFMatrix3d", 9, False, _matrix_row_size(9)),
    X3DType.SFMatrix4d: ("SFMatrix4d", 16, False, _matrix_row_size(16)),
}
```

Leave `_struct_literal` and `_MF_STRUCT_ELEM` exactly as they are (this task only changes how `row_size` gets its value, not how it's consumed).

- [ ] **Step 4: Run the full defaults test file to confirm nothing broke**

Run: `uv run pytest tests/test_defaults.py -v`
Expected: all PASS, including the new `test_struct_arity_row_size_matches_sqrt_of_count`.

- [ ] **Step 5: Write the failing test for struct-type coverage completeness**

Add to `tests/test_defaults.py`:

```python
def test_every_matrix_shaped_special_struct_has_a_struct_arity_entry():
    # generator.py's SPECIAL_STRUCTS defines the actual C++ struct shapes
    # (which ones wrap a nested 2D array vs. flat scalars). _STRUCT_ARITY
    # must have an entry for every one of them, or default_expr_for silently
    # falls through to a bare unbraced token-string fallback for the missing
    # type -- not just a Clang warning, but invalid C++ in the initializer
    # position. This test is the coverage safety net: it fails loudly at
    # test time instead of failing obscurely at C++ compile time.
    from x3d_cpp_gen import generator
    from x3d_cpp_gen.emit.defaults import _STRUCT_ARITY
    from x3d_cpp_gen.model.types import resolve_x3d_type

    covered_struct_names = {arity[0] for arity in _STRUCT_ARITY.values()}
    # SFImage is intentionally NOT in _STRUCT_ARITY -- it has bespoke
    # default_expr_for handling (a std::vector<unsigned char> data member with
    # no meaningful literal default), so it's an expected, documented gap.
    expected_gap = {"SFImage"}
    missing = [
        name for name in generator.SPECIAL_STRUCTS
        if name not in covered_struct_names and name not in expected_gap
    ]
    assert not missing, (
        f"SPECIAL_STRUCTS entries with no _STRUCT_ARITY coverage: {missing}. "
        f"A field of this type with a spec default will silently emit an "
        f"unbraced token list instead of a valid struct literal. Add an "
        f"entry to _STRUCT_ARITY in emit/defaults.py."
    )
    # Cross-check the other direction too: every _STRUCT_ARITY struct name
    # must actually be a real SPECIAL_STRUCTS entry (catches typos).
    extra = covered_struct_names - set(generator.SPECIAL_STRUCTS)
    assert not extra, f"_STRUCT_ARITY names a struct SPECIAL_STRUCTS doesn't define: {extra}"
```

- [ ] **Step 6: Run test to verify it fails**

Run: `uv run pytest tests/test_defaults.py::test_every_matrix_shaped_special_struct_has_a_struct_arity_entry -v`
Expected: FAIL — `import generator` in `tests/test_defaults.py`'s test needs `from x3d_cpp_gen import generator` to resolve, which it should (the package is installed editable via `uv run`), so this should actually PASS immediately since the current `_STRUCT_ARITY`/`SPECIAL_STRUCTS` already agree. If it fails with an import error, fix the import; if it fails with a real coverage gap, that's a genuine finding — investigate before proceeding (there should be none: `SPECIAL_STRUCTS` has 14 entries, `_STRUCT_ARITY` covers 13 of them, and `SFImage` is the one documented, intentional gap).

- [ ] **Step 7: Confirm PASS**

Run: `uv run pytest tests/test_defaults.py -v`
Expected: all PASS.

- [ ] **Step 8: Regenerate and confirm byte-identical output**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git status --short generated_cpp_bindings/
```
Expected: no output from `git status --short` (this task only adds derivation/validation, doesn't change any emitted value — `math.isqrt(9)==3` and `math.isqrt(16)==4` match the hand-typed values exactly).

- [ ] **Step 9: Commit**

```bash
git add src/x3d_cpp_gen/emit/defaults.py src/x3d_cpp_gen/generator.py tests/test_defaults.py
git commit -m "gen: derive matrix row_size from count, add struct-arity coverage test

row_size was a hand-typed second number that could silently disagree with
count -- exactly the class of bug that caused the Clang -Wmissing-braces
break just fixed in PR #73. Derive it via math.isqrt with a self-consistency
assert instead. Also add a coverage-completeness test so a future
SPECIAL_STRUCTS entry with no matching _STRUCT_ARITY row fails at test time
(invalid C++ initializer) rather than silently at compile time far from the
real cause."
```

---

### Task 2: Extend range diagnostics to constrained initializeOnly fields

**Files:**
- Modify: `src/x3d_cpp_gen/emit/descriptors.py:23-58` (add `has_range_diagnostics` property), `:302-357` (`build_descriptor` — split `wants_validation`)
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja:268`, `:285` (broaden the `selectattr` filter)
- Modify: `src/x3d_cpp_gen/templates/class_template.cpp.jinja:110`, `:127` (same)
- Test: `tests/test_descriptors.py`

**Interfaces:**
- Consumes: `FieldDescriptor.access_type`, `.range_collect_body` (both already exist).
- Produces: new `FieldDescriptor.has_range_diagnostics` property (`bool`), independent of `has_constraints`. Templates key range-diagnostic emission (the `checkRanges<Name>()` static + its call from `validateRanges()`) off `has_range_diagnostics` instead of `is_settable and has_constraints`. The throwing `constraint_checks`/`validate<Name>()` path is UNCHANGED (still `inputOutput`-only via `has_constraints`/`is_settable`), because only `inputOutput` fields have a public typed setter to protect.

- [ ] **Step 1: Write the failing test**

Add to `tests/test_descriptors.py` (check the existing file's fixture/import style first — the pattern below assumes a `field` factory similar to what's already there; adjust field construction to match the existing `X3DField`-building helper in that file):

```python
def test_initializeonly_constrained_field_gets_range_diagnostics_not_throwing_validation():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="order", type="SFInt32", accessType="initializeOnly",
        x3d_name="order", default="4", min_inclusive="0", max_inclusive="5",
    )
    d = build_descriptor(field)

    # initializeOnly must NOT get the throwing validate<Name>() path -- it has
    # no public typed setter to protect (data-layer writes always go through
    # set<Name>Unchecked by design).
    assert d.constraint_checks is None
    assert not d.has_constraints

    # But it MUST get the non-throwing diagnostic-collection path, so an
    # out-of-range authored value is at least surfaced via validateRanges()/
    # collectRangeWarnings() instead of vanishing silently.
    assert d.range_collect_body is not None
    assert d.has_range_diagnostics


def test_inputoutput_constrained_field_still_gets_both_paths():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="transparency", type="SFFloat", accessType="inputOutput",
        x3d_name="transparency", default="0", min_inclusive="0", max_inclusive="1",
    )
    d = build_descriptor(field)

    assert d.constraint_checks is not None
    assert d.has_constraints
    assert d.range_collect_body is not None
    assert d.has_range_diagnostics


def test_unconstrained_field_gets_neither():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="name", type="SFString", accessType="inputOutput",
        x3d_name="name", default="",
    )
    d = build_descriptor(field)

    assert d.constraint_checks is None
    assert not d.has_constraints
    assert d.range_collect_body is None
    assert not d.has_range_diagnostics
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `uv run pytest tests/test_descriptors.py -k "range_diagnostics or both_paths or neither" -v`
Expected: FAIL — `test_initializeonly_constrained_field_gets_range_diagnostics_not_throwing_validation` fails because `range_collect_body` is currently `None` for `initializeOnly` (the shared `wants_validation` gate excludes it); `has_range_diagnostics` doesn't exist yet (`AttributeError`).

- [ ] **Step 3: Add the `has_range_diagnostics` property**

In `src/x3d_cpp_gen/emit/descriptors.py`, add next to the existing `has_constraints` property (around line 227-228):

```python
    @property
    def has_constraints(self) -> bool:
        return self.constraint_checks is not None

    @property
    def has_range_diagnostics(self) -> bool:
        """True when this field gets a non-throwing checkRanges<Name>() static.

        Broader than has_constraints: inputOutput AND initializeOnly fields
        with spec range bounds both get the non-throwing diagnostic path
        (only inputOutput additionally gets the throwing validate<Name>(),
        since only it has a public typed setter to protect).
        """
        return self.range_collect_body is not None
```

- [ ] **Step 4: Split `wants_validation` in `build_descriptor`**

In `src/x3d_cpp_gen/emit/descriptors.py`, replace lines 335-357 (from `# Constraint resolution:` through the `range_collect_body = (...)` assignment):

```python
    # Constraint resolution: a color type always clamps to [0,1]; otherwise use
    # the spec-declared inclusive bounds.
    is_color = x3d_type in _COLOR_TYPES
    lo = field.min_inclusive
    hi = field.max_inclusive
    if is_color:
        lo, hi = "0", "1"
    has_bounds = (
        x3d_type is not None
        and (field.min_inclusive is not None
             or field.max_inclusive is not None
             or is_color)
    )
    # Throwing validation (validate<Name>() called from the public setter)
    # only makes sense where a public typed setter exists to protect:
    # inputOutput only. Matches the original template gate exactly.
    wants_throwing_validation = access == "inputOutput" and has_bounds
    # Non-throwing diagnostic collection (checkRanges<Name>(), surfaced via
    # validateRanges()/collectRangeWarnings()) is broader: inputOutput OR
    # initializeOnly. Both have a data-layer write path (set<Name>Unchecked
    # for initializeOnly, same for constrained inputOutput) that bypasses the
    # throwing check, so both need a way to surface an out-of-range authored
    # value as a structured diagnostic instead of losing it silently.
    wants_range_diagnostics = (
        access in ("inputOutput", "initializeOnly") and has_bounds
    )
    constraint_checks = (
        _render_constraints(field.name, x3d_type, lo, hi)
        if wants_throwing_validation else None
    )
    range_collect_body = (
        _render_range_collect(field.name, x3d_type, lo, hi)
        if wants_range_diagnostics else None
    )
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `uv run pytest tests/test_descriptors.py -v`
Expected: all PASS.

- [ ] **Step 6: Broaden the template filters**

In `src/x3d_cpp_gen/templates/class_template.hpp.jinja`, change line 268:
```jinja
{% set _constrained = reflection_fields | selectattr('is_settable') | selectattr('has_constraints') | list %}
```
to:
```jinja
{% set _constrained = reflection_fields | selectattr('has_range_diagnostics') | list %}
```

And line 285:
```jinja
{% set _own_constrained = fields | selectattr('is_settable') | selectattr('has_constraints') | list %}
```
to:
```jinja
{% set _own_constrained = fields | selectattr('has_range_diagnostics') | list %}
```

Leave line 301 (`{% if field.is_settable and field.has_constraints %}`, the throwing `validate<Name>()` private static) UNCHANGED — that path must stay `inputOutput`-only.

In `src/x3d_cpp_gen/templates/class_template.cpp.jinja`, apply the identical change at line 110 and line 127 (same before/after text as above).

- [ ] **Step 7: Regenerate and inspect the diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git diff --stat generated_cpp_bindings/
```
Expected: a nonzero diff — every node with an `initializeOnly` field that has `minInclusive`/`maxInclusive` now gets a `checkRanges<Name>()` static + a `validateRanges()` call for it. Read a sample diff (e.g. `git diff generated_cpp_bindings/x3d/nodes/NurbsPatchSurface.hpp` or grep the diff for `uOrder`/`vOrder`) to confirm the new static compiles-looking C++ (a `checkRanges` declaration in the `protected:` section, a definition in the `.cpp` appending a `RangeDiagnostic` on out-of-range).

- [ ] **Step 8: Run the full pytest suite**

Run: `uv run pytest -v 2>&1 | tail -40`
Expected: all pass, including `tests/test_golden_tree.py` (it will now compare against the freshly-regenerated tree you just wrote in Step 7 — since you already regenerated in-place, the golden-tree gate compares committed-vs-committed and passes trivially; the REAL check is the full pytest run below after committing).

- [ ] **Step 9: Full local build + ctest**

Run:
```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"
```
Expected: 100% tests passed. This is the step that actually proves the new `checkRanges<Name>()` statics compile and the existing `x3d_extract_tests`/`x3d_parse_tests` (which exercise `validateRanges()`/`collectRangeWarnings()`) still pass with the broadened field set.

- [ ] **Step 10: Commit**

```bash
git add src/x3d_cpp_gen/emit/descriptors.py src/x3d_cpp_gen/templates/class_template.hpp.jinja src/x3d_cpp_gen/templates/class_template.cpp.jinja generated_cpp_bindings/ tests/test_descriptors.py
git commit -m "gen: surface range diagnostics for constrained initializeOnly fields

initializeOnly fields with spec minInclusive/maxInclusive (e.g. NURBS
uOrder/vOrder, X3DDamperNode.order) were excluded from BOTH the throwing
validate<Name>() (correctly -- they have no public setter to protect) AND
the non-throwing checkRanges<Name>()/validateRanges() diagnostic path
(incorrectly -- X3DReflection.hpp's own docs promise out-of-range authored
values 'are kept, validateRanges()/collectRangeWarnings() recover them as
structured diagnostics'). Split the single wants_validation gate into
wants_throwing_validation (inputOutput only, unchanged) and
wants_range_diagnostics (inputOutput OR initializeOnly)."
```

---

### Task 3: Fail loudly on unmatched enum default tokens

**Files:**
- Modify: `src/x3d_cpp_gen/emit/defaults.py:214-242` (`enum_default_expr`)
- Test: `tests/test_enums.py` (note: `tests/test_enums.py::test_enum_default_value_fallback_to_first_member` currently PINS the silent-fallback behavior as intended — this step replaces that test, it does not add alongside it)

**Interfaces:**
- Consumes: `EnumDef` from `model/enums.py` (unchanged).
- Produces: `enum_default_expr` now raises `ValueError` for an unmatched SF-enum default token instead of silently substituting the first member. MF-enum unmatched tokens are still dropped (per-token, not fatal — a single typo'd token in a large space-separated MF default shouldn't kill generation the same way an entirely-wrong SF default should), but generation now emits a loud `print(...)` warning naming the exact node/field/token so a dropped token is at least visible, matching the project's existing "skip is legitimate only when explicit/visible" fail-closed philosophy already established in `tests/test_cli_fail_closed.py`.

- [ ] **Step 1: Read the current test being replaced**

Run: `uv run pytest tests/test_enums.py -k fallback_to_first_member -v` and read `tests/test_enums.py` around that test name to see its exact current assertions before changing them.

- [ ] **Step 2: Write the new failing test**

In `tests/test_enums.py`, REPLACE `test_enum_default_value_fallback_to_first_member` (find it by name, delete the old body) with:

```python
def test_enum_default_expr_raises_on_unmatched_sf_token():
    from x3d_cpp_gen.emit.defaults import enum_default_expr
    from x3d_cpp_gen.model.enums import EnumDef, EnumMember

    enum_def = EnumDef(
        name="alphaModeChoices", cpp_name="AlphaModeChoices",
        base_type="SFString",
        members=[EnumMember(value="OPAQUE", cpp_name="OPAQUE"),
                 EnumMember(value="MASK", cpp_name="MASK")],
    )
    with pytest.raises(ValueError, match="alphaModeChoices"):
        enum_default_expr(enum_def, "NOT_A_REAL_TOKEN")


def test_enum_default_expr_sf_matches_correctly():
    from x3d_cpp_gen.emit.defaults import enum_default_expr
    from x3d_cpp_gen.model.enums import EnumDef, EnumMember

    enum_def = EnumDef(
        name="alphaModeChoices", cpp_name="AlphaModeChoices",
        base_type="SFString",
        members=[EnumMember(value="OPAQUE", cpp_name="OPAQUE"),
                 EnumMember(value="MASK", cpp_name="MASK")],
    )
    assert enum_default_expr(enum_def, "MASK") == "AlphaModeChoices::MASK"


def test_enum_default_expr_mf_drops_unmatched_tokens_with_warning(capsys):
    from x3d_cpp_gen.emit.defaults import enum_default_expr
    from x3d_cpp_gen.model.enums import EnumDef, EnumMember

    enum_def = EnumDef(
        name="fooChoices", cpp_name="FooChoices",
        base_type="MFString",
        members=[EnumMember(value="A", cpp_name="A"),
                 EnumMember(value="B", cpp_name="B")],
    )
    result = enum_default_expr(enum_def, '"A" "NOT_REAL" "B"')
    assert result == "std::vector<FooChoices>{FooChoices::A, FooChoices::B}"
    captured = capsys.readouterr()
    assert "fooChoices" in captured.out
    assert "NOT_REAL" in captured.out
```

Make sure `import pytest` is present at the top of `tests/test_enums.py` (check first; add if missing).

- [ ] **Step 3: Run tests to verify they fail**

Run: `uv run pytest tests/test_enums.py -k "unmatched_sf_token or sf_matches_correctly or drops_unmatched_tokens" -v`
Expected: FAIL — the SF case currently returns the first member silently instead of raising; the MF case currently drops silently with no printed warning.

- [ ] **Step 4: Implement**

Replace `enum_default_expr` in `src/x3d_cpp_gen/emit/defaults.py` (lines 214-242):

```python
def enum_default_expr(enum_def, default: Optional[str]) -> Optional[str]:
    """C++ default initializer for an enum-typed field.

    For a single-valued (SF) enum field this is ``EnumClass::MEMBER``. For a
    multi-valued (MF) enum field it is ``std::vector<EnumClass>{MEMBER, ...}``
    built from the (quoted, whitespace-separated) default tokens.

    An SF default token that doesn't match any member is a spec/generator
    mismatch (a renamed token, a typo, a newer spec revision) -- raising here
    turns a silently-wrong generated default into a loud generation-time
    failure, rather than emitting a default that happens to compile but does
    not match what the X3D spec actually says. An MF default with SOME
    unmatched tokens among otherwise-valid ones is not escalated the same
    way (a single bad token in a longer list is much lower-blast-radius than
    an entirely wrong SF default) but is printed loudly so it's visible.
    """
    cpp = enum_def.cpp_name
    if not enum_def.is_multi:
        if default is None:
            return None
        member = enum_def.member_for_value(default)
        if member is None:
            raise ValueError(
                f"Enum default token {default!r} does not match any member "
                f"of SimpleType {enum_def.name!r} (known: "
                f"{[m.value for m in enum_def.members]}). This is a spec/"
                f"generator mismatch -- check for a renamed token or a UOM "
                f"version drift, and update model/enums.py's parsing or the "
                f"UOM source, not this generator."
            )
        return f"{cpp}::{member.cpp_name}"

    # Multi-valued enum field: vector of enum members. Unmatched tokens are
    # dropped (not fatal -- see docstring) but printed so they're visible.
    toks = tokenize_mfstring(default) if default else []
    members = []
    for t in toks:
        m = enum_def.member_for_value(t)
        if m is None:
            print(f"WARNING: enum default token {t!r} on SimpleType "
                  f"{enum_def.name!r} does not match any member; dropping "
                  f"it from the generated default.")
            continue
        members.append(m)
    if not members:
        return f"std::vector<{cpp}>{{}}"
    body = ", ".join(f"{cpp}::{m.cpp_name}" for m in members)
    return f"std::vector<{cpp}>{{{body}}}"
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `uv run pytest tests/test_enums.py tests/test_defaults.py tests/test_default_expr.py -v`
Expected: all PASS.

- [ ] **Step 6: Regenerate and confirm no generation-time crash**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
```
Expected: generation completes successfully with NO `ValueError` raised — this proves the current UOM 4.0 spec has no enum-default/member mismatches (if it DID raise here, that would itself be a real finding to investigate before proceeding, not something to silence).

- [ ] **Step 7: Confirm no output diff**

Run: `git status --short generated_cpp_bindings/`
Expected: empty (this task only changes error-handling behavior for a currently-unhit code path; the happy path is unchanged).

- [ ] **Step 8: Full pytest + commit**

Run: `uv run pytest -v 2>&1 | tail -20`
Expected: all pass.

```bash
git add src/x3d_cpp_gen/emit/defaults.py tests/test_enums.py
git commit -m "gen: fail loudly on an unmatched SF-enum default token

enum_default_expr silently substituted the first enum member when a spec
default token didn't match any known member -- fabricating a wrong-but-
compiling default instead of failing. This is strictly worse than a loud
failure: conformance/fidelity.py exists precisely because a dropped/mangled
default is the audit's historical #1 bug class, and this path bypassed that
check from inside the generator itself. SF mismatches now raise ValueError;
MF mismatches (lower blast radius -- one bad token among many valid ones)
still drop the token but now print a visible warning naming the exact
SimpleType and token."
```

---

### Task 4: Move reflection-descriptor resolution out of the backend

**Files:**
- Modify: `src/x3d_cpp_gen/emit/descriptors.py` (add `build_reflection_descriptors`)
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py:90-155` (delegate to it)
- Test: `tests/test_descriptors.py`

**Interfaces:**
- Consumes: `X3DNode`, `dependency_graph: Dict[str, List[str]]` (both already used by `CppHeaderBackend.emit`).
- Produces: new function `build_reflection_descriptors(node, nodes, dependency_graph, enum_defs=None, *, reaches_root, ancestors) -> List[FieldDescriptor]` in `emit/descriptors.py`, doing exactly what `CppHeaderBackend.emit`'s inline loop (lines 115-155) currently does: build descriptors for `node.fields` (own + inherited), resolve each one's `inherited_from` qualifier by finding which ancestor actually declares it (not the UOM's raw, sometimes-wrong `inheritedFrom` attribute), and drop phantom fields (declared-nowhere-in-the-C++-hierarchy). `CppHeaderBackend.emit` calls this instead of inlining the logic, passing its own `_ancestors` static method as the `ancestors` callable. This makes the phantom-drop and diamond-disambiguation logic directly unit-testable without spinning up the full backend/template pipeline.

- [ ] **Step 1: Write the failing test**

Add to `tests/test_descriptors.py`:

```python
def test_build_reflection_descriptors_drops_phantom_fields():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    # "ghost" claims inheritedFrom="NeverDeclaresIt" but no node in the
    # hierarchy actually owns a field named "ghost" -- this is the UOM
    # phantom-field pattern (9 such fields exist in the real 4.0 spec).
    child = X3DNode(
        name="Child", base_type="Parent",
        fields=[
            X3DField(name="ghost", type="SFBool", accessType="inputOutput",
                     x3d_name="ghost", inherited_from="NeverDeclaresIt"),
            X3DField(name="real", type="SFBool", accessType="inputOutput",
                     x3d_name="real"),
        ],
    )
    parent = X3DNode(name="Parent", fields=[])
    nodes = {"Child": child, "Parent": parent}
    dependency_graph = {"Child": ["Parent"], "Parent": []}

    descriptors = build_reflection_descriptors(
        child, nodes, dependency_graph,
        own_field_names={"Child": {"real"}, "Parent": set()},
        ancestors=["Parent"],
    )

    names = {d.x3d_name for d in descriptors}
    assert "real" in names
    assert "ghost" not in names, "phantom field (declared nowhere) must be dropped"


def test_build_reflection_descriptors_qualifies_inherited_fields_by_true_declarer():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    # "color" is claimed inheritedFrom="WrongClass" in the UOM data, but is
    # actually declared by "RealBase" per own_field_names -- the resolver
    # must trust own_field_names (derived from the actual C++ hierarchy),
    # not the UOM's raw inheritedFrom attribute.
    child = X3DNode(
        name="Child", base_type="RealBase",
        fields=[
            X3DField(name="color", type="SFColor", accessType="inputOutput",
                     x3d_name="color", inherited_from="WrongClass"),
        ],
    )
    nodes = {"Child": child}
    dependency_graph = {"Child": ["RealBase"], "RealBase": []}

    descriptors = build_reflection_descriptors(
        child, nodes, dependency_graph,
        own_field_names={"Child": set(), "RealBase": {"color"}},
        ancestors=["RealBase"],
    )

    assert len(descriptors) == 1
    assert descriptors[0].inherited_from == "RealBase"


def test_build_reflection_descriptors_leaves_own_fields_unqualified():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    node = X3DNode(
        name="Leaf", fields=[
            X3DField(name="size", type="SFVec3f", accessType="inputOutput",
                     x3d_name="size"),
        ],
    )
    descriptors = build_reflection_descriptors(
        node, {"Leaf": node}, {"Leaf": []},
        own_field_names={"Leaf": {"size"}},
        ancestors=[],
    )
    assert descriptors[0].inherited_from is None
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `uv run pytest tests/test_descriptors.py -k build_reflection_descriptors -v`
Expected: FAIL — `ImportError: cannot import name 'build_reflection_descriptors'`.

- [ ] **Step 3: Implement `build_reflection_descriptors`**

Add to `src/x3d_cpp_gen/emit/descriptors.py`, after `build_descriptors` (end of file):

```python
def build_reflection_descriptors(node, nodes, dependency_graph,
                                 *, own_field_names, ancestors,
                                 enum_defs: Optional[Dict] = None) -> List[FieldDescriptor]:
    """Build the FULL reflection field set for ``node`` (own + inherited),
    with each descriptor's ``inherited_from`` resolved to the class that
    ACTUALLY declares it, and phantom fields dropped.

    The UOM flattens inherited fields into every node so the reflection
    table doesn't need to walk the C++ base chain -- but its raw
    ``field/@inheritedFrom`` attribute does not always name the class that
    actually declares the field (a UOM data quirk). This function re-derives
    the true declaring ancestor from ``own_field_names`` (each class's ACTUAL
    own-declared field set, keyed by the field's wire/x3d_name) instead of
    trusting the UOM attribute, and drops any field that names no real
    declarer anywhere in the hierarchy (a "phantom" field -- 9 such fields
    exist in the 4.0 UOM; there is no accessor/member for them anywhere in
    the generated C++, so keeping them in the reflection table would emit a
    call to a nonexistent accessor).

    ``own_field_names``: ``{class_name: {wire_field_name, ...}}`` for every
    class in the hierarchy (own-declared fields only, not inherited).
    ``ancestors``: the ordered list of ``node``'s transitive base classes
    (nearest-first is not required; the first ancestor found declaring the
    field wins, matching the prior inline behavior in CppHeaderBackend).
    """
    descriptors = list(build_descriptors(node.fields, enum_defs))
    own_here = own_field_names.get(node.name, set())
    resolved: List[FieldDescriptor] = []
    for d in descriptors:
        if d.x3d_name in own_here:
            d.inherited_from = None
            resolved.append(d)
            continue
        declaring = next(
            (a for a in ancestors if d.x3d_name in own_field_names.get(a, set())),
            None,
        )
        if declaring is None:
            # Phantom field: dropped, not emitted into the reflection table.
            continue
        d.inherited_from = declaring
        resolved.append(d)
    return resolved
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `uv run pytest tests/test_descriptors.py -v`
Expected: all PASS.

- [ ] **Step 5: Delegate from `CppHeaderBackend.emit`**

In `src/x3d_cpp_gen/backends/cpp_header.py`, replace lines 115-155 (from `for node in nodes.values():` through `reflection_descriptors = resolved`) with:

```python
        from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

        for node in nodes.values():
            descriptors = build_descriptors(get_own_fields(node), self.enum_defs)
            ancestors = self._ancestors(node.name, dependency_graph)
            reflection_descriptors = build_reflection_descriptors(
                node, nodes, dependency_graph,
                own_field_names=own_field_names, ancestors=ancestors,
                enum_defs=self.enum_defs,
            )
```

(This removes the now-redundant inline `reflection_descriptors = list(build_descriptors(node.fields, ...))` assignment and the manual resolution loop that followed it — everything from the old `reflection_descriptors = list(...)` line through the old `reflection_descriptors = resolved` line is replaced by the 3-line call above. The `own_here`/`ancestors` local variables the old loop computed inline are gone too — `ancestors` is now computed once per node right before the call, `own_here`/`declaring`/`resolved` live entirely inside `build_reflection_descriptors` now.)

Double check the `all_bases`/`dependencies`/`is_root`/`emit_reflection` computation immediately below (previously right after the old inline loop) is untouched and still reads `node`, `dependency_graph` etc. correctly — it doesn't depend on anything the removed block computed, so it should need no changes.

- [ ] **Step 6: Run the full pytest suite**

Run: `uv run pytest -v 2>&1 | tail -40`
Expected: all PASS, INCLUDING `tests/test_golden_tree.py` and `tests/test_emission.py` — this step must produce byte-identical output to before, since it's a pure refactor (same logic, moved location). If golden-tree fails, the refactor introduced a behavior change — diff the failure output carefully before proceeding; do not "fix" the golden files to match, find the logic discrepancy.

- [ ] **Step 7: Regenerate explicitly and confirm zero diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git status --short generated_cpp_bindings/
```
Expected: empty.

- [ ] **Step 8: Commit**

```bash
git add src/x3d_cpp_gen/emit/descriptors.py src/x3d_cpp_gen/backends/cpp_header.py tests/test_descriptors.py
git commit -m "gen: extract reflection-descriptor resolution into emit/descriptors.py

inherited_from qualifier resolution (re-deriving the true declaring ancestor
from the UOM's sometimes-wrong inheritedFrom attribute) and phantom-field
dropping lived inline in CppHeaderBackend.emit, mutating descriptor
dataclasses in place -- the thickest semantics in the emit path, in the
layer generate_cpp_bindings's own docstring says only 'renders the thin
template.' Extracted to build_reflection_descriptors() in emit/descriptors.py
with direct unit tests for the phantom-drop and re-qualification cases,
which previously had no test touching them directly (only indirectly via
golden-tree equality)."
```

---

### Task 5: Unify C++ string-literal escaping

**Files:**
- Modify: `src/x3d_cpp_gen/emit/naming.py` (add `cpp_str`)
- Modify: `src/x3d_cpp_gen/emit/defaults.py` (`cpp_string_literal` becomes a thin wrapper or is replaced by call sites using `cpp_str`)
- Modify: `src/x3d_cpp_gen/generator.py` (`_cpp_str` replaced by the shared one)
- Modify: `src/x3d_cpp_gen/backends/cpp_header.py` (register `cpp_str` as a Jinja filter)
- Modify: `src/x3d_cpp_gen/templates/class_template.hpp.jinja` (apply the filter to previously-unescaped interpolations)
- Test: `tests/test_naming.py`

**Interfaces:**
- Consumes: nothing new.
- Produces: `emit.naming.cpp_str(s: str) -> str` — takes a raw Python string, returns the ESCAPED BODY (matching `cpp_string_literal`'s existing contract: caller wraps in the surrounding quotes) suitable for embedding in a C++ string literal, escaping `\`, `"`, `\n`, `\t`, `\r` (the union of what the two existing inconsistent implementations covered). Registered as a Jinja filter named `cpp_str` on the header backend's `Environment`, usable in templates as `{{ value | cpp_str }}`.

- [ ] **Step 1: Write the failing test**

Add to `tests/test_naming.py`:

```python
def test_cpp_str_escapes_backslash_and_quote():
    from x3d_cpp_gen.emit.naming import cpp_str
    assert cpp_str('a"b\\c') == 'a\\"b\\\\c'


def test_cpp_str_escapes_control_chars():
    from x3d_cpp_gen.emit.naming import cpp_str
    assert cpp_str("line1\nline2\ttabbed\rcr") == "line1\\nline2\\ttabbed\\rcr"


def test_cpp_str_handles_comment_terminator():
    # A description containing "*/" must not be able to close a /** ... */
    # Doxygen block early -- cpp_str itself only escapes string-literal
    # metacharacters, but this test documents the exact input that would
    # break an @details comment if a caller used raw interpolation instead
    # of routing through cpp_str (or a comment-safe filter) -- see Step 5.
    from x3d_cpp_gen.emit.naming import cpp_str
    # cpp_str's job is string-literal safety, not comment safety -- "*/"
    # inside a STRING LITERAL is completely inert, this assertion just pins
    # that cpp_str does not mangle it.
    assert cpp_str("ends with */ here") == "ends with */ here"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_naming.py -k cpp_str -v`
Expected: FAIL — `ImportError: cannot import name 'cpp_str'`.

- [ ] **Step 3: Implement `cpp_str` in `emit/naming.py`**

Read the current top of `src/x3d_cpp_gen/emit/naming.py` first (`pascal`, `sanitize_field_name`, `CPP_RESERVED_KEYWORDS` already live there) to match its docstring style, then add:

```python
def cpp_str(s: str) -> str:
    """Escape a Python string into the body of a C++ double-quoted literal.

    Escapes backslash, double-quote, newline, tab, and carriage return -- the
    union of what this generator's two previously-separate, inconsistent
    escaping implementations covered (emit.defaults.cpp_string_literal used
    to escape all five; generator._cpp_str used to escape only backslash and
    quote). One shared implementation now backs every quoted interpolation
    site, including Jinja templates via the registered ``cpp_str`` filter.
    """
    if s is None:
        return ""
    out = []
    for ch in s:
        if ch == '\\':
            out.append('\\\\')
        elif ch == '"':
            out.append('\\"')
        elif ch == '\n':
            out.append('\\n')
        elif ch == '\t':
            out.append('\\t')
        elif ch == '\r':
            out.append('\\r')
        else:
            out.append(ch)
    return ''.join(out)
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_naming.py -v`
Expected: all PASS.

- [ ] **Step 5: Route the two existing Python call sites through it**

In `src/x3d_cpp_gen/emit/defaults.py`, replace the `cpp_string_literal` function body (lines 14-30) with a thin re-export so existing callers (`generator.py`'s `# noqa: F401` re-export, any test importing `cpp_string_literal` directly) keep working unchanged:

```python
from x3d_cpp_gen.emit.naming import cpp_str as cpp_string_literal  # noqa: F401
```//
Delete the old `def cpp_string_literal(s: str) -> str:` body entirely (the 14-line hand-rolled loop) — this one import line replaces it.

In `src/x3d_cpp_gen/generator.py`, replace `_cpp_str` (lines 273-276):

```python
from x3d_cpp_gen.emit.naming import cpp_str as _cpp_str  # noqa: F401
```

Delete the old 4-line `def _cpp_str(s: str) -> str:` body. Every existing call site (`_cpp_str(token)` in `gen_enums_header`) keeps working unchanged since the name and single-argument signature are preserved. NOTE: `generator.py`'s old `_cpp_str` returned the FULLY QUOTED literal (`f'"{escaped}"'`, including the surrounding quotes), while `cpp_str`/`cpp_string_literal` return only the escaped BODY. Check every call site of `_cpp_str` in `generator.py` (there are two, both in `gen_enums_header` around lines 246 and 257) and wrap them in quotes explicitly since the import above changes the contract:

```python
            lines.append(f"    case {cpp}::{m.cpp_name}: return {_cpp_str_literal(token)};")
```
becomes (defining a tiny local wrapper right where `_cpp_str` used to be, instead of aliasing bare):
```python
def _cpp_str_literal(s: str) -> str:
    """A fully-quoted C++ string literal for ``s``."""
    return f'"{_cpp_str(s)}"'
```
and update both call sites in `gen_enums_header` (lines ~246 and ~257) from `_cpp_str(token)` to `_cpp_str_literal(token)`.

- [ ] **Step 6: Run the full pytest suite to catch every affected call site**

Run: `uv run pytest -v 2>&1 | tail -40`
Expected: all PASS. If `test_golden_tree.py` fails, it means Step 5's quote-wrapping fix wasn't applied to every call site — grep `generator.py` for remaining bare `_cpp_str(` calls and fix them.

- [ ] **Step 7: Register the Jinja filter and apply it to unescaped template interpolations**

In `src/x3d_cpp_gen/backends/cpp_header.py`, add to the `Environment` setup in `__init__` (near the existing `env.filters['pascal'] = pascal` line):

```python
        from x3d_cpp_gen.emit.naming import cpp_str
        env.filters['pascal'] = pascal
        env.filters['cpp_str'] = cpp_str
```

In `src/x3d_cpp_gen/templates/class_template.hpp.jinja`, apply `| cpp_str` to every previously-unescaped quoted interpolation:

- Line 62: `return "{{ container_field.type }}";` -> `return "{{ container_field.type | cpp_str }}";`
- Line 70: `return "{{ container_field.default }}";` -> `return "{{ container_field.default | cpp_str }}";`
- Line 81: `{% for enum in container_field.enumerations %}"{{ enum }}"{% if not loop.last %}, {% endif %}{% endfor %}` -> `{% for enum in container_field.enumerations %}"{{ enum | cpp_str }}"{% if not loop.last %}, {% endif %}{% endfor %}`
- Line 104: `return "{{ component.name }}";` -> `return "{{ component.name | cpp_str }}";`
- Line 156: `{% for t in field.acceptable_node_types %}"{{ t }}"{% if not loop.last %}, {% endif %}{% endfor %}` -> `{% for t in field.acceptable_node_types %}"{{ t | cpp_str }}"{% if not loop.last %}, {% endif %}{% endfor %}`

Leave `@details {{ field.description }}` (line 22 and the field-level one around line 123) UNCHANGED for now — `cpp_str` escapes STRING-LITERAL metacharacters, not Doxygen COMMENT metacharacters (`*/`), and applying it there would be a no-op for the actual risk (a description containing `*/` breaking a `/** ... */` block) while implying a fix that isn't one. That's a distinct, smaller follow-up (a comment-safe filter, e.g. replacing literal `*/` with `*\/`) — leave a one-line comment noting it's out of scope for this task rather than silently doing nothing:

Add directly above the class-level `@details` line (around line 21-22):
```jinja
{#- NOTE: class_description/specification_url/field descriptions are NOT run
    through cpp_str here -- that escapes STRING-LITERAL metacharacters, not
    Doxygen COMMENT metacharacters (a description containing "*/" would still
    break this block). Not addressed in this pass; needs a comment-safe
    filter, not cpp_str. -#}
```

- [ ] **Step 8: Regenerate and inspect the diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git diff --stat generated_cpp_bindings/
```
Expected: LIKELY zero diff (the current UOM 4.0 spec's `containerField.type`/`.default`, `component.name`, `acceptableNodeTypes` values are almost certainly all already escape-safe plain identifiers) — this task is defense-in-depth for future spec data, not a fix for a currently-visible bug. If there IS a diff, read it: it means some current value contains a character `cpp_str` now correctly escapes that was previously emitted raw (a genuine latent bug this task fixes) — inspect and confirm the new output is the CORRECT C++ before proceeding.

- [ ] **Step 9: Full pytest + build verification**

Run: `uv run pytest -v 2>&1 | tail -30`
Expected: all PASS.

If Step 8 showed a nonzero diff, additionally run:
```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"
```
Expected: 100% tests passed.

- [ ] **Step 10: Commit**

```bash
git add src/x3d_cpp_gen/emit/naming.py src/x3d_cpp_gen/emit/defaults.py src/x3d_cpp_gen/generator.py src/x3d_cpp_gen/backends/cpp_header.py src/x3d_cpp_gen/templates/class_template.hpp.jinja generated_cpp_bindings/ tests/test_naming.py
git commit -m "gen: unify C++ string-literal escaping into one shared formatter

Three inconsistent implementations existed: emit.defaults.cpp_string_literal
escaped backslash/quote/newline/tab/CR; generator._cpp_str escaped only
backslash/quote; several Jinja template interpolations (containerField.type/
.default, component.name, acceptableNodeTypes) were entirely unescaped,
safe today only because the UOM data happens to be tame -- the same 'works
for the data we've seen' posture that hid the matrix-brace bug. One shared
emit.naming.cpp_str now backs every quoted interpolation, registered as a
Jinja filter for template use."
```

---

### Task 6: Extract the duplicated MFEnum wire tokenizer

**Files:**
- Modify: `src/x3d_cpp_gen/emit/reflection.py` (emit a shared `parseEnumTokens` helper)
- Modify: `src/x3d_cpp_gen/templates/class_template.cpp.jinja:66-88` (call it instead of inlining)
- Test: new `tests/test_reflection.py`

**Interfaces:**
- Consumes: nothing new from other modules.
- Produces: `X3DReflection.hpp` gains a free function `std::vector<std::string> parseEnumTokens(const std::string& s)` — splits `s` on whitespace/comma exactly like the current per-node inline loop, returning the raw token substrings (callers still do their own `from_string` per token; this only extracts the TOKENIZING loop, not the enum-name resolution, since resolution is type-specific per enum class and can't be shared).

- [ ] **Step 1: Read the current reflection.py structure**

Run: `sed -n '1,50p' src/x3d_cpp_gen/emit/reflection.py` and read the rest of the file to find where free functions/helpers get appended to the generated header (look for where `isValueField`/similar helpers are emitted, around the lines Fable's review cited: 104-117), to match the existing emission style (a list of strings appended to `lines`, matching `gen_enums_header`'s pattern already seen in `generator.py`).

- [ ] **Step 2: Write the failing test**

Create `tests/test_reflection.py`:

```python
"""emit.reflection: the generated X3DReflection.hpp support header."""

from x3d_cpp_gen.emit.reflection import gen_reflection_header


def test_reflection_header_declares_parse_enum_tokens():
    header = gen_reflection_header()
    assert "parseEnumTokens" in header
    assert "std::vector<std::string> parseEnumTokens" in header


def test_reflection_header_still_declares_field_table_type():
    # Characterization check: this new helper must not replace/break the
    # existing FieldTable/FieldInfo declarations already in this header.
    header = gen_reflection_header()
    assert "FieldTable" in header
    assert "FieldInfo" in header
```

- [ ] **Step 3: Run test to verify it fails**

Run: `uv run pytest tests/test_reflection.py -v`
Expected: FAIL — `parseEnumTokens` not yet present in the generated header text.

- [ ] **Step 4: Implement — add `parseEnumTokens` emission**

In `src/x3d_cpp_gen/emit/reflection.py`, find the point in `gen_reflection_header()` where free-standing helper functions are appended to the `lines` list (near the `isValueField`/`X3DFieldType` helpers Fable's review located around lines 104-117) and add, in the same style (append to the same `lines` list, inside the `namespace x3d::core { ... }` block, before its closing brace):

```python
    lines.append("")
    lines.append("// Whitespace/comma-delimited token split for MFEnum wire values")
    lines.append("// (e.g. \"OPAQUE MASK\" or \"OPAQUE,MASK\"). Shared by every")
    lines.append("// generated node's MFEnum reflection set-thunk so the split logic")
    lines.append("// lives in exactly one place instead of once per generated .cpp.")
    lines.append("inline std::vector<std::string> parseEnumTokens(const std::string& s) {")
    lines.append("    std::vector<std::string> out;")
    lines.append("    std::size_t i = 0;")
    lines.append("    while (i < s.size()) {")
    lines.append("        while (i < s.size() && (s[i] == ' ' || s[i] == '\\t' ||")
    lines.append("               s[i] == '\\n' || s[i] == '\\r' || s[i] == ',')) ++i;")
    lines.append("        std::size_t j = i;")
    lines.append("        while (j < s.size() && s[j] != ' ' && s[j] != '\\t' &&")
    lines.append("               s[j] != '\\n' && s[j] != '\\r' && s[j] != ',') ++j;")
    lines.append("        if (j > i) out.push_back(s.substr(i, j - i));")
    lines.append("        i = j;")
    lines.append("    }")
    lines.append("    return out;")
    lines.append("}")
```

Confirm `#include <vector>` and `#include <string>` are already present near the top of the generated header (they should be, given `FieldTable`/`FieldInfo` already use both) — if not, add them to the header preamble in the same function.

- [ ] **Step 5: Run test to verify it passes**

Run: `uv run pytest tests/test_reflection.py -v`
Expected: both PASS.

- [ ] **Step 6: Replace the inline tokenizer in the .cpp template**

In `src/x3d_cpp_gen/templates/class_template.cpp.jinja`, replace lines 68-82 (the `{% if field.runtime_field_type == "MFEnum" %}` branch's tokenizing loop, from `{{ field.cpp_type }} vec;` through the closing of the `while (i < s.size())` loop, i.e. everything up to but NOT including the `dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::move(vec));` line) with:

```jinja
                {% if field.runtime_field_type == "MFEnum" %}
                {{ field.cpp_type }} vec;
                for (const auto& tok : parseEnumTokens(s)) {
                    {{ field.enum_cpp_name }} ev;
                    if (from_string(tok, ev)) vec.push_back(ev);
                }
                dynamic_cast<{{ class_name }}&>(n).{{ field.reader_setter_call }}(std::move(vec));
                {% else %}
```

(This keeps the surrounding `{% else %}...{% endif %}` for the SFEnum branch exactly as it was — only the MFEnum tokenizing loop's body changes, from the manual whitespace/comma-walking `while` loops to a `for` over `parseEnumTokens(s)`.)

- [ ] **Step 7: Regenerate and diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git diff --stat generated_cpp_bindings/
```
Expected: a diff touching every generated node `.cpp` file with an `MFEnum` field (grep the UOM/existing golden for `MFEnum` fields to know which ones — likely a small handful) plus `X3DReflection.hpp` gaining the new function. Read one sample diff (e.g. `git diff generated_cpp_bindings/x3d/nodes/<SomeNodeWithMFEnum>.cpp`) to confirm the new `for (const auto& tok : parseEnumTokens(s))` loop reads correctly.

- [ ] **Step 8: Full pytest + real build**

Run: `uv run pytest -v 2>&1 | tail -30`
Expected: all PASS.

```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"
```
Expected: 100% tests passed — this is the step that actually proves the new shared tokenizer produces IDENTICAL parse results to the old per-node inline loop (any existing runtime test that round-trips an MFEnum field's wire value will catch a regression here).

- [ ] **Step 9: Commit**

```bash
git add src/x3d_cpp_gen/emit/reflection.py src/x3d_cpp_gen/templates/class_template.cpp.jinja generated_cpp_bindings/ tests/test_reflection.py
git commit -m "gen: extract the MFEnum wire tokenizer into a shared reflection helper

~20 lines of whitespace/comma-parsing C++ were inlined into every generated
node's MFEnum reflection set-thunk, duplicated once per affected node with
no direct unit test asserting its parse behavior. Extracted to a single
parseEnumTokens() free function emitted once into X3DReflection.hpp; every
generated .cpp now calls it instead of reimplementing the split."
```

---

### Task 7: Delete confirmed-dead code

**Files:**
- Modify: `src/x3d_cpp_gen/model/types.py` (remove `TypeKind`, `TypeRegistry.cpp_type`, `_CPP_TYPE`)
- Modify: `src/x3d_cpp_gen/generator.py` (remove `annotate_default_exprs`)
- Delete: `src/x3d_cpp_gen/backends/base.py`
- Test: none new (this is pure deletion of already-unreferenced code; the existing suite is the safety net)

**Interfaces:** none — nothing else may reference any of these (verified below before deleting).

- [ ] **Step 1: Re-verify each item is genuinely unreferenced (do not trust the earlier grep from memory — rerun it against the current tree)**

Run each of these from the repo root and confirm the ONLY hits are the definition sites themselves:

```bash
grep -rn "TypeKind\|\.kind(" src/ tests/ --include="*.py" | grep -v "model/types.py:1[0-9]:\|model/types.py:2[0-9]:\|model/types.py:22[0-9]:\|model/types.py:23[0-9]:"
grep -rn "TypeRegistry.cpp_type\|_CPP_TYPE" src/ tests/ --include="*.py" | grep -v "model/types.py"
grep -rn "annotate_default_exprs" src/ tests/ --include="*.py"
grep -rln "from x3d_cpp_gen.backends.base\|backends\.base\b" src/ tests/ --include="*.py"
```

Expected: the first three commands show zero hits outside their own definitions (confirming `TypeKind`/`.kind()`, `TypeRegistry.cpp_type`/`_CPP_TYPE`, and `annotate_default_exprs` are each genuinely dead); the fourth shows zero hits at all (confirming nothing imports `backends.base`). If ANY of these greps surfaces an external reference, STOP — do not delete that item, it's not actually dead; investigate the reference first.

- [ ] **Step 2: Delete `TypeKind` and the `cpp_type`-related dead code from `model/types.py`**

Remove:
- The entire `class TypeKind(Enum): ...` block (lines 19-26).
- The `_CPP_TYPE = { ... }` dict (lines 86-130).
- The `cpp_type` classmethod on `TypeRegistry` (lines 211-213: `@classmethod\n    def cpp_type(cls, t: "X3DType") -> str:\n        return _CPP_TYPE[t]`).
- The `kind` classmethod on `TypeRegistry` (lines 220-233: `@classmethod\n    def kind(cls, t: "X3DType") -> TypeKind: ...`).

Leave everything else in the file untouched: `X3DType`, `_COMPONENTS`, `_NO_MOVE_OVERLOAD`, `TypeRegistry.resolve`/`resolve_or_raise`/`components`/`is_multi`/`needs_move_overload`/`runtime_tag`, and `resolve_x3d_type` are all genuinely load-bearing (confirmed via the greps used to ground this plan — `TypeRegistry.components`, `.is_multi`, `.needs_move_overload`, `.runtime_tag`, `.resolve`/`resolve_or_raise` are all called from `emit/descriptors.py` and `emit/defaults.py`).

Also update `resolve_or_raise`'s error message (currently references `_CPP_TYPE` at line 206): change
```python
                f"member and a _CPP_TYPE entry in model/types.py (and a "
```
to
```python
                f"member and a FIELD_TYPE_MAPPING entry in generator.py "
```
(since `_CPP_TYPE` no longer exists — the actual load-bearing C++ type mapping, per this same investigation, is `generator.py`'s `FIELD_TYPE_MAPPING`, so point the error message at the table that's actually authoritative instead of the one just deleted).

- [ ] **Step 3: Delete `annotate_default_exprs` from `generator.py`**

Remove the entire function (lines 330-337):
```python
def annotate_default_exprs(fields) -> None:
    """Compatibility shim: populate field.default_expr in place.

    Default-literal computation moved to emit.defaults; this remains so legacy
    callers that annotate parser dataclasses keep working.
    """
    for f in fields:
        f.default_expr = compute_default_expr(f)
```

- [ ] **Step 4: Delete `backends/base.py`**

```bash
rm src/x3d_cpp_gen/backends/base.py
```

- [ ] **Step 5: Run the full pytest suite**

Run: `uv run pytest -v 2>&1 | tail -40`
Expected: all PASS. If anything fails with an import error, one of Step 1's greps missed a reference — find it (likely an `__init__.py` re-export list somewhere) and either keep that specific item or update the reference; do not silently re-add the deleted code without understanding why it was referenced.

- [ ] **Step 6: Regenerate and confirm zero diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git status --short generated_cpp_bindings/
```
Expected: empty (none of this task's deletions touch anything on the actual code-generation path).

- [ ] **Step 7: Commit**

```bash
git add src/x3d_cpp_gen/model/types.py src/x3d_cpp_gen/generator.py
git rm src/x3d_cpp_gen/backends/base.py
git commit -m "gen: delete confirmed-dead code (TypeKind, cpp_type/_CPP_TYPE, annotate_default_exprs, Backend protocol)

Each verified independently unreferenced outside its own definition site
before deletion (grepped src/ and tests/ fresh, not from memory). TypeKind/
.kind() and TypeRegistry.cpp_type()/_CPP_TYPE were dead duplicates of the
actually-load-bearing generator.FIELD_TYPE_MAPPING; annotate_default_exprs
had zero callers anywhere; backends/base.py's Backend protocol was never
imported by anything, including its own intended consumer generator.py."
```

---

### Task 8: Fail closed on unsupported field types and enum boundedness

**Files:**
- Modify: `src/x3d_cpp_gen/parser.py:58-93` (`validate_field_type`, `_parse_fields`, `parse_x3d_model`)
- Modify: `src/x3d_cpp_gen/cli.py` (thread a new flag through, matching the existing `--no-test` sanctioned-skip pattern)
- Modify: `src/x3d_cpp_gen/model/enums.py:82-94` (`_is_bounded`)
- Test: `tests/test_parser.py`, `tests/test_cli_fail_closed.py`, `tests/test_enums.py`

**Interfaces:**
- Consumes: nothing new.
- Produces: `parse_x3d_model` now returns `(nodes: Dict[str, X3DNode], skipped: List[Tuple[str, str, str]])` — a list of `(node_name, field_name, raw_type)` for every field skipped due to an unsupported type. `cli.py`'s `main()` treats a nonempty `skipped` list as a hard failure UNLESS a new `--allow-unsupported-fields` flag is passed (default off), mirroring the existing `--no-test` "skip is legitimate only when explicit" pattern this repo already established (see `tests/test_cli_fail_closed.py`'s docstring). `_is_bounded` gets a narrower, less string-fragile match (see Step 6).

- [ ] **Step 1: Write the failing test for `_parse_fields` skip accumulation**

Add to `tests/test_parser.py` (check the existing file's XML-fixture-building style first and match it — the sketch below assumes `lxml.etree.fromstring` building a minimal `<AbstractNodeType>` element; adjust to match existing helpers in that file):

```python
def test_parse_fields_returns_skipped_unsupported_types(capsys):
    from lxml import etree
    from x3d_cpp_gen.parser import _parse_fields

    xml = """
    <AbstractNodeType name="TestNode">
      <field name="ok" type="SFBool" accessType="inputOutput"/>
      <field name="bad" type="SFTotallyMadeUpType" accessType="inputOutput"/>
    </AbstractNodeType>
    """
    node_element = etree.fromstring(xml)
    field_type_mapping = {"SFBool": "bool"}
    xs_types = {}

    fields, skipped = _parse_fields(node_element, field_type_mapping, xs_types, "TestNode")

    assert len(fields) == 1
    assert fields[0].name == "ok"
    assert skipped == [("TestNode", "bad", "SFTotallyMadeUpType")]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_parser.py -k skipped_unsupported -v`
Expected: FAIL — `_parse_fields` currently returns just `fields` (a list), not a `(fields, skipped)` tuple, so unpacking raises `ValueError: too many values to unpack` or similar.

- [ ] **Step 3: Implement — thread `skipped` through the parser**

In `src/x3d_cpp_gen/parser.py`, replace `_parse_fields` (lines 62-93):

```python
def _parse_fields(node_element, field_type_mapping: dict, xs_types: dict,
                  node_name: str):
    """Parse all <field> children of a node into X3DField objects.

    Fields whose type is unsupported are logged and skipped (never yielded as
    None) so that no None ever leaks into a node's field list. Returns
    ``(fields, skipped)`` where ``skipped`` is a list of
    ``(node_name, field_name, raw_type)`` for every skip, so the caller can
    decide whether an unsupported type is tolerable (see cli.py's
    --allow-unsupported-fields) instead of it silently shrinking the API.
    """
    parsed = []
    skipped = []
    for field in node_element.findall('.//field'):
        raw_type = field.get('type')
        if not validate_field_type(raw_type, field_type_mapping, xs_types):
            print(f"WARNING: skipping field '{field.get('name')}' on node "
                  f"'{node_name}': unsupported type '{raw_type}'")
            skipped.append((node_name, field.get('name'), raw_type))
            continue
        cpp_field_type = xs_types[raw_type][0] if raw_type in xs_types else raw_type
        raw_name = field.get('name', '')
        parsed.append(X3DField(
            name=sanitize_field_name(raw_name),
            x3d_name=raw_name,
            type=cpp_field_type,
            accessType=field.get('accessType', 'inputOutput'),
            default=field.get('default'),
            description=field.get('description', ''),
            min_inclusive=field.get('minInclusive'),
            max_inclusive=field.get('maxInclusive'),
            base_type=field.get('baseType'),
            acceptable_node_types=field.get('acceptableNodeTypes').split('|')
                if field.get('acceptableNodeTypes') else None,
            inherited_from=field.get('inheritedFrom'),
            simple_type=field.get('simpleType'),
        ))
    return parsed, skipped
```

Update `parse_node` (the function immediately below, which calls `_parse_fields` twice — once for mixins, once for the shared abstract/concrete path) to thread `skipped` through and merge it into its own return. Change its signature to also return `skipped`:

```python
def parse_node(node_element, field_type_mapping: dict, xs_types: dict,
               *, is_abstract: bool, is_mixin: bool = False):
    """Parse a single node element (mixin / abstract / concrete) into an
    (X3DNode, skipped) pair. See _parse_fields for the ``skipped`` shape.
    """
    node_name = node_element.get('name')
    iface = node_element.find('.//InterfaceDefinition')
    description = (iface.get('appinfo') or "") if iface is not None else ""
    specification_url = (iface.get('specificationUrl') or "") if iface is not None else ""

    component = _parse_component_info(node_element)

    if is_mixin:
        if iface is not None:
            fields, skipped = _parse_fields(node_element, field_type_mapping, xs_types, node_name)
        else:
            fields, skipped = [], []
        return X3DNode(
            name=node_name, fields=fields, base_type=None, is_abstract=is_abstract,
            class_description=description, specification_url=specification_url,
            component=component,
        ), skipped

    inheritance = node_element.find('.//Inheritance')
    base_type = inheritance.get('baseType') if inheritance is not None else None
    additional_base_types = [a.get('baseType')
                             for a in node_element.findall('.//AdditionalInheritance')
                             if a.get('baseType')]
    fields, skipped = _parse_fields(node_element, field_type_mapping, xs_types, node_name)
    return X3DNode(
        name=node_name, fields=fields, base_type=base_type,
        additional_base_types=additional_base_types, is_abstract=is_abstract,
        container_field=_parse_container_field(node_element),
        class_description=description, specification_url=specification_url,
        component=component,
    ), skipped
```

Update `parse_x3d_model` to collect and return the aggregate `skipped` list:

```python
def parse_x3d_model(uom_file: str, field_type_mapping: dict, xs_types: dict):
    all_skipped = []
    try:
        tree = etree.parse(uom_file)
    except (etree.ParseError, IOError) as e:
        print(f"Failed to parse XML file {uom_file}: {e}")
        return {}, all_skipped

    try:
        root = tree.getroot()
        nodes = {}
    except Exception as e:
        print(f"Failed to process XML tree: {e}")
        return {}, all_skipped

    for obj in root.findall('.//AbstractObjectTypes/AbstractObjectType'):
        node, skipped = parse_node(
            obj, field_type_mapping, xs_types, is_abstract=True, is_mixin=True)
        nodes[obj.get('name')] = node
        all_skipped.extend(skipped)

    for node_element in root.findall('.//AbstractNodeTypes/AbstractNodeType'):
        node, skipped = parse_node(
            node_element, field_type_mapping, xs_types, is_abstract=True)
        nodes[node_element.get('name')] = node
        all_skipped.extend(skipped)

    for node_element in root.findall('.//ConcreteNodes/ConcreteNode'):
        node, skipped = parse_node(
            node_element, field_type_mapping, xs_types, is_abstract=False)
        nodes[node_element.get('name')] = node
        all_skipped.extend(skipped)

    return nodes, all_skipped
```

- [ ] **Step 4: Run parser tests**

Run: `uv run pytest tests/test_parser.py -v`
Expected: all PASS, including the new test. Other existing tests in this file that call `parse_x3d_model`/`parse_node`/`_parse_fields` directly will need their call-site unpacking updated to match the new tuple returns — read the failures and fix each one (unpack `nodes, skipped = parse_x3d_model(...)` etc.) rather than reverting the signature change.

- [ ] **Step 5: Update `cli.py`'s call site + add the `--allow-unsupported-fields` flag**

Run `grep -n "parse_x3d_model\|add_argument" src/x3d_cpp_gen/cli.py` to find the exact current call site (around line 178, per earlier grep) and the `build_parser()` function's existing `argparse` flag definitions (to match its style, e.g. how `--no-test` is defined).

Update the call site from `nodes = parse_x3d_model(str(spec), FIELD_TYPE_MAPPING, XS_TYPES)` to:

```python
    nodes, skipped_fields = parse_x3d_model(str(spec), FIELD_TYPE_MAPPING, XS_TYPES)
    if skipped_fields and not args.allow_unsupported_fields:
        print(f"ERROR: {len(skipped_fields)} field(s) were skipped due to "
              f"unsupported types (this shrinks the generated API silently "
              f"unless you pass --allow-unsupported-fields):")
        for node_name, field_name, raw_type in skipped_fields:
            print(f"  {node_name}.{field_name}: type={raw_type!r}")
        return 1
```

(Place this right after the `parse_x3d_model` call, before whatever currently uses `nodes` next — read the surrounding ~20 lines of `main()` to confirm the exact insertion point and that `return 1` matches this function's existing early-return convention for other failure paths in the same function.)

Add the flag to `build_parser()`, next to the existing `--no-test` definition, matching its style:

```python
    parser.add_argument(
        "--allow-unsupported-fields", action="store_true",
        help="Do not fail when the UOM spec contains a field whose type "
             "this generator doesn't support (default: fail closed).",
    )
```

- [ ] **Step 6: Tighten `_is_bounded`'s prose match**

In `src/x3d_cpp_gen/model/enums.py`, replace `_is_bounded` (lines 82-94):

```python
def _is_bounded(appinfo: str) -> bool:
    """A SimpleType is a closed vocabulary iff its appinfo says so.

    Matches the UOM's actual declaration phrasing ("this list is bounded")
    rather than a bare substring check for "bounded" -- the UOM marks open
    vocabularies with the word "unbounded" (itself containing "bounded" as a
    substring), so a bare substring match already special-cased that one
    known collision; a tighter phrase match reduces the surface for a FUTURE
    unrelated appinfo phrase that happens to contain "bounded" (e.g. "value
    space is bounded above but the list itself is open") to be misread as a
    closed-vocabulary declaration.
    """
    text = (appinfo or "").lower()
    if "unbounded" in text:
        return False
    return "this list is bounded" in text or "list is bounded" in text
```

- [ ] **Step 7: Run the enums test and check for a real behavior change**

Run: `uv run pytest tests/test_enums.py -v`
Expected: check carefully — if any EXISTING test currently passes an appinfo string that says just "bounded" without the fuller phrase "list is bounded", this tightened match will now return `False` where it used to return `True`, and that test will fail. Read any such failure: it is either (a) a test fixture using an unrealistic/abbreviated appinfo string that should be updated to the real UOM phrasing, or (b) evidence the real UOM data itself uses a shorter phrase than assumed — in case (b), loosen the match to `"bounded" in text` again but keep this task's docstring investigation as a documented, deliberate decision rather than reverting silently. Do not proceed to Step 8 until you've determined which case applies by actually reading the real UOM XML's appinfo strings (`grep -o 'appinfo="[^"]*bounded[^"]*"' <path-to-uom-xml>`, find the UOM file path via `grep -rn "uom" mise.toml` or `cli.py`'s default spec path).

- [ ] **Step 8: Regenerate and confirm no unexpected skip/enum diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
```
Expected: exits 0 with no `ERROR: N field(s) were skipped` message (confirms the current UOM 4.0 spec has zero unsupported-type fields today — if this DOES print skips, that's a genuine pre-existing gap this task has now surfaced; do not add `--allow-unsupported-fields` to the regen command to silence it without first understanding what's being skipped and why).

```bash
uv run python scripts/gen_profile_tables.py
git status --short generated_cpp_bindings/
```
Expected: empty (Step 6's `_is_bounded` change should not alter which SimpleTypes are classified as bounded for the real 4.0 UOM, per Step 7's investigation).

- [ ] **Step 9: Full pytest suite**

Run: `uv run pytest -v 2>&1 | tail -40`
Expected: all PASS.

- [ ] **Step 10: Commit**

```bash
git add src/x3d_cpp_gen/parser.py src/x3d_cpp_gen/cli.py src/x3d_cpp_gen/model/enums.py tests/test_parser.py tests/test_cli_fail_closed.py tests/test_enums.py
git commit -m "gen: fail closed on unsupported field types; tighten enum-boundedness match

An unsupported field type was skip-with-print, silently shrinking the
generated API with generation staying green -- inconsistent with the
fail-closed philosophy this repo already established for the compile/test
path (tests/test_cli_fail_closed.py). parse_x3d_model now accumulates every
skip and cli.py fails closed unless --allow-unsupported-fields is passed,
mirroring the existing --no-test sanctioned-skip pattern. Also tightened
_is_bounded's prose match from a bare 'bounded' substring to the UOM's
actual declaration phrasing, reducing false-positive risk from an unrelated
future appinfo string."
```

---

### Task 9: Extract a shared chunk-and-brace helper (lowest priority — diff-review finding, not a fable finding)

**Files:**
- Modify: `src/x3d_cpp_gen/emit/defaults.py` (`_struct_literal`'s matrix-row loop, `_MF_STRUCT_ELEM`'s element-chunking loop in `default_expr_for`)
- Test: `tests/test_defaults.py`

**Interfaces:**
- Consumes: nothing new.
- Produces: `_chunk_braced(vals: List[str], size: int, *, pad_short: bool, floaty: bool = True) -> List[str]` — a private helper in `defaults.py` returning the list of `"{v0, v1, ...}"` brace-wrapped chunk strings, parameterized for the two truncation semantics the earlier verify pass identified as genuinely different (matrix rows zero-pad short input via the existing arity guard before chunking; MF-struct elements drop a ragged trailing remainder). Both `_struct_literal`'s row-building and `default_expr_for`'s `_MF_STRUCT_ELEM` element-building call it instead of independently reimplementing the slice-and-brace loop.

NOTE: per the earlier verify pass, this is a genuinely marginal win (the two call sites differ enough in surrounding shape — one further wraps in `struct + "{{" + ... + "}}"`, the other prefixes each chunk with the element struct name and joins flat into a `std::vector<Struct>{...}`) that a shared helper only extracts the innermost "slice + join + brace" step, not the whole expression-building logic. Do this task LAST, and if it turns out to make either call site harder to read than the status quo, it is fine to stop and leave a comment explaining why extraction wasn't worth it instead of forcing it.

- [ ] **Step 1: Write the failing test**

Add to `tests/test_defaults.py`:

```python
def test_chunk_braced_groups_without_padding():
    from x3d_cpp_gen.emit.defaults import _chunk_braced
    assert _chunk_braced(["1", "2", "3", "4"], 2, pad_short=False) == ["{1, 2}", "{3, 4}"]


def test_chunk_braced_drops_ragged_remainder_when_not_padding():
    from x3d_cpp_gen.emit.defaults import _chunk_braced
    assert _chunk_braced(["1", "2", "3"], 2, pad_short=False) == ["{1, 2}"]


def test_chunk_braced_zero_pads_short_final_group_when_padding():
    from x3d_cpp_gen.emit.defaults import _chunk_braced
    # 5 values, chunk size 4 -> one full chunk, one short chunk padded to 4.
    result = _chunk_braced(["1", "2", "3", "4", "5"], 4, pad_short=True, floaty=True)
    assert result == ["{1, 2, 3, 4}", "{5, 0.0f, 0.0f, 0.0f}"]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/test_defaults.py -k chunk_braced -v`
Expected: FAIL — `ImportError: cannot import name '_chunk_braced'`.

- [ ] **Step 3: Implement `_chunk_braced`**

Add to `src/x3d_cpp_gen/emit/defaults.py`, above `_struct_literal`:

```python
def _chunk_braced(vals: List[str], size: int, *, pad_short: bool,
                  floaty: bool = True) -> List[str]:
    """Slice ``vals`` into ``size``-sized groups, brace-wrapping each.

    ``pad_short=True`` zero-pads a short final group to exactly ``size``
    (matrix rows: the caller has already zero-padded the WHOLE list to a
    multiple of ``size`` via the arity guard, so this only matters if a
    caller passes an un-padded list directly). ``pad_short=False`` drops a
    ragged trailing remainder instead (MF-struct elements: a multi-element
    default with a trailing partial element is intentionally truncated, not
    padded with synthetic zeros the spec never declared).
    """
    zero = "0.0f" if floaty else "0.0"
    if pad_short:
        vals = list(vals)
        while len(vals) % size != 0:
            vals.append(zero)
        stop = len(vals)
    else:
        stop = len(vals) - len(vals) % size
    return [
        "{" + ", ".join(vals[i:i + size]) + "}"
        for i in range(0, stop, size)
    ]
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/test_defaults.py -k chunk_braced -v`
Expected: all PASS.

- [ ] **Step 5: Route `_struct_literal`'s matrix-row loop through it**

Replace the `if row_size:` branch inside `_struct_literal`:

```python
    if row_size:
        rows = _chunk_braced(vals, row_size, pad_short=False, floaty=floaty)
        return struct + "{{" + ", ".join(rows) + "}}"
```

(`pad_short=False` here because `_struct_literal` already zero-padded the FULL `vals` list to exactly `count` elements a few lines above, via the existing `while len(vals) < count: vals.append(zero)` loop — by the time `_chunk_braced` runs, `len(vals) == count` and `count % row_size == 0` always holds for the square-matrix entries this function handles, so there's never a ragged remainder to pad OR drop; `pad_short` is moot at this call site and `False` is simplest.)

- [ ] **Step 6: Route `_MF_STRUCT_ELEM`'s element-chunking loop through it**

In `default_expr_for`, replace the `elems = [...]` list comprehension (inside the `if x3d_type in _MF_STRUCT_ELEM:` branch):

```python
    if x3d_type in _MF_STRUCT_ELEM:
        struct, count, floaty = _MF_STRUCT_ELEM[x3d_type]
        vals = _scalar_list(d)
        if len(vals) < count:
            return f"std::vector<{struct}>{{}}"
        # Chunk the flat scalar list into element-sized groups so a MULTI-element
        # default (e.g. Extrusion.crossSection = the 5-point square, spine = the
        # 2-point segment) emits every element, not just the first. Trailing
        # scalars that do not fill a whole element are dropped.
        chunks = _chunk_braced(vals, count, pad_short=False, floaty=floaty)
        elems = [struct + chunk for chunk in chunks]
        return f"std::vector<{struct}>{{" + ", ".join(elems) + "}"
```

Note: this call site previously discarded `_floaty` (named with a leading underscore because it was unused: `struct, count, _floaty = _MF_STRUCT_ELEM[x3d_type]`) — now it's actually needed by `_chunk_braced`'s zero-pad fallback path (even though `pad_short=False` here means the pad path never triggers, `_chunk_braced` still requires the parameter), so rename it from `_floaty` to `floaty` in the unpacking.

- [ ] **Step 7: Run the full defaults + default_expr test files**

Run: `uv run pytest tests/test_defaults.py tests/test_default_expr.py -v`
Expected: all PASS.

- [ ] **Step 8: Regenerate and confirm zero diff**

Run:
```bash
uv run x3d-cpp-gen --out generated_cpp_bindings
uv run python scripts/gen_profile_tables.py
git status --short generated_cpp_bindings/
```
Expected: empty (pure refactor, same output for both call sites).

- [ ] **Step 9: Full pytest + commit**

Run: `uv run pytest -v 2>&1 | tail -30`
Expected: all PASS.

```bash
git add src/x3d_cpp_gen/emit/defaults.py tests/test_defaults.py
git commit -m "gen: extract a shared chunk-and-brace helper for matrix rows and MF-struct elements

_struct_literal's matrix row-chunking and default_expr_for's _MF_STRUCT_ELEM
element-chunking independently reimplemented 'slice flat list into
fixed-size groups, brace each, join' with different truncation semantics
(zero-pad vs. drop-ragged). Extracted the shared slice-and-brace step into
_chunk_braced(pad_short=...), parameterized for both semantics, so a future
fix to one no longer risks silently drifting from the other."
```

---

## Execution Order

Tasks 1-8 are independent of each other (each touches a disjoint slice of the generator) except: Task 9 touches the same function (`_struct_literal`) Task 1 modifies, so **Task 9 must run after Task 1** (rebase/merge Task 1 first, or do them in the same worktree sequentially). All other tasks can be done in parallel across separate worktrees/branches/PRs if desired — this matches the "one PR per independent group" pattern already used for the P1 punch-list sweep.

Suggested order (fable-priority as requested, with Task 9 slotted right after Task 1 as a natural follow-on): **1 → 9 → 2 → 3 → 4 → 5 → 6 → 7 → 8**.

## Explicitly Deferred (not in this sweep)

- **Full consolidation of per-type fact tables into `model/types.py`'s `TypeRegistry`** as the single source of truth for `generator.py`'s `FIELD_TYPE_MAPPING`/struct bodies and `test_template.cpp.jinja`'s `defaultsEqual` overloads (fable finding #1's full scope). Task 1 closes the specific instance that caused a real bug (`_STRUCT_ARITY` row_size) plus a coverage safety net, but the broader migration — re-deriving ~40 `FIELD_TYPE_MAPPING` entries and 14 hand-written test-template overloads from one canonical table while proving byte-identical golden output across all 685 generated files — is large enough to deserve its own plan, not a task inside this sweep.

---

## Self-Review

**Spec coverage:** all 8 fable findings have a task (1↔#1-partial, 2↔#2, 3↔#3, 4↔#4, 5↔#5, 6↔#6, 7↔#7, 8↔#8); both diff-review findings D1/D2 are folded into Task 1, D3 is Task 9. The one item NOT a task is fable's full TypeRegistry-consolidation vision — explicitly called out above as deferred, not silently dropped.

**Placeholder scan:** every step shows complete code (no "add appropriate error handling", no "similar to Task N" without the actual code repeated). Task 8's Step 5 asks the implementer to `grep` for the exact current call site/insertion point rather than hand-typing a fabricated line number for `cli.py` (which was not fully read during planning) — this is a deliberate "verify against the real file before editing" instruction, not a placeholder, and is consistent with this project's own stated review discipline.

**Type consistency:** `has_range_diagnostics` (Task 2) is introduced once and used identically in both templates. `build_reflection_descriptors`'s keyword-only `own_field_names`/`ancestors` signature (Task 4) matches between its definition and both call sites (the test file and `cpp_header.py`). `cpp_str` (Task 5) is defined once in `emit/naming.py` and imported by name everywhere else — no renaming drift. `_chunk_braced` (Task 9) is defined once and both call sites pass `pad_short` explicitly (no relied-upon default that could silently diverge).
