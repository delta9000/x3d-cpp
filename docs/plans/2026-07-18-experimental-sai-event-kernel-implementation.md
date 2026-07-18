# Experimental SAI Event Kernel Implementation Plan

> **For implementation:** Use test-driven development task-by-task. Keep all
> changes uncommitted while the event interface remains experimental.

**Goal:** Add a timestamped, causal, atomic event-cascade reference model to
`x3d::sai::experimental` without depending on the current runtime.

**Architecture:** A move-only `event_batch` stages external event seeds against
one immutable scene revision. Commit propagates over the existing route graph
in private state, loop-breaks per route, publishes inputOutput storage once at
quiescence, and queues field observations for explicit drain.

**Tech stack:** C++20, standard library only, doctest, CMake/CTest.

---

### Task 1: Event vocabulary and access boundary

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/types.hpp`
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/src/kernel.cpp`
- Test: `experimental/sai/tests/semantic_kernel_test.cpp`

1. Write a story constructing `event_batch` with a strong `event_time`, sending
   to `inputOnly`, rejecting `outputOnly` and `initializeOnly`, and proving a
   rejected batch publishes nothing.
2. Build and run `x3d_sai_experimental`; verify RED on missing event APIs.
3. Add `event_time`, `event_delivery`, `event_result`, move-only `event_batch`,
   `execution_context::events`, scalar typed/dynamic `send`, poisoning, and base
   revision capture.
4. Implement transient inputOnly commit with no revision advance.
5. Run the focused suite until GREEN.

### Task 2: InputOutput publication and ownership-safe node values

**Status:** Completed.

**Files:** Same as Task 1.

1. Write stories proving inputOutput delivery updates one immutable revision,
   typed and dynamic sends yield the same trace, and node/MFNode sends reject a
   foreign context before staging.
2. Verify RED.
3. Reuse descriptor/type/lifecycle validation and context-bearing node/span
   overloads; do not duplicate authoring authority checks.
4. Publish an ordered change set only after complete batch validation.
5. Run focused tests until GREEN.

### Task 3: Causal ROUTE propagation and loop breaking

**Status:** Completed.

**Files:** Same as Task 1.

1. Add one-hop, multi-hop, and cyclic-route stories using inputOutput fields.
2. Verify RED.
3. Build an authored-order route adjacency view from staged routes, attach
   causal predecessor/route index to deliveries, and allow each route to fire
   at most once per batch.
4. Ensure one timestamp is preserved through the trace and quiescence publishes
   once.
5. Run focused tests until GREEN.

### Task 4: Fan-in and portability evidence

**Status:** Completed.

**Files:** Same as Task 1.

1. Add a two-route fan-in story and a duplicate-seed rejection story.
2. Verify RED.
3. Add structured portability issues to `event_result`; retain deterministic
   diagnostic order without declaring it portable semantics.
4. Reject duplicate seed fields before cascade and publish nothing on failure.
5. Run focused tests until GREEN.

### Task 5: Field observation and reentrancy

**Status:** Completed.

**Files:** Same as Task 1.

1. Add stories for output-capable field subscription, commit-without-callback,
   cancellation before drain, callback exceptions, and a callback submitting a
   later timestamped batch.
2. Verify RED.
3. Generalize subscription bookkeeping to distinguish scene and field channels
   while retaining move-only RAII and shared immutable callback ownership.
4. Queue event deliveries at commit and dispatch outside locks through the
   existing `drain()` boundary.
5. Track the last published event time and reject time regression atomically.
6. Run focused tests until GREEN.

### Task 6: Evidence, example, and verification

**Status:** Completed.

**Files:**
- Modify: `experimental/sai/examples/author_inspect.cpp`
- Modify: `experimental/sai/README.md`
- Modify: `CMakeLists.txt`
- Modify: `docs/conformance/sai-invariants.yaml`
- Modify: `docs/conformance/sai-services.yaml`
- Regenerate: `docs/conformance/SAI-BASELINE.md`

1. Extend the composed example with one external inputOutput event, ROUTE
   delivery, coherent snapshot inspection, and explicit event drain.
2. Add stable per-invariant CTests only for demonstrated event properties.
3. Map experimental services as partial; keep live capability false until the
   complete required live surface exists.
4. Run clang-format, Release/Werror, ASan/UBSan, focused CTests, conformance
   validation, full Python tests, the behavior suite, and `git diff --check`.
5. Confirm the branch remains uncommitted and the existing runtime unchanged.
