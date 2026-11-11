# Experimental SAI Context Apertures Design

**Status:** Approved direction; firewalled experimental proposal.

## Objective

Close the semantic gap between strong execution-context ownership and useful
cross-context inspection. The kernel must make local lookup isolation,
EXPORT/IMPORT aliasing, source identity, and snapshot behavior executable
without pretending that imported nodes are locally owned or that atomic
multi-context mutation has already been solved.

This slice models explicit apertures. It does not model Inline loading, PROTO
expansion, IS binding, cross-context ROUTE execution, or runtime publication.

## Identity and authority

A local `node` remains owned by exactly one execution-context generation. An
export binds an ordered, unique export name to a local node without changing
that node's identity. An import binds an ordered, unique local alias to one
export in another active context owned by the same browser.

Crossing the aperture yields `imported_node`, not `node`. The imported handle
records importer authority, source generation, source node identity, and local
alias. It is an inspectable proxy and has no implicit conversion to a local
node. Existing containment and retained-field authoring APIs therefore cannot
adopt it accidentally.

Local DEF-style names and imported aliases occupy one local lookup collision
domain. Export names occupy a separate namespace in their source context. A
node may have multiple local, export, and imported aliases; aliases never
become semantic identity.

## Authoring surface

`scene_edit` adds four explicit operations:

- export a local node under a unique export name;
- remove an export by enumerated binding;
- import a named export from another execution context under a local alias;
- remove an import by enumerated binding.

Import staging resolves the source export against one immutable source
revision and records the source context, generation, export name, and resolved
node ID. The source must belong to the same browser and be active. Commit
revalidates the source generation and export binding; a stale or remapped
source fails atomically rather than publishing a dangling alias.

Import update is expressed as remove plus add in one edit. This preserves one
mutation vocabulary and lets the returned `change_set` explain the remapping
in authored order.

## Snapshot and inspection surface

`scene_snapshot` exposes ordered `exports()` and `imports()` ranges. Export
bindings resolve to ordinary local `node` handles. Imported aliases resolve to
`imported_node` proxies.

When a parent snapshot is created, it captures an immutable source-state
pointer for each imported binding after releasing the parent context lock. The
snapshot therefore offers coherent descriptor, dynamic-field, and value reads
for imported nodes while retaining old source revisions for old snapshots.
There is no claim of a globally atomic snapshot spanning multiple contexts;
each captured source revision is reported and independently coherent.

Imported field discovery returns `imported_field`, a read-only aperture view
carrying importer authority and captured source identity. Typed checked views
remain available, but no imported field is accepted by `scene_edit::set` or
`event_batch::send` in this slice.

## Lifecycle and failure

World replacement invalidates imported handles and subscriptions through the
existing generation rules. A source export may change after a parent snapshot;
the old snapshot continues to inspect its captured source revision, while a
new parent snapshot observes the latest source state if the binding identity
remains valid.

The following are structured, non-mutating failures:

- cross-browser import;
- inactive or stale source context;
- unknown or remapped export at commit;
- duplicate export name;
- duplicate local DEF/import alias;
- imported lookup through the wrong importer snapshot;
- containment or retained-field authoring attempted with imported authority.

No callback is generated until a successful parent edit commit. Source field
changes do not synthesize parent scene changes; imported snapshot observation
is pull-based until a future multi-context live publication design exists.

## Acceptance stories

The slice is acceptable only when executable stories demonstrate:

1. Ordinary named lookup never sees another context's local names.
2. An explicit export/import aperture resolves one shared source identity.
3. DEF and import aliases collide atomically in one local namespace.
4. Cross-browser and stale-generation imports are inert structured failures.
5. Export/import ranges are ordered removal tokens; no private lookup key is
   required to reverse authoring.
6. Imported descriptor, typed, and dynamic reads agree through one captured
   source revision.
7. Old parent snapshots remain stable after source mutation while new parent
   snapshots observe the new source revision.
8. Imported nodes cannot enter local containment or retained-field mutation.
9. Source export remapping between staging and commit poisons the parent edit
   and publishes nothing.

The kernel continues to advertise inspection and authoring, but not complete
live execution.
