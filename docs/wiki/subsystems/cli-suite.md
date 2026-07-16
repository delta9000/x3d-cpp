---
title: CLI Suite
summary: The x3d CLI — convert, validate, extract, canonicalize, and sim commands; the first SDK consumer.
tags: [subsystem, cli, convert, validate, extract, canonicalize, sim]
updated: 2026-07-16
related:
  - ../architecture.md
  - ../guides/gate-system.md
  - ../decisions/0001-ext-firewall.md
---

# CLI Suite

The `x3d` CLI is a thin binary over the `x3d::sdk` façade — the SDK's first real
consumer. Its purpose is to prove the SDK does real work, dogfood the `x3d::sdk`
public API end-to-end, and surface any gaps before the VR CAVE forces the issue.
Commands are registered in a dispatch table (`kCmds[]` in `tools/x3d_cli.cpp`) so
adding a new subcommand requires only a new entry and a handler; the binary itself
is a thin glue layer.

## Purpose

Each command exercises a different SDK seam and is validated by its own oracle:
the two parse/serialize commands use an X3DJSAIL differential, the extract command
closes a self-oracle round-trip through `StlReader`, canonicalize uses tiered
idempotence and X3DJSAIL byte-comparison, and sim uses determinism + behavioral
correctness on crafted fixtures. Together they provide permanent CI regression
coverage over the CLI as a whole.

## Key files

| File / directory | Role |
|---|---|
| `tools/x3d_cli.cpp` | `main` + `kCmds[]` dispatch table + all five command handlers |
| `tools/x3d-cli/stl_write.hpp` | Binary-STL writer (core, NOT ext-gated); inverse of `StlReader` |
| `tools/x3d-cli/sim_runtime.hpp` | `x3d::sim::attachFullRuntime()` — wires every behavior system for headless tick |
| `tools/x3d-cli/sim_tracer.hpp` | `x3d::sim::FieldTracer` — snapshot-diff field-change tracer |
| `tools/x3d-cli/scene_equiv.hpp` | `x3d_cli::sceneEquivalent()` — reflection-based scene comparison for the convert round-trip gate |
| `tools/x3d-cli/cli_gate.cpp` | Differential validation gate: validate-diff vs X3DJSAIL + convert round-trip |
| `tools/x3d-cli/canon_gate.cpp` | X3DC14N tiered gate: idempotence (T1) + tolerant diff vs X3DJSAIL (T2) + byte-exact (T3) |
| `tools/x3d-cli/goldens/` | Committed gate artifacts: `subset.txt`, `validate-verdicts.tsv`, `cli-gate-baseline.tsv`, `canon-gate-baseline.tsv`, golden sim traces. (`canonical-goldens/` is generated on demand by `mise run canon-golden-gen` — needs JDK 25 — and is not committed.) |

## The five commands

### `x3d convert <in> [-o <out>] [-f xml|vrml|json]`

Converts an X3D scene between encodings. Input encoding is sniffed automatically
(XML / ClassicVRML / VRML97 / JSON; gzip variants `.x3dz` / `.x3dvz` / `.wrz` are
transparent). Output encoding is inferred from the `-o` extension (`.x3d` → XML,
`.x3dv` → ClassicVRML, `.json` → JSON) or forced with `-f`. Body:
`sdk::parseFile` → `XmlWriter` / `VrmlWriter` / `JsonWriter` → output (or stdout
if no `-o`). Out-of-range and proto/inline warnings are surfaced to stderr but are
non-fatal.

**Known limitation:** `VrmlWriter` emits ClassicVRML (ISO 19776-2), not true
VRML97; `.wrl` output falls back to ClassicVRML with a stderr warning.

**Oracle:** own-round-trip equivalence — `cli_gate` converts each subset file to
every other encoding, reparses the output (using the source file's directory as
`baseUrl` for symmetric Inline/EXTERNPROTO expansion), and asserts
`sceneEquivalent(src, roundtrip)`. X3DJSAIL `-validate` over our converted output
is an optional periodic snapshot (not a permanent CI gate) to confirm validity per
the reference.

### `x3d validate <in> [--json]`

Validates an X3D scene and reports diagnostics. Six checks run in sequence:

1. **Range diagnostics** — out-of-range field values collected by `sdk::parseFile` (`doc.rangeWarnings`).
2. **Proto/inline warnings** — unresolved `EXTERNPROTO`, missing declarations, unresolved `Inline` URLs (`doc.protoWarnings` / `doc.inlineWarnings`).
3. **Profile-fit** — walks every node via `nodeTypeName()` + a hardcoded
   node→`(component, level)` table (built from the generated headers); finds the
   minimal X3D 4.0 profile that contains all components used and flags nodes that
   exceed a declared profile. Profile table sources: ISO/IEC 19775-1:2023 Annexes B–F.
4. **Duplicate `<meta>`** — same `(name, content)` pair appearing more than once in `<head>`.
5. **Unused ProtoDeclare / ExternProtoDeclare** — a prototype declared with no corresponding `ProtoInstance` in the scene.
6. **IFS/ILS coord-without-index** — `IndexedFaceSet` or `IndexedLineSet` has a `Coordinate` node with point data but an empty `coordIndex` (4.0+-only to avoid false positives on older files).

Output: human-readable grouped-by-category report by default; `--json` for
machine-readable form. Exit codes: 0 clean, 3 issues found, 2 parse/IO failure,
1 usage error.

**Oracle:** `cli_gate` diffs our VALID/INVALID verdict per file against
`validate-verdicts.tsv` (captured from X3DJSAIL `-validate` over the 204-file
XML corpus subset — X3DJSAIL `-validate` is XML-only). `cli_gate --gate` enforces baseline-regression: a baseline-PASS
item that now fails is a CI failure; FAIL→PASS is a noted improvement.

### `x3d extract <in> [-o out.stl]`

Exports the tessellated geometry of an X3D scene to a single merged binary STL
(world-space, one triangle soup). Pipeline:

```
parseFile → X3DExecutionContext::buildSceneGraph + buildFrom
          → SceneExtractor::fullSnapshot()
          → for each RenderItem: transform MeshData triangles to world space
          → x3d::cli::writeStlBinary(positions, normals)
```

`SceneExtractor` is used exactly as a renderer would — the dogfooding payoff.
`MeshBuilder` supports `NurbsCurve` and `NurbsPatchSurface` (see
[ADR-0040](../decisions/0040-nurbs-tessellation-first-party.md)); geometry types
it does not support (2D primitives such as `Rectangle2D`/`Circle2D`/`Disk2D`,
and the deferred trimmed/swept/swung NURBS surfaces — NRB-3) are counted and
reported to stderr, never fatal. A zero-geometry scene writes a valid empty
STL (0 triangles) with a stderr note.

**STL writer placement:** `tools/x3d-cli/stl_write.hpp` (`x3d::cli::writeStlBinary`) is core,
NOT behind the `X3D_CPP_BUILD_EXT` firewall — STL export is a consumer-side
operation, not an X3D dialect extension. The self-oracle test (re-import via
`StlReader`) is ext-gated because `StlReader` lives in `runtime/ext/`. See
[ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md).

**Oracle (self-oracle):** export→`StlReader`→re-import as `PackedMesh`,
then compare triangle count and AABB within tolerance. Verified live: a `Box`
scene produces 12 facets; a representative scene produced 554.

### `x3d canonicalize <in> [-o out.x3d]`

Emits the X3D Canonical Form (X3DC14N) of a scene — a deterministic, byte-stable
XML serialization for diffing, deduplication, VCS storage, and signatures. Input
may be any supported encoding (`parseFile` sniffs it); output is always canonical
XML. Canonical rules (from X3DJSAIL de-risk observations):

- XML prolog + version-appropriate X3D DOCTYPE line.
- Attributes **sorted alphabetically** within each element; **single-quote** delimiters.
- **2-space-per-level** indentation; one element per line.
- **Minimal number format** (shortest round-trip; `0.8` not `0.800000`; integers without `.0`).
- DEF/USE, ProtoDeclare/field/connect, ROUTE, IMPORT/EXPORT, `<head>` meta, schema-namespace attrs preserved.

