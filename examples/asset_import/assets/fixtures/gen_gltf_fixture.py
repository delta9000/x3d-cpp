#!/usr/bin/env python3
"""Generate examples/asset_import/assets/fixtures/twobox.glb (reproducible).

A self-contained binary glTF exercising the full ImportScene surface: two boxes
(parent + child node) sharing one PBR metallic-roughness material with an
embedded 2x2 PNG baseColor texture (stored in the GLB bin chunk), one
perspective camera node, and one KHR_lights_punctual point light. Stdlib only,
so the committed .glb is regenerable without third-party packages.
"""
import json
import struct
import zlib
import pathlib


def png_2x2() -> bytes:
    """A 2x2 8-bit RGBA PNG, four solid colors — hand-built, no PIL dependency."""
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", 2, 2, 8, 6, 0, 0, 0)  # 2x2, 8-bit, RGBA
    px = [(255, 0, 0, 255), (0, 255, 0, 255), (0, 0, 255, 255), (255, 255, 0, 255)]
    raw = b""
    for row in range(2):
        raw += b"\x00" + b"".join(struct.pack("BBBB", *px[row * 2 + col]) for col in range(2))
    idat = zlib.compress(raw)
    return sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")


# Unit box: 24 vertices (per-face), 36 indices. Each vertex is (pos3, nrm3, uv2).
faces = [
    ((0, 0, 1), [(-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]),
    ((0, 0, -1), [(1, -1, -1), (-1, -1, -1), (-1, 1, -1), (1, 1, -1)]),
    ((1, 0, 0), [(1, -1, 1), (1, -1, -1), (1, 1, -1), (1, 1, 1)]),
    ((-1, 0, 0), [(-1, -1, -1), (-1, -1, 1), (-1, 1, 1), (-1, 1, -1)]),
    ((0, 1, 0), [(-1, 1, 1), (1, 1, 1), (1, 1, -1), (-1, 1, -1)]),
    ((0, -1, 0), [(-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1)]),
]
uv = [(0, 0), (1, 0), (1, 1), (0, 1)]

V = []  # (px,py,pz, nx,ny,nz, u,v)
idx = []
for n, quad in faces:
    base = len(V)
    for k, p in enumerate(quad):
        V.append((p[0] * 0.5, p[1] * 0.5, p[2] * 0.5, n[0], n[1], n[2], uv[k][0], uv[k][1]))
    idx += [base, base + 1, base + 2, base, base + 2, base + 3]

pos = b"".join(struct.pack("<3f", v[0], v[1], v[2]) for v in V)
nrm = b"".join(struct.pack("<3f", v[3], v[4], v[5]) for v in V)
tex = b"".join(struct.pack("<2f", v[6], v[7]) for v in V)
ind = b"".join(struct.pack("<H", i) for i in idx)
png = png_2x2()


def pad4(b: bytes) -> bytes:
    return b + b"\x00" * ((4 - len(b) % 4) % 4)


blob = b""


def add(data: bytes):
    """Append `data` to the shared buffer at a 4-byte-aligned offset; return (offset, length)."""
    global blob
    blob = pad4(blob)
    start = len(blob)
    blob += data
    return start, len(data)


p_off, p_len = add(pos)
n_off, n_len = add(nrm)
t_off, t_len = add(tex)
i_off, i_len = add(ind)
img_off, img_len = add(png)
blob = pad4(blob)

pos_min = [min(v[i] for v in V) for i in range(3)]
pos_max = [max(v[i] for v in V) for i in range(3)]

gltf = {
    "asset": {"version": "2.0", "generator": "x3d-cpp gen_gltf_fixture"},
    "extensionsUsed": ["KHR_lights_punctual"],
    "extensions": {"KHR_lights_punctual": {"lights": [
        {"type": "point", "color": [1, 1, 1], "intensity": 5.0, "range": 20.0, "name": "lamp"}]}},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [
        {"name": "parent", "mesh": 0, "translation": [0, 0, 0], "children": [1, 2, 3]},
        {"name": "child", "mesh": 0, "translation": [2, 0, 0]},
        {"name": "cam", "camera": 0, "translation": [0, 0, 6]},
        {"name": "lightnode", "extensions": {"KHR_lights_punctual": {"light": 0}},
         "translation": [3, 3, 3]},
    ],
    "cameras": [{"type": "perspective",
                 "perspective": {"yfov": 0.7853982, "znear": 0.1, "zfar": 1000.0, "aspectRatio": 1.0}}],
    "meshes": [{"name": "box", "primitives": [
        {"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3, "material": 0}]}],
    "materials": [{"name": "mat",
                   "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1], "metallicFactor": 0.0,
                                            "roughnessFactor": 0.6, "baseColorTexture": {"index": 0}},
                   "emissiveFactor": [0, 0, 0]}],
    "textures": [{"source": 0}],
    "images": [{"bufferView": 4, "mimeType": "image/png"}],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": len(V), "type": "VEC3",
         "min": pos_min, "max": pos_max},
        {"bufferView": 1, "componentType": 5126, "count": len(V), "type": "VEC3"},
        {"bufferView": 2, "componentType": 5126, "count": len(V), "type": "VEC2"},
        {"bufferView": 3, "componentType": 5123, "count": len(idx), "type": "SCALAR"},
    ],
    "bufferViews": [
        {"buffer": 0, "byteOffset": p_off, "byteLength": p_len, "target": 34962},
        {"buffer": 0, "byteOffset": n_off, "byteLength": n_len, "target": 34962},
        {"buffer": 0, "byteOffset": t_off, "byteLength": t_len, "target": 34962},
        {"buffer": 0, "byteOffset": i_off, "byteLength": i_len, "target": 34963},
        {"buffer": 0, "byteOffset": img_off, "byteLength": img_len},
    ],
    "buffers": [{"byteLength": len(blob)}],
}

# GLB spec: JSON chunk is padded with trailing spaces (0x20), BIN with zeros.
_json_raw = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
jsonb = _json_raw + b"\x20" * ((4 - len(_json_raw) % 4) % 4)
total = 12 + 8 + len(jsonb) + 8 + len(blob)
out = struct.pack("<III", 0x46546C67, 2, total)                 # 'glTF', version 2, total length
out += struct.pack("<II", len(jsonb), 0x4E4F534A) + jsonb       # JSON chunk ('JSON')
out += struct.pack("<II", len(blob), 0x004E4942) + blob         # BIN chunk ('BIN\0')

dest = pathlib.Path(__file__).with_name("twobox.glb")
dest.write_bytes(out)
print(f"wrote {dest} ({len(out)} bytes)")
