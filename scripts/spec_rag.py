#!/usr/bin/env python3
"""X3D spec-prose RAG + structured lookups over a local prose mirror.

Subcommands:
  query "<text>" [-k N]        semantic search of the normative PROSE (default k=6).
                               Bare form also works: spec_rag.py "<text>" [-k N].
  node  <Name> [--version V] [--rag N]
                               structured facts for a node (component, containerField,
                               base type, field count) + its exact prose anchor; --rag N
                               also runs a related-prose search.
  field <Node>.<field> [--version V]
                               type / access / default / range / acceptable-node-types
                               for a field + a best-effort prose line.
  ingest                       (re)build the Qdrant collection from the mirror.

Flags: --version V picks the X3D UOM version for node/field (3.0-4.1, default 4.1);
       -k N limits query hits; --rag N attaches N prose hits to a node lookup.

WHICH ONE: hold a concrete Node or Node.field name? -> use `node` / `field` (exact,
version-aware structural truth + the authoritative prose anchor). Only know the
concept or algorithm? -> use `query`. `node`/`field` beat a fuzzy `query` whenever
you already have the name.

PLUGGABLE BACKENDS (configure via environment variables — no defaults reach a
remote host; bring your own self-hosted services):

  X3D_SPEC_PROSE_DIR  Directory of X3D normative-prose markdown (one *.md per
                      section/page, mirrored from the public web3d.org spec).
                      Required for `query`/`ingest`. No default (unset => empty).
  X3D_EMBED_URL       OpenAI-compatible embeddings endpoint
                      (POST {"model","input"} -> {"data":[{"embedding":[...]}]}).
                      Must return 768-d vectors for the configured model.
                      Default: http://localhost:8080/v1/embeddings
  X3D_EMBED_MODEL     Embedding model name. Default: nomic-embed-text-v1.5
                      (nomic task prefixes: search_document for chunks,
                      search_query for queries).
  X3D_QDRANT_URL      Base URL of a Qdrant vector store (collection
                      `x3d-spec-prose`, 768-d). Default: http://localhost:6333
"""
import json, os, re, subprocess, sys, pathlib

from x3d_cpp_gen.conformance.prose_anchors import (
    load_anchors, section_text, MANIFEST_DIR, PROSE_DIR as PROSE,
)

EMBED_URL = os.environ.get("X3D_EMBED_URL", "http://localhost:8080/v1/embeddings")
EMBED_MODEL = os.environ.get("X3D_EMBED_MODEL", "nomic-embed-text-v1.5")
QDRANT = os.environ.get("X3D_QDRANT_URL", "http://localhost:6333")
COLLECTION = "x3d-spec-prose"
DIM = 768
# The embedding server runs nomic at n_ctx=256 tokens, so chunks must be small.
# ~520 chars of spec text ≈ ~180-210 tokens, leaving headroom for the task prefix.
CHUNK_CHARS = 400
OVERLAP = 70
MAX_INPUT_CHARS = 480  # hard cap per embed input (~195 tokens at the dense ~2.46 c/tok)


def _curl(method, url, payload=None):
    cmd = ["curl", "-sS", "--max-time", "120", "-X", method, url,
           "-H", "Content-Type: application/json"]
    if payload is not None:
        import tempfile
        f = tempfile.NamedTemporaryFile("w", suffix=".json", delete=False)
        json.dump(payload, f); f.close()
        cmd += ["-d", "@" + f.name]
    out = subprocess.run(cmd, capture_output=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr.decode()[:200])
    txt = out.stdout.decode("utf-8", "replace")
    return json.loads(txt) if txt.strip().startswith(("{", "[")) else txt


def embed(texts, prefix):
    """Embed a list of texts with the nomic task prefix; returns list of 768-d vecs."""
    vecs = []
    for i in range(0, len(texts), 32):
        group = texts[i:i + 32]
        cap, r = MAX_INPUT_CHARS, None
        for _ in range(4):  # shrink-and-retry if any input overflows the 256-tok ctx
            batch = [f"{prefix}: {t[:cap]}" for t in group]
            r = _curl("POST", EMBED_URL, {"model": EMBED_MODEL, "input": batch})
            if isinstance(r, dict) and len(r.get("data", [])) == len(group):
                vecs.extend(d["embedding"] for d in r["data"])
                break
            cap = int(cap * 0.7)
        else:
            raise RuntimeError(f"embed error after retries: {str(r)[:200]}")
    return vecs


