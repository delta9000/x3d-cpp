---
title: "ADR-0003: Throw on Range Violation"
summary: Typed field setters throw std::out_of_range; the reflection set thunk is non-validating (lenient) for read-path use.
tags: [adr, range-validation, throw, reflection, lenient-read]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/reflection.md
---

# ADR-0003: Throw on Range Violation

## Status

Accepted — 2026-06-04 (lenient-read split); warning-collection debt closed 2026-06-11.

## Context

The X3D UOM defines hard numeric bounds for several field types: `SFColor` and `SFColorRGBA` components are constrained to `[0, 1]`; other fields carry `minInclusive`/`maxInclusive` annotations (e.g. `beamWidth ≤ π/2`, `skyAngle ≤ π`, `visibilityRange ≥ 0`).

Two pressures pulled in opposite directions:

**The programmatic API must be strict.** When application code calls `setSpecularColor({1.5f, 0.f, 0.f})`, the SDK has been given bad data by the caller. Silently clamping would hide bugs; returning an error code would be un-C++-idiomatic and easy to ignore. Throwing `std::out_of_range` is the standard C++ contract for a value that falls outside a valid domain, and lets callers find mistakes at the earliest possible point.

**The read path must be lenient.** Real corpus files — including files in the official X3D example archive — contain out-of-range values (`specularColor.r > 1`, `beamWidth > π/2`, `skyAngle > π`). Throwing on parse would abort the entire document for a marginal authored value. An ingest SDK that crashes on real-world files is not fit for purpose. The data layer needs to keep what the author wrote.

These two requirements cannot be satisfied by a single code path. A uniform throw-on-set policy fails the corpus; a uniform silent-keep policy removes the programmatic safety net.

Additionally, even a lenient read path should not silently discard the knowledge that a value is out of range. Applications and conformance tooling both need a record of what was kept.

## Decision

The generated bindings maintain a strict/lenient split at the field-write seam:

1. **Typed public setters throw.** `set<Name>(value)` calls a per-field `validate<Name>(value)` helper that throws `std::out_of_range` on any bound violation. Both the copy and move overloads go through this helper. This is the enforcement point for all programmatic callers.

2. **Each constrained field also generates `set<Name>Unchecked(value)`.** This variant assigns the member directly, with no range check. It is not part of the public API contract; it exists solely as the target for the reflection layer and for the event cascade.

3. **The reflection `set` thunk (in `fields()`) routes all writes through `set<Name>Unchecked`.** Readers, codec writers, and the event cascade all write fields via the reflection thunk. They therefore keep out-of-range authored values rather than aborting. This is the data-layer-permissive / typed-API-strict split, by design.

4. **Range violations are surfaced as structured diagnostics after parse.** Each constrained node generates a `virtual void validateRanges(std::vector<RangeDiagnostic>& out) const` override. The free function `collectRangeWarnings(root)` (`runtime/X3DRangeValidate.hpp`) walks the scene graph via reflection and aggregates these per-node checks. The `X3DParse` front door (`runtime/parse/X3DParse.hpp`) calls `collectRangeWarnings` after the scene is built and exposes the result as `X3DDocument::rangeWarnings`. This is a separate structured channel — not silently merged into the per-reader `warnings()` string vector — so the conformance validator can consume it directly.

The `RangeDiagnostic` struct is defined in `generated_cpp_bindings/x3d/core/X3DReflection.hpp`:

```cpp
struct RangeDiagnostic {
    std::string nodeType;   // e.g. "Material"
    std::string defName;    // DEF name if known, else ""
    std::string fieldName;  // e.g. "specularColor"
    std::string detail;     // e.g. "specularColor.r above maximum of 1"
    std::string message() const;
};
```

## Consequences

**Positive:**

- Programmatic callers get an immediate, unambiguous signal when they write a bad value. No silent data corruption, no magic clamping.
- The read path never aborts on a marginal authored value in a corpus file. The SDK is usable against the full official X3D archive (17,719 files; 0 crashes).
- The constraint is expressed once per field. `validate<Name>` is the single source of truth that both the throwing setter and `checkRanges<Name>` (the non-throwing diagnostic variant) derive from, eliminating duplicated bound expressions.
- `RangeDiagnostic` is structured, not a bare string. The M3 conformance validator (see [Conformance Infrastructure](../subsystems/conformance-infra.md)) can consume it directly without reparsing warning text.
- `collectRangeWarnings` is reusable on any subtree — including a programmatically-built scene before serialization — not only on freshly-parsed documents.
- The `rangeWarnings` channel is strictly additive: existing `set*/validate*` behavior is byte-identical; only the new method and struct are added to the generated headers.

**Trade-offs / costs:**

- The `set<Name>Unchecked` family is a generated footgun: calling it bypasses the safety check. It is deliberately not exposed in the public SDK façade (`include/x3d/sdk.hpp`), but it is visible in the generated headers that library-internal code includes. Convention alone, not access control, keeps it off the public API surface.
- Out-of-range values are kept verbatim, never clamped or normalized. This is the correct policy for a data-layer ingest SDK, but a renderer consumer must be prepared to receive field values outside the nominal spec range and decide how to handle them.
- `Vrml97Reader`'s `strict_` mode promotes reader `warnings()` to a throw at end-of-parse. Range diagnostics are kept on the separate structured channel and are NOT merged into `warnings()` in strict mode, so strict-mode fixtures that legitimately carry out-of-range values do not suddenly fail. This is the right call but requires maintainers to remember that the two channels are distinct.
- The `validateRanges`/`checkRanges<Name>` split adds a second generated method per constrained field alongside the existing `set<Name>`/`set<Name>Unchecked` pair, increasing generated-header size for constrained nodes.

## Related

- [Architecture](../architecture.md) — the data-layer / typed-API seam sits between the codec/parse layer and the node bindings.
- [Reflection subsystem](../subsystems/reflection.md) — the `fields()` reflection thunk that routes all reads/writes and targets `set<Name>Unchecked`.
- Design history: `docs/superpowers/specs/2026-06-07-lenient-read-range-warnings-design.md` — full component design, DRY-constraint rationale, and TDD test plan for the warning-collection half of this decision.
- Implementation: `generated_cpp_bindings/x3d/core/X3DReflection.hpp` (`RangeDiagnostic`), `runtime/X3DRangeValidate.hpp` (`collectRangeWarnings`), `runtime/parse/X3DParse.hpp` (parse-front-door integration), `runtime/X3DDocument.hpp` (`rangeWarnings` field).
