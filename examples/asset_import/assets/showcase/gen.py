#!/usr/bin/env python3
# Generate a license-clean cube in OBJ+MTL, glTF 2.0, and USDA — each with a
# distinct PBR material — to showcase the x3d_asset_import converter across the
# three backends (assimp OBJ / assimp glTF / tinyusdz USD). All authored here,
# so there are no third-party asset licensing concerns.
import base64, json, os, struct

OUT = os.path.dirname(os.path.abspath(__file__))

# ---- shared flat-shaded cube (24 verts, per-face normals, 12 tris) ----------
# faces: +X,-X,+Y,-Y,+Z,-Z
faces = [
    ((1,0,0),  [(1,-1,-1),(1,1,-1),(1,1,1),(1,-1,1)]),
    ((-1,0,0), [(-1,-1,1),(-1,1,1),(-1,1,-1),(-1,-1,-1)]),
    ((0,1,0),  [(-1,1,-1),(-1,1,1),(1,1,1),(1,1,-1)]),
    ((0,-1,0), [(-1,-1,1),(-1,-1,-1),(1,-1,-1),(1,-1,1)]),
    ((0,0,1),  [(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]),
    ((0,0,-1), [(1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)]),
]
positions, normals, tris = [], [], []
for n, quad in faces:
    b = len(positions)
    for v in quad:
        positions.append(v); normals.append(n)
    tris += [(b,b+1,b+2),(b,b+2,b+3)]

# ---- OBJ + MTL (warm copper-ish) --------------------------------------------
with open(f"{OUT}/cube.mtl","w") as f:
    f.write("# Authored for the x3d_asset_import showcase (license-clean).\n")
    f.write("newmtl copper\nKd 0.72 0.38 0.20\nKs 0.9 0.6 0.4\nNs 120\n")
with open(f"{OUT}/cube.obj","w") as f:
    f.write("# Authored cube for the x3d_asset_import showcase (license-clean).\n")
    f.write("mtllib cube.mtl\no cube\n")
    for p in positions: f.write(f"v {p[0]} {p[1]} {p[2]}\n")
    for n in normals:   f.write(f"vn {n[0]} {n[1]} {n[2]}\n")
    f.write("usemtl copper\n")
    for a,b,c in tris:  f.write(f"f {a+1}//{a+1} {b+1}//{b+1} {c+1}//{c+1}\n")

# ---- glTF 2.0 (metallic gold; self-contained, embedded buffer) --------------
pos_bytes = b"".join(struct.pack("<3f",*p) for p in positions)
nrm_bytes = b"".join(struct.pack("<3f",*n) for n in normals)
idx = [i for t in tris for i in t]
idx_bytes = b"".join(struct.pack("<H",i) for i in idx)
# pad each section to 4-byte alignment
def pad(b): return b + b"\x00"*((4-len(b)%4)%4)
pos_bytes, nrm_bytes = pad(pos_bytes), pad(nrm_bytes)
blob = pos_bytes + nrm_bytes + pad(idx_bytes)
mn = [min(p[i] for p in positions) for i in range(3)]
mx = [max(p[i] for p in positions) for i in range(3)]
gltf = {
 "asset":{"version":"2.0","generator":"x3d_asset_import showcase gen.py"},
 "scenes":[{"nodes":[0]}], "scene":0, "nodes":[{"mesh":0,"name":"cube"}],
 "materials":[{"name":"gold","pbrMetallicRoughness":{
     "baseColorFactor":[0.95,0.72,0.18,1.0],"metallicFactor":0.1,"roughnessFactor":0.5}}],
 "meshes":[{"name":"cube","primitives":[{"attributes":{"POSITION":0,"NORMAL":1},
     "indices":2,"material":0}]}],
 "buffers":[{"byteLength":len(blob),
     "uri":"data:application/octet-stream;base64,"+base64.b64encode(blob).decode()}],
 "bufferViews":[
     {"buffer":0,"byteOffset":0,"byteLength":len(pos_bytes),"target":34962},
     {"buffer":0,"byteOffset":len(pos_bytes),"byteLength":len(nrm_bytes),"target":34962},
     {"buffer":0,"byteOffset":len(pos_bytes)+len(nrm_bytes),"byteLength":len(idx_bytes),"target":34963}],
 "accessors":[
     {"bufferView":0,"componentType":5126,"count":len(positions),"type":"VEC3","min":mn,"max":mx},
     {"bufferView":1,"componentType":5126,"count":len(normals),"type":"VEC3"},
     {"bufferView":2,"componentType":5123,"count":len(idx),"type":"SCALAR"}],
}
with open(f"{OUT}/cube.gltf","w") as f: json.dump(gltf,f,indent=1)

# ---- USDA (UsdPreviewSurface, teal metallic) --------------------------------
pts = ", ".join(f"({p[0]}, {p[1]}, {p[2]})" for p in positions)
counts = ", ".join("3" for _ in tris)
fvi = ", ".join(str(i) for t in tris for i in t)
nrm = ", ".join(f"({n[0]}, {n[1]}, {n[2]})" for n in normals)
with open(f"{OUT}/cube.usda","w") as f:
    f.write(f'''#usda 1.0
( defaultPrim = "World" upAxis = "Y" )
# Authored cube for the x3d_asset_import showcase (license-clean).
def Xform "World" {{
  def Mesh "Cube" ( prepend apiSchemas = ["MaterialBindingAPI"] ) {{
    int[] faceVertexCounts = [{counts}]
    int[] faceVertexIndices = [{fvi}]
    point3f[] points = [{pts}]
    normal3f[] normals = [{nrm}] ( interpolation = "vertex" )
    rel material:binding = </World/Mat>
  }}
  def Material "Mat" {{
    token outputs:surface.connect = </World/Mat/S.outputs:surface>
    def Shader "S" {{
      uniform token info:id = "UsdPreviewSurface"
      color3f inputs:diffuseColor = (0.13, 0.62, 0.65)
      float inputs:metallic = 0.1
      float inputs:roughness = 0.5
      token outputs:surface
    }}
  }}
}}
''')
print("generated:", ", ".join(sorted(os.listdir(OUT))))
