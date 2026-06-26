#!/usr/bin/env bash
# make_cubemap.sh — regenerate a six-face skybox cubemap (PNG) from a Poly Haven
# equirectangular HDRI. Reproduces examples/cpu_raster/assets/skybox/<name>/.
#
# WHY ffmpeg + ImageMagick (not ImageMagick alone): ImageMagick has no
# equirectangular -> cube-face (gnomonic) projection operator, so the geometric
# reprojection is done by ffmpeg's `v360` filter. ImageMagick does the part it
# is good at: HDR->LDR tone shaping and PNG packaging.
#
# Deps: curl, ffmpeg (with the v360 filter), ImageMagick (`convert`).
# Usage:
#   ./make_cubemap.sh                      # defaults: suburban_garden @ 4k -> 1024px faces
#   ./make_cubemap.sh <slug> <res> <size>  # e.g. ./make_cubemap.sh suburban_garden 2k 512
set -euo pipefail

SLUG="${1:-suburban_garden}"   # Poly Haven asset slug
RES="${2:-4k}"                 # source HDRI resolution (1k/2k/4k/...)
SIZE="${3:-1024}"              # output face edge in pixels
OUT="$(dirname "$0")/$SLUG"
WORK="$(mktemp -d)"
HDR="$WORK/$SLUG.hdr"

# 1) Fetch the equirectangular HDRI (CC0). The download URL comes from the API
#    so it tracks Poly Haven's storage layout.
URL="$(curl -fsS "https://api.polyhaven.com/files/$SLUG" \
  | python3 -c "import json,sys;print(json.load(sys.stdin)['hdri']['$RES']['hdr']['url'])")"
echo "downloading $URL"
curl -fsS -o "$HDR" "$URL"

# 2) Project to six 90-degree rectilinear faces with ffmpeg v360, then tone-map
#    with ImageMagick (gamma + sigmoidal contrast lifts the diffuse scene out of
#    the HDR's sun-relative darkness; the sun itself stays a clipped highlight).
#    Face -> (yaw,pitch). Looking from the origin: front=-Z, back=+Z, right=+X,
#    left=-X, top=+Y, bottom=-Y (X3D Background face convention).
TONE=(-gamma 1.8 -sigmoidal-contrast 3x42%)
mkdir -p "$OUT"
gen() { # name yaw pitch
  ffmpeg -y -hide_banner -loglevel error -i "$HDR" \
    -vf "v360=e:flat:h_fov=90:v_fov=90:yaw=$2:pitch=$3:w=$SIZE:h=$SIZE,format=rgb24" \
    -frames:v 1 "$WORK/$1.png"
  convert "$WORK/$1.png" "${TONE[@]}" "$OUT/$1.png"
  echo "  $OUT/$1.png"
}
gen front 0 0
gen back  180 0
gen right 90 0
gen left  -90 0
gen top   0 90
gen bottom 0 -90

rm -rf "$WORK"
echo "done -> $OUT"

# Orientation caveat: the four side faces tile into a seamless 360 panorama. The
# top/bottom faces use yaw=0 as their roll reference; if a future consumer shows
# a seam at the top/bottom edges, add a per-face `-rotate` in `convert` and pin
# it against the renderer's cube-face test.
