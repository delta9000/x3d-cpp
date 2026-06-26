---
title: Using the RAGs
summary: How to run spec_rag, code_rag, and docs_drift — exact commands, subcommand routing, and the gotchas.
tags: [guide, rag, spec-rag, code-rag, docs-drift, conformance]
updated: 2026-06-21
related:
  - ../knowledge-map.md
---

# Using the RAGs

The project has three semantic-search tools: `spec_rag.py` (ISO prose) and `code_rag.py`
(C++ implementation) for the conformance walk, plus `docs_drift.py` (doc-drift
suggester) for keeping the docs in step with the code. This guide is the hands-on
companion to [Knowledge Map](../knowledge-map.md), which explains what each tool indexes
and when to reach for it. Here the focus is: exact commands, the routing rules you must
follow, and the gotchas that trip people up.

---

## Before you start: prerequisites

Both tools call external services over Tailscale. From any shell that does not have the
Tailscale VPN active, every call fails with a connection-refused or timeout error from the
underlying `curl` call — there is no partial-result fallback.

`code_rag.py` has an additional dependency: the Qwen embedding host (`http://<embedding-host>:8080` on the
private network) must be up. `spec_rag.py` uses a separate, self-hosted nomic
embedding server (`https://<embedding-host>`) and does not depend on the Qwen host.

Always run via `uv run python scripts/<tool>.py`, never as a bare `python` call. The
`uv run` prefix activates the project virtualenv and resolves `tree_sitter_cpp` and
other deps. A bare `python scripts/code_rag.py` will error with an import failure from a
shell that does not have the venv active. (`mise run code-rag` / `mise run docs-drift`
already wrap the call in `uv run`.)

### Machine-local configuration

