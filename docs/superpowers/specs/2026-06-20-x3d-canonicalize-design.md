# `x3d canonicalize` — X3D Canonical Form (X3DC14N), differential vs X3DJSAIL

**Date:** 2026-06-20
**Status:** Approved direction (brainstormed); **de-risked** (oracle + target form confirmed). Ready to build.
**Goal:** Emit the X3D Canonical Form (X3DC14N) of a scene — a deterministic, byte-stable serialization for diffing / dedup / VCS / signatures — validated against X3DJSAIL's `-canonical`, the gold-standard *exact-output* oracle.

## De-risk findings (verified 2026-06-20)
- **Oracle works on JDK 25.** X3DJSAIL `-canonical` throws `UnsupportedClassVersionError` on JDK 17 (its `X3dCanonicalizer` needs class file 69 = Java 25). JDK 25 installs via `mise install java@temurin-25.0.3+9.0.LTS`; with it, `-canonical` runs (`x3dCanonicalizer.isCanonical()=true`) and writes `<name>Canonical.xml`. golden-gen invokes it by explicit path (`$(mise where java@temurin-25.0.3+9.0.LTS)/bin/java`) — NOT activated project-wide.
- **The X3DC14N target form (observed):**
  - XML prolog `<?xml version="1.0" encoding="UTF-8"?>` + the X3D **DOCTYPE/DTD** line preserved.
  - **Single-quote** attribute delimiters.
  - **Attributes sorted alphabetically** within each element (e.g. `<meta content='…' name='…'/>`; `<field accessType appinfo name type value>`).
  - **2-space-per-level** indentation; one element per line.
  - **Minimal number format** (`0.8`, `0`, `0 0 0` — no trailing zeros, no `0.0`).
  - DEF/USE, ProtoDeclare/field/connect, schema-namespace attrs on `<X3D>` all preserved.

## Architecture
A **canonical mode on the XML serializer**, default-OFF (so existing `convert`/round-trip output is byte-identical → golden-safe). Either a `CanonicalOptions`/flag on `runtime/codecs/XmlWriter.hpp` or a focused canonical emitter that walks the scene via reflection (`fields()`). Implementer picks the cleaner; the hard requirement: **zero change to the default writer path** (verify golden zero-drift + the existing codec round-trip tests stay green). The canonical rules:
1. Emit XML prolog + the version-appropriate X3D DOCTYPE.
2. Per element: emit attributes **sorted by name**, single-quoted, X3DC14N-escaped.
3. 2-space-per-level indent, one node per line; children nested.
4. **Minimal number formatting** matching X3D canonical (shortest round-trip representation; `0.8` not `0.800000`, integers without `.0`). This is the precision-sensitive part.
5. Preserve DEF/USE, PROTO statements, ROUTE, IMPORT/EXPORT, head meta, comments per X3DC14N.

`x3d canonicalize <in> [-o out.x3d]` (stdout if no `-o`); exit 0 ok, 2 parse/IO, 1 usage. Input any encoding (`parseFile` sniffs); output is always canonical XML.

## The tiered differential gate (manages the byte-exact risk)
Byte-matching another implementation's float `printf` + default-omission exactly is the genuine risk. So the gate is tiered, strongest-first, and we report each tier's rate:
1. **Idempotence (Java-free, permanent CI):** `canonicalize(canonicalize(x)) == canonicalize(x)` byte-for-byte, over a corpus subset. A canonical form MUST be a fixed point — this is a hard, Java-free correctness gate.
2. **Differential vs X3DJSAIL (JDK-25 golden-gen):** for each subset file, compare OUR canonical output to X3DJSAIL's `-canonical` output with a **tolerant text diff** — numbers compared within tolerance, insignificant whitespace normalized. Agreement here = our canonical form is structurally + numerically equivalent to the reference. This is the real differential and does NOT require replicating X3DJSAIL's exact float strings.
3. **Byte-exact (stretch metric):** the fraction of subset files where OUR canonical output is byte-identical to X3DJSAIL's. Report it; chase the gap (number format, attr escaping, default handling) opportunistically. NOT a hard gate — the tolerant diff (tier 2) is the bar.

golden-gen captures X3DJSAIL's canonical outputs for the subset as committed fixtures (XML, so they version cleanly); the gate (tiers 1+2) runs Java-free against them in CI; tier-3 byte-exact is reported from the same fixtures.

## Error handling / testing
- Unit: canonicalize crafted fixtures → assert the rules (sorted attrs, single-quote, DTD, minimal numbers); idempotence on each.
- Differential: tiers 1–3 over the subset; the first run's tolerant-diff agreement rate + byte-exact rate are the "prove it" deliverable.
- Never crash on malformed input (lenient parse → best-effort canonical + exit per code).

## Scope cuts (v1)
- **XML canonical form only.** Not ClassicVRML/JSON canonical; not EXI/binary (X3DC14N is the XML form).
- Default-value omission: match X3DJSAIL's behavior **if** it omits defaults (TBD-from-data during build — study more canonical samples); the tolerant diff tolerates a difference here, byte-exact would require matching it.
- The Java oracle is golden-gen-only (JDK 25 by path); CI stays Java-free.

## Success criteria
1. `x3d canonicalize <in>` emits a deterministic X3DC14N XML form following the observed rules.
2. **Idempotence holds** byte-for-byte over the subset (tier-1 hard gate).
3. **Tolerant-diff agreement with X3DJSAIL** is high over the subset (tier-2); the byte-exact rate (tier-3) is reported with the gap characterized.
4. Gates: `mise run build` green, golden zero-drift, default writer path byte-identical, generated/runtime-default untouched.
