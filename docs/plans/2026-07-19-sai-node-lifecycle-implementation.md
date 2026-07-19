# Experimental SAI Node Lifecycle Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add explicit, referentially safe node removal and local `noexcept` wrapper disposal without weakening snapshot, event, or cross-context semantics.

**Architecture:** Node removal mutates the staged immutable-scene successor and records removed IDs for commit-time revalidation. Local graph references are checked against the staged state; cross-context imports are represented by a reverse aperture ledger protected by the existing context locks. Wrapper disposal only clears one value object's weak authority and never touches semantic state.

**Tech Stack:** C++20, `tl::expected`, immutable `scene_state` revisions, CMake/CTest, doctest, ASan+UBSan.

---

### Task 1: Unreferenced removal and historical snapshots

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/types.hpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing lifecycle story**

Add a doctest that creates two unreferenced nodes, captures a snapshot and the
first node's typed field, removes the first node, commits, and checks:

```cpp
auto removed = edit.remove_node(first);
REQUIRE(removed);
auto committed = edit.commit();
REQUIRE(committed);
CHECK(committed->changes.back().kind == sai::change_kind::node_removed);
CHECK(old_snapshot.read(old_field));
CHECK_FALSE(context.snapshot().lookup(first.id()));
CHECK(context.snapshot().lookup(second.id()));
CHECK(second.id().value > first.id().value);
```

Register it as `sai_life_node_removal`.

**Step 2: Run the test and verify it is red**

Run:

```bash
cmake --build build-sai-ci -j2 --target x3d_sai_experimental_tests
ctest --test-dir build-sai-ci -R '^sai_life_node_removal$' --output-on-failure
```

Expected: compile failure because `scene_edit::remove_node` and
`change_kind::node_removed` do not exist.

**Step 3: Add the smallest removal operation**

Add `error_code::node_in_use`, `change_kind::node_removed`, and:

```cpp
result<void> scene_edit::remove_node(const node &target);
```

Validate context, generation, and staged existence. Erase the node record,
remove it from `created_nodes`, add it to a new `removed_nodes` set, mark the
edit changed, and append an ordered `node_removed` change carrying the node ID.
Never decrement or reuse `next_node_id`.

**Step 4: Run focused and neighboring lifecycle tests**

Run:

```bash
cmake --build build-sai-ci -j2 --target x3d_sai_experimental_tests
ctest --test-dir build-sai-ci -R '^sai_life_|^sai_edit_' --output-on-failure
```

Expected: PASS.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/include/x3d/sai/experimental/types.hpp \
  experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/src/kernel.cpp experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): remove unreferenced nodes"
```

### Task 2: Staged referential-integrity validation

**Files:**
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write table-driven failing tests**

Build independent fixtures proving `remove_node()` returns `node_in_use` for:

- a root;
- both shared occurrences of one node;
- SFNode and MFNode references, including non-containment node-valued fields;
- a name;
- an export;
- either endpoint of a route.

For every rejection, check `commit()` returns the same poison and a fresh
snapshot has the original revision and node. Register stable tests
`sai_life_remove_references` and `sai_life_remove_nonmutation`.

**Step 2: Verify the focused tests fail**

Run:

```bash
cmake --build build-sai-ci -j2 --target x3d_sai_experimental_tests
ctest --test-dir build-sai-ci -R '^sai_life_remove_' --output-on-failure
```

Expected: removal incorrectly succeeds or commit reports dangling containment.

**Step 3: Add one staged-state reference inspector**

Implement a private helper that scans, in deterministic order:

1. roots;
2. every node record and every descriptor of kind SFNode/MFNode;
3. names;
4. exports;
5. routes.

Return a structured `node_in_use` error with the target node and blocking field
or binding name. Call it before erasing a node. Do not inspect C++ handles,
snapshot ownership, or reachability alone.

**Step 4: Prove explicit atomic detachment**

Add a story that removes/replaces all references first, then calls
`remove_node()` in the same edit and verifies one atomic revision with ordered
reference-removal changes followed by `node_removed`.

Run:

```bash
ctest --test-dir build-sai-ci -R '^sai_life_remove_|^sai_edit_atomic' --output-on-failure
```

Expected: PASS.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): enforce node reference integrity"
```

### Task 3: Local wrapper disposal

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write compile-time and behavioral failures**

Add assertions and a story equivalent to:

```cpp
static_assert(noexcept(std::declval<sai::node &>().dispose()));
static_assert(noexcept(std::declval<sai::typed_node<sai::bindings::Transform> &>()
                           .dispose()));
auto copy = original;
const auto id = original.id();
original.dispose();
original.dispose();
CHECK(original.disposed());
CHECK(original.id() == id);
CHECK_FALSE(copy.disposed());
CHECK(snapshot.describe(copy));
CHECK_FALSE(snapshot.describe(original));
```

Attach an observer and prove disposal produces no callback, revision, or change.
Register `sai_life_wrapper_disposal`.

**Step 2: Verify red compilation**

Run the registered test target. Expected: `dispose()` and `disposed()` missing.

