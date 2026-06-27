---
title: "ADR-0007: Version-Inference Ladder"
summary: A deterministic version-inference ladder resolves the X3D spec version from file signals; VRML97 input floors to 3.0; sub-3.0 version tokens are clamped before reaching any writer.
tags: [adr, version-inference, vrml97, spec-version, ladder]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/parse-readers.md
---

# ADR-0007: Version-Inference Ladder

## Status

Accepted — implemented as M3 VP-2 (commits `7fc6a00`..`53db57a`, 2026-06-16).

## Context

The conformance pipeline must judge each document against its **own** version's
structural oracle (`Vc := Vd`). That binding is trivial when a file declares its
version explicitly, but the corpus contains a substantial tail of unversioned
files and VRML97 legacy input where no version declaration is present:

- The production corpus sweep revealed ~4,516 files that lacked an explicit
  `version` attribute or header token — roughly 133 files whose conformance
  validation was previously skipped for lack of a version binding.
- VRML97 `.wrl` files carry a `#VRML V2.0` header. X3D has no V2.0; if that
  string reached any writer, the output file would carry an invalid `#X3D V2.0`
  header that no standard parser accepts. This was a confirmed latent bug in the
  original `ClassicVrmlReader`/`VrmlWriter` pair.
- The C++ `X3DDocument::version` struct default was `"4.0"` before VP-2. An
  unversioned file parsed by any of the four readers (XML, ClassicVRML, VRML97,
  JSON) would silently inherit that default, judging 3.x-era content against the
  4.x oracle — an incorrect and invisible over-strictness.

Silently falling through to `"4.0"` was wrong because the corpus is dominated by
3.x-era files (the XSD-citation analysis found `x3d-3.1.xsd` cited in ~11,270
files). Defaulting to 4.0 would produce false failures on valid 3.x content.
Defaulting to 4.0 was therefore the wrong choice as a floor.

The design forces from `docs/superpowers/specs/2026-06-13-m3-versioning-design.md`
(§1 and §8) established three requirements:

1. **Deterministic resolution:** every file must produce exactly one version
   binding, never a silent miss that causes it to be skipped.
2. **Confidence-tiered stamps:** downstream consumers (the L3 behavioral plane)
   must be able to distinguish a high-confidence inferred version from a
   low-confidence bare-floor guess, so that low-trust snapshots can be flagged
   rather than silently promoted.
3. **No invalid version tokens in writers:** the string `V2.0` must never reach
   an `#X3D` header. The floor must be applied at the earliest point — the
   reader — not deferred to the writer.

The inference resolver is sequenced **after** the per-version manifests (VP-0/VP-1)
because rung 2 (node-floor) consumes the `node_floor_map()` derived from those
manifests. The VRML97 floor and writer defense are self-contained reader/codec
fixes with no manifest dependency, so they shipped in the same VP-2 batch.

## Decision

We adopted a five-rung (Rung 0–4) deterministic **version-inference ladder** for XML files
lacking a declared `version` attribute, and a **3.0 floor** applied at read time
for VRML97 and any sub-3.0 version token:

**Ladder rungs (Python: `src/x3d_cpp_gen/conformance/version_resolve.py`):**

1. **Rung 0 — declared version** (`VERSION_DECLARED`): if the XML root carries a
   `version` attribute, use it unchanged. Forward-compat files (version > the
   runtime build) are never clamped.
2. **Rung 1 — XSD/DTD citation** (`VERSION_INFERRED_XSD_CITED`, high confidence):
   if the file cites `xsi:noNamespaceSchemaLocation="…x3d-N.xsd"` or a DTD
   SYSTEM URL matching `x3d-N.xsd` / `x3d-N.dtd`, infer `Vd := N`. Validated
   by the `_CITE_RE = re.compile(r"x3d-(\d\.\d)\.(?:xsd|dtd)\b")` pattern.
   This resolved 100 of the 133 previously-skipped files in the corpus sweep.
3. **Rung 2 — node-floor** (`VERSION_INFERRED_NODE_FORCED`, high confidence):
   if any node in the document has its earliest-defining version (per
   `node_floor_map()`, derived from the committed per-version manifests) strictly
   above the bare floor (`3.0`), infer `Vd := max` of those earliest versions.
   This rung fires only when nodes force a version above the floor; ubiquitous
   nodes like `X3D`/`Scene` that exist from 3.0 onward carry no additional signal.
4. **Rung 3 — profile-floor** (`VERSION_INFERRED_PROFILE_FLOOR`): if a `PROFILE`
   attribute is present, map it through `_PROFILE_FLOOR` (e.g. `CADInterchange →
   3.1`, `MedicalInterchange → 3.2`; standard profiles floor to `3.0`).
