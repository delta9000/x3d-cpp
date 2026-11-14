---
title: SAI field values and validity
summary: The value vocabulary of the SAI SDK and the validity rules applied when a field value is accepted.
tags: [sai, reference, values]
updated: 2026-11-14
related:
  - provider-contract.md
---

# SAI field values and validity

These rules apply wherever the SDK accepts a field value: setup authoring,
retained writes, event input, runtime output, MF edits, defaults and ROUTE
destinations. A rejected value returns `invalid_value` (or `type_mismatch` for
a wrong kind) and publishes nothing. Error codes are listed in
[Provider contract](provider-contract.md#errors).

## Value kinds

- `value_kind` has one public spelling: the lowercase form of the exact X3D
  field type, for example `sf_vec3f`, `mf_node`, `sf_color`, `sf_color_rgba`.
- C++ owning types such as `vec3f`, `color3f` and `color4f` describe storage.
  They do not add alternate schema-tag spellings.
- All scalar and multi-field kinds have exact tags, owning storage and distinct
  strong types where C++ primitives would collapse X3D semantics. A `double`
  is not an SFTime.
- Bounded string vocabularies keep their normative SFString or MFString kind
  and use `std::string` or `string_list`. There are no separate enum value
  kinds.

## Equality

| Relation | Behaviour |
|---|---|
| `same_representation` | Storage and event equivalence: IEEE payload bits survive, signed zeros differ, identical NaN payloads match |
| `operator==` | Ordinary numerical comparison |

## Node values

| Value | Meaning |
|---|---|
| Provider `nullopt`, kernel `node{}` | X3D NULL |
| Engaged handle | Must carry valid context authority |
| Bare numeric ID | Inspection data only; never accepted for a write |

MFNode values preserve every NULL position. Non-NULL values keep their type,
context and scope checks.

## Rotations

Applies to SFRotation and MFRotation, including every list element, indexed
edits and defaults.

| Rule | Detail |
|---|---|
| Axis components | Finite and within [-1, 1] |
| Axis length | Squared length within eight `float` epsilons of one, evaluated in `double` |
| Zero axis | Invalid, even when the angle is zero |
| Malformed value | Rejected; never stored and never silently normalized |
| Accepted value | Keeps its exact stored representation, including signed zero |
| Angle | No bound, no finite requirement and no normalization into a principal range |

NaN and infinite angles stay storable where no field-specific range prohibits
them. That is the current reference policy, not a claim that every encoding or
renderer accepts them. Field-specific numeric bounds still apply to all
rotation components.

Sources: a normalized axis is required by
[ISO/IEC 19775-1, 5.3.13](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#SFRotationAndMFRotation),
and the published C++ binding's
[Annex A.4.21](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#SFRotation)
bounds axis components to [-1, 1]. Neither specifies a floating-point tolerance
or requires rejection over normalization. The eight-epsilon window and the
rejection policy are SDK choices.

Implementation: `sai/include/x3d/sai/experimental/value_constraints.hpp`.
Evidence: `sai/tests/rotation_constraints_test.cpp` (CTest
`sai_rotation_constraints`), plus provider-dispatch coverage in the provider
tests.

## Colours

SFColor, SFColorRGBA, MFColor and MFColorRGBA channels must lie in [0, 1], as
specified by the X3D
[colour field types](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/fieldsDef.html).
Out-of-range, NaN and infinite channels are rejected. Valid endpoints and
signed zero keep their representation.

## Numeric ranges

Each field descriptor owns a `numeric_range`: optional minimum and maximum with
inclusive or exclusive flags. Generated UOM bounds survive into catalog and
node discovery and are part of the schema fingerprint.

| Rule | Detail |
|---|---|
| Where checked | Every numeric component, before the value is staged |
| Units | Bounds use canonical units and are checked after authored-unit conversion |
| Precision | Floating endpoints use the component precision; integer bounds retain fractional limits |
| Bounded field | Rejects non-finite components |
| Unbounded field | Keeps its IEEE representation behaviour |
| Descriptor admission | Rejects non-finite limits and intervals with no representable value |

inputOnly and outputOnly ports may *initially* hold their exact field-type
default even when it lies outside the range; declared defaults and later
events must satisfy it. For example, `aspectRatio` starts at zero but can emit
only positive values.

Numeric ranges do not implement coupled rules, such as the all-axes `bboxSize`
sentinel, or string vocabularies.

## Strings

String values are retained exactly as supplied. Vocabulary validation is not
complete:

- The UOM choice lists omit some normative values and mix individual flags
  with whole sequences. A single `"CENTER"` is a valid `Layout.align` value,
  and font styles permit an empty string. The SDK preserves these.
- The
  [published physics vocabulary](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollisionCollection)
  uses underscore spellings such as `SLIP_1`, while the pinned UOM uses hyphens
  for some flags.

## Images

MF editors report malformed images as `invalid_value`. A wrong field kind
remains `type_mismatch`.

## Grouping children

`children` on `X3DGroupingNode` and its derived types rejects a repeated
non-NULL node in the same list. See
[Generated field projection](generated-fields.md#grouping-child-uniqueness).

## Units in the reference kernel

In the kernel API, a finite authored value whose converted result exceeds the
field's finite range is rejected before publication, and authored reads report
the same error when inverse conversion exceeds it. Structural checks
(ownership, access, template restrictions, payload type) run before unit
conversion, so error codes agree between canonical and authored
representations.

Custom field and declaration-interface descriptors accept the derived
categories acceleration, angular_velocity, area, speed and volume; their
conversion uses the declared angle or length base factor. UNIT declarations
stay limited to angle, force, length and mass.
