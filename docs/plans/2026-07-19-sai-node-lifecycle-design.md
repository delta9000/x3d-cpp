# Experimental SAI Node Lifecycle Design

**Status:** Approved for `x3d::sai::experimental`

## Purpose

Define node removal independently from graph detachment, historical inspection,
and C++ wrapper lifetime. The API must make destructive graph changes explicit,
preserve snapshot meaning, and keep wrapper cleanup local and unsurprising.

## Semantic distinctions

Four states remain separate:

1. **Node existence:** whether the current scene revision contains a node record.
2. **Graph reachability:** whether roots or node-valued fields contain occurrences
   of that node.
3. **Semantic references:** whether names, exports, imports, routes, or staged
   events identify the node.
4. **Wrapper usability:** whether one local C++ wrapper still carries authority to
   address its node in its originating context generation.

Destroying any one of these does not implicitly destroy the others.

## Removal contract

`scene_edit::remove_node(const node&)` stages deletion of a node record. Removal
is governed by strict referential integrity:

- It fails while the final staged scene still contains a root occurrence, an
  SFNode/MFNode reference, a name, an export, a local route endpoint, or another
  retained semantic reference to the node.
- References may be removed or rewritten earlier in the same edit, so a complete
  graph rewrite and node removal publish atomically in one commit.
- A C++ `node` wrapper is not a semantic reference and never prevents removal.
- Removal never cascades. The API does not silently detach occurrences, rewrite
  fields, remove names, or remove routes.
- Removal of an already absent node fails through the ordinary structured SAI
  error vocabulary and poisons the edit like other invalid mutations.
- Successful publication emits one ordered `node_removed` change after the
  explicit reference-removal changes that made it legal.

Cross-context imported apertures are semantic references. Until the context
model has an authoritative reverse-reference protocol, a source node exposed
through an active export/import aperture is not removable. The implementation
must not pretend one context can atomically mutate an independent importer.

## Revision and handle behavior

Scene snapshots own immutable revision state:

- A snapshot captured before removal continues to describe and read the removed
  node exactly as it existed in that revision.
- A snapshot captured after removal cannot look the node up.
- A live wrapper created before removal retains its semantic ID, but operations
  against the current revision fail as stale once removal commits.
- Concurrent edits and event batches retain the existing revision rules; a
  removal commit makes an older batch stale rather than surgically editing its
  queued deliveries.

Node IDs are not recycled within a context generation. Removal therefore cannot
make an old wrapper accidentally address a later node.

## Wrapper disposal

`node::dispose() noexcept` releases only that wrapper's local authority:

- It performs no scene mutation, callback, event dispatch, allocation, or
  synchronization that can fail.
- It is idempotent.
- It does not invalidate copies of the wrapper.
- It preserves the wrapper's observable semantic ID for diagnostics while all
  authority-bearing operations reject the disposed wrapper as stale.
- Destruction has the same local resource consequence but no semantic side
  effect; explicit disposal exists for SAI lifecycle parity and deterministic
  wrapper invalidation.

Typed nodes forward the same local disposal semantics without changing their
compile-time tag.

## Error and atomicity rules

Removal and disposal add no exception channel. Removal returns `result<void>`;
disposal is `noexcept`. Referential conflicts identify the blocking node and,
when applicable, field or binding in `sai_error`. A failed removal poisons its
edit, and `commit()` publishes nothing. Validation is performed against the
staged state so operation order within one edit can deliberately establish the
final valid graph.

## Rejected alternatives

### Cascading removal

Automatically removing roots, fields, names, routes, and exports makes a single
call perform an unbounded destructive rewrite. It obscures intent, weakens
change-set auditability, and cannot safely cross context boundaries.

### Current-scene tombstones

Keeping removed records in the current scene would force every lookup, field,
route, traversal, and serializer path to reinterpret existence. Historical
snapshots already preserve the useful audit behavior without infecting the
current model.

### Shared disposal state across wrapper copies

Making disposal invalidate copies turns ordinary value copying into hidden
shared lifecycle coupling. Scene staleness belongs to semantic state; wrapper
disposal remains local value state.

## Verification obligations

Tests must prove:

- unreferenced node removal;
- rejection for roots, shared SFNode/MFNode occurrences, names, exports, routes,
  and active imported apertures;
- atomic explicit detachment followed by removal;
- unchanged publication after a rejected removal;
- old-snapshot readability and new-snapshot absence;
- stale live handles and non-recycled IDs;
- stale queued event batches after a removal commit;
- local, copied, idempotent, callback-free, `noexcept` disposal;
- dynamic and typed wrappers share the contract;
- ordered `node_removed` change evidence.

The service mapping for node disposal remains incomplete until these obligations
pass under the normal, Werror, and sanitizer gates.