The scripts' built-in defaults point at **localhost** (`X3D_QDRANT_URL=http://localhost:6333`,
`X3D_EMBED_URL=http://localhost:8080/v1/embeddings`), but the real Qdrant and (often) the
prose mirror live elsewhere. Nothing is committed that overrides them, so on a fresh
clone a bare `mise run code-rag …` fails with `connection refused on localhost:6333`
until you point it at the right hosts. Set them in **`mise.local.toml`** (gitignored, per
machine — don't commit private hostnames):

```toml
[env]
X3D_QDRANT_URL     = "https://<qdrant-host>"          # Qdrant over Tailscale
X3D_SPEC_PROSE_DIR = "/path/to/x3d_spec_prose"        # spec prose mirror (see below)
```

The embedding endpoint can stay at the localhost default if a local proxy (e.g.
llama-swap on `:8080`) serves both `Qwen3-Embedding-0.6B` and `nomic-embed-text-v1.5`;
otherwise also set `X3D_EMBED_URL` to the Qwen host. `mise env | grep X3D_` confirms what
resolves.

---

## spec\_rag.py — X3D ISO prose search

`scripts/spec_rag.py` searches the local prose mirror at
`$X3D_SPEC_PROSE_DIR/*.md` (63 pages, grabbed from
`https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/`). The mirror
is NOT inside this repo — it typically lives in the sibling **x3d-render** checkout
(`x3d-render/data/x3d_spec_prose/`); point `X3D_SPEC_PROSE_DIR` at it (see
[Machine-local configuration](#machine-local-configuration)). The prose is indexed into
Qdrant collection `x3d-spec-prose` (4,909 chunks, 768-d Cosine, nomic-embed-text-v1.5).
`query` runs purely off the indexed vectors; only `ingest` and the `node`/`field` anchor
lookups read the on-disk mirror.

### Subcommands

```
uv run python scripts/spec_rag.py query "<text>" [-k N]
uv run python scripts/spec_rag.py node  <Name> [--version V] [--rag N]
uv run python scripts/spec_rag.py field <Node>.<field> [--version V]
uv run python scripts/spec_rag.py ingest
```

**`query "<text>" [-k N]`** — semantic cosine search over all prose chunks. Returns up
to `k` ranked snippets (default 6) with file, section, and score. Use this only when you
know a concept or algorithm but not yet the node or field name.

```
uv run python scripts/spec_rag.py query "how texCoord is generated for ElevationGrid"
uv run python scripts/spec_rag.py query "event cascade timing model" -k 10
```

The bare form `spec_rag.py "<text>"` is an alias for `query`.

**`node <Name> [--version V] [--rag N]`** — structured facts from the 3.0–4.1 UOM
manifests: component, containerField, base type, field count — plus the exact prose
section anchor (file, section, source URL). Use this whenever you already have the node
name. It is exact and version-aware, and always faster than a fuzzy `query`. `--rag N`
attaches N related prose hits from the semantic index.

```
uv run python scripts/spec_rag.py node Transform
uv run python scripts/spec_rag.py node ElevationGrid --version 4.1 --rag 4
uv run python scripts/spec_rag.py node Material --version 3.3
```

**`field <Node>.<field> [--version V]`** — type, accessType, default, range, and
acceptable-node-types from the UOM manifest, plus a best-effort prose line extracted from
the node's anchor section. Falls back to a semantic query if the field name does not
appear in the anchor text.

```
uv run python scripts/spec_rag.py field Transform.center
uv run python scripts/spec_rag.py field Material.diffuseColor --version 3.3
uv run python scripts/spec_rag.py field IndexedFaceSet.creaseAngle
```

**`ingest`** — rebuilds the `x3d-spec-prose` Qdrant collection from the prose mirror.
Run only when the mirror has been updated (it is not kept in this repo).

```
uv run python scripts/spec_rag.py ingest
```

### Routing rule

> Hold a concrete node or field name? Use `node` or `field` — they are exact,
> version-aware, and backed by the UOM manifests + authoritative prose anchors. Only know
> a concept or algorithm? Use `query`. `node`/`field` beat a fuzzy `query` whenever you
> already have the name.

Unknown or typo'd subcommands are caught: `nodes Transform` prints "did you mean
'node'?" rather than silently running a prose search for the word "nodes".

### Version flag

`--version` accepts any X3D version from 3.0 to 4.1 (default 4.1). The runtime is a
single 4.1-superset build, so 4.1 is correct for most queries. Use `--version 3.3` only
when auditing a file that is explicitly 3.x and you need to see the older field set.

---

## code\_rag.py — C++ implementation search

`scripts/code_rag.py` searches the repo's hand-written C++ — `runtime/`,
`include/x3d/`, `tools/` — plus optionally the generated bindings tier. Each chunk is
one symbol (function definition, method, or pure-data struct) extracted by tree-sitter-cpp.
The collection is `x3d-cpp-code` (1024-d Cosine, Qwen3-Embedding-0.6B on <embedding-host>).

The convenience alias via mise is `mise run code-rag`.

### Subcommands

```
uv run python scripts/code_rag.py query "<text>" [-k N] [--source test] [--all]
uv run python scripts/code_rag.py symbol <Name>
uv run python scripts/code_rag.py ingest [--source S]
```

**`query "<text>" [-k N] [--source ...] [--all]`** — semantic cosine search over C++
symbols. Default source tier is `runtime` only. Test bodies are prose-rich and tend to
out-rank dense implementation code, so they are opt-in. Results are de-duplicated by
(file, symbol) — windowed chunks of the same symbol collapse to the best-scoring hit.

```
uv run python scripts/code_rag.py query "how are bounds computed"
uv run python scripts/code_rag.py query "event cascade routing" --source test
uv run python scripts/code_rag.py query "interpolation" --all
mise run code-rag query "texture coordinate generation"
```

Repeat `--source` to combine tiers: `--source runtime --source test`.
`--all` includes `runtime`, `test`, and the generated bindings tier.

**`symbol <Name>`** — exact lookup by C++ symbol name. Returns full source text, file,
and line range. Faster and unambiguous when you already know the name.

```
uv run python scripts/code_rag.py symbol BoundsSystem::compute
uv run python scripts/code_rag.py symbol ScriptEngine::tick
uv run python scripts/code_rag.py symbol CycleBreaker::breakContainmentCycles
```

**`ingest [--source S]`** — rebuilds the collection. Without `--source` it recreates the
full collection (all tiers). With `--source runtime` (or `test`, `generated`) it drops
and re-adds only that tier. There is no per-file incremental update: symbol point IDs
embed `start_line`, so a symbol that shifts lines would mint a new point and leave a stale
duplicate; always re-ingest the whole tier.

```
uv run python scripts/code_rag.py ingest
uv run python scripts/code_rag.py ingest --source runtime
mise run code-ingest
```

### Routing rule

> Hold a concrete C++ symbol name? Use `symbol` — exact, no ambiguity. Only know the
> concept or algorithm? Use `query` over the `runtime` tier first. Add `--source test` to
> corroborate with test prose.

---

## docs\_drift.py — doc-drift suggester

`scripts/docs_drift.py` answers a different question from its siblings: not "where is
this implemented?" but **"which docs talk about what I just changed, and might now be
wrong?"** It RAGs a commit's code changes against the *living* docs (`docs/wiki/`,
`docs/sdk/`, `docs/conformance/`) and prints a worklist of pages to review. It is
**advisory — it always exits 0**, never a gate. The gate layer is `mkdocs --strict`
(dead links / orphans) + `conformance-gate` (findings ↔ view); semantic staleness (a
page describing behavior the code no longer has) needs judgment, so this tool *suggests*
rather than blocks. `docs/superpowers/` (dated specs/plans) is excluded by design — it is
the historical record, not updated.

It reuses code\_rag's embedding + Qdrant plumbing (Qwen on <embedding-host>, Qdrant on <qdrant-host>);
the collection is `x3d-cpp-docs` (markdown sections, 1024-d Cosine). Two signals per
changed code file:

- **CITES** (literal) — a doc names the changed file path or a qualified `Class::method`
  / distinctive symbol. Highest confidence: this is exactly how `physics.md` drifted (it
  cited the physics files, then described them wrong). Works even on a stale collection.
- **0.NN** (semantic) — the change embeds near a doc section without naming it; also
  catches "shipped a subsystem with no doc at all".

### Subcommands

```
uv run python scripts/docs_drift.py ingest             # (re)build x3d-cpp-docs; alias: mise run docs-ingest
uv run python scripts/docs_drift.py suggest [<rev>]    # review-list; alias: mise run docs-drift
```

`<rev>` is a commit (checks `<rev>~1..<rev>`), or the keyword `working` (uncommitted
changes vs HEAD, **including untracked new files**), or `auto` — the default — which is
`working` when the tree is dirty and `HEAD` when it is clean. So the bare command checks
whatever you have in flight, which is what you want *before* committing:

```
mise run docs-drift              # your uncommitted work (or HEAD if the tree is clean)
mise run docs-drift working      # force the uncommitted-vs-HEAD check
mise run docs-drift af673bf      # a specific commit
```

**Review the CITES hits first** — a doc that names the code you changed is the most
likely to be stale; a `[NEW FILE]` with no citations probably needs a brand-new page.
Re-run `mise run docs-ingest` after you add or restructure doc pages so the semantic half
stays current (the CITES half is a grep and does not need it).

---

## Doing a conformance clause walk

The intended usage pattern for a conformance audit is:

1. Find the spec clause: `spec_rag.py node <Name>` for structural facts + the prose
   anchor. Add `--rag 4` for related prose.
2. Find the implementation: `code_rag.py query "<clause concept>"`. If you know the
   implementing class, use `code_rag.py symbol <Class::method>` instead.
3. Compare the normative text against the C++ implementation for conformance gaps.

Example (auditing Material shininess):

```
uv run python scripts/spec_rag.py field Material.shininess
uv run python scripts/code_rag.py query "Material shininess specular"
```

---

## Gotchas

**Must use `uv run`** — `python scripts/spec_rag.py` will fail if the virtualenv is not
active. Always prefix with `uv run python`.

**Tailscale must be on** — both tools call `https://<qdrant-host>` for vector
search. `code_rag.py` additionally calls `http://<embedding-host>:8080` for Qwen embeddings.
Connection-refused errors mean one of these is unreachable.

**Qwen-host independence** — `spec_rag.py` does not depend on the Qwen embedding host; it uses the
separate self-hosted nomic host. If the Qwen host is down, `spec_rag.py` still works; `code_rag.py` does not.

**No networked tests** — neither tool has a networked test in the ctest suite. Both are
manually smoked after ingestion. A failed query or ingest surfaces as a `curl` error in
stderr.

**Prose mirror is outside this repo** — the prose mirror lives at
`$X3D_SPEC_PROSE_DIR/*.md` (typically `x3d-render/data/x3d_spec_prose/`). It is not
version-controlled here.
If you re-run `spec_rag.py ingest` and the mirror has stale content, the collection will
reflect whatever is there.

**`node`/`field` need the UOM manifests** — the structured subcommands load manifests
from the package (`x3d_cpp_gen.conformance.prose_anchors`, `MANIFEST_DIR`). These are
part of the installed package; they are present as long as `uv run` resolves the project.

**`code_rag.py ingest` is whole-tier, not per-file** — running ingest on a single file
is not safe (stale duplicates from shifted line numbers). When you add or significantly
refactor a runtime file, re-ingest the full `runtime` tier.

---

See [Knowledge Map](../knowledge-map.md) for the decision table on which tool to reach
for in each situation, and for infrastructure details (Qdrant URLs, embedding models,
collection sizes).
