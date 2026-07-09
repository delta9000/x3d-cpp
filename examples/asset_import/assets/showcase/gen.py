#!/usr/bin/env python3
# Generate a license-clean (p,q) torus knot in OBJ+MTL, glTF 2.0, and USDA — each
# with a distinct PBR material — to showcase the x3d_asset_import converter across
# the three backends (assimp OBJ / assimp glTF / tinyusdz USD). The mesh is authored
# procedurally here, so there are no third-party asset licensing concerns.
import base64, json, math, os, struct

OUT = os.path.dirname(os.path.abspath(__file__))
P, Q = 2, 3            # (2,3) torus knot (trefoil)
NSEG, NRING = 240, 28  # samples along the knot / around the tube
TUBE = 0.42
SCALE = 0.62

def centerline(t):
    r = 2.0 + math.cos(Q * t)
    return (r * math.cos(P * t), r * math.sin(P * t), math.sin(Q * t))

def sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def add(a, b): return (a[0]+b[0], a[1]+b[1], a[2]+b[2])
def mul(a, s): return (a[0]*s, a[1]*s, a[2]*s)
def dot(a, b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def norm(a):
    l = math.sqrt(dot(a, a)) or 1.0
    return (a[0]/l, a[1]/l, a[2]/l)

# centerline samples + unit tangents (central difference, periodic)
C = [centerline(2*math.pi*i/NSEG) for i in range(NSEG)]
T = [norm(sub(C[(i+1) % NSEG], C[(i-1) % NSEG])) for i in range(NSEG)]

# rotation-minimizing (parallel-transport) frames
Nf = [None]*NSEG; Bf = [None]*NSEG
seed = (0, 0, 1) if abs(T[0][2]) < 0.9 else (0, 1, 0)
Nf[0] = norm(sub(seed, mul(T[0], dot(seed, T[0]))))
Bf[0] = cross(T[0], Nf[0])
for i in range(1, NSEG):
    n = sub(Nf[i-1], mul(T[i], dot(Nf[i-1], T[i])))  # project prev normal onto new plane
    Nf[i] = norm(n); Bf[i] = cross(T[i], Nf[i])
# closure: measure residual twist wrapping back to frame 0, distribute it linearly
nres = norm(sub(Nf[0], mul(T[0], dot(Nf[0], T[0]))))
resid = math.atan2(dot(cross(Nf[NSEG-1], nres), T[0]), dot(Nf[NSEG-1], nres))

positions, normals, tris = [], [], []
for i in range(NSEG):
    phi = -resid * i / NSEG
    for j in range(NRING):
        a = 2*math.pi*j/NRING + phi
        d = norm(add(mul(Nf[i], math.cos(a)), mul(Bf[i], math.sin(a))))  # radial = normal
        positions.append(mul(add(C[i], mul(d, TUBE)), SCALE))
        normals.append(d)
# watertight grid, periodic in both directions (no caps)
for i in range(NSEG):
    for j in range(NRING):
        a = i*NRING + j
        b = i*NRING + (j+1) % NRING
        c = ((i+1) % NSEG)*NRING + j
        e = ((i+1) % NSEG)*NRING + (j+1) % NRING
        tris += [(a, b, c), (b, e, c)]  # CCW when viewed from outside the tube

# per-vertex UVs: u wraps around the tube, v runs along the length (tiled so the
# knurl reads as fine surface detail rather than one stretched decal).
U_TILE, V_TILE = 2.0, 34.0
uvs = [(j/NRING*U_TILE, i/NSEG*V_TILE) for i in range(NSEG) for j in range(NRING)]

# ---- procedural knurl base-color texture (license-clean; stdlib PNG, no PIL) --
# A diamond knurl (two diagonal gratings) tinted per material, so the glTF and USD
# knots exercise the converter's full texture pipeline: extract -> re-encode PNG ->
# emit `url` -> the CPU rasterizer samples it. OBJ stays an untextured Phong material.
import zlib
def write_png(path, w, h, px):            # px: flat list of (r,g,b) 0..255, top-left
    raw = bytearray()
    for y in range(h):
        raw.append(0)                     # filter type 0 (None) per scanline
        for x in range(w):
            raw += bytes(px[y*w + x])
    def chunk(typ, data):
        return (struct.pack(">I", len(data)) + typ + data +
                struct.pack(">I", zlib.crc32(typ + data) & 0xffffffff))
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)   # 8-bit truecolour RGB
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
                + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))

def knurl(w, h, tint):
    out = []
    for y in range(h):
        for x in range(w):
            u, v = x/w, y/h
            ridge = 0.5*(math.cos(2*math.pi*(u+v)*4) + math.cos(2*math.pi*(u-v)*4))
            g = 0.60 + 0.40 * (0.5*(ridge+1.0))**0.6      # bright ridges, darker valleys
            out.append(tuple(min(255, int(255*g*t)) for t in tint))
    return out
TEX = 96
write_png(f"{OUT}/knot_gold.png", TEX, TEX, knurl(TEX, TEX, (0.98, 0.80, 0.34)))
write_png(f"{OUT}/knot_teal.png", TEX, TEX, knurl(TEX, TEX, (0.38, 0.92, 0.86)))
def png_data_uri(path):
    return "data:image/png;base64," + base64.b64encode(open(path, "rb").read()).decode()

# ---- OBJ + MTL (warm copper) ------------------------------------------------
with open(f"{OUT}/knot.mtl", "w") as f:
    f.write("# Authored for the x3d_asset_import showcase (license-clean).\n")
    f.write("newmtl copper\nKd 0.72 0.38 0.20\nKs 0.9 0.6 0.4\nNs 90\n")
