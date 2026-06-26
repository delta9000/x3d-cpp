#!/usr/bin/env bash
# Fetch the X3D test corpus for the CLI/canon differential gates.
#
# The corpus differential gates (`mise run cli-gate` / `canon-gate` /
# `cli-gate-regression`) and the full sweep (`mise run corpus`) validate against
# the Web3D X3D Example Archive. This script materializes exactly what the gates
# need — the committed curated subset (tools/x3d-cli/goldens/subset.txt) PLUS its
# transitive Inline/EXTERNPROTO scene dependencies — into the layout the gates
# expect:
#
#   $CORPUS/x3d-code/www.web3d.org/x3d/content/examples/...
#
# Files are pulled from the live archive at www.web3d.org. They are open-source
# under the BSD-style "Web3D Consortium Open-Source License for Models and
# Software" (each scene carries <meta name='license' content='../license.html'/>).
# We download, never redistribute — so there is no bundling obligation here.
#
# Usage:
#   scripts/fetch_corpus.sh [CORPUS_DIR]
#
# CORPUS_DIR defaults to $X3D_CORPUS_DIR, else <repo>/.x3d-corpus (gitignored).
# Re-runs are incremental: already-present files are skipped, so it is cheap to
# run again after the subset grows.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SUBSET="${REPO_ROOT}/tools/x3d-cli/goldens/subset.txt"
CORPUS_DIR="${1:-${X3D_CORPUS_DIR:-${REPO_ROOT}/.x3d-corpus}}"

PREFIX="x3d-code/www.web3d.org/x3d/content/examples/"
LIVE="https://www.web3d.org/x3d/content/examples"
EXROOT="${CORPUS_DIR}/${PREFIX%/}"
JOBS="${X3D_CORPUS_FETCH_JOBS:-16}"

command -v curl    >/dev/null || { echo "ERROR: curl not found on PATH" >&2; exit 2; }
command -v python3 >/dev/null || { echo "ERROR: python3 not found on PATH" >&2; exit 2; }
[[ -f "${SUBSET}" ]] || { echo "ERROR: subset list not found: ${SUBSET}" >&2; exit 2; }

mkdir -p "${EXROOT}"

# Fetch one examples-root-relative path. Prints GOT / HAVE / MISS.
fetch_one() {
  local rel="$1" dest="${EXROOT}/$1"
  [[ -f "${dest}" ]] && { echo "HAVE"; return; }
  mkdir -p "$(dirname "${dest}")"
  if curl -sfL --retry 2 --max-time 60 -o "${dest}" "${LIVE}/${rel}" 2>/dev/null; then
    echo "GOT"
  else
    rm -f "${dest}"; echo "MISS ${rel}"
  fi
}
export -f fetch_one
export EXROOT LIVE

echo "Corpus dir: ${CORPUS_DIR}"
echo "Source:     ${LIVE}"
echo

# ── 1. The curated subset (paths are <prefix>-relative in subset.txt) ─────────
echo "== Fetching curated subset (${SUBSET##*/}) =="
mapfile -t want < <(sed "s#^${PREFIX}##" "${SUBSET}" | grep -vE '^\s*$')
printf '%s\n' "${want[@]}" | xargs -P "${JOBS}" -I{} bash -c 'fetch_one "$@"' _ {} > /tmp/.x3d_corpus_fetch.$$ 2>&1
subset_got=$(grep -c '^GOT'  /tmp/.x3d_corpus_fetch.$$ || true)
subset_miss=$(grep -c '^MISS' /tmp/.x3d_corpus_fetch.$$ || true)
echo "  subset: ${subset_got} fetched, ${subset_miss} unavailable upstream"
if [[ "${subset_miss}" -gt 0 ]]; then
  echo "  (these are absent from the live archive — typically pruned upstream since the"
  echo "   subset was committed; the gates tolerate a few missing subset entries):"
  grep '^MISS' /tmp/.x3d_corpus_fetch.$$ | sed 's/^MISS /    - /'
fi
rm -f /tmp/.x3d_corpus_fetch.$$
echo

# ── 2. Transitive Inline / EXTERNPROTO scene-dependency closure ───────────────
# subset.txt lists files to TEST; validating them needs the scenes they pull in
# via Inline url / EXTERNPROTO url (the latter carry a "#ProtoName" fragment).
echo "== Resolving Inline/EXTERNPROTO dependency closure =="
for pass in $(seq 1 8); do
  python3 - "${EXROOT}" <<'PY' > /tmp/.x3d_corpus_deps.$$
import os, sys, re, posixpath
exroot = sys.argv[1]
refs = set()
url_re = re.compile(r'\burl\s*=\s*([\'"])(.*?)\1', re.S)
for dp, _, fs in os.walk(exroot):
    for fn in fs:
        if not fn.endswith(('.x3d', '.x3dv', '.wrl', '.json')):
            continue
        try:
            txt = open(os.path.join(dp, fn), encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        reldir = os.path.relpath(dp, exroot)
        for _q, body in url_re.findall(txt):
            for tok in re.split(r'[\s,]+', body.strip()):
                tok = tok.strip('"\'').split('#', 1)[0]          # drop EXTERNPROTO #fragment
                if not tok or tok.startswith(('http://', 'https://', 'urn:', 'data:')):
                    continue
                if not tok.lower().endswith(('.x3d', '.x3dv', '.wrl', '.json')):
                    continue
                norm = posixpath.normpath(posixpath.join(reldir.replace(os.sep, '/'), tok))
                if norm.startswith('..') or norm.startswith('/'):
                    continue
                refs.add(norm)
print('\n'.join(sorted(refs)))
PY
  got=$(grep -vE '^\s*$' /tmp/.x3d_corpus_deps.$$ | xargs -P "${JOBS}" -I{} bash -c 'fetch_one "$@"' _ {} | grep -c '^GOT' || true)
  echo "  pass ${pass}: ${got} dependency file(s) fetched"
  [[ "${got}" -eq 0 ]] && break
done
rm -f /tmp/.x3d_corpus_deps.$$
echo

scene_count=$(find "${EXROOT}" -type f \( -name '*.x3d' -o -name '*.x3dv' -o -name '*.wrl' -o -name '*.json' \) | wc -l)
echo "Done. ${scene_count} scene files under ${CORPUS_DIR} ($(du -sh "${CORPUS_DIR}" | cut -f1))."
echo
if [[ -n "${X3D_CORPUS_DIR:-}" || "${CORPUS_DIR}" == "${REPO_ROOT}/.x3d-corpus" ]]; then
  echo "The gates default to this location — run them directly:"
  echo "  mise run cli-gate-regression"
  echo "  mise run corpus"
else
  echo "Point the gates at it:"
  echo "  export X3D_CORPUS_DIR=\"${CORPUS_DIR}\""
fi
