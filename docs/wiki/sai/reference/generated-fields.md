---
title: SAI generated field projection
summary: Which UOM entries become generated SAI fields, the grouping-child uniqueness constraint, and the constraint audits still open.
tags: [sai, reference, generator]
updated: 2026-11-14
related:
  - field-values.md
---

# SAI generated field projection

The SHA-pinned UOM describes both X3D semantics and encoding or host
integration. An XML `<field>` entry alone does not make something an ordinary
SAI value or event port. The parser keeps the source entries; the shared
`resolved_node_fields` projection selects the runtime surface emitted into both
the semantic metadata catalog and the owner-specific typed keys.

## What is projected

| UOM entry | Generated as a field? | Why, and where its role lives |
|---|---|---|
| `DEF`, `USE` | No | DEF labels belong to the named-node service (`define_name`, `undefine_name`, `named`). Reuse means referring to the same context-bearing handle in another occurrence |
| `IS` | No | IS/connect describes prototype interface binding, not an SFNode value that can be edited or routed |
| `class`, `id`, CSS `style` | No | Host/document attributes. The kernel has no HTML, DOM or CSS integration. They are neither eventable nor aliases for DEF |
| `FontStyle.style`, `ScreenFontStyle.style` | Yes | Node-declared text-style fields (the `fontStyleChoices` vocabulary), not the CSS attribute inherited from X3DNode |
| Every other node field | Yes | Source order, types, defaults, access modes and accepted node types are retained |

Consequences:

- There are no generated keys such as `Transform::DEF`, `Transform::USE`,
  `Transform::IS`, `Transform::class_`, `Transform::id` or `Transform::style`.
- CSS `style` is classified by its declaration owner, not by a global spelling
  blacklist.
- The rules apply to the generated standard-node surface. They do not ban
  same-spelled fields in handwritten registries or declaration interfaces.
- The emitted model fingerprint changes with the projected surface, so typed
  bindings from a different projection do not match the registry.
- The raw parsed UOM still retains the excluded attributes for future document
  work. This projection is not an encoding reader or writer, and it does not
  implement prototype instantiation or IS execution.

## Grouping-child uniqueness

The resolved model marks `children` on `X3DGroupingNode` and its transitive
derived types with `unique_non_null_nodes=true`, implementing the same-list
restriction in
[ISO/IEC 19775-1:2023, 10.2.1](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#GroupingAndChildrenNodes).

The flag is emitted in the semantic catalog, included in its fingerprint and
exposed by schema and live descriptor discovery. It is not inferred from a
field name at runtime.

| Case | Result |
|---|---|
| Repeated non-NULL identity in one list | `invalid_value`; no revision or notification published |
| Multiple NULL positions | Legal |
| One child shared across distinct parents | Legal |
| Handwritten MFNode descriptor without the flag | Ordinary duplicate-preserving sequence, even if named `children` |
| Ancestry cycle | Still rejected by the graph-cycle check, including detached cycles |

Retained edits (MF editors and containment append included) admit the complete
staged graph at `validate()` and `commit()`. Direct events, runtime outputs and
ROUTE destinations share the same node-payload admission.

This narrow constraint does not claim that every field named `children`, every
non-grouping node with grouping-like behaviour, or `addChildren`/
`removeChildren` operation semantics has been audited.

## Open constraint audits

Catalog parity does not establish full semantic constraint coverage. The value
rules in force today are in [Field values and validity](field-values.md). Known
gaps:

| Area | State |
|---|---|
| Numeric bounds | Descriptors carry UOM inclusive and exclusive bounds, and all write paths share range admission. Coupled constraints, including sentinel tuples, still need a separate audit |
| String vocabularies | Bounded UOM strings keep SFString/MFString kinds. Allowed members and complete MFString sequences are not yet validated; the UOM's choice lists also need normative review |
| Node references and containment | Accepted node types are copied and used, but the generated adapter marks every SFNode/MFNode field as containment. This is a provisional conservative policy, not an audit of reference versus containment semantics |
| Units | The UOM does not formally annotate per-field unit categories. The catalog reports `unit_categories_complete=false`; no inferred overlay is introduced |

## Sources

| What | Where |
|---|---|
| UOM | `src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml`, especially X3DNode's descriptions of DEF/USE/IS/class/id/style and the FontStyle/ScreenFontStyle style declarations |
| Projection policy | `sai/src/x3d_sai_gen/emit/semantic_fields.py` |
| Adapter policy | `sai/src/metadata.cpp` |

See also the
[X3D font and layout fields](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/layout.html#ScreenFontStyle).

## Evidence

Ordered generator, adapter and key parity is useful for drift but cannot
detect an identical semantic mistake shared by all three. Independent Python
and C++ regressions therefore:

- assert the exact Transform field list
- reject encoding, binding and host attributes throughout the generated
  catalogs
- preserve both real font style fields
- exercise named-node identity and shared occurrences
- reject DEF keys on retained-edit and event paths without publishing a
  revision

The exhaustive generated-key catalog is checked against metadata for every
node and ordered field. These tests do not certify ISO SAI conformance or the
open items above.
