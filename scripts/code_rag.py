#!/usr/bin/env python3
"""X3D code RAG — semantic search over this repo's hand-written C++ runtime.

Sibling of scripts/spec_rag.py (the ISO-prose RAG). Where spec_rag answers
"what does the spec require?", code_rag answers "where/how do we implement it?".
Intended for the clause-by-clause conformance walk: find a clause via spec_rag,
then find its implementing code here.

Subcommands (run via `uv run python scripts/code_rag.py`):
  query "<text>" [-k N] [--source S ...] [--all]   semantic code search (default k=8)
  symbol <Name>                                     exact symbol: full source + file:line
  ingest [--source S]                               (re)build the x3d-cpp-code collection
  "<text>"                                          bare form == query

WHICH ONE: hold a concrete symbol name (BoundsSystem::compute)? -> `symbol` (exact).
Only know the concept/algorithm? -> `query`. `symbol` beats a fuzzy `query` whenever
you already have the name.

SOURCE TIERS: query searches `runtime` code only by default (the clause-walk's "find the
implementation" case — test bodies are prose-rich and otherwise out-rank dense impl). Add
`--source example` for the out-of-SDK consumers under examples/ (cpu_raster, poc_renderer,
asset_import), `--source test` to include tests as corroboration, repeat `--source` to
combine tiers, or `--all` for every tier (runtime + example + test + generated).

PLUGGABLE BACKENDS (configure via environment variables — bring your own
self-hosted services; no default points at a remote host):

  X3D_EMBED_URL    OpenAI-compatible embeddings endpoint
                   (POST {"model","input"} -> {"data":[{"embedding":[...]}]}).
                   Must return 1024-d vectors for the configured model. Query
                   format is "Instruct: <task>\\nQuery: <q>"; documents are
                   embedded raw. Default: http://localhost:8080/v1/embeddings
  X3D_EMBED_MODEL  Embedding model name. Default: Qwen3-Embedding-0.6B
  X3D_QDRANT_URL   Base URL of a Qdrant vector store (collection `x3d-cpp-code`,
                   1024-d). Default: http://localhost:6333

Live ingest/query is a manual smoke (no networked test), matching spec_rag.py.
"""
import json, os, re, sys, subprocess, pathlib, hashlib
from dataclasses import dataclass, field

import tree_sitter_cpp as tscpp
from tree_sitter import Language, Parser

CPP = Language(tscpp.language())

REPO = pathlib.Path(__file__).resolve().parent.parent

# Embedding and vector store configuration (see module docstring).
EMBED_URL = os.environ.get("X3D_EMBED_URL", "http://localhost:8080/v1/embeddings")
EMBED_MODEL = os.environ.get("X3D_EMBED_MODEL", "Qwen3-Embedding-0.6B")
QDRANT = os.environ.get("X3D_QDRANT_URL", "http://localhost:6333")
COLLECTION = "x3d-cpp-code"
DIM = 1024
CODE_SEARCH_TASK = ("Given a question about X3D runtime behavior, retrieve the C++ "
                    "code that implements it.")
# Qwen per-request window is 8192 tok (~24k chars). Split well under that for headroom;
# only a handful of giant symbols ever exceed this.
SPLIT_CHARS = 20000
WINDOW_CHARS = 18000
OVERLAP = 1000

_CMT = re.compile(r"^\s*(//+|/\*+|\*+/?|\*)\s?")

_EXCLUDE_PARTS = ("/vendor/", "/third_party/")
_EXCLUDE_NAMES = ("tinfl.h",)


def _excluded(rel):
    s = "/" + rel.as_posix()
    return any(part in s for part in _EXCLUDE_PARTS) or rel.name in _EXCLUDE_NAMES


def _tag(rel):
    parts = rel.parts
    if parts[0] == "generated_cpp_bindings":
        return "generated"
    if "tests" in parts:
        return "test"
    if parts[0] == "examples":
        return "example"
    return "runtime"


