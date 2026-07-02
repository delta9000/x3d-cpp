#!/usr/bin/env python3
# Hierarchy showcase: a nested transform tree with a DISTINCT primitive per level —
# box hub -> sphere arms -> cone moons — authored in glTF and USD to show
# x3d_asset_import preserves the scene graph (nested <Transform>). A different shape
# (and colour) at each level makes the parent -> child nesting read at a glance.
# OBJ is intentionally omitted: the format has no node transforms / parenting.
# License-clean: all geometry is generated here (no third-party assets).
import base64, json, math, os, struct

OUT = os.path.dirname(os.path.abspath(__file__))

def nrm(a):
    l = math.sqrt(sum(c*c for c in a)) or 1.0
    return tuple(c/l for c in a)

# ---- primitive meshes: return (positions, normals, tris) --------------------
def box():
    faces = [((1,0,0),[(1,-1,-1),(1,1,-1),(1,1,1),(1,-1,1)]),
             ((-1,0,0),[(-1,-1,1),(-1,1,1),(-1,1,-1),(-1,-1,-1)]),
             ((0,1,0),[(-1,1,-1),(-1,1,1),(1,1,1),(1,1,-1)]),
             ((0,-1,0),[(-1,-1,1),(-1,-1,-1),(1,-1,-1),(1,-1,1)]),
             ((0,0,1),[(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]),
             ((0,0,-1),[(1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)])]
    P,N,T=[],[],[]
    for n,q in faces:
        b=len(P)
        for v in q: P.append(v); N.append(n)
        T+=[(b,b+1,b+2),(b,b+2,b+3)]
    return P,N,T

def sphere(rings=20, segs=28):
    P,N,T=[],[],[]
    for i in range(rings+1):
        v=i/rings; theta=v*math.pi
        for j in range(segs+1):
            u=j/segs; phi=u*2*math.pi
            p=(math.sin(theta)*math.cos(phi), math.cos(theta), math.sin(theta)*math.sin(phi))
            P.append(p); N.append(p)  # unit sphere: normal == position
    for i in range(rings):
        for j in range(segs):
            a=i*(segs+1)+j; b=a+1; c=a+segs+1; d=c+1
            T+=[(a,b,c),(b,d,c)]   # CCW when viewed from outside
    return P,N,T

def cone(segs=36):
    P,N,T=[],[],[]
    apex_y, base_y, r = 1.0, -1.0, 1.0
    # side: one apex vertex per segment (so each side face has a proper normal)
    for j in range(segs):
        a0=2*math.pi*j/segs; a1=2*math.pi*(j+1)/segs; am=(a0+a1)/2
        b0=(r*math.cos(a0), base_y, r*math.sin(a0))
        b1=(r*math.cos(a1), base_y, r*math.sin(a1))
        ap=(0.0, apex_y, 0.0)
        # face normal (points outward/up along the slanted side)
        sn=nrm((math.cos(am)*(apex_y-base_y), r, math.sin(am)*(apex_y-base_y)))
        base=len(P)
        P+= [ap,b0,b1]; N+=[sn,sn,sn]
        T.append((base,base+2,base+1))   # CCW from outside
    # base cap
    cbase=len(P); P.append((0.0,base_y,0.0)); N.append((0,-1,0))
    ring0=len(P)
    for j in range(segs):
        a=2*math.pi*j/segs
        P.append((r*math.cos(a),base_y,r*math.sin(a))); N.append((0,-1,0))
    for j in range(segs):
        T.append((cbase, ring0+j, ring0+(j+1)%segs))   # CCW from below (base points -Y)
    return P,N,T

MESHES=[box(), sphere(), cone()]           # 0=box 1=sphere 2=cone
COLORS=[(0.55,0.58,0.66),(0.90,0.62,0.22),(0.20,0.62,0.66)]  # slate / amber / teal

# ---- hierarchy node: one mesh (or None) + one axis-angle rotation + translate/scale.
# Applied order (matches USD xformOpOrder + glTF T*R*S): translate, then rotate, then scale.
def n(mesh, t=(0,0,0), axis='Y', deg=0.0, s=1.0, kids=()):
    return dict(m=mesh, t=t, axis=axis, deg=float(deg), s=s, kids=list(kids))

# Four-level transform tree, built so the rotation at each pivot actually MOVES its
# subtree (translate lives on a *child* of the rotating pivot, not the pivot itself —
# otherwise a rotation just spins a shape in place). This is the round-trip proof:
# each cone moon inherits box <- pivot-fan <- sphere-offset before its own offset.
#   Root  = box hub (mesh)
#    +-- Pivot   (rotateZ = fan angle; no mesh) — swings the whole arm in the view plane
#         +-- Sphere  (translate out +X; mesh) — the arm, carrying its moon
#              +-- Moon (translate further +X, rotateZ -90 to aim apex outward; cone mesh)
arms=[]
for fan in (90, 210, 330):   # three spokes 120 deg apart, in the view plane
    moon   = n(2, t=(2.15,0,0), axis='Z', deg=-90, s=0.55)        # cone moon (grandchild)
    sphere = n(1, t=(3.4,0,0),  axis='Y', deg=0,   s=0.9, kids=[moon])  # sphere arm (child)
    pivot  = n(None, t=(0,0,0), axis='Z', deg=fan, s=1.0, kids=[sphere]) # spoke pivot
    arms.append(pivot)
root = n(0, t=(0,0,0), axis='Y', deg=0, s=1.0, kids=arms)          # box hub at the centre

# ================= glTF ======================================================
blob=bytearray(); bufViews=[]; accessors=[]
def add_mesh(P,N,TRI):
    global blob
    def acc_of(bytez, comp, count, typ, mn=None, mx=None, target=34962):
        off=len(blob); blob.extend(bytez); blob.extend(b"\x00"*((4-len(blob)%4)%4))
        bufViews.append({"buffer":0,"byteOffset":off,"byteLength":len(bytez),"target":target})
        a={"bufferView":len(bufViews)-1,"componentType":comp,"count":count,"type":typ}
        if mn: a["min"]=mn; a["max"]=mx
        accessors.append(a); return len(accessors)-1
    pos_b=b"".join(struct.pack("<3f",*p) for p in P)
    nrm_b=b"".join(struct.pack("<3f",*v) for v in N)
    idx=[i for t in TRI for i in t]; idx_b=b"".join(struct.pack("<I",i) for i in idx)
    mn=[min(p[i] for p in P) for i in range(3)]; mx=[max(p[i] for p in P) for i in range(3)]
    pa=acc_of(pos_b,5126,len(P),"VEC3",mn,mx)
    na=acc_of(nrm_b,5126,len(N),"VEC3")
    ia=acc_of(idx_b,5125,len(idx),"SCALAR",target=34963)
    return pa,na,ia
gmeshes=[]
for mi,(P,N,TRI) in enumerate(MESHES):
    pa,na,ia=add_mesh(P,N,TRI)
    gmeshes.append({"primitives":[{"attributes":{"POSITION":pa,"NORMAL":na},"indices":ia,"material":mi}]})
gnodes=[]
def quat(axis, deg):
    a=math.radians(deg)/2; s=math.sin(a)
    return {'X':[s,0,0,math.cos(a)],'Y':[0,s,0,math.cos(a)],'Z':[0,0,s,math.cos(a)]}[axis]
def emit(nd):
    idx=len(gnodes); gnodes.append(None); ch=[emit(c) for c in nd["kids"]]
    g={"translation":list(map(float,nd["t"])),"rotation":quat(nd["axis"],nd["deg"]),"scale":[nd["s"]]*3}
    if nd["m"] is not None: g["mesh"]=nd["m"]
    if ch: g["children"]=ch
    gnodes[idx]=g; return idx
root_id=emit(root)
gltf={"asset":{"version":"2.0","generator":"showcase gen_hier.py"},"scene":0,
 "scenes":[{"nodes":[root_id]}],"nodes":gnodes,"meshes":gmeshes,
 "materials":[{"pbrMetallicRoughness":{"baseColorFactor":[c[0],c[1],c[2],1.0],
     "metallicFactor":0.1,"roughnessFactor":0.5}} for c in COLORS],
 "buffers":[{"byteLength":len(blob),"uri":"data:application/octet-stream;base64,"+base64.b64encode(bytes(blob)).decode()}],
 "bufferViews":bufViews,"accessors":accessors}
with open(f"{OUT}/scene.gltf","w") as f: json.dump(gltf,f)

# ================= USD =======================================================
def mesh_prim(name, P, N, TRI, color):
    pts=", ".join(f"({p[0]:.4f}, {p[1]:.4f}, {p[2]:.4f})" for p in P)
    nrms=", ".join(f"({v[0]:.4f}, {v[1]:.4f}, {v[2]:.4f})" for v in N)
    cnts=", ".join("3" for _ in TRI); fvi=", ".join(str(i) for t in TRI for i in t)
    return (f'  def "{name}" ( active = false ) {{\n'
            f'    def Mesh "Geom" ( prepend apiSchemas = ["MaterialBindingAPI"] ) {{\n'
            f'      int[] faceVertexCounts = [{cnts}]\n      int[] faceVertexIndices = [{fvi}]\n'
            f'      point3f[] points = [{pts}]\n      normal3f[] normals = [{nrms}] ( interpolation = "vertex" )\n'
            f'      rel material:binding = </Protos/{name}/M>\n    }}\n'
            f'    def Material "M" {{\n      token outputs:surface.connect = </Protos/{name}/M/S.outputs:surface>\n'
            f'      def Shader "S" {{\n        uniform token info:id = "UsdPreviewSurface"\n'
            f'        color3f inputs:diffuseColor = ({color[0]}, {color[1]}, {color[2]})\n'
            f'        float inputs:metallic = 0.1\n        float inputs:roughness = 0.5\n        token outputs:surface\n      }}\n    }}\n  }}\n')
PROTO_NAMES=["Box","Sphere","Cone"]
def usd_node(nd, name, indent):
    pad="  "*indent; s=nd["s"]; ax=nd["axis"]
    body=(f'{pad}def Xform "{name}" (){{\n'
          f'{pad}  float3 xformOp:translate = ({nd["t"][0]}, {nd["t"][1]}, {nd["t"][2]})\n'
          f'{pad}  float xformOp:rotate{ax} = {nd["deg"]}\n'
          f'{pad}  float3 xformOp:scale = ({s}, {s}, {s})\n'
          f'{pad}  uniform token[] xformOpOrder = ["xformOp:translate","xformOp:rotate{ax}","xformOp:scale"]\n')
    if nd["m"] is not None:
        body+=f'{pad}  def "Shape" ( prepend references = </Protos/{PROTO_NAMES[nd["m"]]}/Geom> ) {{}}\n'
    for i,c in enumerate(nd["kids"]): body+=usd_node(c,f"{name}_{i}",indent+1)
    return body+f'{pad}}}\n'
with open(f"{OUT}/scene.usda","w") as f:
    f.write('#usda 1.0\n( defaultPrim = "Root" upAxis = "Y" )\n')
    f.write('# Hierarchy showcase (license-clean): box hub -> sphere arms -> cone moons.\n')
    f.write('def "Protos" ( active = false ) {\n')
    for mi,(P,N,TRI) in enumerate(MESHES): f.write(mesh_prim(PROTO_NAMES[mi],P,N,TRI,COLORS[mi]))
    f.write('}\n')
    f.write(usd_node(root,"Root",0))
print(f"hierarchy: {len(gnodes)} glTF nodes; box+sphere+cone; "
      f"{sum(len(m[2]) for m in MESHES)} tris total")