with open(f"{OUT}/knot.obj", "w") as f:
    f.write("# Authored (2,3) torus knot for the showcase (license-clean).\n")
    f.write("mtllib knot.mtl\no knot\n")
    for p in positions: f.write(f"v {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}\n")
    for n in normals:   f.write(f"vn {n[0]:.5f} {n[1]:.5f} {n[2]:.5f}\n")
    f.write("usemtl copper\n")
    for a, b, c in tris: f.write(f"f {a+1}//{a+1} {b+1}//{b+1} {c+1}//{c+1}\n")

# ---- glTF 2.0 (gold metallic + knurl texture; self-contained, embedded) -----
pos_b = b"".join(struct.pack("<3f", *p) for p in positions)
nrm_b = b"".join(struct.pack("<3f", *n) for n in normals)
uv_b  = b"".join(struct.pack("<2f", *t) for t in uvs)
idx = [i for t in tris for i in t]
idx_b = b"".join(struct.pack("<I", i) for i in idx)  # uint32 indices
pad = lambda b: b + b"\x00"*((4-len(b) % 4) % 4)
pos_b, nrm_b, uv_b = pad(pos_b), pad(nrm_b), pad(uv_b)
blob = pos_b + nrm_b + uv_b + pad(idx_b)
off_n, off_uv, off_i = len(pos_b), len(pos_b)+len(nrm_b), len(pos_b)+len(nrm_b)+len(uv_b)
mn = [min(p[i] for p in positions) for i in range(3)]
mx = [max(p[i] for p in positions) for i in range(3)]
gltf = {
 "asset": {"version": "2.0", "generator": "x3d_asset_import showcase gen.py"},
 "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": "knot"}],
 "materials": [{"name": "gold", "pbrMetallicRoughness": {
     "baseColorTexture": {"index": 0}, "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
     "metallicFactor": 0.45, "roughnessFactor": 0.32}}],
 "textures": [{"source": 0, "sampler": 0}],
 "images": [{"uri": png_data_uri(f"{OUT}/knot_gold.png"), "mimeType": "image/png"}],
 "samplers": [{"wrapS": 10497, "wrapT": 10497}],  # 10497 == REPEAT
 "meshes": [{"name": "knot", "primitives": [{"attributes":
     {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3, "material": 0}]}],
 "buffers": [{"byteLength": len(blob),
     "uri": "data:application/octet-stream;base64," + base64.b64encode(blob).decode()}],
 "bufferViews": [
     {"buffer": 0, "byteOffset": 0,      "byteLength": len(pos_b), "target": 34962},
     {"buffer": 0, "byteOffset": off_n,  "byteLength": len(nrm_b), "target": 34962},
     {"buffer": 0, "byteOffset": off_uv, "byteLength": len(uv_b),  "target": 34962},
     {"buffer": 0, "byteOffset": off_i,  "byteLength": len(idx_b), "target": 34963}],
 "accessors": [
     {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3", "min": mn, "max": mx},
     {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
     {"bufferView": 2, "componentType": 5126, "count": len(uvs), "type": "VEC2"},
     {"bufferView": 3, "componentType": 5125, "count": len(idx), "type": "SCALAR"}],
}
with open(f"{OUT}/knot.gltf", "w") as f: json.dump(gltf, f)

# ---- USDA (teal metallic + knurl texture via UsdUVTexture) ------------------
pts = ", ".join(f"({p[0]:.5f}, {p[1]:.5f}, {p[2]:.5f})" for p in positions)
counts = ", ".join("3" for _ in tris)
fvi = ", ".join(str(i) for t in tris for i in t)
nrm = ", ".join(f"({n[0]:.5f}, {n[1]:.5f}, {n[2]:.5f})" for n in normals)
st = ", ".join(f"({t[0]:.5f}, {t[1]:.5f})" for t in uvs)
with open(f"{OUT}/knot.usda", "w") as f:
    f.write(f'''#usda 1.0
( defaultPrim = "World" upAxis = "Y" )
# Authored (2,3) torus knot for the showcase (license-clean).
def Xform "World" {{
  def Mesh "Knot" ( prepend apiSchemas = ["MaterialBindingAPI"] ) {{
    int[] faceVertexCounts = [{counts}]
    int[] faceVertexIndices = [{fvi}]
    point3f[] points = [{pts}]
    normal3f[] normals = [{nrm}] ( interpolation = "vertex" )
    texCoord2f[] primvars:st = [{st}] ( interpolation = "vertex" )
    rel material:binding = </World/Mat>
  }}
  def Material "Mat" {{
    token outputs:surface.connect = </World/Mat/S.outputs:surface>
    def Shader "S" {{
      uniform token info:id = "UsdPreviewSurface"
      color3f inputs:diffuseColor.connect = </World/Mat/Tex.outputs:rgb>
      float inputs:metallic = 0.45
      float inputs:roughness = 0.32
      token outputs:surface
    }}
    def Shader "Tex" {{
      uniform token info:id = "UsdUVTexture"
      asset inputs:file = @./knot_teal.png@
      float2 inputs:st.connect = </World/Mat/Reader.outputs:result>
      token inputs:wrapS = "repeat"
      token inputs:wrapT = "repeat"
      float3 outputs:rgb
    }}
    def Shader "Reader" {{
      uniform token info:id = "UsdPrimvarReader_float2"
      token inputs:varname = "st"
      float2 outputs:result
    }}
  }}
}}
''')
print(f"generated {len(positions)} verts, {len(tris)} tris:",
      ", ".join(sorted(f for f in os.listdir(OUT) if f.startswith("knot"))))
