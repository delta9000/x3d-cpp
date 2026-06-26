# initializeOnly Write-Path Fix — Design (2026-06-12)

**Status:** approved design, ready for implementation planning.
**Why now:** surfaced mid-PROTO-expansion (M1 closeout). PROTO value-forwarding
writes interface values into body fields through the reflection `set` thunk, which
is null for `initializeOnly` fields — so the most common parametric-PROTO pattern
(interface field → `Box.size` / `Sphere.radius` / geometry index) could not work.
Investigation showed this is a **pre-existing correctness bug** independent of
PROTO: documents cannot set `initializeOnly` fields at all. This fix is sequenced
**before** resuming PROTO Tasks 5–13.

---

## 1. Problem

The generator gates every setter on a single predicate
(`src/x3d_cpp_gen/emit/descriptors.py:198`):

```python
def is_settable(self) -> bool:
    """Has a setter: inputOutput only."""
    return self.access_type == "inputOutput"
```

So the **415** `initializeOnly` field entries across the generated bindings get
**no setter** — neither a typed `set<Name>()` nor a reflection `set` thunk
(`FieldInfo::set` is `nullptr` for them). The hand-written readers consume field
values through reflection and explicitly skip non-writable fields
(`runtime/codecs/XmlReader.hpp:198`: `if (!f.isWritable()) return; //
initializeOnly: skip`). Net effect: every `initializeOnly` value in every parsed
document is silently dropped.

**Empirical confirmation** (current `HEAD`):
- `<Sphere radius='9.5'/>` → parsed `radius` reads back as `1` (the default).
- `<Box size='3 4 5'/>` → parsed `size` reads back as `2 2 2`.
- `IndexedFaceSet.coordIndex` (`MFInt32`, `initializeOnly`) cannot be loaded, so
  indexed-geometry topology is lost.

This contradicts X3D semantics. `initializeOnly` fields **are** author-settable at
initialization (parse) time; they are merely excluded from the runtime event API
(not ROUTE targets, not exposed for runtime mutation via the SAI). The binding
conflated "not event-writable" with "not writable at all."

## 2. Goal

Make `initializeOnly` fields writable at the **data/initialization layer** (so the
readers and PROTO expansion populate them) **without** adding them to the public
runtime/validated setter API. The user-facing typed `set<Name>()` stays
`inputOutput`-only — a public `setSize()` on an `initializeOnly` field would
misrepresent the field as runtime-mutable.

**Non-goals:** changing event/ROUTE semantics; adding public typed setters for
`initializeOnly`; touching `inputOnly`/`outputOnly` handling.

## 3. Design

Emit a data-layer `set<Name>Unchecked()` for `initializeOnly` fields (constrained
or not) and wire the reflection `set` thunk to it. This reuses the existing
lenient-read convention: `set<Name>Unchecked()` is already "the raw write path the
reflection/reader uses, distinct from the validated typed `set<Name>()`" (today it
exists for *constrained* `inputOutput` fields). We extend it to `initializeOnly`.

Resulting invariant — **a public `set<Name>()` exists iff the field is
runtime/event-settable**:

| accessType        | public `setX()` | `setXUnchecked()`     | reflection `set` thunk | routable event sink |
|-------------------|:---------------:|:---------------------:|:----------------------:|:-------------------:|
| inputOutput       | ✅              | if constrained        | ✅                     | ✅                  |
| **initializeOnly**| ❌              | **✅ (new, always)**  | **✅ (new)**           | ❌ (access-gated)   |
| inputOnly         | ❌              | ❌                    | handler (unchanged)    | ✅ (handler)        |
| outputOnly        | ❌              | ❌                    | emit (unchanged)       | n/a (source)        |

### 3.1 Generator — `src/x3d_cpp_gen/emit/descriptors.py`

- Add a predicate for "writable at the data layer":
  `has_data_setter = (access_type in {"inputOutput", "initializeOnly"})`
  (equivalently `is_settable or access_type == "initializeOnly"`). Used to decide
  whether to emit the reflection `set` thunk.
- `reader_setter_call` (the method the reflection write routes to) currently picks
  `set<Name>Unchecked` when `has_constraints`, else `set<Name>`. Extend so an
  `initializeOnly` field **always** routes to `set<Name>Unchecked` (it has no
  public `set<Name>`).
- Keep `is_settable` (which gates the public typed `set<Name>()`) as
  `inputOutput`-only — unchanged.
