# External-session multi-field services

The reference `external_session` supplies size, indexed reads, indexed writes,
append, insertion, removal, and clear for all 21 MF kinds. These are modern C++
proposal names and owning result types, not the published Part 4 class ABI.

Sources:

- [ISO/IEC 19777-4:2025, 7.7.5 getValue](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getValue)
  defines logical-element counts and indexed MF reads
- [ISO/IEC 19777-4:2025, 7.7.6 setValue](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#setValue)
  defines append, insertion, clear, and removal
- [ISO/IEC 19775-2:2015, 6.7 field services](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#FieldServices)
  supplies committed-read and buffered-set semantics

## Read services

`get_size(field)` returns the number of logical elements: one MFVec3f element
counts as one, not three. `get_one_value(field, index)` returns an owning scalar
value. Both consult committed scene state even when the client has accepted or
submitted changes. They enforce field readability, live authority, and session
state. Indexed reads require `index < size`.

For MFNode, `get_one_value(field<node_list>, index)` returns `result<node>`.
The dynamic equivalent is `get_one_node(dynamic_field, index)`. These preserve
context-bearing node authority and NULL values. Dynamic `get_one_value` rejects
MFNode with `type_mismatch` and directs callers to `get_one_node`; it never
presents a numeric node ID as an authorizing handle.

## Mutation services

- `set_one_value` replaces one element without changing length
- `append` adds one element at the end
- `insert_value` shifts existing elements up and permits `index == size`,
  including insertion at index zero into an empty field
- `remove` shifts subsequent elements down and requires `index < size`
- `clear` sets the field to an empty sequence

Typed overloads accept the exact corresponding scalar kind. Dynamic overloads
validate it before accepting a request; for example a double is not SFTime,
and an MFFloat sequence is not an SFFloat element. Image representation, numeric
range, field access, node type, declaration scope, and uniqueness constraints
are validated through the same kernel admission path as whole-field writes.
MFNode mutations require owner-bearing `node` values, with `node{}` representing
NULL. Numeric IDs, including a numeric zero, cannot grant write authority.
Repeated NULL elements remain allowed; non-NULL duplicates are rejected only
when the field's actual descriptor requires uniqueness.

Mutation works on a local copy of the latest value pending in this client, or
committed state if none is pending. Failed attempts neither alter that copy in
the queue nor disturb earlier tickets or callbacks. Processing revalidates the
whole candidate against then-current kernel state, so an accepted node may
still fail if its declaration scope changes before processing.

For inputOnly MF fields, committed state cannot be read. A pending whole-field
write is therefore required before read-modify-write operations. `clear` needs
no readable base and can establish that pending value. This is an explicit
reference-provider policy; these helpers do not expose inputOnly values.

Within a non-nesting update gate, all successful mutations of a field become
one whole-field event, and every accepted call retains its own completion
receipt. Whole-field writes replace the aggregate; later structural/indexed
changes operate on that replacement. Ungated calls each produce a separate
submission and whole-field event, including clearing an already empty field.
The host's release ordering, processing-time clock, and explicit completion
drain are unchanged. Completion capture destruction happens outside locks,
including failed requests whose captures move or destroy the session owner.

## Evidence and boundary

`tests/session_test.cpp` covers all 21 MF kinds, committed reads, exact scalar
kind/format checks, pending-value composition, zero/end/overflow indexes,
inputOnly bases, MFNode NULL/order/authority/uniqueness/declaration scope,
revalidation at processing, host ordering, runtime reentrancy, and callback
ownership. It is part of the existing `sai_external_session` test target.

This closes the MF convenience-service gap in this reference external-session
layer. It does not establish full binding conformance, browser loading,
internal Script execution, or live support in an independent provider.
