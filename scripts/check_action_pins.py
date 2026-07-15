#!/usr/bin/env python3
"""Assert every GitHub Action is pinned to a 40-hex commit SHA.

A floating tag (@v4) is mutable: the owner can repoint it at any commit, so a
green CI run does not describe the code that will run next time.

The check is POSITIVE by design -- it rejects anything that is not 40 hex. The
obvious `grep 'uses:.*@v[0-9]'` is a blocklist: it passes clean on @main,
@master, @stable or any arbitrary tag while claiming every ref is a SHA.

Local refs (./...) and docker:// refs are not pinnable this way and are skipped.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
WORKFLOWS = REPO_ROOT / ".github/workflows"

USES = re.compile(r'^\s*-?\s*uses:\s*([^\s#]+)', re.M)
SHA = re.compile(r"[0-9a-f]{40}")


def main() -> int:
    bad = []
    checked = 0
    for wf in sorted(WORKFLOWS.glob("*.yml")) + sorted(WORKFLOWS.glob("*.yaml")):
        for ref in USES.findall(wf.read_text()):
            if ref.startswith(("./", "docker://")):
                continue
            checked += 1
            _, sep, version = ref.partition("@")
            if not sep or not SHA.fullmatch(version):
                bad.append(f"{wf.name}: {ref}")

    if bad:
        print("UNPINNED actions (must be a 40-hex commit SHA):", file=sys.stderr)
        for b in bad:
            print(f"  {b}", file=sys.stderr)
        print("\nResolve a tag to its commit with:", file=sys.stderr)
        print("  gh api repos/OWNER/REPO/git/ref/tags/TAG --jq .object.sha", file=sys.stderr)
        print("(if that returns a tag object, deref: "
              "gh api repos/OWNER/REPO/git/tags/SHA --jq .object.sha)", file=sys.stderr)
        return 1

    print(f"All {checked} action refs pinned to 40-hex commit SHAs.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
