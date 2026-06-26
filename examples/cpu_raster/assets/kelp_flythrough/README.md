# Kelp Forest flythrough (third-party exhibit)

![Kelp Forest flythrough](../../../../docs/images/gallery/anim-kelp.gif)

*Headless cpu_raster flythrough of the NPS/MOVES Kelp Forest Exhibit (12 s loop;
exhibit © NPS MOVES — see attribution below).* Full-quality WebM:
[`docs/videos/demos/kelp_flythrough.webm`](../../../../docs/videos/demos/kelp_flythrough.webm).
Regenerate both any time with `mise run kelp-flythrough`.

A guided-tour render of a real, complex, multi-file X3D world — the **NPS/MOVES
Kelp Forest Exhibit** — flown through headlessly by the [`cpu_raster`](../../README.md)
example. It exercises the full extraction path on a 600+ item scene: `Inline`
composition (~22 components), `ExternProtoDeclare`/`ProtoInstance` fish and kelp,
`ElevationGrid` terrain, textures, and `TimeSensor → Interpolator → ROUTE`
animation (swaying kelp, swimming fish, a working pump-house mechanism).

`flythrough_kelp.x3d` (committed here) is a small **wrapper** authored for this
repo: a bound `Viewpoint` animated through six of the exhibit's authored
waypoints — *Monterey Bay Aquarium → Side Windows → Star Fish → Pump House →
Bird's Eye → back* — over a 12 s seamless loop, with the exhibit `Inline`d.

## Source & license — please credit

The exhibit itself is **third-party content and is NOT committed** to this repo
(it is ~5 MB across ~55 `.x3d` files + ~150 textures); `fetch_kelp.sh` downloads
it on demand into `exhibit/` (git-ignored), the same pattern as
`mise run corpus-fetch` and the skybox `make_cubemap.sh`.

- **Kelp Forest Exhibit** by **Don Brutzman**, **Naval Postgraduate School (NPS)
  MOVES Institute** — <http://faculty.nps.edu/brutzman/kelp>
  (mirror: <https://x3dgraphics.com/examples/X3dForWebAuthors/KelpForestExhibit/>).
- License (from the scene metadata): *"All content has permissions for free use.
  Please provide credit to the Naval Postgraduate School (NPS) Modeling Virtual
  Environments and Simulation (MOVES) Institute."*

If you redistribute any render of this scene, credit NPS MOVES as above.

## Run it

```sh
# 1. Build the rasterizer (once):
mise run cpuraster

# 2. Fetch the exhibit into ./exhibit/ (needs wget):
examples/cpu_raster/assets/kelp_flythrough/fetch_kelp.sh

# 3a. One-shot, everything wired (fetch-if-needed + render + encode; needs ffmpeg):
mise run kelp-flythrough        # -> kelp_flythrough.webm

# 3b. …or drive the binary yourself — a still:
build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/kelp_flythrough/flythrough_kelp.x3d -o kelp.png -w 800 -H 450

# …or the animated flythrough (PPM frames -> your encoder):
build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/kelp_flythrough/flythrough_kelp.x3d \
    --animate --fps 30 --duration 12 -w 800 -H 450 --frames-dir /tmp/kelp
```

`--animate` runs the standard behavior runtime (TimeSensor + interpolators), so
the kelp, fish, and pump mechanism move; a plain still freezes them in their
authored rest pose. Note the exhibit's `ElevationGrid` rock floor renders solid
thanks to the §13.3.4 surface-winding fix (conformance EXT-003).
