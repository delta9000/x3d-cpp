#!/usr/bin/env python3
"""Minimal glTF (.gltf + .bin) -> X3D converter for the CPU rasterizer.

Extracts POSITION + NORMAL + TEXCOORD_0 + triangle indices from every mesh
primitive (mode 4), applies each node's local transform, and emits ONE
IndexedFaceSet (Coordinate + Normal + TextureCoordinate, normalPerVertex). The
first glTF material's baseColor / metallic-roughness (ORM) / normal textures are
auto-wired into a PhysicalMaterial as ImageTextures (image URIs flattened to
basenames, matching how the textures are vendored next to this file).

The in-repo OBJ/glTF->X3D converter (ADR-0017 Spec 2/3) is not built yet; this is
a deliberately small consumer-side converter for vendoring CC0 demo models.
Single-material assets only (lion_head etc.); multi-material is out of scope.

Usage:  ./gltf_to_x3d.py <in.gltf> <out.x3d>
"""
import json, os, struct, sys

CT = {5120:('b',1),5121:('B',1),5122:('h',2),5123:('H',2),5125:('I',4),5126:('f',4)}
NCOMP = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}

def load(p):
    d = json.load(open(p))
    buf = open(os.path.join(os.path.dirname(p), d['buffers'][0]['uri']), 'rb').read()
    return d, buf

def accessor(d, buf, idx):
    a = d['accessors'][idx]; bv = d['bufferViews'][a['bufferView']]
    fmt, sz = CT[a['componentType']]; nc = NCOMP[a['type']]
    base = bv.get('byteOffset',0) + a.get('byteOffset',0)
    stride = bv.get('byteStride') or sz*nc
    return [struct.unpack_from('<'+fmt*nc, buf, base+i*stride) for i in range(a['count'])]

def node_matrix(n):
    if 'matrix' in n:
        m = n['matrix']; return [[m[c*4+r] for c in range(4)] for r in range(4)]
    t = n.get('translation',[0,0,0]); s = n.get('scale',[1,1,1]); x,y,z,w = n.get('rotation',[0,0,0,1])
    R = [[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
         [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
         [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]]
    return [[R[r][c]*s[c] for c in range(3)]+[t[r]] for r in range(3)]+[[0,0,0,1]]

def xpt(M,p): return [M[r][0]*p[0]+M[r][1]*p[1]+M[r][2]*p[2]+M[r][3] for r in range(3)]
def xdir(M,v): return [M[r][0]*v[0]+M[r][1]*v[1]+M[r][2]*v[2] for r in range(3)]

def image_basename(d, tex_idx):
    if tex_idx is None: return None
    return os.path.basename(d['images'][d['textures'][tex_idx]['source']]['uri'])

def material_textures(d):
    if not d.get('materials'): return {}
    m = d['materials'][0]; pbr = m.get('pbrMetallicRoughness', {})
    def idx(o): return o.get('index') if o else None
    return {
        'base':   image_basename(d, idx(pbr.get('baseColorTexture'))),
        'mr':     image_basename(d, idx(pbr.get('metallicRoughnessTexture'))),
        'normal': image_basename(d, idx(m.get('normalTexture'))),
    }

def main(inp, outp):
    d, buf = load(inp)
    coords, normals, uvs, index = [], [], [], []
    base_v = 0
    for n in d.get('nodes', []):
        if 'mesh' not in n: continue
        M = node_matrix(n)
        for prim in d['meshes'][n['mesh']]['primitives']:
            if prim.get('mode', 4) != 4: continue
            at = prim['attributes']
            pos = accessor(d, buf, at['POSITION'])
            nrm = accessor(d, buf, at['NORMAL']) if 'NORMAL' in at else []
            uv  = accessor(d, buf, at['TEXCOORD_0']) if 'TEXCOORD_0' in at else []
            idx = [t[0] for t in accessor(d, buf, prim['indices'])]
            coords += [xpt(M, p) for p in pos]
            normals += [xdir(M, v) for v in nrm]
            uvs += [(u[0], 1.0 - u[1]) for u in uv]  # glTF V top-down; X3D bottom-up.
            for k in range(0, len(idx), 3):
                index += [base_v+idx[k], base_v+idx[k+1], base_v+idx[k+2], -1]
            base_v += len(pos)

    tx = material_textures(d)
    pt = ' '.join('%.5g %.5g %.5g' % tuple(c) for c in coords)
    ci = ' '.join(map(str, index))
    nv = ' '.join('%.5g %.5g %.5g' % tuple(v) for v in normals)
    tc = ' '.join('%.5g %.5g' % uv for uv in uvs)
    imgs = ''
    for field, fname in (('baseTexture', tx.get('base')),
                         ('normalTexture', tx.get('normal')),
                         ('metallicRoughnessTexture', tx.get('mr'))):
        if fname:
            imgs += f"\n  <ImageTexture containerField=\"{field}\" url='&quot;{fname}&quot;'/>"
    if imgs:
        appearance = ('<Appearance><PhysicalMaterial baseColor="1 1 1" '
                      f'metallic="1" roughness="1">{imgs}\n</PhysicalMaterial></Appearance>')
    else:
        appearance = '<Appearance><Material diffuseColor="0.8 0.8 0.8"/></Appearance>'
    tcnode = f'<TextureCoordinate point="{tc}"/>' if tc else ''
    with open(outp, 'w') as f:
        f.write('<X3D profile="Interchange" version="4.0"><Scene>\n<Shape>\n')
        f.write(appearance + '\n')
        f.write(f'<IndexedFaceSet solid="false" normalPerVertex="true" coordIndex="{ci}">\n')
        f.write(f'<Coordinate point="{pt}"/><Normal vector="{nv}"/>{tcnode}\n')
        f.write('</IndexedFaceSet></Shape>\n</Scene></X3D>\n')
    print(f'wrote {outp}: {len(coords)} verts, {len(index)//4} tris, '
          f'textures={ {k:v for k,v in tx.items() if v} }')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
