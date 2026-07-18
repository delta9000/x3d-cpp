---
title: "ADR-0033: A PROTO May Not Shadow a Built-in Node Name (Security)"
summary: "A ProtoDeclare/ExternProtoDeclare whose name collides with a built-in node type is rejected (quarantined with a diagnostic); the built-in retains precedence. Lenient by default, fatal in strict mode."
tags: [adr, prototypes, security, parsing, ext-firewall, conformance]
updated: 2026-06-25
related:
  - ./0001-ext-firewall.md
  - ../architecture.md
  - ../../conformance/findings.yaml
---

# ADR-0033: A PROTO May Not Shadow a Built-in Node Name (Security)

## Status

Proposed — 2026-06-25. Conformance finding `PROTO-SHADOW`. Sibling to ADR-0001 (ext-firewall).

## Context

§4.4.4.1 (`concepts.md:846`): "Node type names shall be unique in each X3D file. **The results are
undefined** if a prototype is given the same name as a built-in node type or a previously defined
prototype in the same scope." That is too weak for a name-resolution rule a security boundary depends
on: a `PROTO Transform […]` with malicious content silently rebinds every later `Transform { }`. The
same clause already writes a normative security rule for self-reference recursion (`:956`: "it is an
error … X3D browsers shall not honor …"). Brutzman/Mantis 1492 propose the same "it is an error"
treatment for built-in shadowing.

x3d-cpp shadowing is possible **today**: registration has no collision check
(`runtime/parse/ClassicVrmlReader.hpp:797`, `JsonReader.hpp:419` just `push_back`), and resolution
checks the proto table **before** the factory (`ClassicVrmlReader.hpp:295-298`, `JsonReader.hpp:622`),
so a same-named PROTO wins. This routes around the ADR-0001 ext-firewall, which assumes built-in node
names are inviolable trust anchors. The detection seam already exists:
`X3DNodeFactory::registry()` (`generated_cpp_bindings/x3d/nodes/X3DNodeFactory.hpp:23`).

## Decision

At PROTO/EXTERNPROTO **registration**, if the declaration's name is in `X3DNodeFactory::registry()`
(a built-in), **reject it**: do not insert the declaration; the built-in keeps the name and later
instances resolve through the factory as if the rogue PROTO were absent. Record a structured
`ProtoWarning{Kind::BuiltinShadow}` (never silent). Apply the same rule to a PROTO redefining a prior
PROTO in scope (first declaration wins).

- **Default (lenient, per the codebase's lenient-read policy):** quarantine + diagnostic; the scene
  still loads.
- **Strict / conformance mode:** treat any `BuiltinShadow` warning as fatal (unload), for
  security-sensitive embedders.

Legitimate ext-firewall PROTOs use `urn:x3d-cpp-gen:ext:*` names that never collide with the built-in
registry, so they pass untouched — the check only rejects masquerade-as-a-core-node.

## Consequences

**Positive:**
- Closes a silent built-in-subversion vector; built-in semantics become inviolable (composes with
  ADR-0001's firewall).
- Deterministic resolution of the spec's "undefined" (built-in precedence), matching X_ITE and
  FreeWRL — two independent implementations already converged on the same policy.
- Conforming implementations currently diverge on this exact case, not just on how to read
  "results undefined": at least one currently-shipping implementation, X3DOM, lets a same-named
  PROTO take precedence over the built-in — the opposite of the policy adopted here (and by X_ITE
  and FreeWRL). That divergence grounds the built-in-precedence question in an observed
  interoperability gap rather than a purely hypothetical reading of the spec text.
- Auditable via `ProtoWarning`, like recursion / unresolved-extern warnings.

**Trade-offs:**
- A collision check runs at every PROTO registration (negligible: one hash lookup).
- Content that intentionally overrode a built-in via PROTO will lose that override (by design).

## X3D 4.1 alignment

**Aligned.** The Mantis 1492 4.1 direction is built-in-takes-precedence (Seelig/X_ITE) flagged as
"an error / security consideration" (Brutzman) — exactly this policy. The upstream erratum (a
normative NOTE + disallowed EXAMPLE in §4.4.4.1) is in flight.
