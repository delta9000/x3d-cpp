#!/usr/bin/env python3
# Hierarchy showcase: a nested transform tree (root -> 3 arms -> moons), one shared
# knot mesh instanced throughout, authored in USD and glTF to show x3d_asset_import
# preserves the scene graph (nested <Transform>, plus DEF/USE when a mesh is reused).
# OBJ is intentionally omitted: the format has no node transforms / parenting.
# License-clean: geometry is generated here (no third-party assets).
import base64, json, math, os, struct

OUT = os.path.dirname(os.path.abspath(__file__))
P, Q, NSEG, NRING, TUBE = 2, 3, 96, 14, 0.42

def cl(t):
    r = 2.0 + math.cos(Q*t); return (r*math.cos(P*t), r*math.sin(P*t), math.sin(Q*t))
def sub(a,b): return (a[0]-b[0],a[1]-b[1],a[2]-b[2])
def add(a,b): return (a[0]+b[0],a[1]+b[1],a[2]+b[2])
def mul(a,s): return (a[0]*s,a[1]*s,a[2]*s)
def dot(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
def crs(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def nrm(a):
    l=math.sqrt(dot(a,a)) or 1.0; return (a[0]/l,a[1]/l,a[2]/l)

# ---- build one unit-ish knot (positions, normals, tris) ---------------------
C=[cl(2*math.pi*i/NSEG) for i in range(NSEG)]
T=[nrm(sub(C[(i+1)%NSEG],C[(i-1)%NSEG])) for i in range(NSEG)]
Nf=[None]*NSEG; Bf=[None]*NSEG
seed=(0,0,1) if abs(T[0][2])<0.9 else (0,1,0)
Nf[0]=nrm(sub(seed,mul(T[0],dot(seed,T[0])))); Bf[0]=crs(T[0],Nf[0])
for i in range(1,NSEG):
    Nf[i]=nrm(sub(Nf[i-1],mul(T[i],dot(Nf[i-1],T[i])))); Bf[i]=crs(T[i],Nf[i])
nr=nrm(sub(Nf[0],mul(T[0],dot(Nf[0],T[0]))))
res=math.atan2(dot(crs(Nf[NSEG-1],nr),T[0]),dot(Nf[NSEG-1],nr))
POS,NOR,TRI=[],[],[]
for i in range(NSEG):
    phi=-res*i/NSEG
    for j in range(NRING):
        a=2*math.pi*j/NRING+phi
        d=nrm(add(mul(Nf[i],math.cos(a)),mul(Bf[i],math.sin(a))))
        POS.append(mul(add(C[i],mul(d,TUBE)),0.34)); NOR.append(d)
for i in range(NSEG):
    for j in range(NRING):
        a=i*NRING+j; b=i*NRING+(j+1)%NRING
        c=((i+1)%NSEG)*NRING+j; e=((i+1)%NSEG)*NRING+(j+1)%NRING
        TRI+=[(a,c,b),(b,c,e)]

# ---- the hierarchy: (name, translate, rotateY-deg, scale, [children]) -------
def node(name,tx,ry,s,kids): return dict(name=name,t=tx,ry=ry,s=s,kids=kids)
arms=[]
for k in range(3):
    moon=node(f"Moon{k}",(2.7,0,0),40, 0.55, [])          # grandchild
    arms.append(node(f"Arm{k}",(3.6,0,0), k*120, 0.7, [moon]))
root=node("Orrery",(0,0,0),0,1.0,[node("Hub",(0,0,0),0,1.0,[])]+arms)

# ================= USD =======================================================
def usd_xform(nd, indent, mesh_at):
    pad="  "*indent
    s=nd["s"]
    xf=(f'{pad}def Xform "{nd["name"]}" (){{\n'
        f'{pad}  float3 xformOp:translate = ({nd["t"][0]}, {nd["t"][1]}, {nd["t"][2]})\n'
        f'{pad}  float xformOp:rotateY = {nd["ry"]}\n'
        f'{pad}  float3 xformOp:scale = ({s}, {s}, {s})\n'
        f'{pad}  uniform token[] xformOpOrder = ["xformOp:translate","xformOp:rotateY","xformOp:scale"]\n')
    if mesh_at:  # instance the shared knot here via an internal reference
        xf+=f'{pad}  def "Knot" ( prepend references = </Proto/Knot> ) {{}}\n'
    for c in nd["kids"]: xf+=usd_xform(c,indent+1,True)
    xf+=f'{pad}}}\n'; return xf
pts=", ".join(f"({p[0]:.4f}, {p[1]:.4f}, {p[2]:.4f})" for p in POS)
cnts=", ".join("3" for _ in TRI)
fvi=", ".join(str(i) for t in TRI for i in t)
nrms=", ".join(f"({n[0]:.4f}, {n[1]:.4f}, {n[2]:.4f})" for n in NOR)
with open(f"{OUT}/scene.usda","w") as f:
    f.write('#usda 1.0\n( defaultPrim = "Orrery" upAxis = "Y" )\n')
    f.write('# Hierarchy showcase (license-clean). One knot mesh referenced through a\n')
    f.write('# nested Xform tree: Orrery -> {Hub, Arm0..2 -> Moon0..2}.\n')
    # prototype mesh (not itself drawn; referenced by the Xform tree)
    f.write('def "Proto" ( active = false ) {\n')
    f.write('  def Mesh "Knot" ( prepend apiSchemas = ["MaterialBindingAPI"] ) {\n')
    f.write(f'    int[] faceVertexCounts = [{cnts}]\n')
    f.write(f'    int[] faceVertexIndices = [{fvi}]\n')
    f.write(f'    point3f[] points = [{pts}]\n')
    f.write(f'    normal3f[] normals = [{nrms}] ( interpolation = "vertex" )\n')
    f.write('    rel material:binding = </Proto/Mat>\n  }\n')
    f.write('  def Material "Mat" {\n    token outputs:surface.connect = </Proto/Mat/S.outputs:surface>\n')
    f.write('    def Shader "S" {\n      uniform token info:id = "UsdPreviewSurface"\n')
    f.write('      color3f inputs:diffuseColor = (0.85, 0.55, 0.25)\n')
    f.write('      float inputs:metallic = 0.1\n      float inputs:roughness = 0.45\n      token outputs:surface\n    }\n  }\n}\n')
    f.write(usd_xform(root,0,False))

# ================= glTF ======================================================
pos_b=b"".join(struct.pack("<3f",*p) for p in POS)
nrm_b=b"".join(struct.pack("<3f",*n) for n in NOR)
idx=[i for t in TRI for i in t]; idx_b=b"".join(struct.pack("<I",i) for i in idx)
pad=lambda b:b+b"\x00"*((4-len(b)%4)%4); pos_b,nrm_b=pad(pos_b),pad(nrm_b)
blob=pos_b+nrm_b+pad(idx_b)
mn=[min(p[i] for p in POS) for i in range(3)]; mx=[max(p[i] for p in POS) for i in range(3)]
gnodes=[];
def rot_y_quat(deg):
    a=math.radians(deg)/2; return [0,math.sin(a),0,math.cos(a)]
def emit_gnode(nd, mesh_here):
    idx_self=len(gnodes); gnodes.append(None); child_ids=[]
    for c in nd["kids"]: child_ids.append(emit_gnode(c,True))
    g={"name":nd["name"],"translation":list(map(float,nd["t"])),
       "rotation":rot_y_quat(nd["ry"]),"scale":[nd["s"]]*3}
    if mesh_here: g["mesh"]=0
    if child_ids: g["children"]=child_ids
    gnodes[idx_self]=g; return idx_self
root_id=emit_gnode(root,False)
gltf={"asset":{"version":"2.0","generator":"showcase gen_hier.py"},
 "scene":0,"scenes":[{"nodes":[root_id]}],"nodes":gnodes,
 "materials":[{"name":"amber","pbrMetallicRoughness":{"baseColorFactor":[0.85,0.55,0.25,1.0],"metallicFactor":0.1,"roughnessFactor":0.45}}],
 "meshes":[{"name":"knot","primitives":[{"attributes":{"POSITION":0,"NORMAL":1},"indices":2,"material":0}]}],
 "buffers":[{"byteLength":len(blob),"uri":"data:application/octet-stream;base64,"+base64.b64encode(blob).decode()}],
 "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":len(pos_b),"target":34962},
   {"buffer":0,"byteOffset":len(pos_b),"byteLength":len(nrm_b),"target":34962},
   {"buffer":0,"byteOffset":len(pos_b)+len(nrm_b),"byteLength":len(idx_b),"target":34963}],
 "accessors":[{"bufferView":0,"componentType":5126,"count":len(POS),"type":"VEC3","min":mn,"max":mx},
   {"bufferView":1,"componentType":5126,"count":len(NOR),"type":"VEC3"},
   {"bufferView":2,"componentType":5125,"count":len(idx),"type":"SCALAR"}]}
with open(f"{OUT}/scene.gltf","w") as f: json.dump(gltf,f)
print(f"hierarchy: {len(gnodes)} glTF nodes; knot {len(POS)} verts {len(TRI)} tris")
