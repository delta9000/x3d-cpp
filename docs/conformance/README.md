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
| [`sai-service-catalog.yaml`](sai-service-catalog.yaml) | independent ISO 19775-2 service oracle + fail-closed 19777-4 obligation audit | ✏️ reviewed reference |
| [`sai-services.yaml`](sai-services.yaml) | ISO abstract services → current/intended modern C++ surface | ✏️ **edit this** |
| [`sai-invariants.yaml`](sai-invariants.yaml) | semantic axioms, invariants, evidence strength, and proof obligations | ✏️ **edit this** |
| [`SAI-BASELINE.md`](SAI-BASELINE.md) | C++ SAI convergence scorecard | generated — don't edit |

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

# after changing a C++ SAI service/invariant judgment:
mise run sai-baseline            # regenerate SAI-BASELINE.md
mise run sai-conformance-gate    # service schema, cross-link, and drift gate (in CI)
mise run sai-invariants          # invariant schema and coverage gate
mise run sai-conformance         # aspirational strict gate; fails on open gaps
```

Invariant prose is policy, not ordinary documentation. After changing an
invariant `statement`, review the semantic change and replace `review_marker`
with `reviewed:sha256:<first-12-hex>` for the new UTF-8 statement digest. The
gate rejects stale markers. Planned falsification suites and stable test naming
are described in [`../../tests/sai_invariants/README.md`](../../tests/sai_invariants/README.md).

To **close** a gap: set its `status: closed` (or `fixed`) and add the `commit:`.
To **raise** one: append a finding (see the schema header in `findings.yaml`).

## Scope

`findings.yaml` owns *behavioral* conformance (the campaign). Broader engineering
deferrals (build, codecs, extraction milestones) are tracked in the
[GitHub Project](https://github.com/users/delta9000/projects/2) (the deprecated
`../superpowers/BACKLOG.md` previously held them); the campaign methodology
lives in [`../superpowers/specs/2026-06-18-conformance-gap-register.md`](../superpowers/specs/2026-06-18-conformance-gap-register.md).

The SAI registers are narrower: they prevent the modern C++ SAI
effort from converging on class names while leaving observable behavior vague.
The catalog independently inventories the 90 abstract services in ISO/IEC
19775-2:2015. Its C++-specific audit remains explicitly incomplete, so the strict
gate fails closed until all of ISO/IEC 19777-4 has been reviewed. The service map
records current x3d-cpp mechanisms without treating them as complete SAI support,
and ties every service to invariants that executable tests must eventually falsify.