def scope(root=REPO, only=None):
    root = pathlib.Path(root)
    roots = [root / "runtime", root / "include" / "x3d", root / "tools",
             root / "generated_cpp_bindings", root / "examples"]
    out = []
    for base in roots:
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if p.suffix not in (".hpp", ".cpp", ".h") or not p.is_file():
                continue
            rel = p.relative_to(root)
            if _excluded(rel):
                continue
            tag = _tag(rel)
            if only and tag not in only:
                continue
            out.append((p, tag))
    return out


@dataclass
class Symbol:
    file: str
    symbol: str
    kind: str          # "method" | "function" | "struct"
    cls: str
    start_line: int
    end_line: int
    text: str


def _name_from_declarator(node):
    """Drill a (function_)declarator down to its innermost name node text."""
    cur = node
    while cur is not None:
        if cur.type in ("identifier", "field_identifier", "qualified_identifier",
                         "operator_name", "destructor_name", "type_identifier"):
            return cur.text.decode("utf-8", "replace")
        nxt = cur.child_by_field_name("declarator")
        if nxt is None:
            # function_declarator nests its name under "declarator"; if absent,
            # scan named children for the first name-ish node.
            nxt = next((c for c in cur.named_children
                        if c.type in ("identifier", "field_identifier",
                                      "qualified_identifier", "function_declarator",
                                      "operator_name", "destructor_name")), None)
        cur = nxt
    return None


def _enclosing_class(node):
    """Nearest enclosing class/struct name, or '' for namespace/free scope."""
    p = node.parent
    while p is not None:
        if p.type in ("class_specifier", "struct_specifier"):
            nm = p.child_by_field_name("name")
            return nm.text.decode("utf-8", "replace") if nm else ""
        p = p.parent
    return ""


def _class_has_function_def(class_node):
    body = class_node.child_by_field_name("body")
    if body is None:
        return False
    return any(c.type == "function_definition" for c in body.named_children)


def iter_symbols(source, file=""):
    """One Symbol per function_definition (method/free), plus one per pure-data
    class/struct (no method bodies). Bodies of method-bearing classes are captured
    per-method, so the class is not emitted separately (avoids duplicate text)."""
    src = source.encode("utf-8")
    tree = Parser(CPP).parse(src)
    out = []

    def line_of(byte):  # 0-based byte -> 1-based line
        return source.count("\n", 0, byte) + 1

    def walk(node):
        if node.type == "function_definition":
            decl = node.child_by_field_name("declarator")
            raw = _name_from_declarator(decl) if decl else None
            if raw:
                cls = _enclosing_class(node)
                if "::" in raw:                       # out-of-line def: A::b
                    cls = cls or raw.rsplit("::", 1)[0]
                    name = raw
                elif cls:
                    name = f"{cls}::{raw}"
                else:
                    name = raw
                kind = "method" if cls else "function"
                text = source[node.start_byte:node.end_byte]
                out.append(Symbol(file, name, kind, cls,
                                  line_of(node.start_byte), line_of(node.end_byte),
                                  text))
            return  # don't descend into a function body
        if node.type in ("class_specifier", "struct_specifier"):
            if not _class_has_function_def(node):
                nm = node.child_by_field_name("name")
                if nm:
                    name = nm.text.decode("utf-8", "replace")
                    text = source[node.start_byte:node.end_byte]
                    out.append(Symbol(file, name, "struct", "",
                                      line_of(node.start_byte), line_of(node.end_byte),
                                      text))
                return  # pure-data type captured whole; no inner symbols
            # method-bearing class: descend to capture its methods
        for c in node.named_children:
            walk(c)

    walk(tree.root_node)
    return out


def _strip_comment(line):
    return _CMT.sub("", line).rstrip()


def _is_comment_line(line):
    s = line.strip()
    return s.startswith(("//", "/*", "*")) or s.endswith("*/")


def file_header(source):
    """Leading contiguous comment block at the top of the file (after a shebang)."""
    lines = source.splitlines()
    i = 0
    if i < len(lines) and lines[i].startswith("#!"):
        i += 1
    while i < len(lines) and not lines[i].strip():
        i += 1
    out = []
    while i < len(lines) and _is_comment_line(lines[i]):
        out.append(_strip_comment(lines[i]))
        i += 1
    return "\n".join(out).strip()


