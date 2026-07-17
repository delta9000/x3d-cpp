# findings.yaml Status Audit

**Date:** 2026-07-17
**Status:** audit complete; clear-cut corrections applied, judgment calls surfaced below
**Scope:** `docs/conformance/findings.yaml` — the hand-authored JUDGMENT layer (does the
claimed `status` reflect current runtime behavior?). Structural CODE FACTS are auto-derived
by `scripts/conformance_view.py` and were explicitly *not* re-checked here.

## Method

All 208 findings were audited against current runtime code (multi-agent fan-out, one
verification agent per ~16 findings, each flagged discrepancy then adversarially
re-checked by a fresh agent that defaulted to REFUTED). One initial batch stalled on a
single finding and left 15 unaudited; those were swept in a follow-up pass, so coverage is
**208/208**.

Result: **6 confirmed discrepancies, 6 refuted** (the verify pass rejected half the
first-pass flags — see the NSN cluster below), plus one systemic observation.

## Corrections applied (all adversarially CONFIRMED and independently re-verified in code)

| id | was | now | why |
|---|---|---|---|
| `CONF-CRITIC-3` | open ("not verified") | **closed** | touchTime condition-3 is implemented (`PointingSensorSystem.hpp` gates on `pressWasOver_ && stillOver`) and tested (`pointing_sensor_test.cpp` `test_isActive_touchTime`), both present since the initial commit. The finding was carried over from BACKLOG without confirming the behavior already existed. |
| `SEAM-2D-NURBS` | claimed "the entire 2D-geometry component **and all NURBS nodes**" render nothing | narrowed to the 2D-geometry component only | `recognizedGeometryType()` (`MeshBuilder.cpp:953`) now returns true for `NurbsCurve`/`NurbsPatchSurface`, which tessellate (already tracked fixed by `NRB-1`); the trimmed/swept/swung remainder is tracked by `NRB-3`. Only the 2D primitives are still unrecognized. |
| `VIS-MOVIE-DECODE` | note: "Theora/WebM remain follow-ups" | "Theora since shipped as Backend B; WebM remains a follow-up" | `runtime/io/theora/TheoraMovieDecoder.cpp` ships and `seam-status.md` lists MovieDecoder **STABLE** with Theora as the second backend + green swap-test. Status (`fixed`) was already correct; only the note was stale. |

The conformance view was regenerated (`mise run conformance`) and `mise run
conformance-gate` passes.

## Judgment calls surfaced, deliberately NOT auto-edited

**1. The NSN (LoadSensor) `deferred`-vs-`open` convention — the audit disagreed with
itself.** The first pass flagged NSN-1..9 as stale deferrals (their blocking asset-resolver/IO
seam shipped 2026-06-23). The adversarial verify pass **confirmed NSN-1/NSN-2 but refuted
NSN-3..9**, on a strong argument: this file has a deliberate convention where a finding stays
`deferred` (with an explanatory note) once its external seam ships but *before the feature
card itself lands* — flipping only when the implementation ships. Evidence for the convention:
`SCR-005` and `CONF-CRITIC-2` carry the identical "unblocked by the seam (shipped
2026-06-23)" note while staying `deferred`, and `seam-status.md` says "each flips its own
findings when its card ships." `grep -r LoadSensor runtime/` confirms no LoadSensor System
exists, so the behavior is genuinely unimplemented either way.

The unresolved question is purely definitional: does `deferred` mean "blocked on a missing
*seam*" (→ these are now `open`) or "blocked on missing *implementation of any kind*" (→
these stay `deferred`)? The file header says the former ("deferred = blocked on a missing
subsystem/seam/design"); the maintainer's usage is the latter. **This is the maintainer's
convention to set, not mine to flip** — but it affects 8+ findings (NSN-1..9, SCR-005,
CONF-CRITIC-2) and is worth one explicit ruling that then updates the header wording to match.

**2. Dangling `commit:` hashes — systemic, not per-finding.** `TDN-8` cites `commit:
e92042e`, which does not resolve. It is not special: of the ~29 distinct commit hashes
referenced in findings.yaml, only **one** (`5ddbe5d`) resolves in this tree — the repo was
squashed to an "Initial release" commit (766e2ae, 2026-06-25) that orphaned all
pre-squash history. The behavioral claims those findings cite still check out in code; only
the provenance pointers are dead. **Not mass-edited** — dropping them loses provenance that
may resolve in a mirror/unsquashed clone, and it's one decision (keep as historical / drop
all / add a header note), not 28.

**3. `ENC-SFNODE-DEFAULT` — likely stale, but unconfirmable without a corpus run.** Its
blanket claim (an SFNode `<field>` default child "collapses to empty on round-trip") is
contradicted by current code: all three writers serialize `nodeDefault` children
(`XmlWriter.cpp:363`, `VrmlWriter.cpp:248`, `JsonWriter.cpp:306`) and all three readers
populate it (`XmlReader.hpp:386`, `ClassicVrmlReader.cpp` `captureNodeDefault`,
`JsonReader.cpp:310`). But it names specific corpus scenes
(`SeaStarHighResolutionPrototype.x3d`, `NewShape.wrl`) that can't be re-run here, and it may
share the DEF/USE early-return residual documented for `ENC-PROTO-CONNECT` (the USE-reference
path in `XmlWriter.cpp` returns before `attachIsBlocks`). Left `open` pending a corpus check;
the summary should likely be narrowed once confirmed.

## Refinement worth folding into `ENC-PROTO-CONNECT` (no status change)

The finding is correctly `open`, but the audit pinned the residual precisely: the deep
IS/connect case *is* handled and test-covered (`proto_deep_is_roundtrip_test`); what still
drops is IS/connect on a **DEF/USE-reused** proto-body node, because the USE-reference path
in `XmlWriter.cpp` early-returns ("no fields, no children") before `attachIsBlocks` runs.
That is a more actionable description than "dropped on round-trip (5 scenes)."

## Bottom line

findings.yaml is in good shape for a hand-authored judgment file — 3 stale entries out of
208, all minor/low or cosmetic, now corrected. The two things worth a maintainer decision
before launch are definitional, not bugs: the `deferred`/`open` convention (settle it and
align the header wording) and the squash-orphaned commit hashes (keep, drop, or annotate).
