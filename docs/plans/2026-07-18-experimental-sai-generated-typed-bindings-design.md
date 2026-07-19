# Experimental SAI Generated Typed Bindings Design

**Status:** Approved

**Scope:** Phase 1 usability gate for `x3d::sai::experimental`

## Problem

The dynamic inspection path currently leaks into ordinary authoring:

```cpp
auto dynamic = edit.field(node, "translation");
auto typed = dynamic.value().as<vec3f>();
edit.set(typed.value(), value);
```

This repeats facts already known to the generated X3D schema, admits spelling
errors, and introduces two lookup/type failures before the actual write. The
expected-style result API makes this chain composable but does not make it a
good default authoring interface.

Dynamic lookup remains necessary for tools that discover unknown scenes. It
must not be the price paid by generated, schema-aware authoring code.

## Decision

Generate stateless node bindings and typed field keys from the same resolved
UOM model as the runtime metadata. A successfully created `typed_node<Tag>`
can turn one of its generated keys directly into `field<T>` without string
lookup, dynamic type inspection, or another `result` layer:

```cpp
auto transform = edit.create<bindings::Transform>().value();
auto translation = transform.field(bindings::Transform::translation);
auto changed = edit.set(translation, vec3f{1, 2, 3});
```

Failure remains where runtime state actually matters:

- `create<Tag>()` is fallible because the edit, node type, profile, or
  capability can reject creation.
- Generated field acquisition is infallible because the tag, field name,
  value type, access type, and owning node type are compile-time schema facts.
- `read`, `set`, `multi`, events, and `commit` remain fallible because
  generation, revision, lifecycle, context, and payload validity are dynamic.

## Rejected Shapes

### Templated dynamic lookup

```cpp
edit.field<vec3f>(node, "translation")
```

This removes one visible cast but retains runtime spelling and type failures.
It is useful as a convenience for partially dynamic code, not as the generated
authoring contract.

### Generated method per field

```cpp
transform.translation()
```

This is pleasant for isolated calls but makes field keys harder to pass to
generic algorithms, complicates inherited-field composition, and encourages
generated node façades to accumulate behavior. Bindings should remain schema
tokens, not a second runtime node implementation.

### Generated runtime node objects

Reusing the existing `x3d::nodes::*` classes would merge SAI identity with the
current runtime object model and violate the experimental firewall. Generated
SAI bindings carry no scene state and own no runtime nodes.

## Vocabulary

`field_key<Owner, T>` is an immutable generated schema fact containing the X3D
wire name, exact `value_kind`, and `access_type`. Its constructor is not part of
the ordinary public authoring surface; generated declarations create keys.

`typed_node<Tag>` is a small owning handle around the existing semantic `node`.
It adds compile-time node identity but does not add scene identity, storage, or
lifecycle. It exposes:

- `field(key)` for an infallible typed field handle;
- `dynamic()` as an explicit escape to generic SAI algorithms;
- the same stable semantic node ID as the wrapped handle.

Each generated concrete node tag owns a complete, deduplicated field-key set,
including inherited fields. Keys are owner-specific in Phase 1 so a field from
one node type cannot silently construct a handle for another node type.
Generated tags and generated registries also carry the exact semantic-model
fingerprint. `create<Tag>()` verifies that provenance before constructing a
typed node, so a custom same-name registry cannot counterfeit a generated
schema contract. Any successful public registry mutation revokes provenance;
the generated factory stamps it only after population. Typed reads still check the owning variant defensively and
return `type_mismatch`; they never leak `std::bad_variant_access`.
Generic interface/base-node relationships remain a later generated-concepts
layer; Phase 1 does not fake them with an unsafe owner-free key.

## Generation

A dedicated SAI binding emitter writes lightweight headers under an
experimental generated namespace. It does not modify or include the current
generated runtime node classes. It reuses the canonical C++ identifier
sanitizer and maps every exact X3D SF/MF type to the existing experimental
owning value vocabulary.

The generated declarations use only the canonical `sf_*`/`mf_*`
`value_kind` spellings. X3D schema tags retain specification names such as
`sf_color` and `sf_color_rgba`; C++ representations remain `color3f` and
`color4f`.

Generation must fail closed for an unmapped field type, duplicate sanitized
identifier, or descriptor that cannot be represented exactly. Generated-key
tests compare name, kind, access, order, and model fingerprint with the
generated metadata catalog so the two generated views cannot drift.

## Dynamic Interoperation

The dynamic API remains first-class:

```cpp
snapshot.field(node, runtime_name)
```

It continues to return `result<dynamic_field>` because runtime discovery can
fail. A checked conversion from an inspected dynamic node to
`typed_node<Tag>` may be added for tools that discover then specialize; that
conversion must verify the node descriptor and remain fallible. There is no
unchecked public constructor from arbitrary `node` to `typed_node<Tag>`.

Typed and dynamic handles must remain substitutable after acquisition: they
address the same node, descriptor, generation, revision policy, stored value,
event port, and error vocabulary.

## Phase 1 Example and Evidence

`generated_author_inspect.cpp` will use a generated registry and generated
bindings only. It will create, connect, edit, commit, and inspect representative
SF/MF numeric, string, image, and node fields without handwritten descriptors,
field-name strings, or `.as<T>()` calls in the authoring lane.

The current UOM does not provide complete per-field unit categories. The
example may declare generated scene units, but it must not claim a generated
unit-bearing field conversion until an approved metadata overlay or upstream
model supplies that fact. The handwritten semantic unit fixture and
`INV-FIELD-4` codec bridge remain explicitly open.

Required evidence:

- compile-time rejection of a field key used with the wrong typed node;
- direct generated create/field/set/read for scalar and MF fields;
- parity between generated keys and generated metadata;
- dynamic and typed access yielding the same stored value and errors;
- stale generation and revision failures occurring at operations, not field
  acquisition;
- Release/Werror, sanitizer, SAI conformance/invariant, and Python generator
  gates.

## Compatibility Boundary

Everything remains under `x3d::sai::experimental`. Generated spelling and
layout are not ABI commitments. Promotion requires the composed example and
parity tests to demonstrate that generated bindings remove ceremony without
introducing semantics unavailable through dynamic inspection.
