---
title: "Subsystem Name"
summary: "One-sentence summary of what this subsystem does."
tags: [subsystem, TAG]
updated: YYYY-MM-DD
related:
  - ../architecture.md
---

# Subsystem Name

<!-- Copy this template. Replace all placeholder text.
     Remove these HTML comments before committing.
     Every factual claim (path, function name, API) MUST be verified against the codebase. -->

## Purpose

<!-- What problem does this subsystem solve? One short paragraph.
     State its role in the overall system and the boundary it owns. -->

## Key files

<!-- List the primary source files and directories. Use repo-relative paths.
     Only list files that are central — not every file touched by the subsystem. -->

| File / directory | Role |
|---|---|
| `runtime/...` | ... |

## Interfaces and seams

<!-- Describe the public interface this subsystem exposes (types, functions, headers).
     Describe the seam points where other subsystems hook in (virtual base classes,
     registration calls, callbacks, etc.).
     This is the contract — be precise. -->

### Exposed interface

```cpp
// Example: the primary entry point or type
```

### Seam points

- **SeamName** — description of how another subsystem plugs in.

## How it is tested

<!-- What test coverage exists? Reference ctest target names, golden files,
     pytest files, or integration tests by name. -->

- `ctest --preset dev -R <target>` — what it covers.
- Golden files at `tests/golden/...` — what invariant they lock.

## Related specs and ADRs

<!-- Link to ADRs that capture decisions made during implementation, and CITE the
     dated specs that designed this subsystem BY PATH (in backticks — NOT a live
     link; out-of-docs_dir links break mkdocs --strict). See _conventions/CONVENTIONS.md.

     Example forms (replace with real values):
       - [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md)   <- in-wiki: link
       - Spec: `docs/superpowers/specs/2026-06-20-x3d-sim-design.md`   <- out-of-wiki: cite by path
-->

- <!-- add links here -->
