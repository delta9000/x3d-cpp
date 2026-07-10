#!/usr/bin/env python3
"""Docs-drift suggester — RAG a commit's code changes against the living docs and
suggest which pages may now be stale.

Third sibling of scripts/code_rag.py (impl search) and scripts/spec_rag.py (ISO
prose). Where those answer "where is it implemented?" / "what does the spec say?",
this answers "which docs talk about what I just changed, and might now be wrong?".

It is ADVISORY, not a gate: it always exits 0. The gate layer is `mkdocs --strict`
(dead links / orphans) + `conformance-gate` (findings <-> view). Semantic staleness
(a page describing behavior the code no longer has) needs judgment, so this tool
surfaces candidates for a human/agent to review — it never blocks.

Two signals per changed code file:
  * LITERAL CITATION — a doc names the changed file path or a changed symbol. Highest
    confidence: this is exactly how physics.md drifted (it cited the physics files,
    then described them wrong).
  * SEMANTIC (RAG)    — the change embeds near a doc section even without naming it;
    also catches the "shipped a subsystem with no doc at all" gap.

Subcommands (run via `uv run python scripts/docs_drift.py`):
  ingest                          (re)build the x3d-cpp-docs collection from living docs
  suggest [<rev>] [-k N]          suggest docs to review. <rev> = a commit, OR 'working'
                                  (uncommitted vs HEAD, incl. untracked), OR 'auto'
                                  (default: working if the tree is dirty, else HEAD)

Living docs = docs/wiki/, docs/sdk/, docs/conformance/. docs/superpowers/ (dated
specs/plans) is the historical record and is intentionally excluded.

Embeddings + vector store are shared with code_rag.py and configured through the
same X3D_EMBED_URL / X3D_QDRANT_URL environment variables (see code_rag.py's
module docstring). Live ingest/suggest is a manual smoke.
"""
import hashlib
import os
import re
import subprocess
import sys

from code_rag import (DIM, EMBED_MODEL, EMBED_URL, QDRANT, REPO, _curl,
                       iter_symbols)

DOCS_COLLECTION = "x3d-cpp-docs"
DOC_TASK = ("Given a description of a code change, retrieve the documentation "
            "section that describes the affected runtime behavior.")

# Living-doc roots (relative to repo). docs/superpowers/ is excluded on purpose.
DOC_ROOTS = ("docs/wiki", "docs/sdk", "docs/conformance")
# Changed-code roots that have docs worth checking. generated bindings are
# machine-emitted (their "doc" is the generator), so they are excluded; so is
# vendored third-party code (not ours to document). examples/ is included so the
# out-of-SDK consumers (cpu_raster, poc_renderer, asset_import) are drift-checked
# against their subsystem pages — they ship documented behavior too.
CODE_ROOTS = ("runtime/", "tools/", "include/x3d/", "examples/")
CODE_SUFFIXES = (".hpp", ".cpp", ".h")

MAX_PER_FILE = 6  # cap suggestions per changed file (advisory worklist, not a dump)

_HEADING = re.compile(r"^(#{1,4}) +(.*)$")
_HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


# ---------------------------------------------------------------- git helpers
def _git(*args):
    r = subprocess.run(["git", "-C", str(REPO), *args],
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)}: {r.stderr.strip()[:200]}")
    return r.stdout


def _is_doc(path):
    return path.endswith(".md") and any(path.startswith(r + "/") or path == r
                                        for r in DOC_ROOTS)


def _is_code(path):
    return (path.endswith(CODE_SUFFIXES)
            and any(path.startswith(r) for r in CODE_ROOTS)
            and "generated_cpp_bindings" not in path
            and "/third_party/" not in path
            and "/vendor/" not in path)


# Pseudo-revs meaning "the uncommitted working tree (staged + unstaged + untracked)
# vs HEAD", and "auto" = working tree if dirty, else HEAD (the ergonomic default for
# the pre-commit discipline check).
_WORKING = {"working", "wip", "uncommitted", "dirty", "."}


def _is_working(rev):
    return rev.lower() in _WORKING


def resolve_rev(rev):
    """Map 'auto' to 'working' when the tree is dirty, else 'HEAD'."""
    if rev.lower() == "auto":
        return "working" if _git("status", "--porcelain").strip() else "HEAD"
    return rev


def changed_paths(rev):
    """(code_files, doc_files, untracked_set) for a commit or the working tree.
    untracked_set marks brand-new files (no diff base -> whole file is 'changed')."""
    if _is_working(rev):
        tracked = _git("diff", "--name-only", "HEAD").splitlines()
        untracked = _git("ls-files", "--others", "--exclude-standard").splitlines()
        names = tracked + untracked
        unt = set(untracked)
    else:
        names = _git("diff", "--name-only", f"{rev}~1", rev).splitlines()
        unt = set()
    return ([p for p in names if _is_code(p)],
            [p for p in names if _is_doc(p)], unt)


