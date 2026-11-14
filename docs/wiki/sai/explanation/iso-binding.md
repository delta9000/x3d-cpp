---
title: Relationship to the ISO C++ binding
summary: How the SAI SDK relates to ISO/IEC 19777-4 and 19775-2, and where the SDK records its own reading or policy.
tags: [sai, explanation, standards]
updated: 2026-11-14
related:
  - ../reference/service-register.md
---

# Relationship to the ISO C++ binding

## Two ways to map the same services

[ISO/IEC 19777-4](https://www.iso.org/standard/83753.html) is the published C++
language binding for the SAI. Its interfaces keep the shape shared with the
other SAI language bindings, notably Java (ISO/IEC 19777-2): accessor pairs
such as `getUserData`/`setUserData`, an `X3DField` class hierarchy, and a
`CObject` user-data type. That parallel structure is a deliberate choice. It
lets one abstract service description map the same way into every language.

This SDK explores a different trade-off. It maps the same abstract services
(ISO/IEC 19775-2) onto idiomatic C++20:

- owning values instead of borrowed arrays
- `result<T>` errors instead of exceptions for semantic failures
- distinct strong types where C++ primitives would collapse X3D semantics
- handles whose authority the provider checks

It is simply another way of doing it, shared to help the X3D ecosystem. It
doesn't replace the standard and isn't source-compatible with 19777-4.

## Comparing the two

The [service register](../reference/service-register.md) cites the ISO clause
behind each service, so anyone can compare the two mappings service by service.

## Where the SDK records a choice

Some reference pages note that the SDK reads the published text a particular
way, or that the text leaves a policy open. Those notes record the choice this
SDK made. They are not defect claims.

| Topic | SDK choice | Page |
|---|---|---|
| Matrix storage order | Row-major owning values with explicit column-major import and export | [Matrix algebra](../reference/matrix.md) |
| Matrix operations | Return new values; take a value argument in `set`; return the matching matrix or vector type | [Matrix algebra](../reference/matrix.md) |
| Mixed whole and indexed MF writes in one gate | Indexed edits alter the most recent pending occurrence | [Submissions](../reference/submissions.md) |
| Browser properties | One owning map | [Browser services](../reference/browser-services.md) |
| Browser description | An owning `description()` readback | [Browser services](../reference/browser-services.md) |
| User data | Type-erased owning value, immediate updates, conservative retention | [User data](../reference/user-data.md) |
| Rotation axes | Eight-epsilon unit-length window; reject, never normalize | [Field values](../reference/field-values.md) |
| Scene metadata order | Insertion order | [Scene units and metadata](../reference/scene-information.md) |
| Error vocabulary | Structured `sai_error` codes throughout | [Provider contract](../reference/provider-contract.md) |

## Which editions the register targets

- The abstract-service catalog targets ISO/IEC 19775-2:2015.
- The separate C++ binding obligation IDs were transcribed from Web3D's
  historical v3.3 committee draft. ISO/IEC 19777-4:2025 is now the published
  binding. The catalog records both references without treating the old clause
  IDs as an audited mapping to the published edition, so
  `cpp_binding.audit_status` stays `incomplete`.
- The official 4.0 draft service reference is used as supporting evidence in a
  few places, for example effective unit defaults and single-value metadata
  replacement. It is not substituted for the selected baseline.

## What the evidence does and does not establish

Register consistency checks, responsibility links and service counts do not
establish ISO conformance. "Partial" means working, tested behaviour whose
surface or semantics are not yet the whole ISO service. See
[Scope and limits](scope-and-limits.md).
