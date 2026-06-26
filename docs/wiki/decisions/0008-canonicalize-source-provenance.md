---
title: "ADR-0008: Canonicalize Source-Provenance Handling"
summary: x3d canonicalize uses parseFile (which derives baseUrl from the input path) and does not propagate any provenance metadata into the canonical output — the output is a pure content-hash-stable form.
tags: [adr, canonicalize, source-provenance, base-url]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/canonical-xml.md
  - ../subsystems/cli-suite.md
---

# ADR-0008: Canonicalize Source-Provenance Handling

## Status

Accepted — 2026-06-20

## Context

The `x3d canonicalize` command (Task D in the CLI suite) emits the X3D Canonical Form (X3DC14N): a deterministic, byte-stable XML serialization intended for diffing, deduplication, VCS storage, and digital signatures. Two independent forces shaped the source-provenance call.

**The base-URL asymmetry bug exposed a real invariant.** When the convert-roundtrip gate (`cli_gate.cpp`) first ran `parseDocument` on serialized text without a `baseUrl`, relative `<Inline url=.../>` and `<ExternProtoDeclare url=.../>` references stayed as unexpanded stubs, while the original `parseFile` had expanded them into `rootNodes` / `protoInstances`. The gate then reported spurious "not equivalent" verdicts. The regression test `gate_baseurl_roundtrip_test.cpp` reproduces both shapes (a `load=TRUE` Inline with a relative sibling + a scene-root EXTERNPROTO instance) and documents the required fix: any parse of a serialized form that must compare structurally with the source parse needs the same `baseUrl`. The convert gate now passes `fs::path(absPath).parent_path().string()` as `base` to `parseDocument`. This established that `parseFile` and `parseDocument(text, enc, base)` are the two branches of the same seam — `parseFile` derives `base` internally from the file path; `parseDocument` receives it as a parameter.

**The canonicalize path must be base-URL-resolved for round-trip fidelity, but must not embed provenance into the output.** A canonical form is supposed to be a stable content fingerprint. If the output contained the source path, the same logical scene stored at two different filesystem locations would produce different canonical outputs — defeating deduplication and byte-exact comparison. At the same time, if `canonicalize` parsed without a base URL, Inline and EXTERNPROTO expansions in the source file would be left as unexpanded stubs, and the canonical output would represent a different (shallower) scene than the source. The idempotence requirement (tier-1 hard gate: `canon(canon(x)) == canon(x)`) would also be at risk if the two parse passes used different base URLs.

**The Java oracle (X3DJSAIL `-canonical`) uses JDK 25.** `X3dCanonicalizer` requires class file version 69 (Java 25). The oracle is invoked by explicit path (`$(mise where java@temurin-25.0.3+9.0.LTS)/bin/java`) during golden-gen and is never activated project-wide. This means the oracle cannot be in the CI loop; the gate must be Java-free. The tiered differential gate (`canon_gate.cpp`) therefore makes idempotence (tier-1, always Java-free) the hard CI gate, tolerant-diff agreement with committed X3DJSAIL golden fixtures (tier-2) the baseline-locked structural gate, and byte-exact match (tier-3) an informative stretch metric only.

The spec (`docs/superpowers/specs/2026-06-20-x3d-canonicalize-design.md`) approved this direction after a de-risk run confirmed that JDK 25 produces `x3dCanonicalizer.isCanonical()=true` and that the observed X3DC14N form (single-quote delimiters, alphabetically sorted attributes, 2-space indent, minimal number format, DOCTYPE line) is stable.

## Decision

We decided that `x3d canonicalize <in>` always parses using `parseFile(inPath)` — which internally derives `baseUrl` from the input path's parent directory — and serializes using `CanonicalXmlWriter::writeDocument(doc)`. No provenance metadata (source path, timestamp, author) is written into the canonical output. The canonical output is a pure, reproducible content form: identical logical scenes at any filesystem location produce byte-identical canonical output.

Concretely:

- **Parse:** `sdk::parseFile(inPath)` (see `tools/x3d_cli.cpp`, `cmdCanonicalize`). This derives the `baseUrl` identically to the way the convert-roundtrip gate derives it (`fs::path(path).parent_path().string()`), ensuring Inline and EXTERNPROTO expansions are resolved before canonicalization.
- **Serialize:** `sdk::CanonicalXmlWriter::writeDocument(doc)` (`runtime/codecs/CanonicalXmlWriter.hpp`). The writer is a fully separate class from `XmlWriter`; the default writer path has zero risk of change (verified by golden zero-drift).
- **Output content:** XML prolog + version-appropriate DOCTYPE + `<X3D>` with attributes sorted alphabetically, single-quoted, X3DC14N-escaped; 2-space indent; minimal number format via `canonFmtFloat` / `canonFmtDouble` (shortest round-trip representation, integers without `.0` for floats but always with `.0` for doubles to distinguish from `SFInt32` in MF contexts); DEF/USE, PROTO/EXTERNPROTO, ROUTEs, IMPORT/EXPORT, head meta — all preserved, none augmented with provenance.
- **Gate:** tier-1 idempotence is a hard CI gate (`canon_gate.cpp --gate`). Tier-2 tolerant-diff agreement with committed X3DJSAIL golden fixtures is the baseline-locked regression gate. Tier-3 byte-exact match is informative only.

## Consequences

**Positive:**

- Canonical output is a stable content fingerprint: the same logical scene at any path produces the same bytes. This enables deduplication, VCS diffing, and cryptographic signatures over scene content rather than file location.
- `parseFile`-derived `baseUrl` guarantees Inline/EXTERNPROTO expansions are consistent between the source parse and any re-canonicalization — tier-1 idempotence (`canon(canon(x)) == canon(x)`) holds because both passes use the same structural base.
- The default `XmlWriter` path is completely unmodified — `CanonicalXmlWriter` is a separate class with its own attribute-sort and number-format logic. There is zero risk of canonical-mode changes drifting into round-trip or golden output.
- The tiered gate is Java-free in CI: tier-1 needs only the built `x3d` binary; tier-2 runs against committed X3DJSAIL golden fixtures. The oracle (JDK 25) is needed only at golden-gen time.
- The `double` / `SFDouble` / `SFTime` distinction (`canonFmtDouble` always emits a decimal point, `canonFmtFloat` uses an integer fast-path) was chosen to match X3DJSAIL's observed behaviour and to preserve type disambiguation in MF contexts — a subtle correctness invariant captured in code comments in `CanonicalXmlWriter.hpp`.

**Trade-offs / costs:**

- `x3d canonicalize` silently resolves and expands Inline/EXTERNPROTO content before emitting canonical form. A caller who expected the canonical output to represent only the top-level file's explicit content (not its transitive includes) will get more than anticipated. This is the correct behaviour for a content-fingerprint tool but may surprise users who think of canonicalization as a pure syntactic normalization.
- The canonical form is XML only (not ClassicVRML, JSON, or EXI). Non-XML encodings are accepted as input (all four encodings; gzip transparent) but the output is always X3DC14N XML. A "ClassicVRML canonical form" or "JSON canonical form" is out of scope for v1.
- Default-value omission behaviour — whether fields equal to their node default are omitted — matches `CanonicalXmlWriter`'s default-elision logic (same as `XmlWriter`), not necessarily X3DJSAIL's behaviour on a given field. Tier-3 byte-exact gaps from default-omission differences are tolerated; tier-2 (tolerant diff) handles them without requiring a matching policy.
- The JDK 25 constraint on the oracle (`UnsupportedClassVersionError` on JDK 17) means golden-gen cannot run in a standard CI environment without a pinned JDK 25 install via `mise`. This is documented in the de-risk findings and in `gen_canon_goldens.sh`.

## Related

- [Architecture](../architecture.md)
- [CLI Suite](../subsystems/cli-suite.md)
- [Canonical XML writer subsystem](../subsystems/canonical-xml.md)
- Design spec: `docs/superpowers/specs/2026-06-20-x3d-canonicalize-design.md` — full de-risk findings, tiered-gate design, and X3DC14N observed form
- `runtime/codecs/CanonicalXmlWriter.hpp` — the canonical serializer; `canonFmtFloat`, `canonFmtDouble`, `canonEscape`, `x3dDoctype`, `CanonicalXmlWriter::writeDocument`
- `runtime/codecs/XmlWriter.hpp` — the default writer; shares no code paths with `CanonicalXmlWriter` (separation is the invariant)
- `tools/x3d_cli.cpp` (`cmdCanonicalize`) — CLI entry point; shows `parseFile` + `CanonicalXmlWriter` wiring
- `tools/x3d-cli/canon_gate.cpp` — three-tier differential gate (idempotence + tolerant-diff + byte-exact)
- `tools/x3d-cli/gate_baseurl_roundtrip_test.cpp` — regression for the base-URL asymmetry bug that established the `parseFile`/`parseDocument(base)` seam contract
- `tools/x3d-cli/cli_gate.cpp` — convert-roundtrip gate; shows the `base = fs::path(absPath).parent_path()` pattern that `parseFile` mirrors internally