Implemented in `runtime/codecs/CanonicalXmlWriter.hpp` via `sdk::CanonicalXmlWriter`.
The default `XmlWriter` code path is untouched — the canonical writer is a
separate class so existing round-trip output and all golden files stay byte-identical.

**Oracle (tiered):**

| Tier | Check | Gate type |
|---|---|---|
| T1 | Idempotence: `canonicalize(canonicalize(x)) == canonicalize(x)` byte-for-byte | Hard CI gate (100% required); **204/204 (100%)** at ship |
| T2 | Tolerant diff vs X3DJSAIL golden fixtures (numbers within 1e-5, whitespace normalized, `containerField` stripped both sides) | Baseline-locked; **22/202 (11%)** at ship |
| T3 | Byte-exact vs X3DJSAIL | Informative stretch metric; **0/202** at ship |

The T2 gap (11%) was characterized: X3DJSAIL's X3DC14N is source-preserving —
it emits attributes even when equal to spec defaults and preserves SFNode child
source order. Our parse layer discards both (no was-set dirty-bit; SFNode slots
lose source order across readers). Matching it would require reader source-provenance
tracking (a substantial, separate future effort). Our canonical form is a valid,
deterministic, idempotent canonical for our diff/dedup/VCS use cases.

Note: X3DJSAIL `-canonical` requires Java 25 (`UnsupportedClassVersionError` on
JDK 17). Golden-gen uses JDK 25 by explicit path; CI stays Java-free.

### `x3d sim <in> [--fps F] [--ticks N | --duration D] [--move 'a -> b over Ds'] [--watch DEF.field ...] [--json]`

Headless behavior simulation: drives the full event/behavior runtime and traces
every field that changes per tick. This is the SDK's signature CLI capability —
no other X3D tool runs the behavior graph headlessly — and closes the biggest
dogfooding gap (the event runtime was untouched by the other four commands).

**Three units:**

1. **`x3d::sim::attachFullRuntime(scene, ctx)`** (`tools/x3d-cli/sim_runtime.hpp`) — the reusable "wire everything" helper. The caller runs `buildSceneGraph` + `buildFrom` first, then this attaches: `TimeSensorSystem`, the full interpolator set (§19, including spline/squad/ease), `EventUtilitySystem` (§30), `ViewDependentSystem` (§22/§23), `ViewpointBindSystem` (§23.3.1), `KeyDeviceSensorSystem` (§21), `ScriptSystem` (§29, only when `X3D_SIM_HAVE_SCRIPT` is defined), and `attachPhysics()` which — when `X3D_HAVE_PHYSICS` is defined — wires a `PhysicsSystem` + Jolt backend (no-op otherwise). See the [Physics subsystem](../subsystems/physics.md).

2. **Tick loop + drivers.** Ticks `N` times at `1/fps` from `t=0`. Time driver is always active (`ctx.tick(now)`). `--move 'a -> b over Ds'` linearly sweeps the viewer position via `ctx.setHeadPose()`, driving `ViewDependentSystem` → `ProximitySensor` / `VisibilitySensor` / LOD / Billboard.

3. **`x3d::sim::FieldTracer`** (`tools/x3d-cli/sim_tracer.hpp`) — snapshot-diff tracer. Each tick: snapshot every readable non-node field value via `fields()` reflection, diff against the previous snapshot, emit `(node, field, newValue)` deltas. Sensor and interpolator outputs (`enterTime`, `isActive`, `position_changed`, `value_changed`) are `inputOutput`/`outputOnly` fields whose stored values the cascade writes, so they surface naturally. Node names use DEF name if present, else a stable `<Type>#<index>` id. `--watch DEF.field` narrows the trace to specific fields. `--json` emits `[{tick, t, changes:[{node,field,value}]}]` — the golden-regression form.

**Why snapshot-diff, not cascade observer:** `X3DExecutionContext` exposes one per-field delivery observer slot (`cascade_.setFieldObserver`), already bound by `buildSceneGraph` to `classifyDirty` feeding the `DirtyTracker` for transform/bounds propagation. Evicting or chaining that observer is not exposed. The snapshot approach is documented and deterministic.

**Oracle (self-based, strong):**

