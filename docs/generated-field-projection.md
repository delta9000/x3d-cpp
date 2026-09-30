# Generated runtime-field projection

The vendored, SHA-pinned UOM describes both X3D semantics and encoding/host
integration. An XML `<field>` entry alone does not make something an ordinary
SAI value or event port. The parser keeps the source entries; the shared
`resolved_node_fields` projection selects the runtime surface emitted into both
the semantic metadata catalog and owner-specific typed keys.

## Current standalone-kernel policy

- **DEF and USE:** excluded from generated runtime fields. DEF labels belong to
  the execution context's named-node service (`define_name`, `undefine_name`,
  `named`); reusing a node means referring to the same context-bearing handle in
  another occurrence. A generic string write must not create a second, unrelated
  identity store. This does not implement an encoding reader or writer.
- **IS:** excluded from generated runtime fields. IS/connect describes prototype
  interface binding, not an SFNode value that can be independently edited or
  routed. The kernel's declaration records do not implement prototype
  instantiation or IS execution; those remain unsupported.
- **class, id and CSS style:** host/document attributes, excluded from this
  standalone SAI surface. The kernel has no HTML/DOM/CSS host integration. They
  are neither automatically eventable nor aliases for DEF. A future document
  bridge needs a separate, explicit storage and host-integration policy; this
  change does not claim document round-tripping or supply that bridge.
- **FontStyle.style and ScreenFontStyle.style:** retained. These are node-declared
  text-style fields (the `fontStyleChoices` vocabulary), not the CSS `style`
  attribute inherited from X3DNode. CSS style is classified by its declaration
  owner, not by a global spelling blacklist.

These rules apply to the generated standard-node surface. They do not ban
same-spelled fields in explicitly supplied handwritten registries or declaration
interfaces. Runtime fields retain their source order, types, defaults, access
modes and accepted node types. The emitted model fingerprint changes with the
projected surface, so old typed bindings do not match the new registry.

This intentionally removes previously generated keys including `Transform::DEF`,
`Transform::USE`, `Transform::IS`, `Transform::class_`, `Transform::id` and
`Transform::style`. Such keys were accidental API, not working implementations
of naming, prototype binding or HTML integration.

## Constraint audit: still incomplete

This projection repair does **not** establish full semantic constraint coverage.
The following are known gaps, not properties established by catalog parity:

- **Numeric bounds:** the parser records UOM `minInclusive`/`maxInclusive`, but
  neither `schema_field_descriptor` nor executable `field_descriptor` carries
  those bounds. For example, the catalog is not evidence that writes to
  Material.transparency enforce its declared range. A reviewed constraint model
  and shared write/event validation are still needed, including exclusive bounds
  and vector/MF element policies where applicable.
- **Enumerations:** bounded UOM vocabularies select SFEnum/MFEnum owning value
  kinds, but descriptors do not carry allowed-member sets and the kernel does
  not validate member membership. Retaining FontStyle.style's enum kind and
  default is not proof that arbitrary text is rejected. Open/extensible
  vocabularies need to remain distinguishable from bounded ones in that work.
- **Node references and containment:** accepted node types are copied and used
  by the kernel, but the generated adapter still marks every SFNode/MFNode field
  as containment. This is a provisional conservative policy, **not** an audit of
  X3D reference/containment semantics. Each applicable field and declaration
  scope needs review before that classification can be treated as universal.
  Removing IS prevents one known binding construct from being mistaken for a
  containment edge; it does not finish that audit.
- **Units:** the current UOM input does not formally annotate per-field unit
  categories. The catalog continues to report `unit_categories_complete=false`;
  no inferred overlay or new completeness claim is introduced here.

Source evidence is the vendored
[`X3dUnifiedObjectModel-4.0.xml`](../src/x3d_sai_gen/data/X3dUnifiedObjectModel-4.0.xml),
especially X3DNode's descriptions of DEF/USE/IS/class/id/style and the separate
FontStyle/ScreenFontStyle style declarations. The projection policy is in
[`semantic_fields.py`](../src/x3d_sai_gen/emit/semantic_fields.py); the remaining
adapter policy is in [`metadata.cpp`](../src/metadata.cpp).

## Verification boundary

Ordered generator/adapter/key parity remains useful for drift, but cannot detect
an identical semantic mistake shared by all three. Independent Python and C++
regressions now assert the exact Transform field list, reject encoding/binding
and host attributes throughout the generated catalogs, preserve both real font
style fields, exercise named-node identity and shared occurrences, and reject
legacy DEF keys on retained-edit and event paths without publishing a revision.
The raw parsed UOM still retains the excluded attributes for future document
work. These tests do not certify ISO SAI conformance or the pending constraint
items above.
