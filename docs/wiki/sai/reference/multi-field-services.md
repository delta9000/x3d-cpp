---
title: SAI multi-field services
summary: Reference for size, indexed read and write, append, insert, remove and clear on MF fields in the reference external_session.
tags: [sai, reference, kernel, fields]
updated: 2026-11-14
related:
  - submissions.md
---

# SAI multi-field services

The reference `external_session` (`<x3d/sai/experimental/session.hpp>`)
supplies element-level services for all 21 MF kinds. These are C++ SDK names
and owning result types, not the published Part 4 class ABI.

The portable provider exposes the same idea through `write_mode` on a `write`;
its gate rules are in [Submissions](submissions.md).

## Read services

| Call | Returns | Requires |
|---|---|---|
| `get_size(field)` | Number of logical elements (one MFVec3f element counts as one, not three) | Readable field |
| `get_one_value(field, index)` | Owning scalar value | `index < size` |
| `get_one_value(field<node_list>, index)` | `result<node>` | `index < size` |
| `get_one_node(dynamic_field, index)` | `result<node>` | `index < size` |

- Reads consult committed scene state, even when the client has accepted or
  submitted changes.
- They enforce field readability, live authority and session state.
- Node reads preserve context-bearing authority and NULL values.
- Dynamic `get_one_value` rejects MFNode with `type_mismatch` and directs
  callers to `get_one_node`. A numeric node ID is never presented as an
  authorizing handle.

## Mutation services

| Call | Effect | Index rule |
|---|---|---|
| `set_one_value` | Replaces one element; length unchanged | `index < size` |
| `append` | Adds one element at the end | |
| `insert_value` | Shifts existing elements up | `index <= size`, including index zero into an empty field |
| `remove` | Shifts later elements down | `index < size` |
| `clear` | Sets the field to an empty sequence | |

## Validation

- Typed overloads accept the exact corresponding scalar kind.
- Dynamic overloads validate the kind before accepting a request. A double is
  not SFTime, and an MFFloat sequence is not an SFFloat element.
- Image representation, numeric range, field access, node type, declaration
  scope and uniqueness are validated through the same kernel admission path as
  whole-field writes.
- MFNode mutations need owner-bearing `node` values; `node{}` is NULL. Numeric
  IDs, including zero, cannot grant write authority.
- Repeated NULL elements are allowed. Non-NULL duplicates are rejected only
  when the field's descriptor requires uniqueness.

## Pending values

- A mutation works on a local copy of the latest value pending in this client,
  or committed state if none is pending.
- A failed attempt does not alter that copy and does not disturb earlier
  tickets or callbacks.
- Processing revalidates the whole candidate against then-current kernel
  state. An accepted node can still fail if its declaration scope changes
  before processing.

### inputOnly fields

Committed state of an inputOnly MF field cannot be read, so a pending
whole-field write is required before read-modify-write operations. `clear`
needs no readable base and can establish that pending value. This is a
reference-provider policy; these helpers do not expose inputOnly values.

## Update gates

| Case | Events |
|---|---|
| Inside a gate | All successful mutations of a field become one whole-field event; every accepted call keeps its own completion receipt |
| Whole-field write inside a gate | Replaces the aggregate; later structural or indexed changes operate on the replacement |
| Outside a gate | Each call is its own submission and whole-field event, including clearing an already empty field |

Completion capture destruction happens outside locks, including for failed
requests whose captures move or destroy the session owner.

## Sources

- [ISO/IEC 19777-4:2025, 7.7.5 getValue](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#getValue):
  logical-element counts and indexed MF reads
- [ISO/IEC 19777-4:2025, 7.7.6 setValue](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#setValue):
  append, insertion, clear and removal
- [ISO/IEC 19775-2:2015, 6.7 field services](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#FieldServices):
  committed-read and buffered-set semantics

## Evidence and boundary

`sai/tests/session_test.cpp` (CTest `sai_external_session`) covers all 21 MF
kinds, committed reads, exact scalar kind and format checks, pending-value
composition, zero, end and overflow indexes, inputOnly bases, MFNode NULL,
order, authority, uniqueness and declaration scope, revalidation at
processing, host ordering, runtime reentrancy and callback ownership.

This covers the MF convenience services in the reference external-session
layer. It does not establish full binding conformance, browser loading,
internal Script execution or live support in an independent provider.