def leading_comment(source, start_line):
    """Contiguous comment lines immediately above 1-based start_line."""
    lines = source.splitlines()
    i = start_line - 2  # line above the symbol (convert to 0-based, then up one)
    out = []
    while i >= 0 and _is_comment_line(lines[i]):
        out.append(_strip_comment(lines[i]))
        i -= 1
    return "\n".join(reversed(out)).strip()


def embed_text(sym, source):
    label = sym.cls or pathlib.Path(sym.file).name
    return "\n".join(p for p in (
        file_header(source),
        f"[{label}] {sym.symbol}",
        leading_comment(source, sym.start_line),
        sym.text,
    ) if p)


def _payload(sym, source_tag, win):
    return {"file": sym.file, "symbol": sym.symbol, "kind": sym.kind,
            "class": sym.cls, "source": source_tag,
            "start_line": sym.start_line, "end_line": sym.end_line,
            "text": sym.text, "win": win}


def chunks_for_symbol(sym, source, source_tag):
    et = embed_text(sym, source)
    if len(et) <= SPLIT_CHARS:
        return [{"embed": et, "payload": _payload(sym, source_tag, 0)}]
    # Oversize: window over the body; prefix each window with a short locator so the
    # vector still knows which symbol it belongs to.
    label = sym.cls or pathlib.Path(sym.file).name
    head = f"[{label}] {sym.symbol}\n"
    body = sym.text
    chunks, start, win = [], 0, 0
    step = WINDOW_CHARS - OVERLAP
    while start < len(body):
        window = body[start:start + WINDOW_CHARS]
        chunks.append({"embed": head + window, "payload": _payload(sym, source_tag, win)})
        start += step
        win += 1
    return chunks


def _embed_input(texts, role):
    if role == "query":
        return [f"Instruct: {CODE_SEARCH_TASK}\nQuery: {t}" for t in texts]
    return list(texts)


def _curl(method, url, payload=None):
    # Mirrors spec_rag._curl: curl + temp-file JSON payload, parse JSON or raw text.
    cmd = ["curl", "-sS", "--max-time", "180", "-X", method, url,
           "-H", "Content-Type: application/json"]
    tmp_path = None
    try:
        if payload is not None:
            import tempfile
            f = tempfile.NamedTemporaryFile("w", suffix=".json", delete=False)
            tmp_path = f.name
            json.dump(payload, f); f.close()
            cmd += ["-d", "@" + tmp_path]
        out = subprocess.run(cmd, capture_output=True)
        if out.returncode != 0:
            raise RuntimeError(f"curl {url}: {out.stderr.decode()[:200]}")
        txt = out.stdout.decode("utf-8", "replace")
        return json.loads(txt) if txt.strip().startswith(("{", "[")) else txt
    finally:
        if tmp_path is not None:
            os.unlink(tmp_path)


def embed(texts, role):
    """Batched Qwen embeddings (1024-d). Shrink-and-retry on context overflow."""
    vecs = []
    for i in range(0, len(texts), 16):
        group = texts[i:i + 16]
        cap, r = WINDOW_CHARS, None
        for _ in range(4):
            batch = _embed_input([t[:cap] for t in group], role)
            r = _curl("POST", EMBED_URL, {"model": EMBED_MODEL, "input": batch})
            if isinstance(r, dict) and len(r.get("data", [])) == len(group):
                vecs.extend(d["embedding"] for d in r["data"])
                break
            cap = int(cap * 0.7)
            if cap < WINDOW_CHARS:
                print(f"WARNING: embed truncating oversized text to {cap} chars "
                      f"(below WINDOW_CHARS={WINDOW_CHARS}); some content will be dropped.",
                      file=sys.stderr)
        else:
            raise RuntimeError(f"embed error after retries: {str(r)[:200]}")
    return vecs


