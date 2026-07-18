# Experimental SAI Context Apertures Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add explicit, snapshot-coherent EXPORT/IMPORT apertures to
`x3d::sai::experimental` without granting imported nodes local ownership or
cross-context mutation authority.

**Architecture:** Ordered export and import bindings live in immutable
`scene_state` and publish through `scene_edit`. Imports retain source control,
generation, export name, and resolved node identity; parent snapshots capture
immutable source states and expose distinct `imported_node` and
`imported_field<T>` inspection handles. Commit revalidates every staged source
binding before publishing.

**Tech Stack:** C++20, standard library only, doctest, CMake/CTest, YAML
conformance registers.

---

### Task 1: Export bindings and local isolation

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

**Step 1: Write the failing tests**

Add stories that create two contexts with colliding local names, prove lookup
is local, export one local node, inspect the ordered export binding, and remove
it using that enumerated binding.

**Step 2: Run the focused test to verify RED**

Run:

```sh
cmake --build build-sai --target x3d_sai_experimental_tests
```

Expected: compilation fails because `export_binding`, `scene_edit::export_node`,
`scene_edit::remove_export`, and `scene_snapshot::exports` do not exist.

**Step 3: Implement the minimal export model**

Add an owning public `export_binding { std::string name; node_id node; }`.
Store an ordered vector in `scene_state`. Stage export addition/removal in
`scene_edit`, validate local node authority and unique export names, append
ordered `change_kind::export_added`/`export_removed` records, and expose the
immutable range plus named resolution on `scene_snapshot`.

**Step 4: Run focused tests until GREEN**

Run the executable and confirm all existing tests remain green.

**Step 5: Commit**

```sh
git add experimental/sai
git commit -m "feat(sai): add ordered context exports"
```

### Task 2: Explicit imports and unified local alias collisions

**Files:** Same as Task 1.

**Step 1: Write the failing tests**

Add stories importing an exported node from another context owned by the same
browser, rejecting a cross-browser source, rejecting duplicate import aliases,
and rejecting collisions between DEF-style names and imports in both authoring
orders.

**Step 2: Verify RED**

Expected: missing `import_binding`, `imported_node`, `scene_edit::import_node`,
and imported lookup APIs.

**Step 3: Implement minimal authority-bearing imports**

Add public owning inspection data:

```cpp
struct semantic_node_id {
  generation_id generation;
  node_id local;
};

struct import_binding {
  std::string local_name;
  generation_id source_generation;
  std::string exported_name;
  semantic_node_id target;
};
```

Keep the source control pointer private in the state record. Add a distinct
`imported_node` containing importer control/generation, source
control/generation, node ID, and local alias. Do not add conversion to `node`.
Validate same-browser ownership, active source, source export existence, and a
single local collision domain spanning `names` and `imports`.

**Step 4: Run focused tests until GREEN**

Confirm failed imports poison the edit and publish neither aliases nor change
notifications.

**Step 5: Commit**

```sh
git add experimental/sai
git commit -m "feat(sai): add explicit imported node authority"
```

### Task 3: Source revalidation and reversible import authoring

**Files:** Same as Task 1.

**Step 1: Write the failing tests**

Stage a parent import, then remove or remap the source export before committing
the parent edit. Assert `stale_revision` or a dedicated stale-aperture error,
unchanged parent revision, no import, and no callback. Add ordered enumeration
and removal-token stories for successful imports.

**Step 2: Verify RED**

Expected: the parent import currently publishes a stale resolved target or the
removal API is absent.

**Step 3: Implement commit-time aperture validation**

Record each staged import's source state pointer and resolved endpoint. Before
parent publication, lock source contexts in stable generation order without
holding multiple arbitrary locks during callbacks. Confirm active generation,
source revision, export name, and resolved node ID still match. Add
`scene_edit::remove_import(const import_binding&)` and ordered change records.

**Step 4: Run focused tests until GREEN**

Exercise both source-first and parent-first edit sequences and verify failure
is non-mutation.

**Step 5: Commit**

```sh
git add experimental/sai
git commit -m "feat(sai): revalidate aperture publication"
```

### Task 4: Snapshot-coherent imported inspection

**Files:** Same as Task 1.

**Step 1: Write the failing tests**

Resolve an imported node, describe its type and ordered fields, obtain dynamic
and typed imported fields, and read equal values. Retain the parent snapshot,
mutate the source field, and prove the old parent snapshot reads the old value
while a new parent snapshot reads the new source revision.

**Step 2: Verify RED**

Expected: imported descriptor/field APIs are absent.

**Step 3: Add captured source views**

After copying the parent state under its lock, capture each referenced source
state independently. Store those shared immutable states in `scene_snapshot`.
Add distinct `dynamic_imported_field` and `imported_field<T>` checked views,
plus overloads of `describe`, `field`, and `read`. Validate importer context,
source generation, target existence, descriptor kind, and captured revision.

**Step 4: Run focused tests until GREEN**

Confirm no imported read acquires a mutable source reference or changes either
revision.

**Step 5: Commit**

```sh
git add experimental/sai
git commit -m "feat(sai): inspect imported nodes through snapshots"
```

### Task 5: Lifecycle and mutation firewall

**Files:** Same as Task 1.

**Step 1: Write the failing tests**

Replace the browser world and prove old imported handles fail safely. Add
compile-time and runtime stories showing imported nodes/fields cannot be
passed to containment, retained-field setters, or event sends. Confirm source
mutation creates no parent callback without a parent commit.

**Step 2: Verify RED**

Expected: missing stale imported-handle checks; any accidental mutation
overload is exposed by the compile contract.

**Step 3: Implement only required lifecycle checks**

Use existing generation invalidation and structured errors for all imported
inspection. Keep imported handle types absent from authoring/event overloads;
do not introduce a generic conversion escape hatch.

**Step 4: Run focused tests until GREEN**

Run the normal, Release/Werror, and ASan/UBSan focused suites.

**Step 5: Commit**

```sh
git add experimental/sai
git commit -m "test(sai): enforce imported authority firewall"
```

### Task 6: Evidence, composed example, and full verification

**Files:**
- Modify: `experimental/sai/examples/author_inspect.cpp`
- Modify: `experimental/sai/README.md`
- Modify: `CMakeLists.txt`
- Modify: `docs/conformance/sai-invariants.yaml`
- Modify: `docs/conformance/sai-services.yaml`
- Regenerate: `docs/conformance/SAI-BASELINE.md`

**Step 1: Extend the composed example**

Create source and parent contexts, export a source Transform, import it into the
parent, and inspect its typed value through an immutable parent snapshot.

**Step 2: Register stable evidence**

Add CTests for local isolation, aperture identity, collision atomicity, source
revalidation, snapshot coherence, and mutation rejection. Map only demonstrated
partial services and close `INV-ID-3`/`INV-OWN-2` planned falsification markers
only where the tests actually exercise their statements.

**Step 3: Document limits**

State explicitly that cross-context writes, ROUTEs, event cascades, Inline
loading, PROTO/IS, and global atomic snapshots remain unimplemented.

**Step 4: Run complete verification**

Run clang-format, Release/Werror, ASan/UBSan, focused CTests, SAI conformance
generation/gates, full pytest, the behavior suite, and `git diff --check`.

**Step 5: Commit**

```sh
git add CMakeLists.txt docs experimental
git commit -m "docs(sai): map explicit context aperture evidence"
```
