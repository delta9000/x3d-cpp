# M3 Design Spec — Moat-Validation Conformance Validator & Scene-State Snapshotter

**Status:** Design-complete, implementation-ready-to-plan. Produced by the `m3-design` workflow (5-dimension research → synthesis → 3-lens adversarial critique → draft), then controller-reviewed and fact-corrected. **Scope:** design/research only.

**Controller correction (2026-06-13) — ONE foundational fact fixed.** The workflow's own §0 re-verification still got the 4.x corpus counts wrong (it reported "zero 4.1 files, 4.0=14"). Independently re-checked against `<X3D ... version='...'>` root elements: the corpus has **303 real `version='4.1'` files and 259 `version='4.0'` files** (single-quoted root attrs the workflow's grep missed; ~562 genuine 4.x files total). This **strengthens** the design — the moat demo runs on **real 4.0+4.1 corpus data**, with authored fixtures only to cover any 4.1-new nodes underrepresented in the corpus. §0 / §5 / §6 / §9-OD3 are corrected for this; every other part of the design (the L2 circularity partition, profile demotion, determinism-before-goldens gating, encoding-spec-grounded round-trip honesty, the three-way classifier, tiered snapshot storage) stands as written.

**Author note on provenance:** Every disputed factual premise in the prior synthesis and the three adversarial critiques was re-verified against the on-disk repo, UOM, and corpus before this spec was written. Corrected facts are stated inline (§0); the design is built on verified reality — with the single 4.x-count exception the controller fixed above.

---

## Decision resolutions (2026-06-13, post-design — supersede the noted §9 open decisions)

