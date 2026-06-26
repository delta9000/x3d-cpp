# Conformance View

The single zoom-out for X3D SDK conformance. **Start at [`INDEX.md`](INDEX.md).**

## What's here

| File | Role | Edit? |
|------|------|-------|
| [`INDEX.md`](INDEX.md) | the zoom-out: bug picture + component × dimension matrix | generated — don't edit |
| [`components/`](components/) | per-component drill-down (node rows + findings) | generated — don't edit |
| [`model.json`](model.json) | machine/agent/RAG-queryable merged model | generated — don't edit |
| [`findings.yaml`](findings.yaml) | **behavioral judgments — the source of truth** | ✏️ **edit this** |
| [`profiles.yaml`](profiles.yaml) | ISO profile → component-level reference table | ✏️ reviewed reference |

## The split: facts vs judgments

- **Facts** (does a node exist? extract? have a System wired? component/level/profile?)
  are **auto-derived** from the generated bindings, `X3DInterfaceRegistry`, and the
  `runtime/` Systems. They cannot rot — regenerating re-reads the code.
- **Judgments** (does it *behave per ISO prose*? severity? status?) are human
  knowledge and live in **`findings.yaml`** — the one file you edit.

The view joins them: a behavioral node with no System reads `✗ inert`; wired with an
open finding reads `◑ partial`; wired and clean reads `✓`. A finding referencing a
node/interface that doesn't exist is reported loudly (catches stale findings).

## Workflow

```sh
# raise / update / close a behavioral gap → edit findings.yaml, then:
mise run conformance        # regenerate model.json + INDEX.md + components/*.md
mise run conformance-gate   # validate schema + fail on drift (also runs in CI)
```

To **close** a gap: set its `status: closed` (or `fixed`) and add the `commit:`.
To **raise** one: append a finding (see the schema header in `findings.yaml`).

## Scope

`findings.yaml` owns *behavioral* conformance (the campaign). Broader engineering
deferrals (build, codecs, extraction milestones) stay in
[`../superpowers/BACKLOG.md`](../superpowers/BACKLOG.md); the campaign methodology
lives in [`../superpowers/specs/2026-06-18-conformance-gap-register.md`](../superpowers/specs/2026-06-18-conformance-gap-register.md).