1. **Determinism** — same scene + same drivers → byte-identical trace; a test runs sim twice and asserts equality.
2. **Golden traces** — `tools/x3d-cli/goldens/sim-anim.trace.json` and `sim-proximity.trace.json` are committed; a regression test asserts the current trace matches.
3. **Behavioral correctness** — crafted fixtures assert real computed values: a `TimeSensor(cycleInterval=1,loop) → PositionInterpolator(key 0 1, keyValue '0 0 0 10 0 0') → Transform` fixture shows `translation.x = 5.000` exactly at `t ≈ 0.5`; a `ProximitySensor` + `--move` path through its activation region fires `enterTime`/`exitTime` at the expected ticks (enterTick=4, exitTick=7 verified at ship).

## Interfaces and seams

### Exposed interface

The `x3d` binary is the public interface: `x3d <subcommand> [args]`. The dispatch table:

```cpp
static const SubCmd kCmds[] = {
    {"convert",      "Convert an X3D scene between encodings",        cmdConvert},
    {"validate",     "Validate an X3D scene and report diagnostics",  cmdValidate},
    {"extract",      "Export X3D geometry to a binary STL file",      cmdExtract},
    {"canonicalize", "Emit the X3D Canonical Form (X3DC14N)",         cmdCanonicalize},
    {"sim",          "Headless behavior simulation + field-change trace", cmdSim},
};
```

The key SDK types used across all commands:

```cpp
sdk::X3DDocument   doc = sdk::parseFile(path);   // parse any encoding
sdk::XmlWriter     w;  w.writeDocument(doc);     // convert
sdk::VrmlWriter    w;  w.writeDocument(doc);
sdk::JsonWriter    w;  w.writeDocument(doc);
sdk::CanonicalXmlWriter w;  w.writeDocument(doc); // canonicalize
sdk::X3DExecutionContext ctx;
ctx.buildSceneGraph(doc.scene);                  // build node index + cycle guard
ctx.buildFrom(doc.scene);                        // build route graph
sdk::SceneExtractor ex(ctx, doc.scene, opts);    // extract
x3d::sim::attachFullRuntime(doc.scene, ctx);     // sim
ctx.tick(now);
```

### Seam points

- **`x3d::sdk` façade** — all commands go through `x3d/sdk.hpp`; the CLI deliberately does no direct runtime work, so gaps in the façade surface here first.
- **`x3d::cli::writeStlBinary`** (core) / **`StlReader`** (ext-gated) — extract's write/read pair; the self-oracle test crosses the ext-firewall boundary.
- **`x3d::sim::attachFullRuntime`** — the reusable "wire all behavior systems" entrypoint; designed to be extractable to `x3d::sdk` when a second consumer needs it.
- **`attachPhysics(scene, ctx)`** — the physics hook in `sim_runtime.hpp`; wires a `PhysicsSystem` + Jolt backend under `X3D_HAVE_PHYSICS`, a no-op otherwise (see the [Physics subsystem](../subsystems/physics.md)).
- **`x3d_cli::sceneEquivalent`** (`scene_equiv.hpp`) — reflection-based scene comparison; test infrastructure (not in the SDK).

## Differential gate infrastructure

Two gate binaries enforce permanent regression protection, both wired into `mise run ci` via the `cli-gate-regression` task:

**`x3d_cli_gate` (`tools/x3d-cli/cli_gate.cpp`)**

- Reads `tools/x3d-cli/goldens/subset.txt` (204 files) and `validate-verdicts.tsv` (X3DJSAIL reference verdicts).
- Phase 1: validate-diff — runs our validate logic over each subset file, compares VALID/INVALID verdict to the golden, classifies disagreements by category.
- Phase 2: convert round-trip — for up to 80 parseable files, converts to each other encoding, reparses with the source `baseUrl`, and asserts `sceneEquivalent`. The `baseUrl` symmetry is critical: without it, `Inline`/`EXTERNPROTO` expand on the source side but stay as stubs on reparse, producing spurious failures.
- Modes: default (informative, exit 0); `--write-baseline` (writes `cli-gate-baseline.tsv`); `--gate` (regression check: baseline-PASS→FAIL = exit 1; FAIL→PASS = noted improvement). A gate with 0 checks (corpus absent) exits 1, never false-greens.

