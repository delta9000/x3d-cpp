---
title: x3d-cpp knowledge wiki
summary: Runtime architecture, subsystem references, decisions, and guides.
tags: [index]
updated: 2026-11-14
related: []
---

# x3d-cpp knowledge wiki

x3d-cpp generates C++20 node bindings from the X3D 4.0 Unified Object Model
and supplies parsing, scene execution, and extraction APIs. Renderer examples
are separate consumers.

| Reference | Use it for |
|---|---|
| [Architecture](architecture.md) | Layers and data flow |
| [Native host integration](guides/native-host.md) | Session ownership, input, resources, and delta consumption |
| [SAI SDK](sai/index.md) | The C++20 Scene Access Interface: tutorials, how-to guides, reference and explanation |
| [Subsystem source map](coverage.md) | Primary code paths for each subsystem |
| [Seam matrix](seam-status.md) | Interfaces, implementations, and contract tests |
| [Knowledge map](knowledge-map.md) | Specification, implementation, and decision sources |
| [Gate system](guides/gate-system.md) | Verification boundaries |
| [Requirements audit](guides/x3d4-requirements-audit.md) | Normative obligations and host acceptance scope |

The navigation lists subsystem pages, decision records, and operational guides.
`docs/conformance/findings.yaml` owns conformance status. Its generated view is
`docs/conformance/INDEX.md`; schema consistency and source wiring do not establish
full behavioral conformance. Run `mise tasks` for the maintained task catalog.

Authors follow the [wiki conventions](_conventions/CONVENTIONS.md) and the
[ADR](_conventions/adr-template.md) or
[subsystem](_conventions/subsystem-template.md) template.