def _point_id(file, symbol, start_line, win):
    key = f"{file}|{symbol}|{start_line}|{win}".encode("utf-8")
    return int(hashlib.sha1(key).hexdigest()[:15], 16)  # 60-bit stable id


def _points_from_chunks(chunks, vecs):
    pts = []
    for c, v in zip(chunks, vecs):
        pl = c["payload"]
        pts.append({"id": _point_id(pl["file"], pl["symbol"], pl["start_line"], pl["win"]),
                    "vector": v, "payload": pl})
    return pts


def _ensure_collection(recreate):
    if recreate:
        _curl("DELETE", f"{QDRANT}/collections/{COLLECTION}")
    info = _curl("GET", f"{QDRANT}/collections/{COLLECTION}")
    exists = isinstance(info, dict) and info.get("status") == "ok"
    if recreate or not exists:
        _curl("PUT", f"{QDRANT}/collections/{COLLECTION}",
              {"vectors": {"size": DIM, "distance": "Cosine"}})


def _dedup_by_symbol(results, k):
    best = {}
    for h in results:
        p = h["payload"]
        key = (p["file"], p["symbol"])
        if key not in best or h["score"] > best[key]["score"]:
            best[key] = h
    ranked = sorted(best.values(), key=lambda h: h["score"], reverse=True)
    return ranked[:k]


ALL_TIERS = ("runtime", "example", "test", "generated")


def query(q, k=8, sources=("runtime",), all_tiers=False):
    src_list = list(ALL_TIERS) if all_tiers else list(sources)
    v = embed([q], "query")[0]
    body = {"vector": v, "limit": k * 4, "with_payload": True,
            "filter": {"must": [{"key": "source", "match": {"any": src_list}}]}}
    r = _curl("POST", f"{QDRANT}/collections/{COLLECTION}/points/search", body)
    hits = _dedup_by_symbol(r.get("result", []), k)
    if not hits:
        print(f"(no matches in sources {src_list})")
    for h in hits:
        p = h["payload"]
        snippet = re.sub(r"\s+", " ", p["text"]).strip()
        print(f"\n[{h['score']:.3f}] {p['file']}:{p['start_line']}  "
              f"{p['symbol']}  ({p['kind']}, {p['source']})")
        print("  " + snippet[:500] + ("…" if len(snippet) > 500 else ""))
    print("\n# searched runtime code only (default). other tiers: --source example|test   "
          "| all tiers (runtime+example+test+generated): --all   "
          "| exact symbol: code_rag.py symbol <Name>")


def cmd_symbol(name):
    r = _curl("POST", f"{QDRANT}/collections/{COLLECTION}/points/scroll",
              {"filter": {"must": [{"key": "symbol", "match": {"value": name}}]},
               "limit": 50, "with_payload": True})
    pts = (r.get("result") or {}).get("points", []) if isinstance(r, dict) else []
    # Collapse multi-window chunks to one entry per (file,symbol,start_line),
    # preserving all overloads (which differ in start_line).
    seen = {}
    for pt in pts:
        p = pt["payload"]
        seen.setdefault((p["file"], p["symbol"], p["start_line"]), p)
    if not seen:
        print(f"no symbol {name!r} in collection "
              f"(did you mean a search? code_rag.py query {name!r})")
        return
    for p in seen.values():
        print(f"# {p['symbol']}  ({p['kind']}, {p['source']})  "
              f"{p['file']}:{p['start_line']}-{p['end_line']}\n")
        print(p["text"])
        print()