def chunk_file(path):
    """Heading-aware chunks: track the current ## section, split ~CHUNK_CHARS with overlap."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    section = path.stem
    buf, chunks = [], []
    size = 0
    def flush():
        nonlocal buf, size
        text = "\n".join(buf).strip()
        if len(text) > 40:
            chunks.append((section, text))
        # carry overlap tail
        tail = text[-OVERLAP:] if text else ""
        buf = [tail] if tail else []
        size = len(tail)
    for ln in lines:
        h = re.match(r"^#{1,4}\s+(.*)", ln)
        if h:
            if size > 0:
                flush()
            section = h.group(1).strip()[:120]
        buf.append(ln); size += len(ln) + 1
        if size >= CHUNK_CHARS:
            flush()
    if size > 0:
        flush()
    return chunks


def ingest():
    files = sorted(p for p in PROSE.glob("*.md") if p.name != "INDEX.md")
    print(f"ingesting {len(files)} files from {PROSE}")
    # (re)create collection
    _curl("DELETE", f"{QDRANT}/collections/{COLLECTION}")
    _curl("PUT", f"{QDRANT}/collections/{COLLECTION}",
          {"vectors": {"size": DIM, "distance": "Cosine"}})
    pid = 0
    total = 0
    for path in files:
        chunks = chunk_file(path)
        if not chunks:
            continue
        vecs = embed([c[1] for c in chunks], "search_document")
        points = []
        for (section, text), v in zip(chunks, vecs):
            points.append({"id": pid, "vector": v,
                           "payload": {"file": path.name, "section": section, "text": text}})
            pid += 1
        _curl("PUT", f"{QDRANT}/collections/{COLLECTION}/points", {"points": points})
        total += len(points)
        print(f"  {path.name}: {len(points)} chunks")
    print(f"DONE: {total} chunks -> {QDRANT}/collections/{COLLECTION}")


def query(q, k=6, hint=False):
    v = embed([q], "search_query")[0]
    r = _curl("POST", f"{QDRANT}/collections/{COLLECTION}/points/search",
              {"vector": v, "limit": k, "with_payload": True})
    for hit in r.get("result", []):
        p = hit["payload"]
        snippet = re.sub(r"\s+", " ", p["text"]).strip()
        print(f"\n[{hit['score']:.3f}] {p['file']} § {p['section']}")
        print("  " + (snippet[:600] + ("…" if len(snippet) > 600 else "")))
    if hint:
        print("\n# hold a node/field name? structured + version-aware: "
              "spec_rag.py node <Name>  |  spec_rag.py field <Node>.<field>")


def _load_manifest(version):
    p = MANIFEST_DIR / f"x3d-{version}.json"
    if not p.exists():
        print(f"unknown X3D version {version!r} (have 3.0-4.1)"); sys.exit(1)
    return json.loads(p.read_text())


def cmd_node(name, version="4.1", rag_k=0):
    man = _load_manifest(version)
    rec = man["nodes"].get(name)
    if not rec:
        print(f"unknown node {name!r} in X3D {version}")
        return
    comp = rec.get("component") or {}
    print(f"# {name}  [X3D {version}]"
          f"{'  (abstract)' if rec.get('abstract') else ''}")
    print(f"  component: {comp.get('name')} (L{comp.get('level')})  "
          f"containerField: {rec.get('containerField')}  base: {rec.get('baseType')}  "
          f"fields: {len(rec.get('fields', {}))}")
    art = load_anchors()
    a = art["anchors"].get(name)
    if not a:
        fz = art["fuzzy"].get(name)
        extra = f" (fuzzy: {fz['matchedAnchor']})" if fz else ""
        print(f"  prose: [no node-reference anchor]{extra}")
    else:
        print(f"  prose: {a['file']} §{a['section']} {a['title']}  "
              f"-> {a['sourceUrl']}#{a['fragment']}")
        txt = section_text(a)
        print("\n" + (txt if txt else "  [prose mirror not present]"))
    if rag_k:
        print("\n--- related prose ---")
        query(name, rag_k)
    else:
        print(f"\n# related prose: spec_rag.py node {name} --rag 6"
              f"   |   field detail: spec_rag.py field {name}.<field>")


def cmd_field(spec, version="4.1"):
    node, _, field = spec.partition(".")
    if not field:
        print("usage: spec_rag.py field <Node>.<field>")
        return
    man = _load_manifest(version)
    rec = man["nodes"].get(node)
    if not rec or field not in rec.get("fields", {}):
        print(f"unknown field {spec!r} in X3D {version}")
        return
    f = rec["fields"][field]
    print(f"# {node}.{field}  [X3D {version}]")
    print(f"  type: {f['type']}  access: {f['accessType']}  default: {f['default']}  "
          f"range: [{f['minInclusive']}, {f['maxInclusive']}]  "
          f"accepts: {f['acceptableNodeTypes']}")
    art = load_anchors()
    a = art["anchors"].get(node)
    print("\n  prose (best-effort):")
    txt = section_text(a) if a else None
    hit = None
    if txt:
        for ln in txt.splitlines():
            if re.search(rf'\b{re.escape(field)}\b', ln):
                hit = ln.strip()
                break
    if hit:
        print("   " + hit)
    else:
        query(f"{node} {field}", 3)


USAGE = (
    "usage:\n"
    '  spec_rag.py query "<text>" [-k N]                semantic prose search\n'
    "  spec_rag.py node  <Name> [--version V] [--rag N] structured node facts + anchor\n"
    "  spec_rag.py field <Node>.<field> [--version V]   field metadata + prose line\n"
    "  spec_rag.py ingest                               rebuild the Qdrant collection\n"
    '  spec_rag.py "<text>"                             bare form == query\n'
    "run with --help for the full surface and the query-vs-node/field routing rule."
)


def _die(msg, code=2):
    print(f"error: {msg}\n\n{USAGE}", file=sys.stderr); sys.exit(code)


def _pop_k(args):
    if "-k" in args:
        i = args.index("-k"); k = int(args[i + 1]); del args[i:i + 2]; return k
    return 6


_KNOWN_CMDS = ("query", "node", "field", "ingest")


def _edit_dist(a, b):
    """Tiny Levenshtein — good enough to catch typo'd subcommands (nodes->node)."""
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (ca != cb)))
        prev = cur
    return prev[-1]


