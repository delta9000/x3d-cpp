# Experimental SAI Declaration Model Design

**Status:** Approved for `x3d::sai::experimental`

## Purpose

Model PROTO and EXTERNPROTO declarations as first-class semantic objects before
adding instances or IS bindings. Declaration identity, mutable names, interface
metadata, template graph ownership, and external resolution state must remain
distinct and inspectable.

## Identity and namespace

Every declaration receives a monotonically increasing `declaration_id` within
one execution-context generation. IDs are never reused. A declaration handle
contains context authority, generation, and ID; its name is discovered through
the current or captured declaration descriptor.

PROTO and EXTERNPROTO declarations share one ordered declaration namespace.
Names are unique within that namespace but do not collide with DEF or IMPORT
node aliases. Renaming a declaration preserves its ID. Removing and recreating
the same name creates a new ID, so stale handles cannot alias the replacement.

## Unified tagged descriptor

One owning `declaration_descriptor` supports generic enumeration and tooling:

- stable `declaration_id`;
- mutable name;
- `declaration_kind` (`local_proto` or `external_proto`);
- ordered interface fields;
- documentation metadata;
- kind-specific local-body or external-resolution facts.

The public surface does not use an inheritance hierarchy. Consumers switch on
the explicit kind or request a checked local/external view. Common operations
therefore remain one query/mutation vocabulary.

## Interface fields

`interface_field_descriptor` uses the same exact `value_kind`, `access_type`,
owning `value`, default-source, unit-category, and accepted-node-type concepts
as ordinary fields. It is a separate type because a declaration interface is
not a field handle and has no owning node.

Interface validation requires:

- non-empty unique names in authored order;
- exact default representation for initializeOnly/inputOutput fields;
- no authored default for inputOnly/outputOnly fields;
- valid unit categories and compatible scalable kinds;
- node constraints only on SFNode/MFNode kinds.

Task 2.2 records this contract but does not create instance storage or event
endpoints. Task 2.3 applies the ordinary access state machine to instances.

## Local PROTO template scope

A local declaration owns an ordered set of body roots. Template nodes live in
the same context arena and retain normal semantic node IDs, but each node record
has an explicit scope owner: scene/unattached or one declaration ID.

Declaration creation accepts unattached body roots and claims their complete
reachable node-valued closure for that declaration. Claiming is atomic and
rejects:

- a node already owned by another declaration;
- a node reachable from scene roots;
- a closure that crosses into another declaration scope;
- a body root or descendant from another context/generation;
- a containment cycle or dangling node reference.

Once claimed, template nodes cannot become scene roots, node values of scene
nodes, exports, or route endpoints. Their internal fields may only reference
nodes in the same declaration scope. The inverse is also true: a template graph
cannot retain ordinary scene nodes. This prevents a PROTO template from being
mistaken for a live instance graph.

Declaration removal releases its template nodes back to unattached ownership;
it does not delete them. The caller may then reuse or explicitly remove them.
Task 2.3 may instead reject removal while instances retain the declaration, but
Task 2.2 does not invent instance references.

Old snapshots preserve the declaration descriptor and template ownership of
their revision. Current handles to removed declarations become stale.

## EXTERNPROTO state

An external declaration owns:

- an ordered, non-empty URL candidate list;
- its locally declared interface;
- `external_load_state` (`unresolved`, `loading`, `resolved`, or `failed`);
- an optional diagnostic string;
- an optional resolved declaration identity only in the `resolved` state.

Task 2.2 authors unresolved declarations and supports explicit state updates for
future loaders. It does not perform I/O. `unresolved` and `failed` declarations
have no body. `loading` has no invented body. `resolved` requires a distinct,
existing local declaration with a compatible interface; the external
declaration remains its own identity and retains its original URLs.

Resolution updates are transactional. Invalid transitions or incompatible
interfaces publish nothing. Removing the resolved local target is rejected
until the EXTERNPROTO is redirected, unresolved, failed, or removed.

## Authoring and inspection API

The intended surface is:

```cpp
result<declaration> scene_edit::add_local_declaration(
    local_declaration_descriptor descriptor);
result<declaration> scene_edit::add_external_declaration(
    external_declaration_descriptor descriptor);
result<void> scene_edit::rename_declaration(const declaration &, std::string);
result<void> scene_edit::update_declaration(
    const declaration &, declaration_update);
result<void> scene_edit::remove_declaration(const declaration &);

const std::vector<declaration_id> &scene_snapshot::declarations() const noexcept;
result<declaration> scene_snapshot::declaration_at(declaration_id) const;
result<declaration> scene_snapshot::declaration_named(std::string_view) const;
result<declaration_descriptor>
scene_snapshot::describe(const declaration &) const;
```

Add operations return handles immediately within the edit for composition.
Snapshot enumeration is authored order. Mutation tokens are public handles, not
names or vector indexes. Changes carry declaration ID, name, operation kind, and
ordered index without overloading node identity.

## Changes and errors

Declaration changes use dedicated `declaration_change` records inside the
existing `change_set`, rather than encoding declaration IDs into `node_id` or
stringly typed node changes. This keeps node and declaration identity domains
unambiguous. The change set preserves one total operation order across node and
declaration changes through an explicit ordered variant.

Ordinary failures remain `result<T>` with structured `sai_error`. Declaration
errors include optional declaration identity and interface field name. Duplicate
names, stale handles, invalid descriptors, scope conflicts, invalid load-state
transitions, and retained resolution targets are distinct error codes where a
caller can act differently.

## Rejected alternatives

### Name-based identity

Names are scoped mutable aliases. Using them as handles makes rename ambiguous
and lets remove/re-add silently retarget old references.

### Separate polymorphic PROTO and EXTERNPROTO APIs

Most identity, interface, namespace, ordering, inspection, and mutation rules
are shared. A class hierarchy duplicates services and makes generic declaration
tooling depend on allocation and downcasts.

### Ordinary scene nodes as unscoped body roots

Allowing template nodes to remain ordinary scene nodes permits one object to be
both template and live graph state. Expansion, serialization, route scope, and
removal then acquire irreconcilable meanings.

### Eager EXTERNPROTO resolution

Resolution is an external capability, not a descriptor constructor side effect.
Inventing a body or synchronously resolving URLs would erase authored unresolved
facts and couple the semantic kernel to transport.

## Verification obligations

Tests must prove:

- declaration ID/name separation, monotonicity, rename, remove/re-add staleness;
- one ordered namespace shared by PROTO and EXTERNPROTO only;
- complete interface validation and authored field ordering;
- ordered enumerate/find/add/update/remove duals;
- local template closure ownership and scene/declaration scope isolation;
- rejection of cross-declaration, scene-root, route, export, and field leakage;
- old-snapshot preservation after rename, update, and removal;
- unresolved/failed EXTERNPROTO preservation without a body;
- resolved-target compatibility and retention;
- atomic non-mutation for every rejected descriptor, scope, and transition;
- dedicated declaration change identity and total operation order;
- no runtime, parser, transport, or generated-node implementation dependency.

Prototype instances and IS bindings remain explicitly outside this design and
begin only after the declaration model passes an independent stop/go review.
