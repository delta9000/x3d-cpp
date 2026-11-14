---
title: Using the RAGs
summary: Configure and query specification, C++, and documentation indexes.
tags: [guide, rag]
updated: 2026-11-05
related: []
---

# Using the RAGs

The search scripts locate evidence. Read the returned clause or source before
making a capability claim. [Knowledge map](../reference/knowledge-map.md) identifies the
sources that own specifications, decisions, and conformance status.

## Configuration

Run the scripts with `uv run python` to resolve project dependencies.
They use configurable embedding and Qdrant services. The defaults are local:

| Variable | Default |
|---|---|
| `X3D_QDRANT_URL` | `http://localhost:6333` |
| `X3D_EMBED_URL` | `http://localhost:8080/v1/embeddings` |
| `X3D_EMBED_MODEL` for spec search | `nomic-embed-text-v1.5` (768 dimensions) |
| `X3D_EMBED_MODEL` for code and doc search | `Qwen3-Embedding-0.6B` (1024 dimensions) |

Set endpoints in your environment or gitignored `mise.local.toml`:

```toml
[env]
X3D_QDRANT_URL = "https://<qdrant-host>"
X3D_EMBED_URL = "http://<embedding-host>:8080/v1/embeddings"
X3D_SPEC_PROSE_DIR = "/path/to/x3d_spec_prose"
```

A proxy can serve both embedding models at one endpoint. If you override the
model, preserve the collection's dimensions. Reachability depends on your
service configuration; Tailscale is not a script requirement. Network failures
can terminate a search or drift suggestion.

## Specification search

`scripts/spec_rag.py` uses collection `x3d-spec-prose`. For a known node or
field, use the structured lookup; for a concept, use semantic search:

```sh
uv run python scripts/spec_rag.py node Transform --version 4.0
uv run python scripts/spec_rag.py field Material.shininess --version 4.0
uv run python scripts/spec_rag.py query "event cascade timing model" -k 6
```

Structured metadata comes from the selected UOM manifest. `--version` defaults
to 4.1 and accepts manifests from 3.0 through 4.1; that lookup default does not
change the runtime's generated 4.0 node set. Prose anchors read the external
mirror at `X3D_SPEC_PROSE_DIR`. Field lookup can fall back to semantic search
when the local prose has no matching line. `node --rag N` adds semantic hits.

After updating the mirror, `uv run python scripts/spec_rag.py ingest` replaces
the spec collection. Query results reflect the last ingestion.

## C++ search

`scripts/code_rag.py` uses collection `x3d-cpp-code`. It extracts C++ symbols
from `runtime/`, `include/x3d/`, `tools/`, `examples/`, and generated bindings,
excluding vendored code. Queries search the runtime tier by default:

```sh
mise run code-rag query "how are bounds computed"
uv run python scripts/code_rag.py symbol CycleBreaker::breakContainmentCycles
uv run python scripts/code_rag.py query "texture sampling" --source example
uv run python scripts/code_rag.py query "interpolation" --source runtime --source test
uv run python scripts/code_rag.py query "interpolation" --all
```

`symbol` performs an exact indexed lookup. `--all` includes runtime, example,
test, and generated tiers. Read the current file to check indexed line numbers.

`uv run python scripts/code_rag.py ingest` recreates the full collection.
`ingest --source runtime` replaces only that tier; the same applies to `example`,
`test`, and `generated`. Re-ingest the full affected tier after line shifts:
point IDs include source positions, so an additive per-file update would leave
stale entries.

## Documentation drift suggestions

`scripts/docs_drift.py` indexes Markdown sections from `docs/wiki/`
and `docs/conformance/` into `x3d-cpp-docs`. It excludes frozen plan archives.
Suggestions combine literal code citations with semantic similarity:

```sh
mise run docs-ingest
mise run docs-drift working
uv run python scripts/docs_drift.py suggest HEAD
```

`working` compares tracked and untracked work with HEAD. The default `auto`
uses that mode for a dirty tree and HEAD for a clean tree. A commit argument
compares that commit with its parent. Review `CITES` results first and re-ingest
after documentation restructuring.

This is an advisory tool. If the embedding service or vector store is
unavailable, suggestions still report literal citations, label the semantic
check incomplete, and exit successfully. A successful run does not validate the
pages. Use the
[documentation gates](../reference/gates.md#documentation) for links and CTest names,
and source review for semantic accuracy.
