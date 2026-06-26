# Code RAG (`code_rag.py`) — Design

**Date:** 2026-06-18
**Status:** Approved for implementation planning
**Sibling of:** `scripts/spec_rag.py` (the ISO-prose RAG) and `docs/superpowers/specs/2026-06-15-uom-prose-anchors-design.md`

## Purpose

A small semantic-search tool over this repository's hand-written C++ runtime, mirroring
`spec_rag.py`. Its eventual job is to be the **code-side complement of the spec RAG** for a
clause-by-clause X3D conformance walk: given a normative clause (found via `spec_rag.py`),
locate the code that implements that behavior so it can be verified against the prose.

The two tools answer opposite halves of one question:

- `spec_rag.py "<concept>"` → *what does the spec require?* (ISO prose, collection `x3d-spec-prose`)
- `code_rag.py "<concept>"` → *where/how do we implement it?* (this repo's C++, collection `x3d-cpp-code`)

v1 ships **retrieval only**. The automatic clause→code *join* (cross-linking code chunks to
prose anchors) is explicitly deferred to v2 — see [Deferred](#deferred-to-v2).

## Non-goals (v1)

- No clause→code cross-linking / spec-anchor payloads. Deferred to v2.
- No reranker in the default path. The `bge-reranker-v2-m3` cross-encoder is available on the
  same host and is a planned v2 `--rerank` flag, not v1.
- Not a replacement for `spec_rag.py`; the spec RAG keeps using nomic and the `x3d-spec-prose`
  collection, untouched.
- No LSP/clangd call-graph or cross-reference index. Pure per-symbol semantic retrieval.

## Infrastructure

Reuses the self-hosted vector-store + embedding stack, but a **different embedding model and host** than the spec RAG,
chosen for code retrieval quality and context window.

| | spec RAG (unchanged) | **code RAG (this design)** |
|---|---|---|
| Embedding model | `nomic-embed-text-v1.5` | **`Qwen3-Embedding-0.6B`** |
| Host | `<embedding-host>` | **`http://<embedding-host>:8080`** (Tailscale; `<embedding-host>`) |
| Dimensions | 768 | **1024** |
| Per-request context | 256 tok (~480 chars) | **8192 tok** (after config bump; see below) |
| Query format | `search_query: <q>` prefix | **`Instruct: <task>\nQuery: <q>`**; documents embedded raw |
| Qdrant collection | `x3d-spec-prose` | **`x3d-cpp-code`** (1024-d, Cosine) |
| Vector store | `https://<qdrant-host>` (shared) | same Qdrant, new collection |

Reranker (v2): `bge-reranker-v2-m3` on the same `<embedding-host>:8080`, `/v1/rerank`.

**Note on <embedding-host> availability:** <embedding-host> was recorded as *down* on 2026-06-16 but is up as of
2026-06-18 and serves the model. The code RAG depends on <embedding-host> staying reachable; the spec
RAG does not (it stays on the nomic host). This is an accepted trade-off — Qwen is markedly
better for code and lifts the context cap 16×.

### llama-swap config bump (first implementation step)

`Qwen3-Embedding-0.6B` currently caps requests at **2048 tokens**, because its block in
`~/services/llama-swap/config.yaml` sets `--ctx-size 16384` with `--parallel 8`
(16384 ÷ 8 = 2048 per slot). The bump, applied **only** to the `Qwen3-Embedding-0.6B` block:

```diff
-      --ctx-size 16384
-      --parallel 8
+      --ctx-size 32768   # Qwen3-Embedding native max; 0.6B model, KV-cache cost is small
+      --parallel 4       # 32768 / 4 = 8192 tokens per request, keeps 4-way concurrency
```

Result: **8192 tokens/request**, 4 concurrent slots. Restarting llama-swap restarts only that
model on <embedding-host>; reversible (a `.bak` of the config already exists alongside it). No other
model block changes.

#### Why 8192 (measured, not guessed)

Over 3456 brace-matched symbol bodies in `runtime/`, `include/x3d/`, `tools/`:

- p50 = 165 chars, p90 = 1178, p99 = 6329; absolute max ≈ 10.5K tokens.
- At a 2048-tok slot: **36** symbols overflow.
- At 8192 tok: only **3** overflow (a `ClassicVrmlReader` dispatcher, a `MeshBuilder` method,
  one large test body).

The win is not the 3 giants (the safety-net split below handles those regardless) — it is
that the **~33 mid-size, behavior-rich functions** (interpolators, systems, readers in the
2–6K-token band, precisely the code a spec walk scrutinizes) each embed as **one clean
whole-symbol vector** instead of fragmenting across windows.

## Architecture

Single Python script `scripts/code_rag.py`, run via `uv run python scripts/code_rag.py …`,
plus two mise tasks (`code-rag`, `code-ingest`). It reuses `spec_rag.py`'s proven `_curl`
HTTP helper pattern (curl + stdlib JSON, temp-file payloads) and its batch/shrink-retry
embed loop, adapted to the Qwen host, dimension, and query format.

Components, each with one purpose and a clear interface:

1. **`embed(texts, role)`** — POST to <embedding-host> Qwen `/v1/embeddings`. `role="document"` embeds
   raw; `role="query"` wraps each text as `Instruct: <CODE_SEARCH_TASK>\nQuery: <text>`.
   Returns 1024-d vectors. Batches (size ~32) with shrink-and-retry on context overflow,
   mirroring `spec_rag.embed`.
2. **C++ symbol chunker** (`iter_symbols(path) -> [Symbol]`) — tree-sitter-cpp parse →
   one `Symbol` per `function_definition` / method / free function / notable
   `class`/`struct` declaration. Each `Symbol` carries: `text` (full source span),
   `symbol` (`Class::method` or `freeFunc`), `kind`, `class`, `file`, `start_line`,
   `end_line`, and the leading doc-comment + file-header summary folded into an
   **embed-text** field. See [Chunking](#chunking).
3. **`scope(root)`** — file walker producing `(path, source_tag)` pairs and applying the
   include/exclude rules. See [Scope](#scope--source-tagging).
4. **`ingest(only_source=None)`** — (re)create/upsert the `x3d-cpp-code` collection: walk →
   chunk → embed → upsert points. `--source S` re-ingests just one tier without dropping the
   others (selective delete-by-filter then re-upsert).
5. **`query(q, k, sources, include_generated)`** — embed query, Qdrant search with a
   `source` filter, **dedup by `symbol`** (keep best-scoring window), print ranked hits
   (`[score] file:line  symbol  (kind)` + snippet).
6. **`symbol(name)`** — exact structured lookup: fetch the chunk(s) whose `symbol` matches
   (no fuzzy embedding search), print full source + `file:line`. Code analogue of
   `spec_rag.py node`.
7. **CLI dispatch** — subcommands + bare-query fallback + typo "did you mean" guard, copied
   in spirit from `spec_rag.py`.

### Data flow

```
ingest:   files --scope--> (path, source) --tree-sitter--> Symbols
                --embed(document)--> 1024-d vecs --> Qdrant upsert (x3d-cpp-code)

query:    "<text>" --embed(query, Instruct-wrapped)--> 1024-d vec
                --> Qdrant search (filter source) --> dedup by symbol --> ranked print
symbol:   <Name> --> Qdrant scroll/filter by payload.symbol --> full source print
```

## Chunking

**One chunk per symbol.** tree-sitter-cpp yields exact byte spans, robust on this
template-heavy, header-only code (49 of the runtime headers carry inline bodies).

Per symbol:

- **Embed-text** (what gets vectorized): the file-header doc-comment summary (first comment
  block of the file) + enclosing class name + the symbol's own leading doc-comment +
  signature + full body. This is high-signal for "what behavior does this implement" queries.
- **Payload** (what gets returned): always the **full untruncated source span**, plus
  `file`, `symbol`, `kind` (`method`/`function`/`class`/`struct`), `class` (or `""`),
  `source` (`runtime`/`test`/`generated`), `start_line`, `end_line`.

**Window-split safety net:** if a symbol's embed-text exceeds the model cap (~8192 tok after
the bump — expected for only ~3 symbols), split the body into overlapping ~6000-char windows,
**all tagged with the same `symbol`**, so an in-body query still resolves to its symbol. No
symbol is ever silently truncated. Query-time dedup-by-symbol collapses a multi-window symbol
back to one result (best-scoring window wins).

**Free / namespace-level functions and top-level structs** are captured as symbols with
`class=""`. Anonymous and trivially small fragments (< ~40 chars, mirroring spec_rag's floor)
are skipped.

## Scope & source tagging

Every chunk carries a `source` tag so the generated-binding bulk never drowns behavior signal.

| Glob | `source` |
|---|---|
| `runtime/**/*.{hpp,cpp}` (excluding `**/tests/**` and `**/vendor/**`), `include/x3d/*.hpp`, `tools/*.{hpp,cpp}` | `runtime` |
| `**/tests/*.cpp`, `include/x3d/tests/*.cpp`, `tools/tests/*.cpp` | `test` |
| `generated_cpp_bindings/**/*.{hpp,cpp}` | `generated` |

**Excluded entirely:** vendored / third-party — `runtime/script/vendor/**` (duktape),
`runtime/parse/tinfl.h`, `third_party/**` (glad). These are external deps, not our behavior.

**Query default:** `runtime` only (revised post-bring-up — see Shipped note). Tests are
opt-in via `--source test`, all tiers via `--all`. The 681 generated binding files are
searchable only with an explicit `--all` or `--source generated`. Rationale: the primary
clause-walk use case is "find the *implementation* of this clause", and prose-rich test
bodies otherwise out-rank dense implementation methods (e.g. geometry math) on concept
queries — tests remain valuable corroboration, just not the default.

## CLI surface

Mirrors `spec_rag.py`'s routing discipline (and its "did you mean" guard for typo'd
subcommands).

```
code_rag.py query "<text>" [-k N] [--source S ...] [--all]   semantic code search
code_rag.py symbol <Name>                                    exact symbol: full source + file:line
code_rag.py ingest [--source S]                              (re)build the collection (or one tier)
code_rag.py "<text>"                                         bare form == query
code_rag.py --help                                           full surface + routing rule
```

**Routing rule (baked into `--help`):** hold a concrete symbol name (`BoundsSystem::compute`)?
→ use `symbol` (exact). Only know the concept/algorithm? → use `query`. `symbol` beats a fuzzy
`query` whenever you already have the name — same discipline as `spec_rag.py node`/`field`.

`--source` is repeatable (`--source runtime --source test`); `--all` = all three tiers.

### mise tasks

```
code-rag     uv run python scripts/code_rag.py {{arg}}     # query / symbol
code-ingest  uv run python scripts/code_rag.py ingest      # rebuild collection
```

## Error handling

- **<embedding-host> unreachable:** `_curl` raises; surface a clear message naming the host and that the
  code RAG (unlike the spec RAG) depends on <embedding-host>. Non-zero exit.
- **Context overflow on embed:** shrink-and-retry loop (as in `spec_rag.embed`); if a single
  symbol still overflows after the window-split, that should not happen by construction, but
  is logged loudly rather than silently dropped.
- **Empty / unparseable file:** tree-sitter error nodes are tolerated; the file contributes
  whatever symbols parsed, with a warning count printed at end of ingest.
- **Unknown subcommand / flag:** fail loud with the `did you mean` Levenshtein hint and usage,
  exactly as `spec_rag.py`.
- **`symbol <Name>` miss:** print "no symbol <Name> in collection (did you mean a `query`?)".

## Testing

`tests/test_code_rag.py` (pytest, no network — matches `spec_rag.py`, which has no networked
test):

- Parser over a small fixture `.hpp` yields the expected set of symbols with correct
  `symbol`/`kind`/`class` and line ranges.
- The window-split fires exactly at the cap boundary for an oversized fixture symbol, and all
  windows share one `symbol` tag.
- `scope()` excludes vendored dirs (duktape/glad/tinfl) and tags runtime/test/generated
  correctly.
- Embed-text composition includes the file-header + leading doc-comment.

Live ingest + query against Qdrant/<embedding-host> is a **manual** smoke (documented in the script
docstring), consistent with `spec_rag.py` having no live-infra test in CI.

## Deferred to v2

1. **Clause→code cross-linking.** Join code chunks to prose anchors (`prose_anchors`) so a
   spec clause auto-surfaces its implementing symbols. Requires a code→clause heuristic
   (comments rarely cite section numbers); design once v1 retrieval quality is proven.
2. **`--rerank` flag.** Cross-encoder rerank of the top-K embedding hits via
   `bge-reranker-v2-m3` on `<embedding-host>:8080` for precision.
3. Possible migration of `spec_rag.py` to Qwen for parity (out of scope here).

## Implementation order

1. Apply the llama-swap `Qwen3-Embedding-0.6B` context bump on <embedding-host>; verify an 8192-tok
   embed succeeds and dim is 1024.
2. Add `tree-sitter` + `tree-sitter-cpp` to the uv dev group.
3. Build the chunker + `scope()` against fixtures (TDD).
4. Build `embed`/`ingest`/`query`/`symbol` + CLI, reusing `spec_rag.py` helpers.
5. Full ingest; manual smoke queries; wire mise tasks; document in the script docstring and a
   short note in the project docs.

## Shipped

**2026-06-18** — code RAG live on branch `code-rag`. Collection `x3d-cpp-code`: **10,717
points, 1024-d Cosine** over runtime + test + generated tiers (0 file warnings). Unit tests
11/11. Smoke: `query "bottom-up bounding box propagation"` → top hit
`BoundsSystem::propagate` (0.751, runtime); `--source test` cleanly returns only `tests/`
files. Infra: Qwen3-Embedding bumped to 8192 tok/request (llama-swap `--ctx-size 32768
--parallel 4 --batch/ubatch 8192`). One Critical bug caught by the live point-count check and
fixed: deterministic point ids omitted a per-symbol disambiguator, so overloaded symbols
(e.g. 50× `to_string` in `X3Denums.hpp`, `Material::setDiffuseColor` ×2) collided and silently
overwrote each other (694 lost); `start_line` added to the id key. Post-walk tuning: a
9-behavior spec walk confirmed excellent recall (event/animation/binding impls top even in
mixed mode; geometry impls top under runtime-only) and motivated flipping the query default
to `runtime`-only (tests opt-in).
