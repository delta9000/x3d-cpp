# `x3d` CLI — convert + validate, with a captured-golden differential gate vs. Web3D Java tools

**Date:** 2026-06-20
**Status:** Approved design; ready for implementation.
**Goal:** The first real *consumer* of the SDK — a thin `x3d` command-line tool — that proves the SDK does real work AND is differentially validated against the Web3D Consortium's own Java reference tools (X3DJSAIL, Xj3D).

## Why this, why now

The binary/ingestion stack now runs end-to-end but has no consumer; the SDK has been built *ahead* of the CAVE (its stated forcing function). A CLI suite is a lighter forcing function that still exercises the public `x3d::sdk` façade end-to-end, dogfoods its ergonomics, and ships tangible value. "Prove it" = the tool agrees with the canonical implementation across thousands of real files, not just our own tests.

**First cut (decided): `convert` + `validate` only** — the two commands with clean Java oracles. `info`/`extract`/`deps` (no clean oracle) come later.

## The Java oracles (verified)

- **X3DJSAIL** (`org.web3d.x3d.jsail.CommandLine`) — the official X3D Java Scene Access Interface Library. `-validate` (validation always performed), `-canonical` (X3D Canonicalization / X3DC14N), and `-toXML / -toClassicVrml / -toJSON / -toVRML97 / …`. THE oracle for both commands.
- **Xj3D** — the Web3D Java X3D/VRML97 runtime; command-line translator (XML ⇄ ClassicVRML ⇄ VRML97 ⇄ binary) + a Validator (the one X3D-Edit launches). An independent second reference for cross-checking.

## Architecture

A thin `x3d` binary in `tools/` (alongside `tools/corpus_sweep.cpp`), built by CMake, layered entirely over `x3d::sdk`. Homegrown arg parsing — no new dependency. `x3d <subcommand> [args]`, git/ffmpeg-shaped. Commands are deliberately thin glue: the SDK does the work, which is the point (dogfood the façade, surface gaps). **No `util` library yet** — convert/validate are near-pure façade calls; a reusable `x3d::util` layer earns its keep later with `extract`/`deps`.

```
tools/x3d_cli.cpp            # main + subcommand dispatch + arg parse
  convert: sdk::parseFile -> {Xml,Vrml,Json}Writer
  validate: aggregate SDK diagnostics + profile-fit -> report + exit code
```

## `x3d convert <in> [-o <out>] [-f xml|vrml|json]`

- Encoding inferred from extensions: `.x3d`→XML, `.x3dv`→ClassicVRML, `.wrl`→VRML97, `.json`→JSON; `-f` forces the output encoding; gzip transparent (SDK inflates + round-trips already).
- Body: `sdk::parseFile(in)` → the matching writer (`XmlWriter`/`VrmlWriter`/`JsonWriter`) → `out` (or stdout if no `-o`).
- Exit non-zero on parse failure; surface range/proto warnings to stderr (non-fatal).

## `x3d validate <in>` → verdict + report + exit code

Aggregates what the SDK already produces, plus one new check:
- **Range diagnostics** (`collectRangeWarnings`), **proto/inline warnings** (`protoWarnings`/`inlineWarnings`), **version-inference notes**.
- **Profile-fit** (new, small): given the scene's node + component set, report the *minimal* X3D profile it fits (Core / Interchange / Interactive / Immersive / Full) and flag nodes that exceed a declared profile.
- Output: human-readable report by default; `--json` for machine form. Exit 0 = clean, non-zero = issues found. This is the command with the real Java oracle.

## The captured-golden differential gate (the heart of "prove it")

Matches the project's "golden files in git" ethos: run the Java diff **once** over a curated corpus subset, commit the reference outputs/verdicts as fixtures, and gate our CLI against them in **Java-free CI**.

**Curated subset:** a few hundred files from the conformance archive spanning all encodings, all profiles, and the known-gnarly cases (the corpus the SDK already sweeps). Listed in `tools/x3d-cli/goldens/manifest.txt`.

