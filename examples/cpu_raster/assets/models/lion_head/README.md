# lion_head — CC0 demo model

A sculpted lion-head bust (~47k triangles) used to show off the CPU rasterizer's
lighting + textured PBR path on a real, reasonably complex mesh.

- **Source:** [Poly Haven — Lion Head](https://polyhaven.com/a/lion_head) (Tina)
- **License:** **CC0** (public domain) — no attribution required; credited as courtesy.

## Files

| File | What |
|------|------|
| `lion_head.x3d` | The mesh as one `IndexedFaceSet` (Coordinate + Normal + TextureCoordinate, `normalPerVertex`) with a `PhysicalMaterial` wiring the three maps below. Generated — see recipe. |
| `lion_head_diff_1k.jpg` | base color (sRGB) |
| `lion_head_arm_1k.jpg` | ORM pack → `metallicRoughnessTexture` (AO / Roughness / Metalness) |
| `lion_head_nor_gl_1k.jpg` | tangent-space normal map |
| `lion_head_lit.x3d` | Studio scene — `Inline`s the mesh under a warm-key / cool-fill / rim three-point rig over a gradient `Background`. The README gallery's lion shot. |
| `lion_head_garden.x3d` | Alt scene — the mesh in the `suburban_garden` skybox. |

## Regenerating `lion_head.x3d`

The mesh is converted from the CC0 glTF with `../gltf_to_x3d.py` (the in-repo
OBJ/glTF→X3D converter, ADR-0017 Spec 2/3, is not built yet — this is a small
consumer-side converter for vendoring demo models):

```sh
# fetch the CC0 glTF + .bin from Poly Haven (URLs from the API)
slug=lion_head; res=1k
base="https://dl.polyhaven.org/file/ph-assets/Models/gltf/$res/$slug"
curl -fsSO "$base/${slug}_${res}.gltf"; curl -fsSO "$base/${slug}.bin"
# (textures already vendored here; or fetch from .../Models/jpg/$res/$slug/)
python3 ../gltf_to_x3d.py ${slug}_${res}.gltf lion_head.x3d
```

The glTF `.bin` (geometry source) is *not* vendored — `lion_head.x3d` embeds the
geometry; the textures (the part the renderer samples) are vendored.
