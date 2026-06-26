# X3D-CPP-GEN VERSIONING DESIGN

**Status:** DESIGN/RESEARCH. Produced by the `m3-versioning` workflow (4-dim research → synthesis → 3-lens critique → draft), then controller-reviewed and fact-verified. Augments `docs/superpowers/specs/2026-06-13-m3-conformance-design.md`. Records the project's version policy — the future-proofing half of the M3 moat.

**Controller verification (2026-06-13):** the load-bearing claims were independently re-checked and hold: `X3DDocument.version` is read only by the three writers (inert at runtime → the runtime is genuinely mono-UOM); the UOM-schema meta-validator does NOT exist yet (so the zero-edit guard is unbuilt launch work, not an existing safeguard); `componentInfo` carries only `name`+`level` (no `since`/availability → per-version legality requires ingesting+diffing the actual per-version UOMs, not deriving from one); the HAnim version-default skew (2.0 for X3D4 / 1.0 for X3D3) is documented in the generated enums in-tree; 4.1 adds 6 ConcreteNodes (260→266), zero removed; `parser.py` silently `print`+`continue`s on an unmapped field type and `NodeBuilder` silently skips unknown nodes/fields. **This doc's §3b CORRECTS the M3 spec's OD-1 pillar-2:** the UOM and the official `x3d-N.xsd` share Web3D pipeline lineage (the UOM is a generated artifact), so the XSD is a *fidelity* cross-check (catches our generation/parse bugs), **not** an oracle independent of the spec-encoding. The M3 spec OD-1 is amended accordingly.

**Honesty discipline (set once, applies throughout):** this doc distinguishes three lifecycle states on every claim — **BUILT** (verified in-repo today), **DESIGNED-UNBUILT** (specified here, no implementation exists), **GATED** (depends on an empirical check or external artifact not yet run/pinned). The three critiques' central correction was that the prior synthesis labelled load-bearing-but-unbuilt machinery as "RESOLVED." That is fixed: a decision can be *resolved* (the human need not re-litigate the choice) while the artifact is *unbuilt* (the work remains). Both labels appear on every such item.

---

## 0. Thesis, invariant, and the corrected scope of the moat claim

**The moat's future-proofing half is NOT "one running build handles every X3D version."** The runtime is permanently **mono-UOM** (BUILT, verified): `X3DDocument.version` defaults to `"4.0"` and is overwritten per-encoding, but **zero** model/parse/cascade logic branches on it. The generated C++ bakes single-version constants — e.g. `HAnimHumanoid::getDefaultVersion()` returns a literal `HanimVersionChoices::_2_0` and the member initializer is `{_2_0}` (BUILT, verified at `generated_cpp_bindings/HAnimHumanoid.hpp:114,1619`), while that same UOM's enum appinfo states verbatim "default is 2.0 for X3D version 4, and default is 1.0 for X3D version 3" (`generated_cpp_bindings/X3Denums.hpp:567`).

**The future-proofing half is, precisely:** *structural conformance criteria (L2) are pure DATA derived per-version from the external ISO UOM, so the validator judges each document against ITS OWN version's oracle with zero validator-code edits, decoupled from whichever single UOM the runtime was compiled from.* This claim is true **only for the L2 data plane** and **only for additive change within constraint categories the extractor already models** (§6). It is NOT a claim about L3 behavioral snapshots, which are honestly version-coupled and Vr-tagged (§3).

**The locked invariant (governs everything):** every criterion, report record, and snapshot is tagged with — and derived from — a *named* one of three versions (§2). Any code path reaching for "the spec version" must name which of the three. A bare unqualified "the version" is a latent skew bug. There is no global 4.0 assumption AND no global "the version" — there are exactly three.

**What carries the moat (corrected per Critique-3):** the *mechanism* proof needs exactly **two** versions on real data (VP-0). Scoring the 3.x-majority corpus needs **six** versions but proves *conformance breadth*, not future-proofing — it is M3-metric work wearing the versioning hat, and is split out and demand-driven (§9), not billed to the moat or to launch.

---

## 1. Version model — detection per encoding + inference for unversioned files