def changed_new_lines(rev, path, untracked):
    """NEW-side line numbers touched in `path`. None => whole file is new/changed
    (an untracked file has no diff base)."""
    if path in untracked:
        return None
    base = ["diff", "-U0", "HEAD", "--", path] if _is_working(rev) \
        else ["diff", "-U0", f"{rev}~1", rev, "--", path]
    out = set()
    for ln in _git(*base).splitlines():
        m = _HUNK.match(ln)
        if m:
            start = int(m.group(1))
            count = int(m.group(2) or 1)
            out.update(range(start, start + max(count, 1)))
    return out


def changed_symbols(rev, path, lines, untracked):
    """Symbol names whose body overlaps a changed line. The post-state source is
    the working-tree file (working/untracked mode) or `git show rev:path` (commit).
    lines=None (new file) => every symbol is reported."""
    if _is_working(rev) or path in untracked:
        try:
            src = (REPO / path).read_text(encoding="utf-8", errors="replace")
        except OSError:
            return []  # deleted in the working tree
    else:
        try:
            src = _git("show", f"{rev}:{path}")
        except RuntimeError:
            return []  # deleted in this commit
    syms = iter_symbols(src, file=path)
    if lines is None:
        return [s.symbol for s in syms]
    return [s.symbol for s in syms
            if any(s.start_line <= ln <= s.end_line for ln in lines)]


# ---------------------------------------------------------------- markdown chunking
def md_sections(text):
    """Split markdown into (heading, start_line, body) on H1-H4 boundaries.
    Frontmatter / preamble before the first heading is dropped (the H1 repeats
    the title anyway)."""
    out, head, start, buf = [], None, 1, []
    for i, ln in enumerate(text.splitlines(), 1):
        m = _HEADING.match(ln)
        if m:
            if head is not None and buf:
                out.append((head, start, "\n".join(buf)))
            head, start, buf = m.group(2).strip(), i, [ln]
        else:
            buf.append(ln)
    if head is not None and buf:
        out.append((head, start, "\n".join(buf)))
    return out


def _doc_files():
    out = []
    for root in DOC_ROOTS:
        base = REPO / root
        if not base.exists():
            continue
        out += sorted(str(p.relative_to(REPO)) for p in base.rglob("*.md"))
    return out


# ---------------------------------------------------------------- embeddings
def embed(texts, role):
    """Batched Qwen embeddings (1024-d). role=query adds the instruct prefix."""
    if role == "query":
        texts = [f"Instruct: {DOC_TASK}\nQuery: {t}" for t in texts]
    vecs = []
    for i in range(0, len(texts), 16):
        group = [t[:18000] for t in texts[i:i + 16]]
        r = _curl("POST", EMBED_URL, {"model": EMBED_MODEL, "input": group})
        if not (isinstance(r, dict) and len(r.get("data", [])) == len(group)):
            raise RuntimeError(f"embed error: {str(r)[:200]}")
        vecs.extend(d["embedding"] for d in r["data"])
    return vecs


def _point_id(path, start):
    key = f"{path}|{start}".encode("utf-8")
    return int(hashlib.sha1(key).hexdigest()[:15], 16)


# ---------------------------------------------------------------- ingest
def ingest():
    _curl("DELETE", f"{QDRANT}/collections/{DOCS_COLLECTION}")
    _curl("PUT", f"{QDRANT}/collections/{DOCS_COLLECTION}",
          {"vectors": {"size": DIM, "distance": "Cosine"}})
    files = _doc_files()
    print(f"ingesting {len(files)} living docs into {DOCS_COLLECTION}")
    total = 0
    for path in files:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        secs = md_sections(text)
        if not secs:
            continue
        embeds = [f"{path} — {h}\n{body}" for h, _, body in secs]
        vecs = embed(embeds, "document")
        pts = [{"id": _point_id(path, start), "vector": v,
                "payload": {"path": path, "heading": h, "start_line": start,
                            "text": body[:1200]}}
               for (h, start, body), v in zip(secs, vecs)]
        _curl("PUT", f"{QDRANT}/collections/{DOCS_COLLECTION}/points",
              {"points": pts})
        total += len(pts)
    print(f"DONE: {total} sections -> {QDRANT}/collections/{DOCS_COLLECTION}")


# ---------------------------------------------------------------- suggest
def _semantic(query_text, k):
    v = embed([query_text], "query")[0]
    r = _curl("POST", f"{QDRANT}/collections/{DOCS_COLLECTION}/points/search",
              {"vector": v, "limit": k, "with_payload": True})
    return r.get("result", []) if isinstance(r, dict) else []


def _load_docs():
    return {p: (REPO / p).read_text(encoding="utf-8", errors="replace")
            for p in _doc_files()}


# Generic identifiers that match prose everywhere — never citation-worthy on
# their own (a doc saying "update" tells us nothing about NavigationSystem).
_COMMON = {"main", "update", "attach", "step", "drain", "init", "run", "build",
           "report", "clear", "reset", "get", "set", "begin", "end", "apply",
           "emit", "make", "compute", "process", "tick", "advance", "walk"}

