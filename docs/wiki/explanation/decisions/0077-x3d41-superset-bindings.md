---
title: "ADR-0077: Generate the bindings from X3D 4.1 as a version-tagged superset"
summary: "One binding tree generated from the 4.1 UOM; each node type and field carries the version that introduced it, and readers and validate compare it with the declared version."
tags: [adr, generator, bindings, versions]
updated: 2026-11-28
related:
  - 0007-version-inference-ladder.md
  - ../../reference/subsystems/generator.md
  - ../../reference/subsystems/generated-bindings.md
  - ../../reference/subsystems/parse-readers.md
---

# ADR-0077: Generate the bindings from X3D 4.1 as a version-tagged superset

- Status: Accepted
- Date: 2026-11-25

## Context

The bindings were generated from the X3D 4.0 UOM. A 4.1 document read fine at the header,
but its 4.1 nodes (`EnvironmentLight`, `Tangent`, `HAnimPose`, `FontLibrary`,
`InlineGeometry`, `RenderedTexture`) and 4.1 fields (`IndexedFaceSet.tangent`,
`OscillatorSource.periodicWave`, `ImageTexture.width` and others) had no type. The XML
reader kept them as opaque `UnknownNode` records; the ClassicVRML and JSON readers dropped
them; nothing ran them.

Three ways to support 4.1 were weighed in the 2026-11-11 completeness plan:

| Option | How | Cost |
|---|---|---|
| A. Switch to 4.1 | Regenerate from the 4.1 UOM; treat 4.0 files as a subset | A 4.0 file using a 4.1 node passes silently |
| B. 4.1 superset, version-tagged | Regenerate from 4.1; tag every node and field with the version that introduced it | One extra column in reflection |
| C. Two binding trees | Separate 4.0 and 4.1 libraries | Doubles the build; every System handles both |

The repository already commits per-version manifests for X3D 3.0 to 4.1
(`src/x3d_cpp_gen/conformance/manifests/`), so the earliest version defining each node and
field is known without new data.

## Decision

We chose option B.

- The generator's default spec is the packaged 4.1 UOM. 4.1 removes no 4.0 node or
  field. It changes one field type, `StreamAudioSource`/`StreamAudioDestination.streamIdentifier`
  (SFString to MFString, which the ISO 4.0 text already specified), and one default,
  `TextureProjector.upVector` (0 0 1 to 0 1 0). The two agree only for the default
  direction, so the parse front door gives a projector without `upVector` in a document
  declaring a version below 4.1 the old default (`applyVersionDefaults`); 4.0 files keep
  their frame for every direction. It also widens HAnim `loa` to 5.
- Each generated node type implements `X3DVersion introducedIn() const`, and each
  `FieldInfo` carries `X3DVersion since`. Both come from the earliest committed manifest
  that defines the node, or the field on that node (`emit/versions.py`).
- After a read, `collectVersionWarnings` reports each authored node type, field and ROUTE
  or IS endpoint newer than the declared version, and an HAnim `loa` of 5, as a
  `ReaderWarning::Kind::Version`. Generated defaults never count; only fields a reader
  recorded or a route names do. An undeclared version is not compared.
- `x3d validate` reports these as errors (category `version`) and prints the version the
  content needs (`requiredVersion`).
- The authoring default for a new `X3DDocument` is 4.1.

## Consequences

**Positive:**

- A 4.1 file reads as typed nodes in XML, ClassicVRML and JSON and round-trips through all
  three writers.
- Mislabelled files are reported in both directions that matter: a 4.0 file using
  `EnvironmentLight`, and a 3.3 file using `PhysicalMaterial`.
- Workarounds for missing 4.1 fields go away. `SoundSystem` reads
  `OscillatorSource.periodicWave` directly (SND-9).

**Trade-offs / costs:**

- Version tags are only as fine as the manifests: 3.0, 3.1, 3.2, 3.3, 4.0, 4.1. A field
  whose type changed (streamIdentifier) is tagged by its first appearance, not by type.
- Writers do not down-convert. A 4.1 node in a document declared 4.0 is written as is,
  with the declared version; `validate` reports the mismatch.
- The SAI SDK (`sai/`) keeps its own generator pin (4.0); it is a separate project.

## Related

- [ADR-0007: Version inference ladder](0007-version-inference-ladder.md)
- [Generator](../../reference/subsystems/generator.md)
- [Generated bindings](../../reference/subsystems/generated-bindings.md)
- [Parse readers](../../reference/subsystems/parse-readers.md)
