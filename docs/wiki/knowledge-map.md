---
title: Knowledge Map
summary: Where knowledge lives — spec_rag, code_rag, the wiki, dated specs/plans, and when to use each.
tags: [rag, knowledge-map, spec-rag, code-rag]
updated: 2026-06-20
related:
  - index.md
  - architecture.md
---

# Knowledge Map

This page describes the project's knowledge tools — what each one indexes, when to reach for it, and how to run it. The audience is agents and developers doing conformance walks, auditing behavior against the spec, or tracing how an X3D concept is implemented.

## The two RAGs

The project has two complementary semantic-search tools, both built on Qdrant at `<qdrant-host>`:

| Tool | Answers the question | Embedding model | Collection |
|---|---|---|---|
| `spec_rag.py` | What does the X3D ISO spec require? | nomic-embed-text-v1.5 (768-d) @ `<embedding-host>` | `x3d-spec-prose` |
| `code_rag.py` | Where / how does this runtime implement it? | Qwen3-Embedding-0.6B (1024-d) @ `<embedding-host>:8080` | `x3d-cpp-code` |

Both tools live in `scripts/` and are run via `uv run python scripts/<tool>.py`. They require the private vector-store + embedding network to be up; from a bare shell without that network the tools will error (connection refused / timeout).

---

## spec_rag.py — ISO prose RAG + structured lookups

`spec_rag.py` searches the local mirror of the X3D 4.0 (ISO/IEC 19775-1) normative prose at `$X3D_SPEC_PROSE_DIR/*.md`. The prose mirror is separate from the UOM/XSD (which only contain structural facts — nodes, fields, types). The procedural text — texture-coordinate generation algorithms, the event/cascade model, SCP construction, per-primitive mappings — lives only in the prose mirror.

### Subcommands

```
uv run python scripts/spec_rag.py query "<text>" [-k N]
uv run python scripts/spec_rag.py node  <Name> [--version V] [--rag N]
uv run python scripts/spec_rag.py field <Node>.<field> [--version V]
uv run python scripts/spec_rag.py ingest
```

**`query`** — semantic cosine search over all 4,909 prose chunks. Use when you only know a concept or algorithm (e.g. `"how texCoord is generated for ElevationGrid"`). Returns up to `k` ranked snippets (default 6) with file, section, and score. The bare form `spec_rag.py "<text>"` is an alias.

**`node <Name>`** — structured facts pulled directly from the 3.0–4.1 UOM manifests: component, containerField, base type, field count — plus the exact prose section anchor (file, section, source URL) so you can read the normative text. Add `--rag N` to also get N related prose hits. Version defaults to 4.1; use `--version 3.3` for older nodes. Examples:

```
uv run python scripts/spec_rag.py node Transform
uv run python scripts/spec_rag.py node ElevationGrid --version 4.1 --rag 4
```

**`field <Node>.<field>`** — type, accessType, default, range, and acceptable-node-types from the UOM manifest, plus a best-effort prose line extracted from the node's anchor section. Falls back to a semantic query if the field is not in the anchor text.

```
uv run python scripts/spec_rag.py field Transform.center
uv run python scripts/spec_rag.py field Material.diffuseColor --version 3.3
```

**`ingest`** — rebuilds the Qdrant collection from the prose mirror. Run only when the mirror has been updated.

### Routing rule

> Hold a concrete Node or Node.field name? Use `node` / `field` — they are exact and version-aware (structural truth from the UOM manifests + the authoritative prose anchor). Only know the concept or algorithm? Use `query`. `node`/`field` beat a fuzzy `query` whenever you already have the name.

Unknown or typo'd subcommands are caught and fail loud (e.g. `nodes Transform` → "did you mean 'node'?") rather than silently degrading to a search.

### Infrastructure

- Embeddings: `nomic-embed-text-v1.5` (768-d, nomic task prefixes `search_document:` / `search_query:`, n_ctx=256 tokens — chunks are kept to ~400 chars to stay within context).
- Vector store: Qdrant @ `https://<qdrant-host>`, collection `x3d-spec-prose`.
- Source prose: `$X3D_SPEC_PROSE_DIR/*.md` (63 pages from web3d.org; NOT inside this repo — typically `x3d-render/data/x3d_spec_prose/`).

---

## code_rag.py — codebase RAG + symbol lookup

`code_rag.py` is the implementation-side sibling. It answers "where in the C++ runtime does this run?" and is designed for use alongside `spec_rag.py` in a clause-by-clause conformance walk: find the clause via `spec_rag`, then find its implementing code via `code_rag`.

