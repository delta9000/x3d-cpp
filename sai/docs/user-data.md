# Node and field user data

## Published source and SDK boundary

The designated published C++ binding's
[§4.3.3.1, Data storage](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/concepts.html#DataStorage)
requires user-data associations with both node and field identity. Different
references must observe the same association, including after wrapper disposal
and later reacquisition. It permits release once neither applications nor the
scene graph retain the node/field. It does not require immediate reclamation at
that point. The explicit field operations appear in
[Annex A.4.5, X3DField](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#X3DField)
as `setUserData(CObject)` and `getUserData()`.

This implementation addresses `SVC-NODE-USER-DATA-READ/WRITE` and
`SVC-FIELD-USER-DATA-READ/WRITE` in the experimental kernel. It does not claim that
the node methods below are spellings published in Annex A. The owning C++ types,
error vocabulary, immediate-update policy and conservative reclamation policy
below are explicit SDK choices, not normative C++ signatures or a claim of
complete binding/provider conformance. The designated source does not settle
these modern ownership, concurrency or buffering details.

## Public API and ownership

`node` and `dynamic_field` expose:

```cpp
result<user_data_value> user_data() const;
result<void> set_user_data(user_data_value data) const;
```

`typed_node<Tag>` and `field<T>` forward the same operations. Dedicated
`imported_node`/`dynamic_imported_field` views remain outside this projection.
The provider contract carries equivalent owning node/field associations through
its separate authority-bearing handles; see the
[provider contract](portable-contract.md). Other SDK reference families do not
carry them yet. Associations are independent of field access type: input-only, output-only, initialize-only and
inputOutput fields can all carry application data.

```cpp
using namespace x3d::sai::experimental;
auto application_data = user_data_value::make<std::string>("selected item");
auto stored = node_handle.set_user_data(std::move(application_data));
if (!stored) { /* inspect stored.error() */ }

auto data = node_handle.user_data();
if (data) {
  auto text = data->as<std::string>();
  if (text && *text) {
    // *text is shared_ptr<const string>; **text is the stored string.
  }
}
auto cleared = node_handle.set_user_data({});
```

- `user_data_value::make<T>(args...)` constructs and owns a `T`, including a
  move-only type, using shared ownership and type erasure
- Empty `user_data_value` clears the association. A successful empty read is
  distinct from a failed service call. `as<T>()` on an empty value succeeds with
  an empty shared pointer; a nonempty value with a different exact type returns
  `type_mismatch`
- Copying a data value or calling `as<T>()` retains ownership independently of
  subsequent replacement, clearing, wrapper disposal and context lifetime
- The API exposes a `shared_ptr<const T>`, not a borrowed `void*`. It does not
  invoke payload copy constructors or expose unchecked downcasts
- Const access is shallow. A payload may contain pointers or other externally
  shared state; synchronization and ownership of those resources remain the
  application's responsibility

## Identity, lifetime and service policy

A sidecar on the execution context associates data with its node identity or
with the pair of node identity and canonical field name. It is not part of the
immutable scene state. Separate contexts cannot collide even when their local
node IDs and generation numbers match. The `set_` and `_changed` aliases for an
inputOutput field resolve to that field's canonical association; an unknown
field, including an empty field name, cannot access the node's association.
An unchecked typed projection's cached kind/access classification does not
change metadata identity: like `get_definition()`, these services resolve the
actual field descriptor by canonical name. Value access still applies its own
type/access checks.

Disposing one node/field wrapper leaves the association intact for copies and
reacquired wrappers. A field acquired before its node wrapper was disposed
remains independently usable. These live operations reject disposed wrappers,
expired/retired contexts, and nodes absent from the currently published state.
They do not allow unpublished authoring handles to create associations. Reads
return `unknown_node` for removed nodes even if historical snapshots retain
ordinary scene values. Invalid field projections return `unknown_field`.

Existing node/field wrappers carry weak context authority and do not prolong the
context's lifetime. A retained payload does not itself prolong the context's
lifetime unless its application-defined contents hold such an owning reference.

User-data changes are immediate, mutex-serialized metadata operations. They do
not join an authoring edit, external-session update group or event batch, do not
realize a node, and never modify field values, revision numbers, routes,
notifications or event queues. A field obtained through an old snapshot still
reads the current association through this live service; snapshot history does
not freeze or roll back user data. An open scene edit remains valid across
user-data changes and its later commit cannot overwrite them.

This implementation conservatively retains associations until they are
explicitly cleared/replaced or the owning context is destroyed. Node removal
and context retirement do not eagerly release them, including when retained
snapshots or pending edits keep the context alive. Node IDs are not reused, so
retention cannot leak an old association into a new node identity. This policy
can retain data longer than the minimum permitted by the binding and is not
automatic garbage collection.

Applications must clear associations containing owning references to their
context, browser, provider or any object that indirectly owns the context
before those references become unreachable, and before node removal/context
retirement makes the live clear operation unavailable. Otherwise the resulting
shared ownership cycle can keep the context and its data alive. There is no cycle
collector; weak references are preferable when ownership is unnecessary.

## Locking and exception guarantees

Reads retain shared ownership while holding the context mutex; writers validate
against the current state under that mutex and publish with nonthrowing swaps.
Different wrappers may safely read/write concurrently. As with the existing
handles, mutating/discarding the *same wrapper object* concurrently with a call
on it requires application synchronization.

Payload construction occurs before service entry. Replaced, cleared and
rejected payloads are released only after internal locks are gone. Destructors
may reenter ordinary public services; a replacement is already visible when the
old payload is released. A destructor's reentrant write may therefore supersede
the outer write. The API has no explicit destructor callback hook or teardown
event. Final context destruction releases associations as normal C++ owned
members, with no kernel mutex held.

Factory types must be non-array, unqualified object types with nonthrowing
destructors. Throwing from a `noexcept` destructor still terminates the process
under ordinary C++ rules; this API does not intercept that violation. Destructors
that call potentially throwing services must handle exceptions themselves.
Payload construction/allocation failure propagates as an ordinary C++ exception.
Map/key allocation precedes taking ownership, so a failed insertion leaves all
existing associations, scene state and events unchanged. A diagnostic allocation
may likewise throw instead of producing a structured service error.

## Executable evidence

`tests/user_data_services_test.cpp` covers owning/move-only payloads, exact-type
errors, node and canonical field identity, wrapper disposal/reacquisition,
independent field lifetime, field access modes, typed forwarding, foreign-context
isolation, absent/disposed/removed/retired/expired handles, conservative history
retention, replacement/clear/failure destructor reentry, allocation-failure
atomicity, scene/event/revision isolation and concurrent readers/writers with
independently retained payloads and snapshots.
