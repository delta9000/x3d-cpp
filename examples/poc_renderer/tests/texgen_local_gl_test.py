"""Headless GL regression for the local, noise and refraction
TextureCoordinateGenerator modes and generated-coordinate transforms (TXF-2).

A quad faces the camera through a rotated, translated Transform, so its local
positions and normals differ from the eye-space ones. Its 4x1 clamped texture
reads red for u < 0.5 and blue for u > 0.5; authored UVs are u = 0.75 (blue).
For each mode the expected u at a few points is computed here from the Table
18.6 definitions as the CPU reference host implements them
(cpu_raster MaterialShader.hpp texCoordGenUv, including its Perlin noise), and
the GL pixel there must have that colour, in the unlit, Phong and PBR programs.
"""
import math
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 8.0
HALF_HEIGHT = DISTANCE * math.tan(math.pi / 8)  # default fieldOfView, 720p window
ANGLE = 0.6                                       # Transform rotation about +Y
SHIFT = (1.0, 0.5, 0.0)                           # Transform translation
COS, SIN = math.cos(ANGLE), math.sin(ANGLE)


def to_local(p):
    """World (= eye up to the view offset) point -> the Transform's local frame."""
    x, y, z = (p[i] - SHIFT[i] for i in range(3))
    return (COS * x - SIN * z, y, SIN * x + COS * z)  # R(-ANGLE) about +Y


NORMAL_LOCAL = (-SIN, 0.0, COS)  # R(-ANGLE) * (0, 0, 1)


# ---- Table 18.6 as cpu_raster implements it --------------------------------
def u32(v):
    return v & 0xFFFFFFFF


def grad(a, b, c, dx, dy, dz, seed):
    h = u32((a & 255) * 374761393) ^ u32((b & 255) * 668265263) ^ \
        u32((c & 255) * 2246822519) ^ seed
    h = u32((h ^ (h >> 13)) * 1274126177)
    h = (h ^ (h >> 16)) & 15
    u = dx if h < 8 else dy
    v = dy if h < 4 else (dx if h in (12, 14) else dz)
    return (-u if h & 1 else u) + (-v if h & 2 else v)


def perlin(p, seed):
    fade = lambda t: t * t * t * (t * (t * 6 - 15) + 10)
    lerp = lambda a, b, t: a + (b - a) * t
    ix, iy, iz = (int(math.floor(math.fmod(c, 256.0))) for c in p)
    x, y, z = (c - math.floor(c) for c in p)
    a = []
    for k in range(2):
        b = [lerp(grad(ix, iy + j, iz + k, x, y - j, z - k, seed),
                  grad(ix + 1, iy + j, iz + k, x - 1, y - j, z - k, seed), fade(x))
             for j in range(2)]
        a.append(lerp(b[0], b[1], fade(y)))
    return lerp(a[0], a[1], fade(z))


def normalize(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)


def generated_u(mode, eye, local, params):
    p = lambda i, d: params[i] if i < len(params) else d
    if mode == 'SPHERE-LOCAL':
        return NORMAL_LOCAL[0] * 0.5 + 0.5
    if mode == 'COORD':
        return local[0]
    if mode in ('NOISE', 'NOISE-EYE'):
        q = local if mode == 'NOISE' else eye
        q = tuple(q[i] * p(i, 1.0) + p(3 + i, 0.0) for i in range(3))
        return perlin(q, 0)
    if mode in ('SPHERE-REFLECT', 'SPHERE-REFLECT-LOCAL'):
        is_local = mode == 'SPHERE-REFLECT-LOCAL'
        origin = (p(1, 0.0), p(2, 0.0), p(3, 0.0)) if is_local else (0.0, 0.0, 0.0)
        incident = normalize(tuple((local if is_local else eye)[i] - origin[i]
                                   for i in range(3)))
        normal = tuple(-c for c in (NORMAL_LOCAL if is_local else (0.0, 0.0, 1.0)))
        eta = p(0, 1.0)
        d = sum(normal[i] * incident[i] for i in range(3))
        k = 1 - eta * eta * (1 - d * d)
        return 0.0 if k < 0 else incident[0] * eta - normal[0] * (eta * d + math.sqrt(k))
    raise ValueError(mode)