**Vd = document-declared version**, detected per-encoding at the existing reader capture points (all BUILT), with NO behavior change to the readers themselves — only an added *inference resolver* and a *stamp*. Per Critique-3, the inference resolver and the VRML floor are **NOT part of the manifest/data mechanism**; they are a separate *version-resolution & legacy-mapping* subsystem (§9, VP-2) with its own correctness surface. They are documented here for completeness but explicitly fenced out of the VP-0/VP-1 data mechanism so the manifest work can ship without dragging a reader change through ratification.

| Encoding | Detection source (BUILT) | On-absence |
|---|---|---|
| XML `.x3d` | `<X3D version=…>` (`XmlReader.hpp:59-60`) | inference ladder |
| ClassicVRML `.x3dv` | `#X3D V<maj>.<min>` (`ClassicVrmlReader.hpp:145,167`) | inference ladder |
| JSON | `"X3D"."@version"` (`JsonReader.hpp:77-78`) | inference ladder |
| VRML97 `.wrl` | `#VRML V2.0` (`Vrml97Reader.hpp:113-136`) | map to X3D floor (§8) |

**Inference ladder for the ~4,516 unversioned `.x3d` and headerless `.x3dv`** (DESIGNED-UNBUILT) — deterministic, evidence-stamped, replacing today's silent fall-to-`"4.0"`:

1. **XSD citation (strongest file-local signal):** if the file cites `xsd:noNamespaceSchemaLocation="…x3d-N.xsd"`, infer Vd := N. (Corpus: 286/300 `version='3.0'` files cite `x3d-3.0.xsd` — declared version and citation are concordant.)
2. **Node-floor:** else if the file uses any node/field first added at version V (from the §5 delta), infer Vd := **max** such V. **Build-order note (Critique-3):** this step *depends on the per-version deltas already existing*, so the inference subsystem (VP-2) is correctly sequenced **after** the manifests+deltas (VP-0/VP-1). No cycle: deltas are built first as data, inference consumes them later.
3. **Profile-floor:** else if a `PROFILE` is present, infer the lowest X3D version defining that profile.
4. **Lowest-supported floor:** else Vd := **3.0** (maximally permissive structurally), NOT 4.0 — defaulting unversioned files to 4.0 judges 96%-3.x-era content against the newest, strictest oracle.

**Inference confidence is tiered, not flat (Critique-2 fix).** The stamp is not a single `VERSION_INFERRED`; it is one of three, so downstream consumers can weight snapshot reliability:

- `VERSION_INFERRED_XSD_CITED` — high confidence (step 1).
- `VERSION_INFERRED_NODE_FORCED` — high confidence, a used node forces a floor (step 2).
- `VERSION_INFERRED_BARE_FLOOR` — **low** confidence, pure 3.0-floor guess (step 4). L3 snapshots on bare-floor files are flagged low-trust; default-skew abstention (§3) is mandatory for them.

An inferred version NEVER masquerades as declared.

---

## 2. The three version axes — tagging and reconciliation

| Axis | Symbol | What it is | Stamped in | Binding rule |
|---|---|---|---|---|
| **Document** | **Vd** | per-file declared/inferred version | report `version`; snapshot `meta.documentVersion` | detected/inferred per §1 |
| **Criteria / UOM-oracle** | **Vc** | which per-version UOM manifest the L2 criteria derive from | report `uomCriteriaVersion` + `uomManifestHash` | **Vc := Vd, ALWAYS** |
| **Runtime build** | **Vr** | which single UOM the C++ runtime was compiled from | report `runtimeUomVersion` + `runtimeCommit` | **fixed at one superset build** (§4) |

**Normative binding: `Vc := Vd` always; `Vr` held constant.** Each document is judged against its OWN version's manifest at zero rebuild cost (manifest is data). Four divergence cases, each handled:

- **(A) Vc==Vd ≠ Vr — the NORMAL L2 case.** A 3.1 doc gets 3.1 criteria (data) on the 4.1 runtime (backward-compat). Both stamped.
- **(B) Vc==Vd==Vr** — doc matches the build (a 4.1 doc on the 4.1 superset).
- **(C) Vd unknown** → Vc := inferred-Vd (§1), tiered `VERSION_INFERRED_*`.
- **(D) Vd > Vr** (a 4.2 doc on a 4.1 build) → forward-compat (§6): L2 against the 4.2 manifest **iff downloadable+meta-validated**, else **L2-abstain** (never false-pass); L3 graceful (§6).

**Reconciliation:** the three are mechanically separable and **non-collapsible**; each gets its own stamp. L3 snapshots are **double-tagged (Vd, Vr)** so a backward-compat regression (4.1 runtime mis-reading a 3.x doc) is attributable to the *runtime build*. **But double-tagging is documentation, not repair (Critique-2):** a wrong baked default is already wrong *before* it is tagged. §3 specifies the actual repair (abstention), which the tag alone does not provide.

---

## 3. Per-version own-oracle validation, decoupled from the compiled runtime

The decoupling rests on an asymmetry: **L2 is pure data and version-decoupled at the criteria boundary; L3 must run compiled code and is honestly Vr-coupled.**

### 3a. L2 (structural) — version-decoupled by being DATA

Three-tier pipeline:

- **T1 — per-version UOM manifest.** Download `X3dUnifiedObjectModel-{3.0,3.1,3.2,3.3,4.0,4.1}.xml`; run the EXISTING extractor (`parser.py` + `version.py` auto-detect) over EACH. BUILT-verified extractor vocabulary: `findall('.//ConcreteNodes/ConcreteNode')`, `AbstractNodeTypes/AbstractNodeType`, `AbstractObjectTypes/AbstractObjectType`, `.//field`, `componentInfo`, `containerField`, `Inheritance`, `AdditionalInheritance`, `InterfaceDefinition`, plus attrs `acceptableNodeTypes/inheritedFrom/minInclusive/maxInclusive` (`parser.py:70,98,110,133,148,151,179,183,187`). Each manifest is committed JSON pinned `{uomVersion, uomManifestHash=sha256(source), sourceUrl, fetchedDate}`.
- **T2 — version-resolver** maps each document to its manifest via Vd.
- **T3 — L2 validator** loads the manifest matching the **document (Vd), never the runtime (Vr)**. *That one sentence is the entire decoupling.*

**Acknowledged INGEST-boundary runtime-dependence (Critique-2, new gap closed).** L2's *criteria* are runtime-independent, but L2's *input node-list* comes from the 4.1 runtime's parse. The BUILT silent-skip path — `NodeBuilder::beginNode` returns `nullptr` for an unknown type and the caller skips it; unknown fields are "silently skipped" (`NodeBuilder.hpp:10,27,53,69,80`) — means a 3.x node the 4.1 build does not model (removed/renamed) is **invisible to L2, not failed by it**. So "pure-data L2" is criteria-decoupled but **ingest-coupled**. **Fix (DESIGNED-UNBUILT):** add a **wire-vs-parsed reconciliation** — the validator diffs the raw wire node/field set (a cheap tokenizer pass, independent of `NodeBuilder`) against the runtime's parsed set and emits `NODE_DROPPED_BY_RUNTIME` / `FIELD_DROPPED_BY_RUNTIME` for the difference, so a Vr-dropped node surfaces as a stamped finding instead of vanishing before L2.

### 3b. The x3d-N.xsd cross-check — RE-PURPOSED and DE-SCOPED

The UOM is **autogenerated FROM the XSD** via Web3D's `BuildX3dUnifiedObjectModelXmlFile.xslt`. So the XSD is the UOM's **parent, not an independent sibling** — their agreement is XSLT-fidelity, not independence. Two consequences:

1. **Demoted to a fidelity cross-check** (resolved; the report states plainly "UOM and XSD share XSLT lineage"). Its real jobs: catch constraints the flattening XSLT drops (`xs:pattern` regexes, `xs:min/maxInclusive` ranges, `xs:choice` content models), and serve as the **independent node/field COUNT floor** for the meta-validator (§6).
2. **NOT a standing per-version artifact (Critique-3).** A non-independent cross-check that shares a parent does not justify carrying+pinning six large XSDs at launch. The XSD cross-check is a **one-time, on-admission audit**: when a manifest is first admitted, run the XSD count-floor + constraint-drop audit, **record the result and the XSD sha into the manifest's provenance block, then discard the XSD**. We pin the *audit result and the XSD hash*, not the standing file. This removes six files + six pins from the recurring ledger for near-zero loss.

