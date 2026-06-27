#!/usr/bin/env bash
# pick-card.sh — take a Project card into work: convert the draft card to a GitHub
# issue (so the PR can `Closes #N` and auto-move it to Done) and print the agent
# handoff packet. See docs/wiki/guides/card-to-done-workflow.md.
#
# Usage:  scripts/pick-card.sh "<card title or substring>"
#         scripts/pick-card.sh --list           # show pickable (Backlog/Ready) cards
#
# Requires: gh (authed with the `project` scope), jq-free (uses python3).
set -euo pipefail

OWNER=delta9000
PROJECT=2
REPO=delta9000/x3d-cpp

items_json() { gh project item-list "$PROJECT" --owner "$OWNER" --format json --limit 200; }

if [[ "${1:-}" == "--list" ]]; then
  items_json | python3 -c '
import json,sys
for it in json.load(sys.stdin)["items"]:
    if it.get("status") in (None,"Backlog","Ready"):
        print("  [%-8s] %-16s %s" % (it.get("status","-"), it.get("seam","-"), it["title"]))
'
  exit 0
fi

QUERY="${1:?usage: pick-card.sh \"<card title>\"  (or --list)}"

# Resolve the repo's GraphQL node id at runtime (a hardcoded id silently goes
# stale if the repo is recreated/transferred — the draft→issue convert then
# fails with "Could not resolve to a node with the global id ...").
REPO_ID=$(gh repo view "$REPO" --json id -q .id)

# Resolve the card by title substring (case-insensitive); must be unique.
read -r ITEM_ID CONTENT_TYPE TITLE < <(items_json | Q="$QUERY" python3 -c '
import json,sys,os
q=os.environ["Q"].lower()
m=[it for it in json.load(sys.stdin)["items"] if q in it["title"].lower()]
if len(m)==0: sys.exit("no card matches: "+os.environ["Q"])
if len(m)>1: sys.exit("ambiguous ("+str(len(m))+" matches); be more specific:\n  "+"\n  ".join(i["title"] for i in m))
it=m[0]; print(it["id"], it.get("content",{}).get("type","DraftIssue"), it["title"])
') || { echo "$ITEM_ID" >&2; exit 1; }

SLUG=$(echo "$TITLE" | tr '[:upper:]' '[:lower:]' | sed -E 's/[^a-z0-9]+/-/g; s/^-+|-+$//g' | cut -c1-40)
BRANCH="feat/${SLUG}"

echo "Card: $TITLE"
echo "Item: $ITEM_ID ($CONTENT_TYPE)"

# Convert draft → issue if needed (idempotent: skip if already an Issue).
if [[ "$CONTENT_TYPE" == "Issue" ]]; then
  ISSUE_URL=$(items_json | python3 -c '
import json,sys,os
for it in json.load(sys.stdin)["items"]:
    if it["id"]==os.environ["I"]: print(it.get("content",{}).get("url","")); break
' I="$ITEM_ID")
  echo "Already an issue: $ISSUE_URL"
else
  echo "Converting draft → issue in $REPO …"
  ISSUE_URL=$(gh api graphql -f query='
    mutation($item:ID!,$repo:ID!){
      convertProjectV2DraftIssueItemToIssue(input:{itemId:$item, repositoryId:$repo}){
        item { content { ... on Issue { number url } } } } }' \
    -f item="$ITEM_ID" -f repo="$REPO_ID" \
    --jq '.data.convertProjectV2DraftIssueItemToIssue.item.content.url')
  echo "Created issue: $ISSUE_URL"
fi
ISSUE_NUM="${ISSUE_URL##*/}"

cat <<EOF

──────────────────────── AGENT HANDOFF PACKET ────────────────────────
Repo: $(pwd)
  Work on a feature branch off main:  git switch -c $BRANCH
  Open a PR; do NOT commit to main. PR body must include "Closes #$ISSUE_NUM"
  and the ticked Definition of Done.
Issue:  $ISSUE_URL
BUILD RULE: cmake --build --preset dev   (matches mise run build)
GOLDEN RULE: state codegen-free vs codegen-authorized; codegen-free => golden hash byte-identical.
VERIFY: run the card's verification command AND \`mise run ci\` to green before the PR.
DOCS:   update the living docs named on the card in THIS PR (\`mise run docs-drift\` pre-check).
PLAN:   substantial card => design spec + ADR + five-phase workflow first;
        small card => one agent: writing-plans -> test-driven-development -> implement.
See: docs/wiki/guides/card-to-done-workflow.md
───────────────────────────────────────────────────────────────────────

Issue body (acceptance criteria / anchors / verify / docs):
EOF
gh issue view "$ISSUE_NUM" --repo "$REPO" --json body --jq .body 2>/dev/null || echo "(open $ISSUE_URL)"
