---
title: "ADR-0056: Prototype Instance Templates in Node Defaults"
summary: Prototype-instance defaults and node-valued overrides use authored X3DNode wrappers, preserving existing node vectors and DEF/USE identity through materialization.
tags: [adr, proto, parsing]
updated: 2026-09-29
related:
  - ../subsystems/proto-expand.md
  - ../subsystems/parse-readers.md
---

# ADR-0056: Prototype Instance Templates in Node Defaults

## Context

`ProtoField::nodeDefault` holds node pointers. Direct ProtoInstance defaults were
lost because prototype records could not occupy those slots. A separate instance
list would also need an ordering ledger and a second mechanism for DEF/USE
references across interface fields and into the body.

## Decision

Represent instances in node defaults and node-valued `fieldValue` graphs with
`ProtoInstanceTemplate`, an `X3DNode` carrying the authored `ProtoInstance`. Keep
the existing node vectors and DEF table as the authorities for order and identity.

Writers recognize the template before ordinary node reflection and emit
ProtoInstance syntax, using the declaration writer's shared DEF/USE scope.
Expansion materializes required templates into the outer instance's clone map
before cloning graphs and forwarding IS values. An explicit field override
suppresses an otherwise unused default; a body USE still requires its target.
Caller-supplied ordinary nodes retain their original identity. An ephemeral
expansion context memoizes template materialization across caller values and
scene-root USE aliases. Its owning keys prevent identity reuse during the
transaction. Declaration-owned values still use a fresh clone map per outer
instance.

Scene structural instances also occupy their authored root or ordinary child
field positions as templates, including unnamed instances. A weak
`ProtoInstance::placementTemplate` link associates each structural record with
that exact slot identity. Structural records remain authoritative for source
values before expansion; the node vectors own placement and order. Expansion
replaces slots in place, and aliases reuse the resulting primary. Writers use
the structural source for a linked template and do not emit it again from the
structural list. Removed slots are not resurrected: even an expired weak
identity differs from a never-linked programmatic record. Programmatic records
without placement identity retain the existing append/attach behavior.
No process-global or persistent scene cache is introduced.

## Consequences

The template is an authored representation, not a complete native SAI prototype
node. Successful materialization exposes the expanded primary. An unresolved
caller template remains inert in its slot with a diagnostic so writers can
preserve it; a fresh expansion transaction can retry resolution. Generic factory cloning
cannot materialize a template: prototype expansion owns that operation and its
resolver, depth guard, and diagnostics. The process-global clone fallback remains
reserved for extension nodes.
