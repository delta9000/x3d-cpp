# X3D Workbench — Visual Language

> **Status:** draft stub (2026-06-28). Decoupled from
> `2026-06-28-x3d-workbench-architecture.md`, which governs product architecture,
> data contracts, and interaction. This artifact owns *only* the visual layer:
> tokens, component inventory, the status glyph+colour registry, and interaction
> states. It is downstream of the architecture doc and must satisfy the
> constraints that doc sets (notably contract **C3**: status is encoded
> redundantly, never by colour alone).
>
> **Intended living home:** `docs/wiki/` alongside the workbench subsystem page.
>
> This is a stub: it fixes the high-level intent and the hard constraints, and
> lists what the full artifact must specify. It is not yet a complete spec.

## Intent

The UI is a **technical instrument panel**, not a consumer landing page: dark
neutral base, compact rows, thin separators, small stable controls, status colour
used sparingly and consistently, viewport-first composition, no decorative chrome.
It should read like a debugger crossed with a browser dev-tools inspector.

## Hard constraints (inherited from the architecture doc)

- **Redundant status encoding.** Every status is `glyph + label + colour`. The
  glyph alone must disambiguate (WCAG 1.4.1; the eleven-statuses-into-six-colours
  collision; the colourblind failure of green/yellow/red). Colour is reinforcement,
  never the sole channel.
- **Tags are non-colour treatments.** Following LSP's `DiagnosticTag`: *inert*
  (parsed-but-inert / extracted-but-unconsumed) renders **faded**; *fallback*
  renders **marked/struck**. These are orthogonal to the status colour.
- **Green is earned.** Full `Rendered` (every extracted attribute consumed) is the
  only state that gets the "complete" green. `Rendered (partial)` and
  `Nonconformant` must be visually distinct from it.

## What this artifact must specify (TODO)

1. **Tokens** — type scale, row height(s), density modes, separator weight,
   spacing scale, the full colour ramp (and its dark-theme values).
2. **Status registry rows** — for each disposition status in C3
   (`Rendered`, `Rendered (partial)`, `Nonconformant`, `Extracted`, `Parsed`,
   `Runtime`, `Bound`, `Stacked`, `Animated`, `Fallback`, `Default`,
   `Unsupported`, `Ignored`, `Invalid`, `Missing Asset`): `{ id, label, glyph,
   colour, precedence }`. This table is the single source the UI renders from;
   adding a status is a row, not a code change.
3. **Component inventory** — row, chip, field editor, tree node, overlay legend,
   diagnostic line, breadcrumb, corpus grid cell, reconciliation row
   (applied/dropped/default).
4. **Interaction states** — hover, focus, active, selected, disabled, loading,
   empty, error. (Hover ≠ select is an architecture contract; this artifact gives
   it a visual treatment.)
5. **Colour semantics** (sparingly applied):
   - Green: rendered-complete.
   - Blue: selected, bound, current.
   - Yellow: fallback, partial, warning.
   - Red: invalid/error — and consider distinguishing *invalid input* (author's
     fault) from *nonconformant output* (engine's fault).
   - Gray: parsed-only, ignored, inactive.
   - Purple: reserved for ROUTE/event flow or author-shader concepts; never the
     dominant palette.
6. **Overlay styling** — selection outline, bounds, normals/tangents, wireframe,
   skeleton, light vectors, viewpoint frustum, route pulses (dispatched vs
   dropped), unsupported/fallback markers.

## Non-goals

- Full custom theming.
- Anything that belongs to architecture/interaction (selection model, diagnostics
  shape, status semantics) — those live in the architecture doc.
