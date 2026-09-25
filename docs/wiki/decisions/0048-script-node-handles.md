---
title: "ADR-0048: Script SFNode Handles Are Table Ids, Resolved to Owning References"
summary: A script engine never holds a C++ node pointer. Each backend interns nodes into a per-script NodeHandleTable (weak_ptr slots) and gives the engine an unforgeable integer id, stored in a Duktape hidden symbol or a QuickJS class opaque. Resolving an id yields the node's real owning shared_ptr, or null for an unknown id or a destroyed node. This replaces raw pointers split into forgeable JS numbers and rebuilt as no-op-deleter shared_ptrs.
tags: [adr, script, sai, memory-safety, security]
updated: 2026-09-25
related:
  - ../subsystems/system-script-sai.md
  - 0013-js-engine-choice.md
  - 0014-dynamic-field-foundation.md
  - 0022-scriptengine-second-backend-swap-test.md
---

# ADR-0048: Script SFNode Handles Are Table Ids, Resolved to Owning References

## Status

Accepted

## Context

A script sees an `SFNode` as a JS object. Before this decision, both backends put
the raw `X3DNode*` on that object and turned it back into an `SFNode` on the way
out as `SFNode(ptr, [](X3DNode *) {})`, a `shared_ptr` with a no-op deleter. That
caused three defects:

1. **Forgeable handles (QuickJS).** QuickJS has no hidden property keys, so the
   pointer was stored as two ordinary number properties (`"\xff" "x3dNodeLo"` /
   `"…Hi"`). A script could copy them onto a plain object, or edit them, and the
   runtime would dereference the result. Test T15b reproduced this against the
   old code: the forged copy added a live route. Script source comes from
   document content, so this let a hostile X3D file aim the runtime at an
   arbitrary address.
2. **Dangling references.** An `SFNode` a script wrote into a field (for example
   `children`) was a non-owning alias. When the real owner dropped the node, the
   scene graph held a dangling pointer. The alias also had its own control block,
   so `weak_ptr`s taken from it never tracked the real object.
3. **Nothing checked liveness.** A handle to a node the scene had since dropped
   was dereferenced anyway.

The X3D ECMAScript binding (ISO/IEC 19777-1) treats an `SFNode` value as a node
*reference*. When a script routes a node into `children`, that is the same
sharing a DEF/USE produces. So the fix follows the standard: a script-produced
`SFNode` must be an owning reference.

## Decision

- `runtime/script/NodeHandleTable.hpp` holds one table per script, owned by that
  script's `SaiContext` (`nodeHandles()`). `intern(SFNode)` returns a stable
  `uint32_t` id per live node, and each slot holds a `weak_ptr`. A dead slot at a
  recycled address gets a fresh id, so an old handle never aliases a new node.
  `resolve(id)` returns the owning `shared_ptr`. It returns null for an unknown
  id or a destroyed node.
- **Duktape** stores the id under a `\xff`-prefixed key. That is a Duktape hidden
  symbol, which ECMAScript code cannot read, enumerate or create. The
  `SaiContext*` lives in the global stash, which is also unreachable from script.
- **QuickJS** registers a native `X3DNode` class per runtime and stores the id in
  the object's opaque slot. `JS_GetOpaque(v, classId)` checks the class, so a
  plain object never resolves.
- `Browser.addRoute` / `deleteRoute` receive the resolved node. A null node is
  `INVALID_NODE`, and `addRoute` also validates fields, direction and type, as
  in the Script/SAI subsystem page.
- The route callbacks coerce their string arguments first, hold every C++ object
  in an inner scope, and call `duk_error` only after that scope has unwound. When
  Duktape still raised errors by longjmp, each rejected call otherwise leaked a
  node reference (LeakSanitizer caught this).

### Engine error containment (amended 2026-09-25)

Reading a value back from a script can run script code: a global accessor,
`toJSON`, getters, a Proxy trap. In Duktape a throw there happened outside any
`duk_pcall`, so the fatal handler ran and aborted the host process. Any document
with a Script could crash the embedder. Test T15f reproduced this.

- Duktape is compiled as C++ with `DUK_USE_CPP_EXCEPTIONS` (a local change in
  `duk_config.h`, applied only to C++ compiles). Script errors are C++
  exceptions, so they unwind the backend's frames with destructors run. On MSVC
  both Duktape targets build with `/EHs`, because the API is `extern "C"` and now
  throws.
- Every Duktape entry runs under `duk_safe_call` through `protectedRun` /
  `callGlobal`: install, seeding, readback, handler dispatch, `prepareEvents`,
  `initialize`, `eventsProcessed`, `shutdown`. The global lookup is included,
  because a script can make a handler name a throwing accessor.
- A throw during readback drops that one field's event for that callback. It is
  logged, and the script keeps running.
- QuickJS never aborted, but it ignored the pending exception and emitted
  whatever the conversion had half-read. Its readback now drops the field and
  clears the exception, and seeding clears an exception from a throwing setter.
  T15f runs the same five hostile cases against both engines.

### Call budget (amended 2026-09-25)

A handler, or a script's top level, that never returns used to hang the host.
Script source comes from the document, so this is the same exposure as above.

- `ScriptEngine::setCallBudget()` sets the wall-clock budget. The default is
  `kDefaultCallBudget` (2 s), and zero disables it.
- Each public entry into script code gets its own budget: `load` (the top
  level), `initialize`, one `invoke`, `prepareEvents`, `eventsProcessed` and
  `shutdown`. The budget covers reading the call's outputs back.
- Duktape builds with `DUK_USE_INTERRUPT_COUNTER` and `DUK_USE_EXEC_TIMEOUT_CHECK`
  (local `duk_config.h` changes). The heap udata is the script's `CallDeadline`.
  Once the deadline passes, Duktape raises a `RangeError` and keeps raising it
  until the deadline is disarmed, so a `try`/`catch` loop cannot outlast it.
- QuickJS uses `JS_SetInterruptHandler` on the shared runtime, which raises an
  uncatchable "interrupted" error.
- An interrupted call is a script error: it is logged, and the script stays
  loaded. A load whose top level times out fails. T15g covers a plain infinite
  loop, a catch-and-continue loop and a top-level loop against both engines.

## Consequences

- A script can no longer fabricate a node reference. The worst it can do is name
  a node it was given.
- Script-held handles do not keep nodes alive. A pending cascade event or a
  field value does, as it should.
- The table grows with the number of distinct nodes a script has seen over its
  lifetime (one `weak_ptr` plus one map entry each). For realistic scripts this
  is small. A long-running script that touches an unbounded stream of distinct
  nodes would need slot recycling, which is not implemented.
- Tests that passed stack nodes through `SFNode(&node, [](X3DNode *) {})`
  relied on the old borrowed-pointer semantics. They now use `make_shared`.
- A script can no longer crash the host through a throwing accessor, `toJSON`
  or Proxy (see "Engine error containment"), or hang it with a runaway loop
  (see "Call budget"). The embedder's remaining exposure is memory: neither
  engine caps a script's heap today.

## Related

- [Script / SAI Runtime subsystem](../subsystems/system-script-sai.md)
- [ADR-0014: Dynamic Field Foundation](0014-dynamic-field-foundation.md): the
  author-field store, whose raw-pointer keying was fixed in the same change
- Tests: `runtime/script/tests/ecmascript_backend_test.cpp` and
  `quickjs_backend_test.cpp` (T15b, T15c, T15d, T15f, T15g)
