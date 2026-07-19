# Experimental SAI Declaration Model Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add stable, ordered, editable PROTO/EXTERNPROTO declarations with exact interfaces, exclusive template scopes, honest external resolution state, and immutable snapshot behavior.

**Architecture:** Declarations live in the immutable scene revision beside nodes but have their own monotonic identity domain and name namespace. Local declarations claim an exclusive closure of template nodes; external declarations retain URL and load facts and may explicitly reference a compatible local declaration. All mutations stage through `scene_edit` and publish one ordered heterogeneous change sequence.

**Tech Stack:** C++20, `tl::expected`, immutable `scene_state`, `std::variant`, CMake/CTest, doctest, ASan+UBSan.

---

### Task 1: Declaration identity, descriptors, and ordered inspection

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/types.hpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing identity/order story**

Add a test that creates one body-less local PROTO and one unresolved
EXTERNPROTO, commits, and verifies authored order, distinct monotonic IDs,
shared name lookup, kind tags, URL preservation, and no collision with a DEF of
the same spelling. Register `sai_decl_identity_order`.

**Step 2: Verify red compilation**

Run the focused target and expect missing declaration types and methods.

**Step 3: Add owning public types**

Add:

```cpp
struct declaration_id {
  std::uint64_t value = 0;
  friend bool operator==(declaration_id, declaration_id) = default;
};
enum class declaration_kind { local_proto, external_proto };
enum class external_load_state { unresolved, loading, resolved, failed };

struct interface_field_descriptor {
  std::string name;
  value_kind kind;
  access_type access;
  std::optional<value> default_value;
  std::vector<std::string> accepted_node_types;
  std::optional<std::string> unit_category;
};

struct local_declaration_descriptor {
  std::string name;
  std::vector<interface_field_descriptor> interface;
  std::vector<node_id> body_roots;
  std::string appinfo;
  std::string documentation;
};

struct external_declaration_descriptor {
  std::string name;
  std::vector<interface_field_descriptor> interface;
  std::vector<std::string> urls;
  external_load_state load_state = external_load_state::unresolved;
  std::string diagnostic;
  std::optional<declaration_id> resolved_declaration;
  std::string appinfo;
  std::string documentation;
};

using declaration_payload =
    std::variant<local_declaration_descriptor,
                 external_declaration_descriptor>;
struct declaration_descriptor {
  declaration_id id;
  declaration_payload payload;
};
```

Expose `declaration_kind kind(const declaration_descriptor&) noexcept` and
`name(const declaration_descriptor&)` helpers so generic code does not repeat
variant plumbing for common facts.

**Step 4: Add the declaration handle and immutable storage**

Add a context/generation/ID-bearing `declaration` handle. Extend `scene_state`
with `next_declaration_id`, ordered IDs, and an ID-keyed descriptor map. Add
`scene_snapshot::declarations`, `declaration_at`, `declaration_named`, and
`describe`. Add minimal `scene_edit::add_local_declaration` and
`add_external_declaration`, initially accepting empty local bodies and
unresolved external declarations only.

**Step 5: Run focused and snapshot tests**

```bash
cmake --build build-sai-ci -j2 --target x3d_sai_experimental_tests
ctest --test-dir build-sai-ci \
  -R '^sai_decl_identity_order$|^sai_field_pure_read$|^sai_order_' \
  --output-on-failure
```

Expected: PASS.

**Step 6: Commit**

```bash
git add CMakeLists.txt experimental/sai/include/x3d/sai/experimental/types.hpp \
  experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/src/kernel.cpp experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): add declaration identity and inspection"
```

### Task 2: Exact interface and namespace validation

**Files:** Same as Task 1.

**Step 1: Write table-driven failing tests**

Cover empty and duplicate declaration names, one namespace shared across local
and external declarations, duplicate interface names, mismatched defaults,
defaults on inputOnly/outputOnly fields, invalid unit categories, constraints
on non-node fields, empty URL lists, and preserved interface order. Verify each
failure poisons the edit and publishes no revision. Register
`sai_decl_interface_validation` and `sai_decl_failure_nonmutation`.

**Step 2: Verify failures**

Expected: malformed descriptors currently publish.

**Step 3: Implement one validator**

Reuse the existing representation, unit, and node-kind predicates rather than
forking field semantics. Require defaults for initializeOnly/inputOutput only
when authored; absent means the field-type default is determined at instance
creation in Task 2.3. Reject all event-field defaults.

**Step 4: Run the complete field vocabulary and declaration tests**

```bash
ctest --test-dir build-sai-ci \
  -R '^sai_decl_|^sai_field_complete_vocabulary$|^sai_field_value_boundaries$' \
  --output-on-failure
```

Expected: PASS.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): validate declaration interfaces"
```

### Task 3: Rename, update, remove, and stale identity

**Files:** Same as Task 1.

**Step 1: Write failing mutation-dual stories**

Prove rename preserves ID and order, update preserves ID, remove makes the live
handle stale, remove/re-add with the same name gets a larger ID, and old
snapshots retain every old descriptor. Mutation tokens must be declaration
handles, not names or indexes. Register `sai_decl_mutation_duals` and
`sai_decl_snapshot_history`.

**Step 2: Add edit methods**

Implement:

```cpp
result<void> rename_declaration(const declaration &, std::string);
result<void> update_declaration(const declaration &, declaration_payload);
result<void> remove_declaration(const declaration &);
```

Validate context/generation/current staged presence exactly like node handles.
Never reuse IDs or reorder on rename/update. Removal erases the ordered ID and
descriptor; local body release and external target retention arrive in later
tasks before this service is considered complete.

**Step 3: Run declaration, lifecycle, and ordering tests**

Expected: PASS.

**Step 4: Commit**

```bash
git add CMakeLists.txt experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/src/kernel.cpp experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): edit declaration lifecycles"
```

### Task 4: Heterogeneous ordered change evidence

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Modify: `experimental/sai/examples/author_inspect.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