def _near_cmd(token):
    hits = [c for c in _KNOWN_CMDS if _edit_dist(token.lower(), c) <= 2]
    return hits[0] if hits else None


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(USAGE, file=sys.stderr); sys.exit(1)
    cmd = sys.argv[1]
    if cmd in ("-h", "--help", "help"):
        print(__doc__); sys.exit(0)
    rest = sys.argv[2:]
    version = "4.1"
    # NOTE: every flag (--version, --rag, -k) must be stripped from `rest` before any
    # positional (rest[0]) is read, or a flag name would be captured as the positional.
    if "--version" in rest:
        i = rest.index("--version"); version = rest[i + 1]; del rest[i:i + 2]
    if cmd == "ingest":
        ingest()
    elif cmd == "node":
        rag_k = 0
        if "--rag" in rest:
            i = rest.index("--rag"); rag_k = int(rest[i + 1]); del rest[i:i + 2]
        if not rest:
            _die("'node' needs a <Name>, e.g. spec_rag.py node Transform")
        cmd_node(rest[0], version, rag_k)
    elif cmd == "field":
        if not rest:
            _die("'field' needs <Node>.<field>, e.g. spec_rag.py field Transform.center")
        cmd_field(rest[0], version)
    elif cmd == "query":
        if not rest:
            _die("'query' needs <text>")
        k = _pop_k(rest)
        query(" ".join(rest), k, hint=True)
    elif cmd.startswith("-"):
        _die(f"unknown flag {cmd!r}")
    else:
        # Back-compat bare free-text query: spec_rag.py "<text>".
        args = sys.argv[1:]
        near = _near_cmd(cmd)
        # A near-miss command WITH trailing args (e.g. `nodes Transform`) is almost
        # certainly a typo'd subcommand, not a real query -> fail loud.
        if near and len(args) > 1:
            _die(f"unknown command {cmd!r}; did you mean {near!r}? "
                 f"(to search instead: spec_rag.py query {' '.join(args)!r})")
        k = _pop_k(args)
        if near:  # lone near-miss word: run it, but say why
            print(f"# note: {cmd!r} is not a subcommand (did you mean {near!r}?); "
                  f"searching prose for it as free text. see --help", file=sys.stderr)
        elif len(args) == 1:  # lone bareword: make the fallthrough visible
            print(f"# note: no subcommand {cmd!r}; searching prose for it as free text "
                  f"(known: query/node/field/ingest; see --help)", file=sys.stderr)
        query(" ".join(args), k, hint=True)