# (mode, parameter, TextureTransform translation u, sample eye x positions)
CASES = [
    ('COORD', [], 0.0, [-0.5, 1.6, 2.4]),
    ('COORD', [], -1.0, [2.4, 3.2]),  # generated coordinates get the transform
    ('SPHERE-LOCAL', [], 0.0, [0.0, 1.0]),
    ('NOISE', [0.9, 0.9, 0.9, 0.3, 0.2, 0.1], 0.0, [-2.0, -0.7, 0.4, 1.3, 2.6]),
    ('NOISE-EYE', [0.9, 0.9, 0.9, 0.3, 0.2, 0.1], 0.0, [-2.0, -0.7, 0.4, 1.3, 2.6]),
    ('SPHERE-REFLECT', [1.5], 0.0, [-2.0, 4.0]),
    ('SPHERE-REFLECT-LOCAL', [1.2, 3.0, 0.0, 6.0], 0.0, [-2.0, 0.5, 3.0]),
]

MATERIALS = {
    'unlit': '',
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}


def render(tmp, name, material, generator, shift_u):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    corners = [(-6, -6, 0), (6, -6, 0), (6, 6, 0), (-6, 6, 0)]
    points = ' '.join('%.6f %.6f %.6f' % to_local(c) for c in corners)
    transform = f'<TextureTransform translation="{shift_u} 0"/>' if shift_u else ''
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><Viewpoint position="0 0 {DISTANCE}"/>
<Transform translation="{SHIFT[0]} {SHIFT[1]} {SHIFT[2]}" rotation="0 1 0 {ANGLE}">
<Shape><Appearance>{material}
<PixelTexture image="4 1 3 0xFF0000 0xFF0000 0x0000FF 0x0000FF" repeatS="false" repeatT="false"/>
{transform}</Appearance>
<IndexedFaceSet solid="false" coordIndex="0 1 2 3 -1" texCoordIndex="0 0 0 0 -1">
<Coordinate point="{points}"/>
<TextureCoordinate point="0.75 0.5"/>{generator}</IndexedFaceSet></Shape>
</Transform></Scene></X3D>''')
    result = subprocess.run([
        'xvfb-run', '-a', 'env', 'LIBGL_ALWAYS_SOFTWARE=1',
        'GALLIUM_DRIVER=llvmpipe', '__GLX_VENDOR_LIBRARY_NAME=mesa',
        str(renderer), '--screenshot', str(output), str(scene),
    ], timeout=120, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    header, data = output.read_bytes().split(b'\n255\n', 1)
    width, height = map(int, header.split()[1:3])

    def pixel(eye_x):
        column = width // 2 + round(eye_x / HALF_HEIGHT * (height / 2))
        offset = ((height // 2) * width + column) * 3
        return data[offset], data[offset + 2]  # red, blue
    return pixel


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    checked = 0
    for index, (mode, params, shift_u, xs) in enumerate(CASES):
        parameter = f' parameter="{" ".join(map(str, params))}"' if params else ''
        generator = f'<TextureCoordinateGenerator mode="{mode}"{parameter}/>'
        for name, material in MATERIALS.items():
            pixel = render(tmp, f'{index}-{name}', material, generator, shift_u)
            for x in xs:
                eye = (x, 0.0, -DISTANCE)
                u = generated_u(mode, eye, to_local((x, 0.0, 0.0)), params) + shift_u
                if abs(u - 0.5) < 0.2:
                    continue  # too near the red/blue texel boundary to call
                r, b = pixel(x)
                want_red = u < 0.5
                ok = r > b + 40 if want_red else b > r + 40
                assert ok, (mode, name, x, round(u, 3), (r, b))
                checked += 1
    assert checked >= 40, checked
    print(f'GL texgen: local, noise and refraction modes match Table 18.6 '
          f'({checked} samples)')