**`x3d_canon_gate` (`tools/x3d-cli/canon_gate.cpp`)**

- Runs the three-tier canonicalize check over the XML-only subset.
- Tier 1 (idempotence) is a hard CI gate: ANY failure exits non-zero regardless of baseline. Uses subprocess invocation (`popen`) to avoid the `DynamicFieldStore` global singleton accumulating stale entries across two in-process parse calls.
- Tier 2 uses `tolerantDiff()` — normalizes both XML strings to semantic token streams (element names, attribute names, split attribute values), then compares with 1e-5 numeric tolerance; `containerField` and `xmlns:xsd`/`xsd:noNamespaceSchemaLocation` are stripped from both sides.
- Mode flags are identical to `cli_gate`.

**`mise run ci`** depends on `cli-gate-regression`, which runs both gates in `--gate` mode against their committed baselines. `mise run cli-golden-gen` (needs JDK + X3DJSAIL jar) regenerates `validate-verdicts.tsv`; `mise run canon-golden-gen` (needs JDK 25 by explicit path) regenerates `canonical-goldens/`. Baselines are refreshed with `mise run cli-gate-baseline` and committed.

## How it is tested

| Test | What it covers |
|---|---|
| `tools/tests/x3d_cli_test.sh` | Arg parsing; `convert` round-trips in-repo fixtures across all encoding pairs; `validate` produces expected verdict + profile-fit on crafted fixtures; `extract` produces non-empty STL with expected facet count; `canonicalize` round-trips; `sim` trace matches golden on animation + proximity fixtures |
| `x3d_scene_equiv_test` (C++) | `sceneEquivalent` unit test: equal scenes compare equal; a deliberately mutated scene differs |
| `extract_oracle_test` (ext-gated, `tools/x3d-cli/extract_oracle_test.cpp`) | Extract→`StlReader`→re-import round-trip: triangle count + AABB match on `Box`, `Sphere`, `IndexedFaceSet` fixtures |
| `canonicalize_unit_test` (C++, `tools/x3d-cli/canonicalize_unit_test.cpp`) | Crafted fixture → assert sorted attrs, single-quote, DTD, minimal numbers; idempotence |
| `x3d_cli_gate` (differential, `mise run cli-gate`) | Validate-diff vs X3DJSAIL; convert round-trip equivalence over 80 files |
| `x3d_canon_gate` (differential, `mise run canon-gate`) | Idempotence 100% hard gate + tolerant-diff vs X3DJSAIL canonical goldens |
| Golden traces at `tools/x3d-cli/goldens/sim-anim.trace.json`, `sim-proximity.trace.json` | `sim` behavioral correctness + determinism regression |

## Logged limitations

- **Reader source-provenance**: the canon T2 gap (11% agreement with X3DJSAIL) traces to the parse layer discarding per-field was-set bits and SFNode child source order. Closing it would improve both canonical byte-matching and general round-trip fidelity; logged as a separate future effort.
- **True VRML97 output**: `VrmlWriter` emits ClassicVRML (ISO 19776-2) only; `.wrl` falls back with a warning.
- **Planned commands**: `deps` (Inline/texture/extern URL tree, for CAVE packaging) and `info` (scene stats) are the documented next tier.

## Related specs and ADRs

- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — why `stl_write.hpp` is core but the `StlReader` oracle test is ext-gated.
- [Gate System](../guides/gate-system.md) — how `cli-gate-regression` plugs into `mise run ci`.
- [Architecture](../architecture.md) — the overall SDK layer map that the CLI sits atop.
- Spec: `docs/superpowers/specs/2026-06-20-x3d-cli-convert-validate-design.md` — original `convert`/`validate` design including the X3DJSAIL de-risk findings.
- Spec: `docs/superpowers/specs/2026-06-20-x3d-extract-design.md` — `extract` design and STL writer architecture.
- Spec: `docs/superpowers/specs/2026-06-20-x3d-canonicalize-design.md` — X3DC14N rules, tiered gate design, and oracle de-risk (JDK 25 requirement).
- Spec: `docs/superpowers/specs/2026-06-20-x3d-sim-design.md` — headless sim architecture, snapshot-diff rationale, and physics-attach hook.
