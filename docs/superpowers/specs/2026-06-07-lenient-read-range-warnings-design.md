# Lenient-Read Range-Warning Collection — Design (2026-06-07)

**Status:** approved design, pre-implementation. Pays the owed "+ warnings" half
of the lenient-read policy (see [[x3d-cpp-gen-decisions]], lenient read 2026-06-04):
out-of-range authored values are *kept* on read, but nothing currently records
*which* values were kept out of range. This adds structured collection of those.

## Scope (YAGNI)

ONLY range constraints — the ones the strict setters already enforce and the
lenient read path silently keeps:

- `SFColor` / `SFColorRGBA` components constrained to `[0,1]`.
- Numeric fields with `minInclusive` / `maxInclusive` from the UOM (e.g.
  `beamWidth <= π/2`, `skyAngle <= π`).

**Explicitly OUT of scope** (these belong to the M3 conformance validator, not
this debt): enum-membership checks, required-field presence, type validation,
node-acceptability (`acceptableNodeTypes`), structural/DAG checks. Naming is kept
range-specific precisely so it does not imply broader coverage.

## Current mechanism (what exists today)

- Each constrained field generates `set<Name>()` — calls a per-field validator
  that **throws `std::out_of_range`** — and `set<Name>Unchecked()` — assigns with
  no check.
- The reflection `set` thunk (in generated `fields()`) routes reader/cascade
  writes through `set<Name>Unchecked`, so out-of-range authored values are kept
  silently. This is the data-layer-permissive / typed-API-strict split, by design.
- Readers already carry a warning convention: `std::vector<std::string> warnings_`
  exposed via `warnings()`. `Vrml97Reader` additionally has a `strict_` mode that
  promotes any collected warning to a throw at end of parse.

The gap: the `set<Name>Unchecked` path drops the knowledge that a value was out of
range. This design recovers it as a **separate, re-checkable pass** rather than by
instrumenting every reader's set path.

## Components

### 1. Diagnostic type (`RangeDiagnostic`)

Defined in generated `X3DReflection.hpp` (the shared support header every node
already includes via `X3DNode.hpp`):

```cpp
struct RangeDiagnostic {
    std::string nodeType;   // e.g. "Material"
    std::string defName;    // DEF name if known, else ""
    std::string fieldName;  // e.g. "specularColor"
    std::string detail;     // e.g. "component 0 = 1.5 exceeds max 1.0"
    std::string message() const;  // one-line rendering for warnings() merge
};
```

Structured (not a bare string) so M3's conformance validator can consume it
directly later. `message()` renders the human string for the readers' existing
`warnings()` surface.

### 2. Per-node hook (codegen)

A virtual method on the node base:

```cpp
virtual void validateRanges(std::vector<RangeDiagnostic>& out) const;  // X3DNode: no-op
```

Only nodes that **have** constrained fields emit an override (others inherit the
base no-op), keeping golden churn limited to constrained nodes. The override
checks each constrained field and appends a `RangeDiagnostic` per violation. It
does NOT recurse — traversal is the free function's job (§3).

**DRY the constraint:** today the bound logic lives only in the throwing
validator. Refactor codegen so each constrained field emits ONE non-throwing
checker that yields "in range?" + the violated-bound detail. The strict
`set<Name>()` throws on it (behavior unchanged); `validateRanges()` collects from
it. Single source of constraint truth, no duplicated bound expression.

### 3. Graph traversal

A free function, header-only in the runtime layer:

```cpp
std::vector<RangeDiagnostic> collectRangeWarnings(const X3DNode& root);
```

Walks SFNode / MFNode children via the existing reflection child-walk (the same
traversal the codecs use) and calls `validateRanges` on each visited node,
tagging each diagnostic with the node's DEF name when resolvable. Reusable on ANY
scene — including a programmatically-built one before serialization — not just on
freshly-parsed documents.

### 4. Parse-front-door integration

`X3DParse` (`parseDocument` / `parseFile`) runs `collectRangeWarnings` after the
Scene is built and exposes the structured `std::vector<RangeDiagnostic>` on the
parse result as its own accessor (the source of truth).

**Strict-mode interaction (decided):** range diagnostics are a **separate
structured channel**, NOT dumped unconditionally into the per-reader `warnings_`
string vector that `Vrml97Reader::strict_` promotes to a throw. They are *also*
stringified into `warnings()` **only in non-strict mode**, so casual callers see
them without strict-mode CI fixtures (which legitimately carry out-of-range
values) suddenly failing. Rationale: keep strict-mode semantics unchanged and keep
structured data structured.

### 5. Testing (TDD, red→green)

- **Unit a:** construct a node with one out-of-range constrained field →
  `validateRanges` records exactly one diagnostic with the right field name and
  bound detail; an in-range node records none.
- **Unit b:** a small nested scene (constrained field on a child) →
  `collectRangeWarnings(root)` finds it through SFNode/MFNode traversal, with the
  child's DEF name attached.
- **Integration c:** parse a fixture carrying a known out-of-range value (e.g.
  `specularColor` r>1) → the diagnostic surfaces on the parse result's structured
  channel; in non-strict mode it also appears in `warnings()`.
- **Integration d:** an all-in-range document → empty diagnostics, `warnings()`
  unchanged.
- **Regression:** existing strict-mode VRML fixtures stay green (no new throws).

## Golden impact

New `validateRanges` overrides + the `RangeDiagnostic` struct change the generated
headers → new golden sha (expected, additive). All EXISTING generated methods
(`set<Name>`, `set<Name>Unchecked`, `fields()`, reflection thunks) stay
byte-identical, so no behavior regression; only the new method and struct are
added. Regenerate via `mise run gen` and commit the golden per the repo's
golden-file policy.

## Non-goals / follow-ups

- Generalizing to full conformance validation → M3 (this seeds it via the
  structured `RangeDiagnostic`).
- Clamping/normalizing out-of-range values → out of scope; policy is keep + warn,
  not repair.

Related: [[x3d-cpp-gen-decisions]] (lenient-read policy), [[x3d-cpp-gen-build]]
(regenerate + golden gate workflow), the resequencing doc
`2026-06-07-architecture-validation-and-resequencing.md` (this is "Step 0").