# Citation tiers, strongest first. A doc that names the changed *file* is the
# most likely to be stale; a qualified Class::method next; a bare class/function
# name last. (Semantic hits score < ~0.7, so they always rank below citations.)
_TIER = {"path": 1.0, "qual": 0.95, "name": 0.9}


def _distinctive(name):
    """A bare (unqualified) identifier specific enough to be a citation needle."""
    return (name not in _COMMON and len(name) >= 6
            and (any(c.isupper() for c in name[1:]) or "_" in name))


def _needles(path, syms):
    """[(needle, tier)] for one changed file: the path, plus qualified symbols
    and distinctive class / free-function names. Generic method tails dropped."""
    out = [(path, "path")]
    for name in syms:  # syms are symbol-name strings (e.g. "NavigationSystem::flyUpdate")
        if "::" in name:
            out.append((name, "qual"))
            cls = name.split("::", 1)[0]
            if cls[:1].isupper() and len(cls) >= 5:
                out.append((cls, "name"))
        elif _distinctive(name):
            out.append((name, "name"))
    return out


def _citations(needles, docs):
    """{doc_path: (score, why)} for docs literally naming a needle. Name-tier
    needles match on word boundaries; path/qualified match as substrings."""
    res = {}
    for needle, tier in needles:
        score = _TIER[tier]
        if tier == "name":
            pat = re.compile(rf"\b{re.escape(needle)}\b")
            hit = pat.search
        else:
            hit = lambda text, n=needle: n in text
        for path, text in docs.items():
            if hit(text):
                prev = res.get(path)
                if prev is None or score > prev[0]:
                    res[path] = (score, f"cites `{needle}`")
    return res


def suggest(rev, k=5):
    rev = resolve_rev(rev)
    working = _is_working(rev)
    code, doc_changes, untracked = changed_paths(rev)
    if working:
        subject = "(uncommitted working tree vs HEAD)"
        label = "working tree"
        scope = "changed"
    else:
        subject = _git("log", "-1", "--format=%s", rev).strip()
        label = rev[:12]
        scope = "this commit"
    print(f"\n# docs-drift suggest — {label}  \"{subject}\"")
    if not code:
        print(f"  no tracked code files {scope} — nothing to check.")
        return
    if doc_changes:
        print(f"  docs already updated ({scope}, excluded): "
              f"{', '.join(doc_changes)}")
    docs = _load_docs()
    already = set(doc_changes)

    for path in code:
        lines = changed_new_lines(rev, path, untracked)
        syms = changed_symbols(rev, path, lines, untracked)
        new_file = path in untracked
        cited = _citations(_needles(path, syms), docs)
        qtext = f"{subject}\nfile: {path}\nchanged symbols: {', '.join(syms) or '(file-level)'}"
        sem = _semantic(qtext, k)

        candidates = {}  # doc -> (score, why)
        for d, (score, why) in cited.items():
            if d not in already:
                candidates[d] = (score, why)
        for h in sem:
            d = h["payload"]["path"]
            if d in already or d in candidates:
                continue
            candidates[d] = (h["score"], f"~ {h['payload']['heading']}")

        kind = "NEW FILE" if new_file else (", ".join(syms) if syms else "file-level change")
        print(f"\n  {path}  [{kind}]")
        if not candidates:
            print("    (no doc references this change — if it shipped new behavior, "
                  "it may need a NEW page)")
            continue
        ranked = sorted(candidates.items(), key=lambda kv: -kv[1][0])
        for d, (score, why) in ranked[:MAX_PER_FILE]:
            tag = "CITES" if score >= 0.9 else f"{score:.2f}"
            print(f"    {tag:>5}  {d}   ({why})")
        if len(ranked) > MAX_PER_FILE:
            print(f"    … +{len(ranked) - MAX_PER_FILE} more (showing top {MAX_PER_FILE})")

    print("\n# advisory only (exit 0). review the CITES hits first — a doc that "
          "names changed code is the most likely to be stale.")


USAGE = (
    "usage:\n"
    "  docs_drift.py ingest                  (re)build the x3d-cpp-docs collection\n"
    "  docs_drift.py suggest [<rev>] [-k N]  suggest docs to review\n"
    "    <rev>  a commit (checks <rev>~1..<rev>) | 'working' (uncommitted vs HEAD,\n"
    "           incl. untracked) | 'auto' = working if dirty else HEAD (default)\n"
)

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args or args[0] in ("-h", "--help", "help"):
        print(__doc__ if args[:1] in (["--help"], ["help"]) else USAGE)
        sys.exit(0)
    cmd, rest = args[0], args[1:]
    if cmd == "ingest":
        ingest()
    elif cmd == "suggest":
        k = 5
        if "-k" in rest:
            i = rest.index("-k"); k = int(rest[i + 1]); del rest[i:i + 2]
        suggest(rest[0] if rest else "auto", k)
    else:
        print(f"error: unknown command {cmd!r}\n\n{USAGE}", file=sys.stderr)
        sys.exit(2)