**OD-1 — parser/model provenance (THE MOAT CRUX). RESOLVED, with a design consequence.** User attests: the hand-written parser was authored from **UOM-sourced information** — it learns the legal node/field vocabulary from the generated, UOM-derived bindings (factory + reflection), **not** from an independent hand-authored table. **Consequence:** the L2-independent check "parser rejects UOM-forbidden wire" (§2-a) is therefore **circular** — the generated system checking itself — and is **demoted to L2-self-consistency (non-moat)**, exactly as the spec's own OD-10 contingency required. The non-circular moat now stands on **two pillars**:
1. **Generation FIDELITY (non-circular):** defaults / types / containerFields **materialize correctly on real constructed runtime objects** matching the UOM. This tests runtime construction against the spec's declared data (the audit found 7 generation bugs of exactly this kind) — not a reflection tautology.
2. **FIDELITY cross-check (the XSD) — AMENDED 2026-06-13 (see the versioning design §3b).** The earlier framing called the official `x3d-N.xsd` an oracle *independent of the UOM*. **That was over-claimed:** the UOM is a Web3D-**generated** artifact sharing the same Web3D pipeline lineage as the XSD, so XSD↔UOM agreement is **fidelity, not independence** — it catches *our* generation/parse bugs (genuinely independent of *our code*) and pipeline drift, but does NOT independently corroborate that Web3D's encoding matches ISO. So the XSD is **demoted to a fidelity cross-check** (also serving as the node/field-count floor for the meta-validator) and **de-scoped to a one-time on-admission audit** (record the result + XSD hash; don't carry six standing XSDs). It does **not** by itself defeat the circularity a skeptic would raise.
3. **The genuinely-independent anchors against the STANDARD** (kept): the one-time **ISO-19775-prose hand-check** per spec-revision (verifies a sample of the mechanism's *output* against the actual standard text), and the **non-gating differential-vs-other-runtime** column. These are what answer "does the UOM/model match ISO," vs the XSD's "does our model match Web3D's own schema artifact."

Net framing (corrected): *generation gives fidelity-to-the-UOM by construction (pillar 1, non-circular, catches generation bugs); the XSD gives fidelity-to-Web3D's-schema (pillar 2, catches our pipeline bugs but shares spec-encoding lineage); correctness-vs-ISO rests on the prose hand-check + differential.* The honest moat claim is **fidelity-by-construction + a once-per-spec-rev human anchor to the standard**, not "an independent oracle." (Apply to §2's L2 partition and §3's truth ranking; full treatment in the versioning design.)

**OD-3 — 3.x version skew. RESOLVED.** A **genuine X3D 3.3 UOM is live** at `https://www.web3d.org/specifications/X3dUnifiedObjectModel-3.3.xml` (`version="3.3"`, real ConcreteNode content, regenerated 2025-12-21), and the parallel URL pattern almost certainly yields 3.0/3.1/3.2. So **option (a) — source genuine 3.x UOMs from Web3D — is viable**, and the "3.x oracle gap" is a **download, not a launch blocker**. Plan: the MVP still kicks off on the real **4.x set (562 files)** for the headline moat demo; the genuine 3.x UOMs are pulled in next so the 3.x-majority corpus validates against its **own-version** oracle, reaching ~the whole corpus with spec-derived truth.

**OD-4 — profile axis. RESOLVED (controller recommendation; user deferred).** **Drop "× profile" from the moat matrix.** Keep the spec-derived **(version × component × level)** matrix (`componentInfo` is UOM-resident; profiles are not). Profile gating stays deferred to the separately-labeled, non-moat annex side-table, only if/when profile conformance is promoted to a shipped axis.

---

## 0. Verified ground truth (the factual foundation, corrected)

The earlier synthesis reasoned against an imagined repo. The three critiques flagged this; all flags were checked. The corrected, load-bearing facts this spec is built on:

| Claim in prior synthesis | Verified reality | Consequence for M3 |
|---|---|---|
| "Both UOMs on disk; 3.2/4.0/4.1 present" | **Only the 4.0 UOM is a production artifact** (`src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml`). A **genuine 4.1 UOM exists** but lives in `tests/fixtures/X3dUnifiedObjectModel-4.1.xml`, carrying a real Web3D provenance header (`Online at https://www.web3d.org/specifications/X3dUnifiedObjectModel-4.1.xml`). A separate `…-4.1-synthetic.xml` fixture also exists and **differs** from the genuine one. **No 3.0/3.1/3.2/3.3 UOM exists anywhere.** | Moat demo uses the **genuine** 4.1 (not the synthetic); 4.1 must be **promoted to a first-class versioned artifact** (§7). 3.x oracle gap is a **launch blocker**, not a footnote (§3, OD-7). |
| "303 corpus files declare 4.1; 6 versions co-resident 3.0–4.1" | **CONTROLLER-CORRECTED:** the corpus DOES have real 4.x content — **303 `<X3D version='4.1'>` files + 259 `version='4.0'` files** (~562 genuine 4.x; precise root-element counts). The bulk is still 3.x (3.1 dominant) and the 4.x set is a minority, but it is NOT empty. (The workflow's "zero 4.1 / 4.0=14" was a single-quote grep miss.) | The "conformance moves with the spec on real data" demo **has real data**: run on the 259 v4.0 + 303 v4.1 corpus files directly. Authored 4.1 fixtures are only needed to cover specific 4.1-*new* nodes/fields underrepresented among those 303 (§5, M3.0). |
| "containerField is NOT a UOM field attribute" | **WRONG.** containerField is **UOM-resident** as 730 per-node `<containerField default="…" type="…"/>` elements. | containerField is **spec-derived (RANK-1)**, *not* generator-derived. Strengthens the moat; removes a §3 caveat. |
| "version.py auto-detection ~80% built" | **TRUE and verified.** `src/x3d_cpp_gen/model/version.py` exists, auto-detects version from the UOM root attribute, threads version through parse→model→emit, nothing locked to 4.0. | Version-tolerance mechanism is genuinely already built. The longevity claim survives. |
| "`x3d-conformance/` is a curated component-organized subset" | **WRONG.** `find x3d-conformance -name '*.x3d'` = 16,866 = the **entire** corpus. It IS the root. | The "sentinel set" and "curated golden set" **do not exist** and are **explicit scoped construction work** (§5). |
| "to_chars / ≥6-sig-digit invariant already grounded" | **WRONG.** No `to_chars` in `runtime/` or bindings; writers use no precision control. | The determinism substrate is **greenfield** and gets its **own gating phase before any golden** (§5, M3-D). |
| Profile→component membership as a UOM oracle | **Absent.** 0 `ProfileInfo`/`ProfileContentModel`; per-profile "may-be-ignored" obligation matrix effectively absent (1 stray hit). `componentInfo` (per-field component+level) **is** present (534). | **Profile axis is NOT spec-derived.** It is demoted out of the moat score (§2, §6). Per-field component/level **is** a valid moat oracle. |
| Pull getters drive a stable identity | All getters (`worldTransform`, `localBounds`, `worldBounds`, `pick`, `viewMatrix`, `changedNodes`) are keyed by **`const X3DNode *`** (raw pointers). | A **pointer→structural-path-id translation layer** must exist before snapshots are deterministic (§4, §5 M3-D). |

**These corrections do not re-architect the design — the moat thesis survives — but they re-scope the MVP, demote the profile axis, gate the determinism work, and re-ground the headline demo.**

---

## 1. Context + the precise moat-claim M3 proves

**The thesis under test.** Generating the typed C++ object model from the **external ISO Unified Object Model** wins on conformance and future-proofing versus hand-authoring the model. There is no prior art: USD generates from its *own* internal schema; Castle hand-authors its defs. x3d-cpp-gen bets on the *external standard* as the generation source.

**The falsifiable claim M3 demonstrates (re-grounded on verified reality):**

> For every node, field, **default**, **containerField**, and ROUTE in a scene, structural legality is decided **solely by the versioned UOM artifact**, with **zero hand-authored spec facts** in the validator. When the genuine ISO 4.1 UOM replaces the 4.0 UOM, all structural conformance criteria re-derive from that one file with **no validator code edit**, and the resulting criteria/report delta is **exactly and only** the UOM's structural delta — demonstrated on **authored 4.1 fixture files** that exercise the 4.1-added nodes/fields.

**The diff IS the evidence.** The moat is proven when the 4.1 drop-in produces a delta confined to the 4.1 spec delta, mechanically and auditably.

**Two honesty boundaries on the claim (addressing critique B and the prior framing):**

1. **The version-bump demo proves future-proofing/maintenance-cost, not correctness.** A bug in the UOM→criteria transform reproduces identically in 4.0 and 4.1 and still looks clean. Therefore the demo is **paired** with a small set of **4.1-new criteria hand-checked against the ISO 19775 text** (a one-time, recorded, human cross-check — not a maintained table) so the mechanism's *output* is anchored to the standard at least once per spec rev.

2. **Profiles are NOT proven by the moat.** The profile→component-membership tables live in ISO Annexes, *not* the UOM (verified: 0 structured profile elements). Any profile-gating check is sourced from a **separately-labeled, schema-checked, version-stamped side-table that NEVER counts toward the moat score** (§2, §6). The §1 "solely by the UOM" claim is scoped to **node/field/default/containerField/route/component-level** legality — the axes that **are** UOM-resident.

**Moat-validating vs runtime-exercising is a first-class reporting axis** and is split *within* L2 to defeat the circularity critique (§2).

---

## 2. Layered conformance model (headless, 4 levels) + the circularity partition

Four levels, each with a **distinct oracle** — which is exactly what makes the three-way discriminator mechanical (§3).

| Level | Measures (headless) | Oracle | Attribution |
|---|---|---|---|
| **L0 — Lexical/Encoding** | Every corpus file in all 4 encodings parses to a node tree without error; header/PROFILE/COMPONENT/UNIT/META recognized; dirty whitespace tolerated and **normalized, not dropped** | The file / its re-parse | Runtime (parsers) + UOM-vocabulary |
| **L1 — Round-trip/Serialization** | parse→write→parse **fixed-point convergence** at the *model* layer (not byte); numeric values preserve ≥6 significant digits; cross-encoding model-equality | Self (re-parse idempotence) | Runtime (writers) |
| **L2 — Structural/Model** | Per node vs the UOM (see partition below) | **The versioned UOM** | **MOAT (partitioned)** |
| **L3 — Behavioral/State** | Run N ticks at fixed timestamps + seeded inputs; snapshot deterministic state | Analytic (closed-form) + golden | Runtime |

### L2 is partitioned to defeat the circularity attack (critique B)

The parsed graph is built by the hand-written parser populating **generated** bindings emitted from the **same** UOM. So a naïve "graph vs UOM" check is partly the generator agreeing with itself via reflection. We split L2 explicitly and **only the independent half counts toward the moat score**:

- **L2-self-consistency (NOT moat, reported separately):** reflected FieldTable field-existence/type/accessType isomorphism to the UOM node def. This is codegen self-consistency — valuable as a regression net, **but it is the generator agreeing with itself.**
- **L2-independent (THE MOAT):**
  - (a) **Parser rejects UOM-forbidden wire** — feeding a field/node the UOM does not define is refused. *(Independent: the parser is the system under test against the spec vocabulary.)*
  - (b) **Defaults materialize on constructed objects** — every unset field equals the UOM default on a real constructed object (the audit's #1 historical bug). *(Independent: tests runtime construction, not reflection tautology.)*
  - (c) **containerField correctness** vs the UOM's per-node `<containerField>` (verified UOM-resident).
  - (d) **acceptableNodeTypes** honored on SF/MFNode fields.
  - (e) **DEF uniqueness + USE resolves to a type-compatible prior DEF.**
  - (f) **ROUTE topology** legal (endpoints exist; source out/inout; dest in/inout; field types match; redundant routes ignored).
  - (g) **Per-field component + level** legality (verified UOM-resident via `componentInfo`).

**Profile gating is explicitly NOT in L2-independent** — it has no UOM oracle (§1, §6).

**The citable moat = L2-independent + L1 round-trip idempotence + the zero-edit version-bump demo.** L2-self-consistency and L3 are reported but labeled non-moat. This directly answers "does M3 prove the moat or just regression-test against its own bindings": the **partition is the answer**, made visible in every report.

**Parser-independence caveat (for the human, OD-10):** even L2-independent (a) has hidden circularity *if the hand-written parser was authored by consulting the generated bindings*. The human must attest to parser authoring provenance; this is the true crux of the moat and cannot be verified from files alone.

### L3 execution order is normative and asserted

Tick = **(1) bind-resolution → (2) sensors → (3) routes-to-quiescence → (4) particles → (5) physics** (ISO 4.4.8.3). L3 asserts the *order*, not just final values. The architecture doc's "binding read at top of tick" *is* spec step 1.

### The L2/L3 ROUTE split (no double-count)

ROUTE **type-match + endpoint-access legality** = static graph property → **L2-independent (f)**. ROUTE **cascade behavior** (one-event-per-field-per-timestamp, loop-breaking) = dynamic → **L3**.

### Formally OUT (principled boundaries, stated in the report, not gaps)

Lighting/shading, texture sampling, pixels/framebuffer, anti-aliasing, BRDF, font rasterization/Text-layout pixels — the "presentation" half of browser-conformance 6.2.3. **The NIST `ConformanceNist` archive is OUT**: its per-test oracle is a rendered reference JPEG (human-judged), which a headless renderer-agnostic runtime cannot produce by construction. **OUT-but-headless-adjacent, deferred to L3+ (§8):** Script/SAI execution, audio rendering, collision-response navigation. **The sharp line:** state that is a deterministic function of (scene, time, input) is IN; state that is a function of a rasterizer is OUT.

---

## 3. Reference-truth per level + the three-way discriminator

**Truth-source ranking by durability (decisive ordering):**

1. **Spec-derived (UOM)** — RANK 1. The moat made measurable. Includes containerField (verified UOM-resident) and per-field component/level. Regenerates from the spec artifact.
2. **Self / round-trip fixed-point** — RANK 2. Eternal, zero external dependency; proves *consistency, not correctness*. Never the sole layer.
3. **Analytic (closed-form)** — RANK 3. The only behavioral-**correctness** oracle needing no renderer: interpolator math; Transform composition `T·C·R·SR·S·-SR·-C`; bounds; picking; binding-stack invariants. Durable; bounded authoring cost.
4. **Golden snapshots** — RANK 4. Catches *our drift, not correctness*. **A regression net, never a correctness oracle.** Provenance-stamped (§4).
5. **Differential vs X_ITE/Castle/FreeWRL** — RANK 5. **Reported as a non-gating column** (promoting it into the report per critique B gives the report one external corroborator without importing their bugs into CI). Disagreement opens a ticket, never fails CI.

**Profile side-table truth source (the demoted axis):** sourced from an ISO-Annex scrape into a **generated, version-stamped, schema-checked side-table**, explicitly labeled **NOT-pure-spec-derived**, with its own meta-validator (§7). It **never contributes to the L2-independent/moat score.** Resolving its existence is a launch prerequisite *only if* profile-gating ships in the MVP — and it does **not** (§5).

### The closed un-oracled gap (critique A.4 / B / "the alarm that doesn't fire")

The classifier is mechanical only if every state dimension trips *something* under regression. So we **forbid golden-only coverage** for any state that is a deterministic function of (scene, time, input): **every L3 concern must carry at least a coarse invariant assertion** even where no exact closed-form exists:

- Interpolators: exact closed-form at keys/endpoints/clamp **+** monotonic-fraction invariant.
- World transforms: exact for single-placement **+** orthonormality/affine-validity invariant for all (catches sign-error transforms that still yield a valid-looking Mat4).
- Bounds: world-AABB **contains** all child world-AABBs (containment invariant).
- Binding stacks: **exactly one** bound per stack/layer; isBound consistency (conservation invariant).
- Picking: hit point lies **on** the reported geometry's world-AABB surface or interior (containment invariant).

This guarantees an OUR-REGRESSION in an un-oracled surface still trips an invariant, so it cannot masquerade as INTENDED-CHANGE.

### The three-way discriminator (first-class mechanism)

Every snapshot/report record is **provenance-stamped**: `uomVersion` + `uomManifestHash`, `validatorVersion`, `runtimeCommit`, `corpusId` (frozen revision). A CI diff is **mechanically classified**:

- **SPEC-CHANGE** → `uomManifestHash` moved **AND** the delta is confined to the **computed expected-ripple set** of the UOM diff. **The ripple set is computed, not eyeballed:** from the UOM 4.0→4.1 diff we derive (i) added/removed nodes/fields → affected criteria; (ii) **changed defaults** → the exact set of snapshots whose default-elided output flips (default-elision means a changed default ripples into every file using that node). The classifier compares the observed delta to this *computed* ripple set. A delta inside the set = SPEC-CHANGE; a delta outside it during a spec bump = **flagged for human review**, never auto-blessed. This makes default-change ripple mechanical (closes critique A.4 hole 2).
- **OUR-REGRESSION** → `uomManifestHash` **unchanged** AND output moved AND it **violates a still-valid L2-independent spec assertion or an L3 analytic/invariant assertion**. **CI red — the alarm that matters.**
- **INTENDED-CHANGE** → `uomManifestHash` unchanged, only a golden moved, **no spec/analytic/invariant assertion broke.** A human ratifies via an **explicit blessing commit** (new golden + one-line rationale + recorded hash). **Never auto-blessed.**

The spec-derived and analytic layers **outrank** golden so the classifier is mechanical. This reuses the ritual of the existing header-golden regime (`check_golden.sh` + tracked sha256). **NEVER auto-bless.**

---

## 4. Snapshotter + harness + report architecture

**Two artifacts, one walker.** A deterministic **state snapshot** and a **glTF-Validator-shaped conformance report**, both from one harness that promotes `scripts/corpus_sweep.cpp` (recursive walker + ext-sniff + `signature()` normalization) into a CI driver.

**Report contract (copy glTF-Validator — three ecosystems converged on this shape):**

```
{ validatorVersion, x3dSpecVersion, uomManifestHash, runtimeCommit, corpusId,
  validatedAt (EXCLUDED from diffs), uri (corpus-root-relative),
  encoding, profile, version, parseStatus,
  summary{ numErrors, numWarnings, numInfos, numHints, truncated },
  messages[ { code, severity(0=Error/1=Warning/2=Info/3=Hint),
              pointer(node-path-id + field), message(template), data? } ],
  snapshotRef }
```

Publish a versioned in-repo JSON-Schema. **Counts separated from messages** so CI gates on `numErrors` without parsing prose.

**Issue identity = stable UPPER_SNAKE hierarchical CODE, never prose.** `CATEGORY_SUBJECT_PREDICATE`: `FIELD_VALUE_OUT_OF_RANGE`, `CONTAINERFIELD_MISMATCH`, `ROUTE_TYPE_MISMATCH`, `PROTO_UNRESOLVED_EXTERNPROTO`, `ENCODING_HEADER_VERSION_INVALID`, `VERSION_DECLARED_BUT_NODE_NEWER`, `NODE_UNRECOGNIZED_FORWARD_COMPAT`. Codes are **append-only, never renumbered, tombstoned not deleted**, each carrying `severity` (overridable via a glTF-style policy layer: `ignoredIssues`/`onlyIssues`/`severityOverrides`/`maxIssues`) and `sinceVersion`. A checked-in `ISSUES.md` is auto-generated so taxonomy changes are loud in diffs. **Categorize by ISO clause-6 domain** (Syntax/Encoding, Profile-Component, Field-Value, Structure/Link, Proto, Behavior/Runtime), not by node type.

**Validator registry (USD's lesson — registry, not if-ladder).** A `Validator` interface { code-family, applicable level (file|scene|node), run→issues }; register instances; assemble named **suites** ("x3d-4.0-Full", "regression-min"). **Generate as many validators as possible from the UOM** (ranges, enums, containerField, defaults, component/level membership, acceptableNodeTypes). Reuse the existing reflection walk (`runtime/X3DRangeValidate.hpp::collect`) as the universal traversal substrate — including its `const X3DNode*` onPath cycle guard and `if(!f.isReadable())continue;` InputOnly-MFNode guard.

**Generalize `RangeDiagnostic`/`ProtoWarning` into one `ConformanceFinding`** { stable code, severity, truth-layer that produced it, node-path-id locator, detail, spec provenance (UOM version + ISO clause + `specificationUrl`) }. One uniform structured stream, CI-diffable.

**UOM-schema meta-validator (critical longevity guard, critique A.6).** The UOM is the Web3D consortium's tooling artifact; its element/attribute *schema* (`ConcreteNode`, `componentInfo`, `containerField`, `containerFieldChoices*`, `InterfaceDefinition`) can be restructured between releases **independent of the X3D standard.** Because criteria are *derived not authored*, a silently-drifted schema yields a **green run against a subtly-wrong manifest** — invisible rot. **Mitigation: a meta-validator asserts every ingested UOM conforms to the exact element/attribute shape the manifest extractor depends on, failing LOUDLY on drift.** This is the one guard against silent manifest rot and is a launch requirement.

### Snapshot (canonical document, RFC-8785 / JCS style)

Pre-order DFS, sections: `meta` (all provenance pins + tick schedule + float-quantum + `modelHash`), `nodes` (path-id, type, def?, component/level, **non-default fields only**, per-node worldTransform/localBounds/worldBounds/isBound), `routes` (sorted), `bindingStacks` (bottom→top), `diagnostics`, `perTickDeltas` (the `changedNodes()` dirty-set + new transforms).

**Four determinism axes, each pinned — and the pointer→path-id layer is GREENFIELD work that gates goldens (critique A.3):**

1. **Time** — never wall-clock; fixed `(t0=0, dt, N)` injected per-run into `tick(now)`; recorded in `meta`. The single-timestamp cascade makes this fully determining.
2. **Float** — `std::to_chars` shortest-round-trip for the **model layer** (verified: not yet used anywhere — greenfield); quantize to a documented decimal grid (default **1e-6**) *before* shortest-form for the **derived-math layer** (world transforms/bounds, exposed to FMA/reassociation drift). Normalize −0.0→0.0; snap |x|<eps→0; NaN/±Inf→sentinels. **Reject usddiff's "exact bytes, no tolerance"** as the anti-pattern.
3. **Node identity** — **structural path-id** (chain of containerField-or-fieldName/index), **never pointers**. Verified: all pull getters are `const X3DNode*`-keyed and `changedNodes()` ordering is pointer/allocation-dependent — so a **translation layer between the pointer-keyed reality and the canonical path-id snapshot must be built and unit-tested before any golden is committed.** USE'd nodes emitted once at the lexicographically-least path; referenced by id; DEF carried as an attribute, not identity.
4. **Ordering** — `fields()` is declaration-ordered; DFS over field order; **never serialize an unordered_map in iteration order** (routes sorted by tuple; warnings by (path,field,code); stacks bottom→top).

**Run-twice-and-diff self-check** (same-process and cross-process, and **cross-toolchain gcc-vs-clang**) asserting byte-identity. **No golden may be committed until run-twice-diff is green across both toolchains and the float quantum is empirically pinned** (OD-5).

### The one real codegen dependency

`componentName()`/`componentLevel()` are static-per-concrete-type, unreachable from `X3DNode*` generically. Add a virtual `nodeComponent() → {name,level}` (or a generated static name→component table) so per-component/level rollups work. Additive → one expected golden-sha churn. (OD-3.)

### State DRIVE vs DEPEND + sequencing vs M2e/M2.5

The snapshotter **DEPENDS on the already-shipped pull surface** (`X3DExecutionContext`: `worldTransform`, `localBounds`, `worldBounds`, `boundViewpoint`, `pick`, `viewMatrix`, `dirtyTracker().changedNodes()` — all verified implemented, all `const X3DNode*`-keyed). It is **not** a new API to drive; by consuming the pull surface it **defines the minimal extraction read-out and is the forcing function** proving the pull-of-changed-set shape is sufficient.

The one thing it legitimately **DRIVES into M2.5**: the **missing-readout list** it surfaces by being unable to snapshot without them — per-leaf (multi-placement) world transforms, per-node component/level, a stable path-id accessor, material/light/camera payload. **M3 writes the extraction seam's requirements by failing to snapshot without them.**

**Sequencing (decisive):**
1. **snapshotter-v1 on existing pull getters NOW** (M2a–d done). Accept the **single-placement world-transform approximation** for USE'd nodes (consistent with M2a/M2b's "first-seen parent canonical"), **flagged in the snapshot.** Do **not** block on M2e (LOD).
2. **Harvest the missing-readout list** from v1.
3. **Freeze the M2.5 extraction API to satisfy that real consumer** — resolving §1's pull-vs-push by fiat: **pull-only for M3; push deferred.**
4. **snapshotter-v2 reads the frozen seam** (per-leaf multi-placement transforms; M2e section, non-blocking).

So roadmap **M2e → M2.5 → M3** is amended to: **snapshotter-v1 (now, pre-M2.5) → M2.5 freeze → snapshotter-v2; M2e slots in later as a non-blocking snapshot section.** The snapshotter runs **in parallel with the moat MVP, not after it** (critique C).

---

## 5. Phased plan — minimal MVP that proves the thesis FAST, then full

The MVP is the **smallest moat kernel**, not the full sweep. The earlier MVP bundled full-16,866 sweep + report schema + scoreboard + matrix + classifier *before a single moat check* — that front-loads **runtime-exercising scaffolding** and contradicts the moat-vs-runtime axis. Corrected:

### Phase M3.0 — MVP: "Prove the Moat" (tiny, days not weeks)
1. **UOM→criteria manifest generator (4.0)** — closure of node→component/level/containerField/field-set/defaults/ranges/acceptableNodeTypes.
2. **UOM-schema meta-validator** (§4) — guards the extractor against schema drift from day one.
3. **L2-independent checks** via the existing reflection walk: parser-rejects-forbidden-wire, default-materialization, containerField, acceptableNodeTypes, DEF/USE, ROUTE topology, component/level.
4. **Run on the real corpus 4.x set — 259 `version='4.0'` + 303 `version='4.1'` files** (controller-corrected; the corpus is NOT empty of 4.x). Add a small **authored fixture set** only for any 4.1-*new* nodes/fields not exercised by those 303 (committed; keeps the 4.1-delta fully covered).
5. **The headline demo:** promote the **genuine** `X3dUnifiedObjectModel-4.1.xml` to a first-class versioned artifact; drop it in; re-run; show the criteria/report delta == the **computed UOM ripple set**, **zero validator edits**, on **real 4.1 corpus files** (303 of them) plus the targeted 4.1-new fixtures. Pair with the one-time hand-check of a few 4.1-new criteria against ISO 19775 text (§1).
6. Add the codegen `nodeComponent()` virtual. Wire the three-way classifier on `uomManifestHash`.

**Explicitly NOT in M3.0:** full 16,866 sweep, report JSON-Schema publication, scoreboard JSONL, the conformance matrix — these are **runtime-breadth/operational** and move to M3.1+.

### Phase M3-D — Determinism substrate (its own gating phase, runs in parallel with M3.0)
- Implement + unit-test the **pointer→structural-path-id translation layer**, the **canonical float emitter** (`to_chars` + documented decimal grid), and the **run-twice byte-identity self-check** on a 50-file set.
- **Empirically pin the gcc-vs-clang float quantum** (OD-5) on a sampled set.
- **Gate:** no golden corpus may be committed until run-twice-diff is green across **both** toolchains. A golden on an unpinned quantum is a liability.

### Phase M3.1 — L1 round-trip + cross-encoding + the operational scaffolding
- parse→write→parse fixed-point (model-equality with explicit canonical normalization); ≥6-sig-digit check; cross-encoding equality on a sampled subset.
- Stand up the **glTF-shaped report + JSON-Schema**, the **validator registry/suites**, and the **full-16,866 nightly sweep** (here, not in MVP — this is runtime breadth).

### Phase M3.2 — Snapshotter-v1 + L3 analytic (curated set) — runs in parallel from the start
- Deterministic snapshot on existing pull getters (depends on M3-D); fixed tick schedule; analytic + invariant oracles (§3) for interpolators/Transform/bounds/binding/picking.
- **Construct the sentinel + curated-golden sets as explicit scoped work** (verified: `x3d-conformance/` is the whole corpus, so no curated subset exists to lean on). Sentinel = one representative per occupied component×level; golden set = low hundreds, human-reviewable.
- Golden snapshots in git-LFS / content-addressed store (§8) with the blessing ritual. Harvest the M2.5 missing-readout list.

### Phase M3.3 — Full: M2.5-frozen snapshotter-v2 + full behavior trace + scoreboard
- snapshotter-v2 on the frozen seam (multi-placement transforms; M2e section); per-tick behavior-trace deltas; named profile/component suites (profile side-table, if approved); append-only JSONL scorecard time-series; differential-vs-other-runtimes **reported column** (non-gating).

**CI topology throughout:** the **sentinel subset** with committed behavior-goldens = the **fast PR gate**; the **full 16,866 sweep** (sharded across cores; per-file wall-cap; gcc+clang+ninja+ccache all verified present) = the **nightly/release monitored report**, gated on the **three-way diff vs baseline, NEVER on absolute conformance %**, with **per-cell regression detectable even in the 14-file v4.0 cell.**

---

## 6. The citable conformance metric

A **(version × component × level)** matrix — **profile is dropped from the moat matrix** because it has no UOM oracle (verified). Each cell = pass-rate over corpus files touching that component, against the UOM-derived denominator.

**De-overfitting (critique C):** 96% of the corpus is profile=Immersive and 67% is v3.1, so a single "corpus-frequency-weighted headline number" reduces to "how well do we do on 3.1/Immersive" and **hides the long tail the moat most needs.** Therefore:
- **Absolute counts are dominant; percentages secondary.**
- Publish a **separate per-cell coverage map** so empty/near-empty cells (the 4.x/Full long tail) are **LOUD, not weighted away.**
- The single frequency-weighted headline is **dropped or heavily caveated**; if retained it is explicitly labeled "dominated by 3.1/Immersive — not predictive of 4.x conformance."
- **L2-independent (moat) and L3 (runtime) scores are reported separately and labeled** (§2). L2-self-consistency is reported but **excluded** from the moat figure.
- **Self-assessment qualifier:** numerator and denominator are both self-derived; the public metric is named **"self-assessed spec-criteria pass-rate"** with the differential-vs-other-runtimes column as the only external corroborator.

Persisted as an **append-only JSONL scorecard committed in-repo** (git-as-time-series), each row pinned with `(corpusId, uomVersion, runtimeCommit)` so the number is **reproducible bit-for-bit and CI-diffable.**

---

## 7. Longevity guarantees

**The single invariant locked now:** *conformance criteria and snapshot records are ALWAYS tagged with, and derived per, spec version — no global 4.0 assumption anywhere.* Verified already substantially built (`version.py` auto-detects from the UOM root and threads version through the pipeline). This is a **launch requirement** (the corpus already spans 3.0–4.0; 4.1 is coming), not a decade-out concern.

- **Criteria derived, never authored** — a spec move = re-ingest one XML, ~zero validator edits. Guarded by the **UOM-schema meta-validator** so silent extractor mis-parse fails loud.
- **Genuine-artifact discipline** — only **genuine ISO/Web3D UOMs** are first-class (the real 4.1 carries a Web3D provenance header; the `-synthetic` fixture is **never** used for moat proof). Each UOM is checked in as a **version-stamped artifact** with its `uomManifestHash`.
- **Default-elision** (emit only non-default fields, defaults from UOM-derived getters) makes snapshots version-tolerant: a 4.1-added field with a default leaves existing snapshots byte-unchanged.
- **Stable append-only issue codes** (never reused/renumbered, tombstoned) keep 2026 bucket counts comparable to 2031.
- **Three-way mechanical classifier** keyed on `(uomManifestHash, computed-ripple-set, blessing-commit presence)` keeps every diff attributable — including under default changes.
- **Determinism-as-contract** — pure function of (document bytes, UOM version, tick schedule, float-quantum); run-twice-diff + **cross-toolchain** self-check; pinned toolchain + clang-format; **empirically-pinned** float quantum on the derived layer as the cross-decade compiler-migration hedge.
- **Spec anchors localized** — L1's precision floor (≥6 sig digits, exponent ≥[−12,12]) points at the **encoding** specs (ISO 19776-x), **not** the architecture clause 6 (which mandates neither lossless round-trip nor a precision). A future binary-encoding precision change perturbs only L1.
- **UOM-diff report (4.0→4.1 added/removed/changed) as a first-class artifact** (both UOMs on disk) so spec-change drift is auto-explained and feeds the ripple-set computation.
- **Profile side-table** (if it ships) is a **separately version-stamped, schema-checked, explicitly-non-spec-derived** longevity surface with its own meta-validator, re-validated every spec rev, **never counted as moat.**

---

## 8. Scope boundaries + backlog deferrals with triggers

**Explicitly OUT (principled, stated in the report):** all rendering/presentation (lighting, texture, pixels, font-raster, BRDF) per browser-conformance 6.2.3; differential-vs-other-runtime as a **CI gate** (it ships as a non-gating reported column); NIST-image pass/fail (oracle is a rendered JPEG).

**Backlog deferrals (with triggers):**

| Deferred item | Trigger to pull in |
|---|---|
| Script/SAI execution conformance | When the runtime ships a headless Script engine; add as L3+ behavioral suite. |
| Audio rendering | When a headless audio-graph evaluator exists. |
| Collision-response navigation | When the avatar/navigation model lands (collision *geometry* is already headless/IN). |
| M2e LOD snapshot section | When M2e ships; slots into snapshotter-v2 as an additive section (non-blocking). |
| 3.x per-version UOMs | If/when ISO 3.x UOM XML artifacts are sourced from Web3D; until then 3.x is judged via the superset strategy (OD-7). |
| Profile-gating field-obligation matrix | Only if profile conformance is promoted to a shipped axis; requires the annex scrape (OD-1). Currently NOT in MVP. |

**Snapshot artifact lifecycle (designed now, NOT left open — critique A.5):** **TIERED is a HARD requirement** — full **report** over 16,866 files; full **state-snapshot goldens ONLY over the curated sentinel set** (low hundreds, human-reviewable). Per-file snapshots go in **git-LFS or an out-of-tree content-addressed store keyed by `modelHash`**, never as tens of thousands of in-tree JSON goldens (that becomes an unclonable, unreviewable, rubber-stamped 40k-file diff within a few years). The **scoreboard JSONL stays in-repo**; the per-file snapshots do not. In-tree blessed-golden count is **capped to a human-reviewable number.**

---

## 9. Open decisions for the human

1. **Parser authoring provenance (the moat crux, critique B).** Was the hand-written parser authored independently of the generated bindings, or by consulting them? If the latter, even L2-independent (a) "parser rejects illegal wire" has hidden circularity. **The human must attest.** This is the single most important unresolved item — the entire moat rests on it. *Highest priority.*
2. **Genuine vs synthetic 4.1 UOM.** Confirm `tests/fixtures/X3dUnifiedObjectModel-4.1.xml` is the unmodified ISO/Web3D artifact (its header asserts so) and approve **promoting it to a first-class versioned artifact** for the moat demo. The `-synthetic` fixture must never be used for moat proof.
3. **3.x version-skew strategy (OD, load-bearing — ~96% of corpus is 3.x).** Verified: only the 4.0 UOM is production; **no 3.x UOM exists.** But the 4.x set is **~562 real files** (259 v4.0 + 303 v4.1), so scoping the MVP to 4.x is a genuine real-data MVP, not a toy. Pick one explicitly: (a) source genuine 3.0–3.3 UOMs from Web3D; (b) scope the moat to v4.0+ files (the ~562) for the MVP and state 3.x as future work; or (c) "4.0-UOM-as-superset" with a **generated** (not hand-authored) 4.0-minus-3.x delta side-table and a bounded, *measured* delta budget, reporting 3.x as "validated against 4.0 superset, N known-removed-features excluded." **Do not let an allowlist silently absorb the 3.x majority.** *Recommendation: (b) for the MVP — now viable on real 4.x data — (a) longer-term to reach the 3.x majority.*
4. **Profile axis fate.** Drop "× profile" from the citable matrix entirely (recommended for MVP), or build the schema-checked, separately-labeled, non-moat annex side-table (only if profile conformance is promoted). *Recommendation: drop from MVP; defer.*
5. **Unversioned-file policy.** ~4,516 `.x3d` carry no X3D version attribute. Define a default-version resolution rule (e.g., infer from profile, or treat as the lowest supported version with a recorded `VERSION_INFERRED` info). *Needs a rule before the per-version-manifest logic is complete.*
6. **Manifest emission form** — generated C++ header (compile-time, byte-golden discipline) vs committed JSON data file (auditable/diffable) vs live UOM read. *Recommendation: committed JSON for scoreboard auditability.*
7. **componentName/componentLevel virtualization** — accept the additive golden-sha churn of a virtual `nodeComponent()`, or a generated static name→component table (avoids touching every node)?
8. **Diagnostics unification** — physically extend `X3DDocument.rangeWarnings` + `protoWarnings` into one tagged `ConformanceFinding` stream (touches the reflection emitter `reflection.py`) vs a sibling channel? *Recommendation: unify.*
9. **Float quantum default (model layer)** — pin empirically via the M3-D gcc-vs-clang two-toolchain run before fixing; derived layer is 1e-6 by recommendation. **Blocking for goldens.**
10. **Unknown-node forward-compat policy** — ISO is silent. Treat an unknown-but-well-formed node (a 4.2 node in a 4.1-built validator) as **L0 pass + L2 `NODE_UNRECOGNIZED_FORWARD_COMPAT` (graceful)** rather than L2 failure, so the validator ages into future specs. *Recommendation: graceful; confirm.*
11. **Scorecard storage for a decade** — in-repo JSONL (git-diffable; repo grows slowly since it's append-only rows, not per-file snapshots) vs external durable store. *Recommendation: in-repo for the scorecard; per-file snapshots out-of-tree (§8).*

---

**Referenced files (absolute paths):** `<repo-root>/src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml`, `<repo-root>/tests/fixtures/X3dUnifiedObjectModel-4.1.xml`, `<repo-root>/tests/fixtures/X3dUnifiedObjectModel-4.1-synthetic.xml`, `<repo-root>/src/x3d_cpp_gen/model/version.py`, `<repo-root>/runtime/events/X3DExecutionContext.hpp`, `<repo-root>/runtime/X3DRangeValidate.hpp`, `<repo-root>/runtime/X3DDocument.hpp`, `<repo-root>/scripts/corpus_sweep.cpp`, `<repo-root>/scripts/check_golden.sh`, `<repo-root>/docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md`; corpus + UOMs at `<x3d-render-workspace>/testdata` (16,866 `.x3d`; 3.x-dominant (3.1 the plurality) with a real minority 4.x set of **259 `version='4.0'` + 303 `version='4.1'`** root elements — controller-verified; `x3d-conformance/` == corpus root, i.e. there is no separate curated subset).