5. **Rung 4 — bare floor** (`VERSION_INFERRED_BARE_FLOOR`, low confidence):
   infer `Vd := "3.0"`. This is the maximally permissive structural choice —
   judging an unknown file against the 4.x oracle would be strictly more wrong.
   L3 behavioral snapshots on bare-floor files are flagged low-trust.

**VRML97 and sub-3.0 floor (C++ runtime):**

- `ClassicVrmlReader::parseHeaderLine` (`runtime/parse/ClassicVrmlReader.hpp`)
  immediately assigns `doc.version = "3.0"` for any `#VRML`/`#VRML97` magic
  token, and clamps any sub-3.0 major (`major < 3`) to `"3.0"`.
- `Vrml97Reader::onHeaderLine` (`runtime/parse/Vrml97Reader.hpp`) sets
  `doc.version = "3.0"` on all three VRML branches: missing header, unrecognised
  magic, and the normal `#VRML V2.0` success path.
- `VrmlWriter::headerVersion` (`runtime/codecs/VrmlWriter.hpp`) applies a
  defensive second-layer clamp so that a hand-built `X3DDocument` with
  `version = "2.0"` still emits `#X3D V3.0 utf8`, never `#X3D V2.0`.
- `X3DDocument::version` struct default changed from `"4.0"` to `"3.0"`
  (`runtime/X3DDocument.hpp:78`), so unversioned files parsed by readers that
  do not emit a version token inherit the correct bare floor at the C++ layer too.

**VRML97 stamp:** the mapping is transparent — `doc.version` after parsing is
`"3.0"` and writers emit a valid `#X3D V3.0 utf8` header. The design document
specifies a `VERSION_MAPPED_FROM_VRML97` stamp for conformance reports
(designed, not yet wired into the Python conformance sweep as of VP-2 close).

## Consequences

**Positive:**

- The 133 previously-skipped corpus files now receive a version binding and are
  included in conformance validation (corpus sweep: `skipped_no_manifest` went
  from 133 to 0 after VP-2; `validated` count rose from 16,717 to 16,850).
- Version confidence is explicit and machine-readable: downstream consumers can
  filter out bare-floor (low-trust) files from L3 snapshots without re-examining
  the files.
- The `#X3D V2.0` writer bug is closed at both the reader (two layers: `#VRML`
  magic check + sub-3.0 major clamp) and the writer (defensive `headerVersion()`
  clamp), so the invalid header cannot appear even if a document is built by hand
  with a legacy version string.
- The C++ bare-floor default aligns with the Python conformance ladder — both
  floor to `"3.0"`, so the two planes agree on version binding for headerless
  input.
- The node-floor rung is data-driven off the committed per-version manifests
  (`src/x3d_cpp_gen/conformance/manifests/x3d-{3.0..4.1}.json`) via
  `node_floor_map()` in `src/x3d_cpp_gen/conformance/node_floor.py`, so it
  automatically reflects any new manifest admitted to the set.

**Trade-offs / costs:**

- **Rung 2 depends on manifest sequencing.** The node-floor rung cannot run
  until at least the 3.0 manifest exists; it is gated on VP-0/VP-1 manifest
  delivery. The rung is built but its coverage improves incrementally as more
  manifests are committed.
- **Bare-floor is a structural guess.** A file in rung 4 may be 3.x or 4.x;
  flooring it to 3.0 applies the least-strict oracle and accepts content that
  a later-version oracle would reject. The low-trust stamp (`VERSION_INFERRED_BARE_FLOOR`)
  surfaces this, but the guess cannot be verified without additional file signals.
- **Profile-floor (rung 3) is coarse.** The `_PROFILE_FLOOR` table maps only
  known profiles; an unknown profile name falls through to the bare 3.0 floor.
  The table must be maintained when new profiles appear in later spec revisions.
- **`VERSION_MAPPED_FROM_VRML97` stamp not yet wired into the conformance report**
  (logged as VP2-L1 in the deprecated, historical `docs/superpowers/BACKLOG.md`). The floor is applied
  correctly in C++; the report-level stamp is a follow-up.
- **VRML97 node/field dialect mapping** (the `Vrml97Dialect` rename table in
  `runtime/parse/Vrml97Dialect.hpp`) covers the known 606-file corpus. Any
  VRML97 node not in the rename table is silently skipped; this is consistent
  with the lenient-read policy but means unknown VRML97 nodes do not fail.

## Related

- [Architecture](../architecture.md)
- [Parse Readers subsystem](../subsystems/parse-readers.md)

Design record: `docs/superpowers/specs/2026-06-13-m3-versioning-design.md` (§1, §8, §9b VP-2).
Implementation plan: `docs/superpowers/plans/2026-06-16-m3-vp2-version-inference.md`.
Backlog follow-ups (VP2-L1/L2): the [GitHub Project](https://github.com/users/delta9000/projects/2).