- The unchecked method must be emitted for `initializeOnly` fields. Today it is
  emitted only for constrained `is_settable` fields; the condition becomes:
  emit `set<Name>Unchecked` when `(is_settable and has_constraints)` **or**
  `access_type == "initializeOnly"`.

### 3.2 Template — `src/x3d_cpp_gen/templates/class_template.hpp.jinja`

- **Public `set<Name>()` block** (currently `{% if field.is_settable %}`, ~line
  186): unchanged — still `inputOutput`-only.
- **`set<Name>Unchecked()` block** (currently `{% if field.is_settable and
  field.has_constraints %}`, ~line 394): widen to also emit for `initializeOnly`
  fields. For an unconstrained `initializeOnly` field the body is a raw member
  assignment (no validation — there is nothing to check).
- **Reflection `set` thunk** (currently `{% if field.is_settable %}`, ~lines 266 /
  305): change the guard to `has_data_setter` and route the call through
  `reader_setter_call` (which now resolves `initializeOnly` → `Unchecked`).

### 3.3 Readers / runtime — no logic change

- `XmlReader::applyAttribute` and `attachChild` gate on `f.isWritable()`
  (`bool(set)`). Once the `set` thunk is non-null for `initializeOnly`, these paths
  populate the field with no code change. **Bonus:** `initializeOnly` SFNode/MFNode
  fields now accept children too.
- The stale comment `// initializeOnly: skip` (`XmlReader.hpp:199`) is updated to
  reflect that only `outputOnly` (and `inputOnly`) remain non-writable. Mirror in
  any sibling reader with the same comment.
- The ClassicVRML/VRML readers route field writes through the same reflection
  `set`; they gain `initializeOnly` population automatically. Confirm no reader has
  an independent access-type guard that excludes `initializeOnly`.

### 3.4 Event bridge — no change (with a guard test)

`X3DSceneBridge`'s `isRoutableSink` accepts only `InputOnly`/`InputOutput`, so an
`initializeOnly` field — now writable — still cannot be a ROUTE target. A test
asserts a ROUTE to an `initializeOnly` field is rejected, locking this in.

## 4. Golden impact

Regenerating emits a new `set<Name>Unchecked` method and a non-null reflection
`set` thunk for 415 fields, so **every node header changes** and the golden sha256
changes. This is expected and approved. The plan regenerates, updates the golden
hash in the drift gate (`scripts/check_golden.sh` / `tests/test_golden_tree.py`),
and re-verifies.

## 5. Testing (TDD)

**pytest (generator/emit):**
- A generated `initializeOnly` field (e.g. `Sphere.radius`) has a
  `setRadiusUnchecked` method and a non-null reflection `set` thunk, and **no**
  public `setRadius`.
- An `inputOutput` field is unchanged (public `setX` present).

**C++ (runtime, run not just compiled):**
- `<Sphere radius='9.5'/>` → `getRadius() == 9.5`.
- `<Box size='3 4 5'/>` → `getSize() == {3,4,5}`.
- An `IndexedFaceSet` with a non-empty `coordIndex` loads the indices.
- An `initializeOnly` value survives an XML round-trip (read → write → read).
- A ROUTE whose sink is an `initializeOnly` field is rejected by the bridge.

**Gates:** regenerate, update golden hash, full `pytest` + `ctest` green.

## 6. Definition of done

- Generator emits `set<Name>Unchecked` + reflection `set` thunk for all
  `initializeOnly` fields; no public typed `set<Name>()` added for them.
- Readers populate `initializeOnly` scalar **and** node fields; round-trip
  preserves them.
- `initializeOnly` fields remain non-routable sinks (test-locked).
- Golden regenerated, hash gate updated, `pytest` + `ctest` green.
- PROTO Task 4's test can revert to the `Box.size` (`initializeOnly`) case and pass
  — verified when PROTO resumes.

## 7. Notes / risks

- **Scope of the golden churn:** the diff is large (415 fields × method + thunk)
  but mechanical and uniform; review focuses on a few representative nodes
  (`Box`, `Sphere`, `IndexedFaceSet`) plus the template diff.
- **Naming:** `set<Name>Unchecked` on an *unconstrained* field reads slightly oddly
  (nothing to "uncheck"), but reusing the established name keeps one convention for
  "data-layer raw write" rather than introducing a parallel `initX()` spelling. The
  invariant "public `setX` ⟺ runtime-settable" is the payoff.
- **Other write sites:** verify no codec/test relies on `initializeOnly` fields
  being *absent* from the writable set (e.g. a writer that double-emits, or a test
  asserting `isWritable()` is false for `initializeOnly`). Update such assertions to
  match the corrected contract.
