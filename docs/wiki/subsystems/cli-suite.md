---
title: CLI Suite
summary: The x3d CLI — convert, validate, extract, canonicalize, and sim commands; the first SDK consumer.
tags: [subsystem, cli, convert, validate, extract, canonicalize, sim]
updated: 2026-11-11
related:
  - ../architecture.md
  - ../guides/gate-system.md
  - ../decisions/0001-ext-firewall.md
---

# CLI suite

`tools/x3d_cli.cpp` implements the `x3d` command. It uses the SDK parser,
writers, execution context, and extractor.

## Commands

The top-level CMake build produces `build/x3d`.

| Command | Behavior |
|---|---|
| `x3d convert input -o output -f xml\|vrml\|json` | Parse a document and serialize it in the selected encoding |
| `x3d validate input --json` | Report value, ROUTE, range, expansion, reader, and profile-fit diagnostics |
| `x3d extract input -o output.stl` | Export extracted triangle geometry as binary STL |
| `x3d extract input -o output.glb` (or `-f glb`) | Export the extracted scene as binary glTF 2.0: nodes with world matrices, shared meshes, materials, textures (local PNG/JPEG files and PixelTextures embedded), lights, and viewpoints as cameras. See [glTF export](extract.md#gltf-export) for what is left out |
| `x3d canonicalize input -o output.x3d` | Write canonical XML |
| `x3d sim input --ticks 120` | Tick the behavior runtime and trace changed fields |

`VrmlWriter` emits ClassicVRML. A `.wrl` output request warns rather than
providing a separate VRML97 writer. Extraction uses the geometry supported by
[MeshBuilder](extract.md); line and point items do not become STL triangles.
Canonical output is described in [canonical XML](canonical-xml.md).

## Validation

`x3d validate` exits 0 when it finds nothing and 3 otherwise. Its diagnostic
categories:

| Category | Severity | Source |
|---|---|---|
| `value` | warning | A malformed authored value the lenient reader coerced: an unparsable number, the wrong SF tuple arity, or a trailing partial MF tuple (`checkValueSyntax`). |
| `route` | error | A ROUTE whose endpoint DEF does not exist (IMPORTed names exempt), or that the runtime bridge rejects for an unknown field, access type or field-type mismatch (`checkRoutes`). |
| `range` | warning | An out-of-range value (`collectRangeWarnings`). |
| `proto`, `inline`, `node`, `profile` | warning or error | Expansion, reader-recovery and profile-fit diagnostics. |
| `dup-meta`, `unused-proto`, `ifs-coord-index`, `ils-coord-index` | warning | Checks shared with the X3DJSAIL differential gate. |

A `node` warning for an unknown element means the XML reader kept it as opaque
content: `convert` and `canonicalize` to XML re-emit it unchanged, while
ClassicVRML and JSON output keep only its type name and DEF.

The differential gate leaves `value` and `route` out of its verdict until its
X3DJSAIL baseline is refreshed against them.

## Simulation

`--fps` selects the step rate. `--ticks` or `--duration` selects the run length.
`--move 'a -> b over Ds'` drives head position. `--watch DEF.field` restricts
field tracing, and `--json` selects structured output.

`tools/x3d-cli/sim_runtime.hpp` attaches the standard runtime and optional
Script and physics implementations when linked. `sim_tracer.hpp` compares
readable reflected field values between ticks. This reports value changes,
not every occurrence of an event with an unchanged value.

## Differential checks

`tools/x3d-cli/cli_gate.cpp` compares validation verdicts with committed
X3DJSAIL fixtures and checks conversion round trips.
`tools/x3d-cli/canon_gate.cpp` checks canonical idempotence and, when reference
fixtures exist, compares canonical output with X3DJSAIL.

The regression mode rejects a previously passing baseline row that now fails.
Canonical idempotence is required independently of differential baselines.
Run `mise tasks` for the gate and fixture-generation tasks in
[the task definitions](https://github.com/delta9000/x3d-cpp/blob/main/mise.toml).

## Provenance and conformance

Readers retain authored scalar-field presence and child order for writers and
UNIT conversion. See [parse readers](parse-readers.md) and
[codec writers](codecs-writers.md). Remaining fidelity and conformance gaps
are recorded in the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml).
