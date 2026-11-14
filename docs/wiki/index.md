---
title: x3d-cpp documentation
summary: Entry point to the tutorials, how-to guides, reference and explanation for the runtime SDK and the SAI SDK.
tags: [index]
updated: 2026-11-14
related: []
---

# x3d-cpp documentation

x3d-cpp is a headless C++20 X3D runtime SDK. It parses a scene, advances its
behavior, and hands meshes, materials, lights and cameras to your renderer. The
node bindings are generated from the X3D 4.0 Unified Object Model.

The documentation follows [Diataxis](https://diataxis.fr/). Pick the kind of
page that matches what you need now.

| You want to | Go to |
|---|---|
| Learn by building something that works | [Tutorials](#tutorials) |
| Get one task done | [How-to guides](#how-to-guides) |
| Look up exact behavior | [Reference](#reference) |
| Understand why it is built this way | [Explanation](#explanation) |
| Use the Scene Access Interface | [SAI SDK](sai/index.md), a tree of the same shape |

## Tutorials

- [Load, run and extract your first scene](tutorials/first-scene.md)
- [Tour the x3d command line](tutorials/cli-tour.md)

## How-to guides

Embed the runtime:

- [Build and install the SDK](how-to/build-and-install.md)
- [Integrate a native host](how-to/integrate-a-native-host.md)
- [Supply resources through the seams](how-to/supply-resources.md)
- [Add a behavior System](how-to/add-a-behavior-system.md)
- [Drive the context and extractor directly](how-to/use-the-low-level-api.md)
- [Build a scene in code](how-to/build-a-scene-in-code.md)
- [Measure tick phases](how-to/measure-tick-phases.md)

Work on x3d-cpp:

- [Verify a change](how-to/verify-a-change.md)
- [Regenerate the golden bindings](how-to/regenerate-goldens.md)
- [Add a CLI command](how-to/add-a-cli-command.md)
- [Audit a requirement](how-to/audit-a-requirement.md)
- [Search with the RAG tools](how-to/search-with-the-rags.md)
- [Report a bug with a scene](how-to/report-a-bug.md)

## Reference

| Page | Contents |
|---|---|
| [SDK capabilities](reference/capabilities.md) | What the SDK surface covers, one row per capability |
| [Native host contract](reference/native-host-contract.md) | Session, mutation, delta, lifetime and resource rules |
| [Seam matrix](reference/seam-matrix.md) | Interfaces, implementations and contract tests |
| [API stability](reference/api-stability.md) | What 0.x releases promise |
| [Examples](reference/examples/index.md) | Every example program and what it shows |
| [Build and mise tasks](reference/build-and-tasks.md) | Presets, targets, options and tooling |
| [Gate system](reference/gates.md) | What each verification gate checks |
| [Subsystem source map](reference/source-map.md) | Primary code paths, with a link to every subsystem page |
| [Knowledge map](reference/knowledge-map.md) | Which source answers which question |
| [X3D 4.0 requirements audit](reference/x3d4-requirements-audit.md) | Normative obligations and host acceptance scope |
| [Documentation conventions](reference/doc-conventions.md) | Where pages go and how they are written |

Subsystem pages live under `reference/subsystems/`; the navigation groups them
by layer. Conformance status is not in this tree: `docs/conformance/findings.yaml`
owns it, and `docs/conformance/INDEX.md` is its generated view.

## Explanation

- [Architecture](explanation/architecture.md)
- [The tick model](explanation/tick-model.md)
- [Seams, ports and backends](explanation/seams.md)
- Decision records (ADRs) under `explanation/decisions/`, listed in the
  navigation. New ones start from the
  [ADR template](explanation/decisions/0000-template.md).
