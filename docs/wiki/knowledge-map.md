---
title: Knowledge map
summary: Where specifications, decisions, implementation, and conformance evidence live.
tags: [reference, rag]
updated: 2026-11-05
related: []
---

# Knowledge map

Use the source appropriate to the question:

| Question | Source |
|---|---|
| What does the standard require? | Versioned ISO prose; `spec_rag.py` locates clauses and UOM metadata |
| Where is the behavior implemented? | Current C++ source; `code_rag.py` locates symbols |
| How does a subsystem work? | [Subsystem source map](coverage.md) and subsystem pages |
| Why was a design chosen? | Decision records in the wiki navigation |
| What conformance gaps remain? | `docs/conformance/findings.yaml` and its generated view |
| What can I run? | `mise tasks` |
| Which docs might a code change affect? | `docs_drift.py` suggestions, followed by source review |

[Using the RAGs](guides/rags.md) owns configuration, commands, and index-refresh
instructions.

The old dated specs and plans were removed from the tree; browse them at
[commit db23737](https://github.com/delta9000/x3d-cpp/tree/db23737f43f9ec16d983583834c40c70c810bf63/docs). Use current subsystem pages and ADRs for present
behavior and decisions.