Genuinely-independent oracles remain: the one-time ISO-19775-prose hand-check per spec-rev (kept; see §3c for why it is load-bearing), and the non-gating differential-vs-other-runtime column.

### 3c. L3 (behavioral) — version-coupled, Vr-tagged, with a DATA-DRIVEN abstention trigger

L3 runs the single superset runtime's backward-compat. The central correctness hole (Critique-2, verified): **default-elision over 3.x docs produces invisibly-wrong snapshots.** A 3.x `HAnimHumanoid` that legally omits `version` (true default 1.0) is materialized by the runtime as the baked `2.0` and then default-elided — the snapshot silently asserts 2.0. Same pattern for the `emissiveColor` 0/1 line across the 3.x→4.x boundary. Double-tagging does not repair this; the value is wrong before tagging.

**Resolution — the cross-version default/constraint-diff table drives mandatory abstention (DESIGNED-UNBUILT):**

1. **Build the skew table mechanically** by diffing the six UOMs' `field@default` / `@minInclusive` / `@maxInclusive` / `acceptableNodeTypes` attributes (§5). Every field whose default or constraint differs across the 3.x/4.x line is enumerated (`HAnimHumanoid.version`, `*.emissiveColor`, et al.).
2. **L3 ABSTAINS, it does not snapshot, for any default-elided occurrence of a skew-table field on a doc where Vd ≠ Vr.** It emits `BEHAVIOR_DEFAULT_VERSION_SKEW` (a stamped non-result), NOT a snapshot that bakes the Vr default into a Vd document.
3. The skew table **is** the previously-undefined abstention trigger (Critique-2 "abstention has no detector"). Abstention is now data-driven, not aspirational: a field is in the table or it is not.
4. The appinfo-prose-only lifecycle that the *structural* diff cannot see (e.g. `GeoOrigin` removed-in-3.3/restored-in-4.0, documented only in prose at `GeoOrigin.hpp:16-18`) is the reason the **ISO-prose hand-check per spec-rev is retained as the backstop** for L3 — see §5's demoted ripple claim.

L3 over the 3.x-majority corpus is therefore **scoped and honest**: it snapshots where the runtime is provably backward-faithful (no skew-table field elided) and abstains where it is not. It never over-claims behavioral conformance on 3.x. (Alternative considered and deferred: generate Vd-parameterized default accessors — real code, defeats the zero-edit framing for L3, revisit only if abstention coverage proves too thin.)

---

## 4. Superset-build vs per-version-build — DECIDED

**Decision (resolved): ONE superset runtime build (promote genuine 4.1 to first-class Vr) + per-version DATA manifests for 3.0..4.1. Per-version *runtime* builds rejected.**

Rationale:
1. **L2 needs no runtime of version V at all** — the manifest is data; per-version L2 builds buy nothing.
2. **For L3, N parallel runtimes would 6× build/CI cost, fork codegen output, and STILL not be more correct** than one backward-compatible 4.1 superset (X3D 4.1 is *designed* to read 3.x per ISO).
3. The superset + Vr-tagging + §3c abstention is the **honest** design: a backward-compat defect surfaces as a stamped (Vd, Vr) L3 finding or a `BEHAVIOR_DEFAULT_VERSION_SKEW` abstention — a useful signal, not a hidden pass.

The moat claim ("conformance moves with the spec, per-version, zero edits") is carried **entirely by the L2 data layer where it is true**; L3 makes the weaker, explicitly-Vr-tagged, abstention-guarded backward-compat claim.

---

## 5. Per-version delta + computed ripple-set — split by level

The single 4.0→4.1 ripple demo generalizes to "swap the manifest → criteria re-derive," but the ripple claim is **split by validation level** because the prior synthesis over-claimed L3 ripple as a static set (Critique-3):

