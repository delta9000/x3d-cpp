---
title: Activation ownership
summary: Which context owns activated scene state, how it retires, who owns author fields, and the process-wide diagnostics.
tags: [subsystem, execution, lifetime]
updated: 2026-11-14
related:
  - execution-context.md
---

# Activation ownership

This page continues [Execution context](execution-context.md).

A mutable native node graph belongs to one active execution context. The host
owns that activation and serializes construction, ticks, field calls and
teardown; there is no hidden event loop and no concurrent-destruction guarantee.
`RuntimeSession` owns the document, context and extractor. A caller may retain a
node `shared_ptr` beyond the session, but that does not retain its runtime.

- Native-installed bindable, interpolator (including spline, NURBS and
  geospatial), follower, event-utility and H-Anim motion input handlers carry
  weak lifetime leases.
- Behavior handlers require both the context and the individual `System` to
  survive; bindable handlers require their context-owned `BindingSystem`.
- Context-owned timing filters and key/trigger write listeners are guarded
  too.
- After retirement, calling these retained native input handlers is a no-op:
  readable field storage remains, and ordinary native storage setters still
  work.
- The guards do not traverse retained nodes, retain the scene, or clear a
  handler installed by a later activation.
- They also cover attachment failure before a new system has reached
  `addSystem`, and session-constructor unwinding.
- Standalone `BindingSystem` and all standard callback-owning systems retire
  in their destructor bodies before member-owned captures or state can be
  destroyed; a capture destructor that invokes a retained node therefore sees
  an inert handler.
- `bool X3DExecutionContext::retireCallbacks() noexcept` is an idempotent,
  terminal revocation seam.
- It returns `false`, leaving the activation unchanged, while a native guarded
  input handler or `tick()`, `process`, or `writeField` is in flight.
- The host must defer replacement/destruction until its current runtime call
  has returned.
- Destroying the context/session reentrantly is a contract violation and fails
  closed with `std::terminate`; the guards are not a lock or a mechanism to
  make deletion from a running callback safe.
- No further runtime operations may be made on a retired activation.
- The session destructor retires callbacks before extractor teardown; the
  context destructor independently retires them before any context-owned
  member is destroyed.
- The low-level context path therefore has the same retained-node protection.
- Custom `System` implementations must wrap escaping void handlers with
  `ctx.guardCallback(*this, handler)` (or predicate filters with
  `ctx.guardInputFilter(*this, filter)`) to opt into that protection.
- A custom most-derived owner must also call the protected
  `retireCallbacksBeforeDestruction()` at the start of its destructor, before
  its members are destroyed.
- The `System` base destructor alone is too late for a member/capture
  destructor that invokes an escaped handler.
- This applies equally to further subclasses of built-in systems.
- Install custom handlers after full construction; if a custom constructor
  installs handlers and then can throw, it must retire them before
  member-unwinding begins.
- It does not apply to unguarded callbacks supplied by an embedder, dangling
  native raw pointers, concurrent native access, or arbitrary structural
  mutation.
- Projection selection is fixed per context; see [geospatial
  ownership](geospatial.md#world-ownership-and-migration).
- Author fields now have the explicit scene ownership described below.

### Author-field ownership

- Every fresh `Scene` owns a fresh `shared_ptr<DynamicFieldStore>` in
  `scene.authorFields`.
- Shallow Scene copies deliberately share both node identity and that store;
  they are not independent worlds.
- `ProtoDeclaration::authorFields` retains its template/default fields after
  an EXTERN resolver's temporary document dies.
- `RuntimeSession` binds its context to its owned document's store.
- Retaining only a node does not retain that Scene store; retain the explicit
  shared owner too when author fields must remain readable after world
  teardown.
- For low-level setup, prefer `X3DExecutionContext
  ctx(doc.scene.authorFields)`.
- An untouched default context binds once when `buildSceneGraph`, `buildFrom`
  or `buildRoutes` receives a Scene.
- Calling `ctx.authorFields()` first fixes its fresh standalone owner; later
  attempts to switch owners, or to bind while events are pending or after a
  tick, throw before changing scene state.
- Rebuild all concrete runtime consumers for this source/ABI change.
- Never replace a Scene's owner while it is active.
- Inline adoption shares author-field entries for its imported nodes,
  preserving one value identity without sharing the entire store.
- Detachment drops only the parent view: a retained child Scene or cached
  GeoLOD tile keeps its author data.
- Redisplaying a cached tile imports those entries again before system
  attachment.
- Authored GeoLOD rootNode fields use a per-LOD retained entry view while
  inactive, then return to the active Scene on redisplay.
- Conflicting live entries for one imported node are rejected.
- Copied FieldInfo thunks are inert after their tracked node expires, even
  before any store lookup sweeps the entry; they also reject calls on a
  different node identity.
- The removed zero-argument `dynamicFieldStore()` has no global or
  thread-local compatibility fallback.
- Use `*scene.authorFields`, `*declaration.authorFields`, or
  `ctx.authorFields()` at the appropriate ownership boundary, and pass that
  store to `effectiveFields(node, store)`.
- See
  [ADR-0057](../../explanation/decisions/0057-scene-owned-author-fields.md)
  for the clone and serializer migration.
- This does not establish general SAI, Script, PROTO or Geo conformance.

### Process-wide diagnostics

- `TransformSystem::localMatrixCallCount()` and
  `X3DExecutionContext::pickCallCount()` return process-wide diagnostic
  totals, not per-world measurements or semantic scene state.
- Their backing counters use relaxed atomic increments and reads so separate
  owner-thread worlds can contribute without a diagnostic data race.
- This does not make an individual scene/context thread-safe, nor synchronize
  scene changes.
- The concurrent `runtime_diagnostic_counters_test.cpp` test drives four
  distinct owner-thread worlds and checks their combined totals after joining
  those threads.
- The two public backing variables now have atomic type; callers should use
  the unchanged `uint64_t` accessors rather than copying a backing counter
  object.

### Source compatibility and migration

- `X3DExecutionContext`, `BindingSystem`, and `System` (therefore all built-in
  and custom derived systems) explicitly delete copy construction, copy
  assignment, move construction and move assignment.
- Their callbacks and internal references bind to a stable owner address.
- The previous context and binding-system copies, and stateless system copies,
  really compiled; they were not safe activation clones.
- Context copy/move assignment already failed because its cascade holds a
  reference; those deletions make the existing restriction explicit.
- Some stateful systems already failed to instantiate copies because they own
  `unique_ptr` state.
- `RuntimeSession` was already noncopyable/nonmovable.
- Keep these owners in place or transfer `unique_ptr`/`shared_ptr` ownership,
  rather than moving the owner object.
- To duplicate an activation, create a fresh context and fresh systems, then
  attach them to a separately owned node graph.
- To re-use retained nodes, first retire the previous activation outside all
  runtime calls, then attach the replacement.
- This is a source-compatibility tightening; downstream applications need a
  rebuild and call-site audit.