def ingest(only_source=None):
    """(Re)build the x3d-cpp-code Qdrant collection.

    Safe ingest modes only: no --source = whole-collection recreate; --source S =
    delete-by-filter for tier S then re-add. There is NO safe per-file/incremental
    upsert: start_line is part of the point id, so a symbol that shifts lines would
    mint a new id and leave the old point as a stale duplicate.
    """
    only = {only_source} if only_source else None
    _ensure_collection(recreate=(only_source is None))
    if only_source:  # drop just this tier before re-adding
        _curl("POST", f"{QDRANT}/collections/{COLLECTION}/points/delete",
              {"filter": {"must": [{"key": "source",
                                    "match": {"value": only_source}}]}})
    files = scope(only=only)
    print(f"ingesting {len(files)} files "
          f"(sources: {only_source or 'runtime,test,generated'})")
    total, warned = 0, 0
    for path, tag in files:
        try:
            src = path.read_text(encoding="utf-8", errors="replace")
            syms = iter_symbols(src, file=str(path.relative_to(REPO)))
        except Exception as e:
            warned += 1; print(f"  ! {path}: {str(e)[:80]}"); continue
        chunks = [c for s in syms for c in chunks_for_symbol(s, src, tag)]
        if not chunks:
            continue
        vecs = embed([c["embed"] for c in chunks], "document")
        pts = _points_from_chunks(chunks, vecs)
        _curl("PUT", f"{QDRANT}/collections/{COLLECTION}/points", {"points": pts})
        total += len(pts)
        print(f"  {path.relative_to(REPO)}: {len(syms)} symbols, {len(pts)} chunks")
    print(f"DONE: {total} chunks, {warned} file warnings -> "
          f"{QDRANT}/collections/{COLLECTION}")


USAGE = (
    "usage:\n"
    '  code_rag.py query "<text>" [-k N] [--source S ...] [--all]  semantic code search\n'
    "  code_rag.py symbol <Name>                                  exact symbol + file:line\n"
    "  code_rag.py ingest [--source S]                            (re)build the collection\n"
    '  code_rag.py "<text>"                                       bare form == query\n'
    "run with --help for the full surface and the query-vs-symbol routing rule."
)
_KNOWN_CMDS = ("query", "symbol", "ingest")


def _die(msg, code=2):
    print(f"error: {msg}\n\n{USAGE}", file=sys.stderr); sys.exit(code)


def _edit_dist(a, b):
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


def _pop_k(args):
    if "-k" in args:
        i = args.index("-k"); k = int(args[i + 1]); del args[i:i + 2]; return k
    return 8


def _pop_sources(args):
    srcs, all_ = [], False
    while "--source" in args:
        i = args.index("--source"); srcs.append(args[i + 1]); del args[i:i + 2]
    if "--all" in args:
        all_ = True; args.remove("--all")
    return (tuple(srcs) if srcs else ("runtime",)), all_


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(USAGE, file=sys.stderr); sys.exit(1)
    cmd = sys.argv[1]
    if cmd in ("-h", "--help", "help"):
        print(__doc__); sys.exit(0)
    rest = sys.argv[2:]
    if cmd == "ingest":
        only = None
        if "--source" in rest:
            i = rest.index("--source"); only = rest[i + 1]; del rest[i:i + 2]
        ingest(only)
    elif cmd == "symbol":
        if not rest:
            _die("'symbol' needs a <Name>, e.g. code_rag.py symbol BoundsSystem::compute")
        cmd_symbol(rest[0])
    elif cmd == "query":
        if not rest:
            _die("'query' needs <text>")
        sources, all_ = _pop_sources(rest); k = _pop_k(rest)
        query(" ".join(rest), k, sources, all_)
    elif cmd.startswith("-"):
        _die(f"unknown flag {cmd!r}")
    else:
        args = sys.argv[1:]
        near = _near_cmd(cmd)
        if near and len(args) > 1:
            _die(f"unknown command {cmd!r}; did you mean {near!r}? "
                 f"(to search instead: code_rag.py query {' '.join(args)!r})")
        sources, all_ = _pop_sources(args); k = _pop_k(args)
        if near:
            print(f"# note: {cmd!r} is not a subcommand (did you mean {near!r}?); "
                  f"searching as free text. see --help", file=sys.stderr)
        elif len(args) == 1:
            print(f"# note: no subcommand {cmd!r}; searching as free text "
                  f"(known: query/symbol/ingest; see --help)", file=sys.stderr)
        query(" ".join(args), k, sources, all_)