**Step 3: Implement value-local invalidation**

Add a per-wrapper boolean and:

```cpp
bool node::disposed() const noexcept;
void node::dispose() noexcept;
```

`dispose()` resets only that wrapper's weak context and marks it disposed while
preserving ID and generation. Add identical forwarding methods to
`typed_node<Tag>`. Default copying deliberately copies current value state; a
copy made before disposal remains usable.

**Step 4: Run disposal, ownership, and stale-handle tests**

Run:

```bash
ctest --test-dir build-sai-ci \
  -R '^sai_life_wrapper_disposal$|^sai_own_|^sai_life_stale_handle$' \
  --output-on-failure
```

Expected: PASS.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): make wrapper disposal local"
```

### Task 4: Cross-context aperture integrity

**Files:**
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing cross-context story**

Create a source scene, export a node, import it into another scene, then remove
the source export and attempt to remove the source node. Require `node_in_use`
while the committed importer retains the aperture. Remove the import in the
importer, then remove the export and node successfully. Also prove a failed or
abandoned importer edit creates no reverse dependency. Register
`sai_life_import_reference`.

**Step 2: Verify the test fails**

Expected: source removal succeeds because the source currently has no reverse
knowledge of committed imports.

**Step 3: Add a lock-protected reverse aperture ledger**

Add an internal record to `context_control` containing importer weak identity,
importer generation, local import name, and target node. At importer commit:

- lock the union of base and staged source contexts plus the importer using the
  existing deterministic lock order;
- validate staged apertures;
- remove that importer generation's old reverse records from base sources;
- add records for staged imports only after validation succeeds;
- publish importer state and ledger changes within the same locked boundary.

At source commit, revalidate every `removed_nodes` entry against live reverse
records while holding the source lock. Expired or inactive importer records may
be pruned; a live committed record returns `node_in_use`. This commit-time check
closes the race between staging removal and a concurrent importer commit.

**Step 4: Run import, concurrency-boundary, and removal tests**

Run:

```bash
ctest --test-dir build-sai-ci \
  -R '^sai_life_import_reference$|^sai_context_import_|^sai_conc_boundary$|^sai_life_remove_' \
  --output-on-failure
```

Expected: PASS without deadlock.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): protect imported node lifetimes"
```

### Task 5: Event and current-handle consequences

**Files:**
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write stale-current and queued-event stories**

Prove a pre-removal live wrapper fails against a post-removal snapshot with
`stale_handle`. Start an event batch targeting the node, commit removal, then
prove the batch commit returns `stale_revision` and publishes no delivery.
Finally create another node and prove its ID differs from the removed ID.
Register `sai_life_removed_handle` and `sai_life_removed_event`.

**Step 2: Run red/green focused tests**

The existing revision check should already make the event test green. If the
handle or ID test fails, change only the minimal validation or allocation rule;
do not introduce tombstones or queued-event cancellation side effects.

Run:

```bash
ctest --test-dir build-sai-ci -R '^sai_life_removed_' --output-on-failure
```

Expected: PASS.

**Step 3: Commit executable evidence**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "test(sai): prove removal lifecycle consequences"
```

### Task 6: Conformance evidence and phase gate

**Files:**
- Modify: `docs/conformance/sai-invariants.yaml`
- Modify: `docs/conformance/SAI-BASELINE.md`
- Modify: `experimental/sai/README.md`
- Modify: `docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md`

**Step 1: Update only proven claims**

Map the new stable tests to lifecycle, ownership, atomicity, event, and context
invariants. Mark `SVC-NODE-DISPOSE` implemented only if all design obligations
are executable; otherwise retain an explicit planned marker. Document removal
versus disposal in the README.

**Step 2: Regenerate and run all gates**

Run:

```bash
mise run sai-baseline
mise run sai-invariants
mise run sai-conformance-gate
uv run pytest
mise run golden
cmake --build build-sai-ci -j2 --target \
  x3d_sai_experimental_tests x3d_sai_experimental_example \
  x3d_sai_experimental_generated_example x3d_cpp_all_headers
ctest --test-dir build-sai-ci -R '^sai_|^x3d_sai_experimental' \
  --output-on-failure
cmake --build build-sai-san -j2 --target x3d_sai_experimental_tests
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sai-san \
  -R '^sai_life_' --output-on-failure
git diff --check
```

Expected: every gate passes. Record unrelated repository failures separately;
do not modify out-of-scope runtime tests.

**Step 3: Request independent stop/go review**

The reviewer must attempt dangling local references, cross-context races,
wrapper-copy invalidation, historical-snapshot corruption, ID reuse, and hidden
cascade effects. Resolve all Important/Critical findings.

**Step 4: Commit**

```bash
git add docs/conformance/SAI-BASELINE.md \
  docs/conformance/sai-invariants.yaml experimental/sai/README.md \
  docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md
git commit -m "docs(sai): close node lifecycle obligations"
```

Do not begin prototype declaration work until the Phase 2.1 stop/go review is
GO.
