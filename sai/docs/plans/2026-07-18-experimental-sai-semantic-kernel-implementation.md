# Experimental SAI Semantic Kernel Implementation Plan

> **For implementation:** Use test-driven development for every behavior. Do not commit while the interface remains experimental and under review.

**Goal:** Produce a firewalled, compiling `x3d::sai::experimental` reference kernel whose executable user stories settle the initial semantic seams.

**Architecture:** A C++20 library under `experimental/sai/` owns a small immutable-state scene model. Public handles carry semantic identity; private control blocks own revisions, generations, subscriptions, and async request state. No existing runtime type is used.

**Tech stack:** C++20, CMake, doctest, standard library only.

---

### Task 1: Experimental target and fundamental vocabulary

**Status:** Completed.

**Files:**
- Create: `experimental/sai/include/x3d/sai/experimental/types.hpp`
- Create: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Create: `experimental/sai/src/kernel.cpp`
- Create: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

1. Write a compile-and-behavior test for `result<T>`, stable IDs, descriptors,
   registry construction, browser creation, and an empty initial snapshot.
2. Configure/build the target and verify RED because the experimental header is
   absent.
3. Implement the minimal vocabulary, registry, browser, context, and snapshot.
4. Build and run the focused CTest until GREEN.

### Task 2: Identity, committed authoring, and occurrence traversal

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`

1. Add a story creating Group/Transform nodes, attaching one Transform twice,
   committing, and observing one node ID through two occurrences.
2. Verify RED on missing edit/occurrence services.
3. Implement staged node creation, node-valued fields, roots, atomic commit,
   immutable snapshots, and occurrence traversal with cycle rejection.
4. Run the focused test until GREEN.

### Task 3: Unified typed/dynamic fields, errors, and change sets

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`

1. Add stories proving typed/dynamic substitutability, structured type/access
   failures, ignored-operation poisoning, non-mutation on failure, and ordered
   before/after changes.
2. Verify RED on the missing field and error behavior.
3. Implement descriptor lookup, `dynamic_field`, `field<T>`, reads, writes,
   access validation, edit poisoning, and ordered `change_set` publication.
4. Run focused tests until GREEN.

### Task 4: Names, routes, snapshots, conflicts, and stale handles

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`

1. Add stories for scoped name lookup, route inspection, two edits from one
   revision, old snapshot stability, and world-replacement invalidation.
2. Verify RED.
3. Implement names/routes in staged state, optimistic revision comparison,
   generation validation, and browser world replacement.
4. Run focused tests until GREEN.

### Task 5: Observation and reentrant dispatch

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`

1. Add stories proving commit invokes no callback, drain ordering, cancellation,
   move-only RAII, destruction safety, and a callback committing another edit.
2. Verify RED.
3. Implement observer registration, queued change sets, move-only subscription,
   explicit drain, and lock-free callback invocation.
4. Run focused tests until GREEN.

### Task 6: Async generation safety and composed user story

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Create: `experimental/sai/examples/author_inspect.cpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Modify: `docs/conformance/sai-services.yaml`
- Regenerate: `docs/conformance/SAI-BASELINE.md`

1. Add tests for request cancellation, stale completion after replacement, valid
   completion, and one composed author/edit/inspect/observe flow.
2. Verify RED.
3. Implement load tickets and generation checks, then compile the example.
4. Map only genuinely demonstrated services/invariants in the convergence
   register; do not mark the experimental proposal as production support.
5. Run focused tests, normal SAI gates, full pytest, C++ behavior tests, and
   `git diff --check`.