The collection is built from the repo's hand-written C++ — `runtime/`, `include/x3d/`, `tools/` — using tree-sitter-cpp to extract one chunk per symbol (function definition, method, or pure-data struct). The generated bindings tier (`generated_cpp_bindings/`) is indexed separately and excluded from the default query.

### Subcommands

```
uv run python scripts/code_rag.py query "<text>" [-k N] [--source test] [--all]
uv run python scripts/code_rag.py symbol <Name>
uv run python scripts/code_rag.py ingest [--source S]
```

Convenience alias via mise: `mise run code-rag query "<text>"`.

**`query`** — semantic cosine search over C++ symbols. Default source tier is `runtime` only (hand-written implementation files). Test bodies are prose-rich and tend to out-rank dense implementation code, so they are opt-in:

```
uv run python scripts/code_rag.py query "how are bounds computed"
uv run python scripts/code_rag.py query "event cascade routing" --source test
uv run python scripts/code_rag.py query "interpolation" --all
```

`--source test` adds the `test` tier; `--all` includes tests and generated bindings. Results are de-duplicated by (file, symbol) — windowed chunks of the same symbol collapse to the best-scoring hit.

**`symbol <Name>`** — exact lookup by symbol name: returns full source text, file, and line range. Use when you already know the C++ name (e.g. `BoundsSystem::compute`). Faster and unambiguous compared to a query.

```
uv run python scripts/code_rag.py symbol BoundsSystem::compute
uv run python scripts/code_rag.py symbol ScriptEngine::tick
```

**`ingest [--source S]`** — rebuilds the collection. Without `--source` it recreates the full collection (all tiers). With `--source runtime` (or `test`, `generated`) it drops and re-adds only that tier. There is no per-file incremental update: symbol point IDs include `start_line`, so a shifted symbol would mint a new point and leave a stale duplicate.

### Routing rule

> Hold a concrete C++ symbol name? Use `symbol` — exact, no ambiguity. Only know the concept or algorithm? Use `query` over the `runtime` tier first. Add `--source test` to corroborate with test prose.

### Infrastructure

- Embeddings: `Qwen3-Embedding-0.6B` (1024-d, query format `Instruct: <task>\nQuery: <q>`, documents embedded raw) @ `http://<embedding-host>:8080`. **The Qwen embedding host is a separate host from the spec-RAG nomic/Qdrant infra** — `spec_rag.py` does not depend on it; `code_rag.py` does.
- Vector store: Qdrant @ `https://<qdrant-host>`, collection `x3d-cpp-code` (~11,314 points).
- Source scope: `runtime/`, `include/x3d/`, `tools/`, `generated_cpp_bindings/` (vendor/ and tinfl.h excluded).

---

## Reliability caveats

Both tools call external services. From a shell without the Tailscale VPN, every call fails (connection refused on Qdrant / <embedding-host>). The tools surface the error from the underlying `curl` call — there is no partial-result fallback.

`code_rag.py` has an additional dependency: the Qwen embedding host must be up. `spec_rag.py` uses a separate self-hosted nomic server and does not depend on the Qwen host.

Neither tool has a networked test in the test suite; both are manually smoked after ingestion.

---

## Planned: project_rag (third pillar)

A third RAG pillar is planned but not yet built: `project_rag` would index the project's own knowledge corpus — dated specs (`docs/superpowers/specs/`), plans, the deprecated `BACKLOG.md`, conformance findings (`docs/conformance/findings.yaml`), and this wiki. The intended use is retrieval over the project's own accumulated design decisions and historical context, complementing `spec_rag` (ISO prose) and `code_rag` (C++ implementation). No timeline is committed.

---

## When to use which

| You have | Reach for |
|---|---|
| A concept or algorithm to look up in the spec | `spec_rag.py query "<concept>"` |
| A Node name, want its spec facts | `spec_rag.py node <Name>` |
| A Node.field, want type/access/default | `spec_rag.py field <Node>.<field>` |
| An implementation question about this codebase | `code_rag.py query "<concept>"` |
| A C++ symbol name you want to find | `code_rag.py symbol <Name>` |
| Doing a conformance clause walk (spec → impl) | `spec_rag.py node/query` first, then `code_rag.py query` to find the implementation |
| Building/updating the knowledge base | `spec_rag.py ingest` / `code_rag.py ingest` |

---

See [Architecture](architecture.md) for how the runtime layers that `code_rag` searches are structured.
