---
title: "SAI architecture: one contract, two backends"
summary: Why the SAI SDK has one provider-owned contract, what the frontend and backends each own, and how the SDK relates to the runtime.
tags: [sai, explanation, architecture]
updated: 2026-11-14
related:
  - ../../explanation/decisions/0076-fold-sai-into-x3d-cpp.md
  - ../../explanation/decisions/0047-sai-sister-repo-split.md
  - ../../reference/subsystems/system-script-sai.md
---

# SAI architecture: one contract, two backends

## One contract

Applications use a single type, `provider::service`. The same scene, context,
node and field handles work while a scene is being set up and after it is
activated. There is no separate "offline" API and "hosted" API, and no second
handle family to translate between. Earlier adapter APIs of that shape were
consolidated into this contract.

That choice keeps application code backend-blind. In
`sai/examples/provider_scene.cpp` every operation sits in a function that takes
`provider::service &`; the backend is chosen once, in `main`.

## What the frontend owns

The frontend (`x3d::sai`) is compiled code shared by every backend. It owns:

- handle authority: who may use a scene, context, node or field handle
- admission: validation, update gates and resource budgets
- receipts: the record of what happened to each request
- notification delivery: queueing callbacks until the host drains them
- shutdown order

It has no reference-kernel dependency and no x3d-cpp scene dependency.

## What a backend owns

A backend implements `provider::backend` and owns the actual scene. The
frontend holds exactly one backend by composition. Numeric node IDs are a
backend storage address and never reach applications as authority.

| | Reference backend | Native backend |
|---|---|---|
| Storage | Kernel snapshots and isolated edit commits | `X3DDocument` during setup, `RuntimeSession` after activation |
| Execution | The reference kernel's synchronous event model | Native interpolation, ROUTE propagation, transforms and extraction |
| Extras | Snapshots, atomic editors, kernel sessions, checkpoints | Session adoption, presentation feed |

The native backend does not mirror a reference scene. There is one scene, and
it is the native one.

## Why the host drives time

The service installs no renderer, window, hidden clock, resource access or
event loop. Input is queued by clients, evaluated only when the host calls
`pump`, and observed only when the host calls `dispatch_notifications`. This
makes the SDK usable inside a host that already has its own frame loop, and it
makes every test deterministic.

A consequence is that "accepted" does not mean "applied". An owning receipt
records each request's outcome, even after the service closes. See
[Submissions, turns and notifications](../reference/submissions.md).

## Reference-only extensions

`kernel.hpp`, `scene_snapshot`, `scene_edit`, the strict-time
`external_session`, runtime checkpoints and browser services belong to the
reference kernel. They are useful on their own, for example as an authoring
model with atomic edits and immutable history, but they are not requirements
placed on another backend and they do not define another portable handle
family. Their design is explained in
[Reference kernel design](kernel-design.md).

## How the SDK relates to the runtime

- `sai/` is its own CMake project and uv package. The ordinary x3d-cpp runtime
  build never adds it, and the SDK has no build or link dependency on the
  runtime.
- `adapters/sai_provider` is the only bridge between the two.
- SAI headers live under `x3d/sai/experimental`, never under the runtime's
  `x3d/core` or `x3d/nodes`. The coexistence check enforces this.
- The SAI generator (`x3d-sai-gen`) reuses the runtime generator's front end
  (`x3d-cpp-gen`: UOM parser, type model, field descriptors) and reads the same
  SHA-pinned UOM. A change under `src/x3d_cpp_gen/` therefore also runs the
  `sai` CI job.
- Script nodes use the separate in-process
  [`SaiContext`](../../reference/subsystems/system-script-sai.md). Its runtime pointers
  are not portable service handles.

## History

The kernel was split into a sister `x3d-sai` repository on 2026-07-19
([ADR-0047](../../explanation/decisions/0047-sai-sister-repo-split.md), now superseded) and
folded back with its history on 2026-11-11
([ADR-0076](../../explanation/decisions/0076-fold-sai-into-x3d-cpp.md)). Commits before the
2026-07-19 seed commits are provenance, not a buildable sequence, so
`git bisect` does not work back across that import boundary. The pre-split
branch head is preserved as the tag `archive/sai-semantic-kernel`.