> **De-risked 2026-06-20 (smoke-tested X3DJSAIL.4.0.full.jar, JDK 17):** `-validate` works perfectly (`validate results: success, no problems noted`); `-toJSON` works; BUT `-toClassicVrml`/`-toVRML97` hit a known X3DJSAIL XSLT-stylesheet-extraction quirk (`getTempFileFromX3dJsailJar … 'X3DJSAIL.jar' not found in CLASSPATH`) — a rabbit hole we will NOT chase. So the convert-differential is **pivoted off X3DJSAIL's converter** onto its rock-solid `-validate`: we don't compare to their conversion *output*; we use them to **validate OUR output** (right thing to test — our converted files are valid X3D per the reference — and encoding-agnostic + quirk-free). Working invocation: `cd <jarcache> && java -cp X3DJSAIL.jar org.web3d.x3d.jsail.CommandLine <file> -validate`. (Exact-output comparison via Xj3D's translator = documented follow-up.)

**Phase A — golden-gen (Java, on demand): `mise run cli-golden-gen`**
- Needs a JDK + the X3DJSAIL full jar (fetched/cached into a gitignored dir; ~65 MB).
- Per subset file, captures a committed fixture: **X3DJSAIL's `-validate` verdict** → a normalized record (valid/invalid + issue categories). (Xj3D cross-check = follow-up.)

**Phase B — gates**
- **validate (permanent, Java-free CI): `mise run cli-gate`** (wired into `mise run ci`) — run our `x3d validate`; diff our verdict against the captured reference verdict per file. Divergence = our validator disagrees with the Web3D reference → a bug, or a documented/justified difference (an allowlist with reasons). **The killer differential test.**
- **convert (permanent, Java-free CI):** our-own round-trip equivalence — for each subset file, `x3d convert` to each target encoding, **reparse our output through our own reader and compare the resulting `Scene` to the source's `Scene`** for semantic equivalence (same nodes/fields/routes/DEF graph). No Java, no X3DC14N. (Accepted trade: a reader bug normalizes both sides identically and could mask a divergence; the validate gate + corpus round-trip audit cover reader correctness independently.)
- **convert-validity (periodic Java confidence snapshot, in golden-gen):** run X3DJSAIL `-validate` on OUR convert *outputs* → confirms our converted files are valid X3D per the reference. Captured as a snapshot report, NOT a permanent CI gate (keeps CI Java-free).

**The first golden-gen run's divergence list IS the "prove it" deliverable** — it's the worklist of places our CLI disagrees with the canonical implementation, each either a real bug to fix or a justified, documented difference.

## Scene-equivalence comparison

A small `sceneEquivalent(Scene&, Scene&)` helper (used by the convert gate): compares root-node structure, per-node type + field values (normalized: numeric tolerance, MF ordering as authored), the ROUTE set, and the DEF/USE graph. Lives in the gate harness, not the SDK (it's test infrastructure). Reuses the SDK's reflection (`fields()`) to be node-agnostic.

## Error handling

- CLI: clear stderr messages + meaningful exit codes (0 ok, 1 usage error, 2 parse/IO failure, 3 validation issues). Never crash on malformed input (the SDK is lenient; the CLI surfaces diagnostics).
- golden-gen: a file X3DJSAIL itself rejects/can't convert is recorded as such (reference-says-invalid is a valid fixture, not a harness error).

## Testing

- **Unit** (C++, no Java, in `mise run ci`): arg parsing; `convert` round-trips a handful of in-repo fixtures across all encoding pairs; `validate` produces expected verdicts + profile-fit on crafted fixtures (clean, range-violation, profile-exceeding, unresolved-extern).
- **Differential** (the captured-golden gate above): the Java-validated layer.
- **`sceneEquivalent`** gets its own unit tests (equal scenes equal; a deliberately-mutated scene differs).

## Out of scope (first cut)

`info` / `extract` / `deps` (no clean Java oracle); `x3d::util` reusable library (earns its keep with those); implementing X3DC14N canonicalization (the parse-and-compare trick avoids it); EXI/binary encodings; the live-CI-Java option (rejected — keeps CI Java-free).

## Success criteria

1. `x3d convert` round-trips every encoding pair on the in-repo fixtures + reparses identically.
2. `x3d validate` produces a correct profile-fit + diagnostic report; `--json` machine-readable.
3. `mise run cli-golden-gen` produces committed fixtures from X3DJSAIL + Xj3D over the curated subset.
4. `mise run cli-gate` runs Java-free and passes (after the first divergence list is triaged: bugs fixed, justified differences allowlisted with reasons).
5. The divergence triage report is committed — the artifact that *proves* agreement with the Web3D reference.