**Step 1: Write the failing interleaving story**

In one edit create a node, add a declaration, rename it, change a field, and
remove the declaration. Require one total ordered sequence with distinct node
and declaration identity types. Register `sai_decl_change_order`.

**Step 2: Introduce dedicated records**

Rename the current record to `graph_change`. Add `declaration_change_kind`
(`added`, `renamed`, `updated`, `removed`) and `declaration_change`. Define:

```cpp
using semantic_change = std::variant<graph_change, declaration_change>;
struct change_set {
  revision_id before_revision;
  revision_id after_revision;
  std::vector<semantic_change> changes;
};
```

Provide `change_kind_of`, `node_change`, and `declaration_change_of` checked
helpers so callers need not use unchecked `std::get`. Migrate current tests and
examples mechanically. Do not retain a second node-only change list.

**Step 3: Verify node-only behavior parity**

Run every existing `sai_edit_*`, `sai_evt_*`, and example test plus the new
interleaving test. Existing ordering and payload facts must remain identical.

**Step 4: Commit**

```bash
git add experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/src/kernel.cpp experimental/sai/examples/author_inspect.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp CMakeLists.txt
git commit -m "feat(sai): order declaration and graph changes"
```

### Task 5: Exclusive local template-node scope

**Files:** Same as Task 1.

**Step 1: Write failing ownership stories**

Construct a local body with multiple roots, shared descendants, SFNode and
MFNode edges. Prove its closure is claimed by the declaration and preserved in
authored root order. Reject scene-root nodes, exports, names, routes, fields
crossing scene/template scope, a node already claimed by another declaration,
and cross-context handles. Prove declaration removal releases—rather than
deletes—the template closure. Register `sai_decl_template_scope`,
`sai_decl_scope_leakage`, and `sai_decl_scope_release`.

**Step 2: Add explicit node scope to records**

Add an optional owner declaration ID to each node record. Claim the complete
node-valued closure atomically when adding/updating a local declaration. Reject
any existing scene semantic reference to the closure. Extend every node-valued
write, root, name, export, route, and removal validator to enforce equal scope.
Commit-time validation walks both scene roots and each declaration's body roots
without treating template nodes as live scene occurrences.

**Step 3: Prove lifecycle interaction**

Template ownership itself retains nodes against `remove_node`. After
`remove_declaration`, the released nodes are unattached and may be rooted,
reclaimed by another declaration, or removed in the same edit.

**Step 4: Run graph, lifecycle, ownership, and declaration tests**

Expected: PASS under Werror.

**Step 5: Commit**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): isolate prototype template scopes"
```

### Task 6: Honest EXTERNPROTO resolution state

**Files:** Same as Task 1.

**Step 1: Write failing state-machine stories**

Prove unresolved, loading, failed, and resolved descriptors retain ordered URLs
and diagnostics without an invented body. A resolved external declaration must
target an existing distinct local declaration with an interface compatible in
name, kind, access, order, constraints, and defaults. Register
`sai_decl_external_state` and `sai_decl_external_compatibility`.

**Step 2: Implement transitions and retention**

Permit explicit transactional state updates only. A resolved external
declaration retains its target: local rename is allowed by identity, but local
update to an incompatible interface and local removal return a structured
retained-reference error until the external is redirected/unresolved/failed or
removed. Revalidate targets at commit to close concurrent edit races.

**Step 3: Run external declaration and failure-atomicity tests**

Expected: PASS with no I/O or runtime dependency.

**Step 4: Commit**

```bash
git add CMakeLists.txt experimental/sai/src/kernel.cpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): model external prototype resolution"
```

### Task 7: Conformance evidence and declaration stop/go gate

**Files:**
- Modify: `docs/conformance/sai-services.yaml`
- Modify: `docs/conformance/sai-invariants.yaml`
- Modify: `docs/conformance/SAI-BASELINE.md`
- Modify: `experimental/sai/README.md`
- Modify: `docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md`

**Step 1: Map only executable declaration services**

Update PROTO/EXTERNPROTO enumerate/find/add/update/remove entries only where the
new kernel and stable tests satisfy their obligations. Keep instance creation,
IS binding, loading, and runtime behavior planned.

**Step 2: Run all gates**

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
  -R '^sai_decl_' --output-on-failure
git diff --check
```

Expected: all gates pass.

**Step 3: Request independent stop/go review**

The reviewer must attack name/identity aliasing, scope leakage, body closure,
old snapshots, external state invention, resolution retention, change ordering,
ID reuse, and service overclaim. Resolve every Important/Critical finding.

**Step 4: Commit**

```bash
git add docs/conformance docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md \
  experimental/sai/README.md
git commit -m "docs(sai): close declaration model obligations"
```

Do not begin prototype instances or IS bindings until this stop/go review is
GO.