- **5 pairwise UOM diffs as committed artifacts:** 3.0→3.1, 3.1→3.2, 3.2→3.3, 3.3→4.0, 4.0→4.1. Each diff = mechanically-computed {nodes ±, fields ±, **defaults changed**, **ranges/acceptableNodeTypes changed**, deprecations}.
- **Verified delta to encode (BUILT-checkable):** 4.0→4.1 = **+6 nodes / −0** (EnvironmentLight, FontLibrary, HAnimPose, InlineGeometry, RenderedTexture, Tangent). 3.x additive deltas follow the component annexes.
- **Two delta classes needing ripple handling:**
  1. **Changed defaults** — the marquee skew case (feeds §3c's abstention table).
  2. **Removals/deprecations** — `TwoSidedMaterial` deprecated → `Appearance.backMaterial` added (3.3→4.0). Since Vc:=Vd, removals only bite in forward-compat (D) and the superset-L3 ingest case (§3a reconciliation).

**Ripple, split by level (Critique-3 fix):**

- **L2 ripple = a static set-diff (BUILT-tractable, kept).** "Observed L2 criteria/report delta == COMPUTED ripple set of that UOM diff" is a true, mechanical equality. Demoted wording: it equals the **STRUCTURALLY-VISIBLE** ripple set. Appinfo-prose-only changes are explicitly NOT mechanically captured and are backstopped by the per-spec-rev ISO-prose hand-check (§3c).
- **L3 ripple is NOT a static set.** A changed default on a ROUTEd field propagates through the cascade in ways a static node-usage scan cannot predict. The changed-default ripple **demo is scoped to L2 only**. For L3, the skew table (§3c) drives *abstention*, not a ripple prediction. We do NOT claim a computed L3 ripple, and "flag for human review" is NOT used to backfill an unspecified L3 computation.

**Headline (corrected):** "drop in any of the six official ISO UOMs → the L2 structural criteria re-derive mechanically, zero edit; and for any version pair, the observed **L2** criteria/report delta == the **structurally-visible** computed ripple of that UOM diff." Strictly stronger than the single-bump demo, on downloadable genuine artifacts, with the L3 and prose-only caveats stated, not hidden.

---

## 6. Forward-compat + zero-edit re-derivation + GUARDED LIMITS (the meta-validator)

This section is where the prior synthesis was most over-stated. **Every zero-edit assurance below is guarded by a meta-validator that DOES NOT EXIST today** (verified: the only matches for "meta-validator" across the repo are in the M3 design doc; zero implementation). Until built, "fail-loud-on-restructure" is vacuous. The decisions are resolved; the artifacts are DESIGNED-UNBUILT and are **launch blockers for any zero-edit claim.**

### 6a. The extractor degrades silently on drift — verified

`parser.py::_parse_fields` does `print(...) + continue` on an unmapped field type (`parser.py:72-75`) — the field vanishes from the node, hence the manifest, hence is never validated. And `findall` returning `[]` (e.g. if Web3D renames `ConcreteNodes`) is **indistinguishable from a legitimately empty section** — the manifest is built *smaller* and **passes MORE documents**. This is exactly the "green run against a subtly-wrong manifest = invisible rot" the meta-validator must catch. The field-type table is a hand-authored 41-entry `FIELD_TYPE_MAPPING` + `XS_TYPES` (`generator.py:16,63`); the X3D SF/MF universe is closed (low risk), but `xs:*` base types are an open category where a new one is silently dropped.

### 6b. The meta-validator — specified, and its required tests (DESIGNED-UNBUILT, LAUNCH BLOCKER)

Per-version-parameterized; **no manifest is admitted until it passes.** It MUST:

1. **Assert presence + cardinality of every XPath the extractor reads** (the exact list in §3a) and **every attribute** (`@type/@accessType/@default/@minInclusive/@maxInclusive`, `componentInfo@name/@level`, `Inheritance@baseType`, `acceptableNodeTypes`, `inheritedFrom`).
2. **Enforce a NODE-COUNT / FIELD-COUNT FLOOR per version** (e.g. 4.0 must yield ~260 ConcreteNodes; a manifest extracting 12 nodes must FAIL). This is the *only* defense against silent under-extraction, since `findall == []` looks identical to an empty section.
3. **Cross-check extracted node-count against the x3d-Vd.xsd element count** as the independent floor (the de-scoped on-admission XSD audit of §3b feeds exactly here).
4. **Run the extractor in a STRICT MODE that RAISES (not `print`+`continue`) on an unmapped field type** during manifest generation, so a future `xs:*` halts the build instead of silently shrinking the oracle.
5. **Have a guard-the-guard regression:** feed a deliberately schema-mutated UOM (one renamed element) and assert the meta-validator FAILS. This is the only proof the guard actually guards.

### 6c. Zero-edit re-derivation and its honest boundary

**Mechanism (DESIGNED-UNBUILT):** ISO ships 4.2 → download `X3dUnifiedObjectModel-4.2.xml` → run extractor (zero edits) → **meta-validator must pass** → commit manifest + provenance. L2 conformance for 4.2 ships before any C++ recompile.

**The boundary, generalized (Critique-3 — lead with this, not the enumerated six):**

- Zero-edit holds for **new INSTANCES of constraint categories the extractor already models** (a new node, a new field with an existing SF/MF type + range/enum/containerField, a changed default).
- Zero-edit does **NOT** hold for a **new CATEGORY of constraint**. **4.0-over-3.x is the worked example of a category change** (UnlitMaterial / PBR / `backMaterial` semantics), NOT merely a "changed default." A category-introducing major bump needs a **bounded, named extractor extension** — and the meta-validator (6b) is what FIRES to tell you a category appeared, instead of silently mis-extracting.
- **The meta-validator is the boundary detector.** Forward-compat is "zero-edit" only until the UOM schema *shape* restructures, at which point the meta-validator fails LOUD and forces the extractor update. Fail-loud-on-restructure beats silent-rot — but only once 6b is built.

### 6d. Forward-compat runtime gracefulness — DESIGNED-UNBUILT, today it is silent data loss

The prior synthesis called the runtime's unknown-node skip "partially real" forward-compat. **Verified false in the conformance sense:** `NodeBuilder::beginNode` returns `nullptr` and the caller skips; a dropped unknown **parent silently discards its KNOWN children**, orphaning their DEF/USE targets and ROUTEs, with zero diagnostic (`NodeBuilder.hpp:53,69,80`). So a 4.2 node in a 4.1 build is dropped from the scene graph, and the loss is **not even bounded to the unknown node.** "Graceful" is true only in the trivial doesn't-crash sense.

**Policy (resolved) + work (DESIGNED-UNBUILT):** a well-formed node from a version newer than Vr → **L0 pass + `NODE_UNRECOGNIZED_FORWARD_COMPAT`**, never an L2 failure. The skip MUST **record the dropped subtree's known children as `ORPHANED_BY_FORWARD_COMPAT_DROP` findings** (reusing the §3a wire-vs-parsed reconciliation), so the loss is stamped and bounded-reported. Until this is built, the doc does **not** describe forward-compat as real. **Vd > Vr L2 forward-validation** requires the newer manifest downloadable + meta-validated; otherwise L0-pass + forward-compat-stamped + **L2-abstain**, never false-pass.

### 6e. External-dependency hardening — offline-first (Critique-1 fix)

The forward half cannot silently depend on web3d.org being up, unchanged, and URL-stable (the live 4.1 UOM is "under development"/mutable per G2). Therefore: **pin an OFFLINE copy of each admitted UOM (and the XSD audit result + XSD hash) in-repo with sha.** "Manifest source unreachable" is treated as **L2-abstain, never silent-pass.** The downloadable URL is a *refresh* mechanism, not a *runtime* dependency.

---

## 7. Versioned goldens / snapshot / metric

- **Every** record and snapshot is tagged `{Vd (+ inference tier if inferred), Vc + uomManifestHash, Vr + runtimeCommit, corpusId}`. L3 snapshots are double-tagged (Vd, Vr).
- **Default-elision keeps snapshots version-tolerant for unchanged defaults** — but a **changed default across a bump is handled by §3c abstention, not by silent baking.** This is the key correction: elision is version-tolerant only for non-skew fields; skew fields abstain.
- **Metric = (Vd × component × level).** Profile axis dropped (no UOM oracle). Denominator from version-Vd's OWN manifest (a 4.1-added component simply has no 3.x cell — legitimate). **Absolute counts dominant, percentages secondary** — the corpus is XSD-3.1-dominant (~11,485 / 11,270 cite `x3d-3.1.xsd`), so any frequency-weighted headline collapses to "3.1/Immersive" and is heavily caveated; the per-cell map stays LOUD so the 4.x/Full long tail is visible.
- **Report the L2 (moat, version-decoupled) plane and the L3 (runtime, Vr-tagged, abstention-guarded) plane SEPARATELY and LABELED.** A `runtimeUomVersion` provenance column per cell makes it literal: "3.1 docs, judged vs 3.1 manifest, executed on 4.1 runtime."
- **Pin the UOM source-hash AND the XSD-audit result+hash per version** (the XSD itself is not carried — §3b), so "which exact spec bytes produced this manifest + cross-check" is reproducible a decade out.
- Scorecard = append-only in-repo JSONL keyed `(corpusId, uomVersion, runtimeCommit)`; per-file snapshots out-of-tree, content-addressed by modelHash (M3 §8).

---

## 8. The legacy (VRML97) floor — fenced into the legacy-mapping subsystem

Per Critique-3, this is **reader/resolver work, not manifest-data work**; it ships in VP-2 as part of the version-resolution & legacy-mapping subsystem, decoupled from the manifest mechanism.

- **Bug fix (BUILT-confirmed defect):** VRML97 `.wrl` flows toward an **invalid** `#X3D V2.0` writer header (X3D has no V2.0). Remap at read time to a valid X3D version.
- **Floor:** VRML97 maps to **X3D 3.0** (resolved), applying the field-keyword renames (eventIn/field/exposedField/eventOut → inputOnly/initializeOnly/inputOutput/outputOnly) and node-name dialect remap. Vd(VRML97):=3.0; Vc:=3.0 manifest; the `.wrl` gets a real structural oracle instead of parse-only.
- **Stamp** `VERSION_MAPPED_FROM_VRML97` so the 3.0 attribution is transparent.
- **Open enumeration work:** the VRML97→X3D node/field mapping table (606 `.wrl` files) enumerated against the 3.0 UOM. `#VRML V1.0` continues to throw (`Vrml97Reader.hpp:131`); missing header → V2.0 → 3.0.

---

## 9. Phased plan + maintenance ledger + open decisions

### 9a. The HONEST maintenance ledger (Critique-3 — replaces "a download, not a build")

Per version admitted, the standing recurring cost is:

| Artifact | Count/version | Recurring? |
|---|---|---|
| UOM source (offline-pinned) + sha | 1 + 1 | yes |
| XSD **audit result** + XSD hash (XSD itself NOT carried — §3b) | 1 | one-time on admission |
| Pairwise UOM-diff artifact | n−1 across the set | yes |
| L2 ripple-set (computed, committed) | per diff | yes |
| **Meta-validator PASS** (blocks admission; can force an extractor edit, per G3) | 1 | yes, gating |

So six versions ≈ **6 UOM pins + 6 XSD audit records + 5 diffs + 5 ripple-sets + 6 meta-validator passes**, with the explicit caveat that the meta-validator on 3.x shape (G3) **can force extractor code** — i.e. "zero edits" is contradicted by a remaining gate, and that contradiction is surfaced here for ratification, not buried. This is the real recurring burden the human ratifies — not "6 JSON files."

### 9b. Phases (MVP cut to the minimal thesis proof)

- **VP-0 — the future-proof thesis, minimal, on real data (LAUNCH).** {4.0 manifest, genuine **4.1** manifest, **meta-validator built + passing on both** (§6b — the linchpin, built BEFORE any zero-edit claim), the Vd/Vc/Vr triple-stamp, the **L2** computed-ripple demo on the 303 real 4.1 + 259 real 4.0 corpus files}. This ALONE proves "drop in the next ISO UOM → criteria re-derive, zero edit" on genuine artifacts, with minimal standing inventory. Resolves OD-2 (promote 4.1), OD-10 (forward-compat policy). **Forward-compat runtime stamping (§6d) and wire-vs-parsed reconciliation (§3a) land here too**, because without them VP-0's forward-compat claim is silent data loss.
- **VP-1 — CONFORMANCE BREADTH, demand-driven (NOT launch, NOT "future-proofing").** Relabelled per Critique-3: the four 3.x manifests + metric extension prove *coverage*, owned by M3's metric, **not** the moat. Pull a 3.x manifest only when someone needs to score that version (demand-driven), each meta-validated + sha-pinned + diffed; build the §5 skew table as 3.x manifests land (it feeds §3c L3 abstention). Resolves OD-3 as option (a) but **incrementally**, not a launch batch of four.
- **VP-2 — version-resolution & legacy-mapping subsystem (separate design surface).** §1 inference ladder (tiered stamps), §8 VRML97→3.0 floor + read-time remap fix, §3b XSD on-admission audit wiring, §3c L3 abstention fully wired. Sequenced after VP-0/VP-1 because §1 node-floor inference consumes the §5 deltas.

### 9c. Open decisions — RESOLVED vs DESIGNED-UNBUILT, re-labelled against reality

1. **Vc:=Vd always; Vr fixed at one 4.1 superset — RESOLVED (§2/§4).** No per-version runtime builds (they buy nothing). *Ratify the choice.*
2. **Unversioned-file rule = the §1 ladder, floor 3.0 not 4.0, tiered stamps — RESOLVED, DESIGNED-UNBUILT (VP-2).** *Ratify the policy; note the resolver is unbuilt.*
3. **VRML97 floor = X3D 3.0 — RESOLVED, DESIGNED-UNBUILT (VP-2).** *Ratify 3.0 (vs a higher 3.x).*
4. **XSD = on-admission fidelity audit, not a standing per-version artifact, not an independent oracle — RESOLVED (§3b), overrides M3 OD-1 pillar-2.** *Ratify the report wording "UOM and XSD share XSLT lineage."*
5. **Forward-compat = graceful + STAMPED — POLICY RESOLVED, MACHINERY DESIGNED-UNBUILT (§6d).** *Ratify the policy; note `NODE_UNRECOGNIZED_FORWARD_COMPAT` + orphan-recording are unbuilt and that today the path is silent data loss.*
6. **The meta-validator, wire-vs-parsed reconciliation, strict-mode extractor, and L3 default-skew abstention — DESIGNED-UNBUILT, LAUNCH BLOCKERS for the respective claims (§3a/§3c/§6b/§6d).** *Ratify that no zero-edit, no decoupled-L2, and no L3-conformance claim is made in any published M3 result until these are built and the guard-the-guard regression (§6b.5) is green.*

### 9d. Empirical gates (cheap, must run before reliance)

- **G1:** byte-download 3.1 and 3.2 UOMs and count ConcreteNodes (3.0/3.3 confirmed real; 4.x on disk). High-confidence real (root `@version` read clean; headers self-describe as full listings), but **byte-verify node content before promoting to first-class manifests.** Prerequisite for VP-1, not a footnote.
- **G2:** re-pin the 4.1 fixture to a sha of a dated live download (live 4.1 is mutable; key detection on the ROOT `@version`, not a naive grep — the fixture's non-root token scans as "1.0" yet the +6-node delta + Web3D header confirm genuine 4.1). Prerequisite for VP-0.
- **G3:** confirm the existing extractor runs UNMODIFIED over a downloaded 3.1 UOM — the strongest test of the data-decoupling claim. **If the 3.x ConcreteNode/componentInfo shape differs from the 4.x extractor's assumptions, the meta-validator (§6b) MUST flag it** (that is its job), and an extractor edit is then required — making explicit that "zero-edit" is bounded by G3's outcome.
