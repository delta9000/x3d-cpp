#!/usr/bin/env bash
# fetch_kelp.sh — mirror the third-party NPS/MOVES "Kelp Forest Exhibit" X3D
# scene (KelpForestMain.x3d + its ~22 Inline components and textures) into
# ./exhibit/ so the flythrough_kelp.x3d wrapper can Inline it.
#
# The exhibit is NOT committed to this repo (it is ~5 MB of third-party content);
# it is fetched on demand, like `mise run corpus-fetch` and the skybox
# make_cubemap.sh. ./exhibit/ is git-ignored.
#
# Source & license (see also README.md):
#   Kelp Forest Exhibit by Don Brutzman, Naval Postgraduate School (NPS)
#   MOVES Institute — https://x3dgraphics.com/examples/X3dForWebAuthors/KelpForestExhibit/
#   "All content has permissions for free use. Please provide credit to the
#    Naval Postgraduate School (NPS) MOVES Institute."
#
# Deps: wget.  Usage:  ./fetch_kelp.sh
set -euo pipefail

BASE="https://x3dgraphics.com/examples/X3dForWebAuthors/KelpForestExhibit/"
DEST="$(dirname "$0")/exhibit"
command -v wget >/dev/null || { echo "fetch_kelp: wget not found"; exit 1; }

mkdir -p "$DEST"
# Mirror the .x3d component scenes + their textures (skip thumbnails/javadoc/.wrl).
wget -q -r -np -nH --cut-dirs=3 -R "*.wrl,index.html*" \
  -A "x3d,jpg,jpeg,png,gif" \
  -X "/examples/X3dForWebAuthors/KelpForestExhibit/_thumbnails,/examples/X3dForWebAuthors/KelpForestExhibit/javadoc" \
  -P "$DEST" "$BASE"

x3d=$(find "$DEST" -name '*.x3d' | wc -l)
img=$(find "$DEST" \( -name '*.jpg' -o -name '*.png' -o -name '*.gif' -o -name '*.jpeg' \) | wc -l)
echo "fetched $x3d .x3d + $img textures into $DEST ($(du -sh "$DEST" | cut -f1))"
echo "Now render:  see README.md (or  mise run kelp-flythrough)"
