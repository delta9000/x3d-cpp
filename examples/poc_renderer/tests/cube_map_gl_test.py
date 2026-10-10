"""Headless GL regression for ComposedCubeMapTexture environment mapping (REQ-CUBE).

§34.2.2: an environment texture is indexed by a direction, the (s, t, r)
coordinate its TextureCoordinateGenerator produces, or with no generator the
camera-space reflection vector. Figure 34.1 puts the front face at -Z, back at
+Z, left at -X, right at +X, top at +Y and bottom at -Y.

A radius-2 sphere (an IndexedFaceSet, so it can carry a generator) faces a
camera on +Z. Its six 1x1 faces have distinct colours, so a pixel names the
face its lookup direction pierced. The same scenes and expectations are pinned
for the CPU reference host by examples/cpu_raster/tests/cube_map_test.cpp.
An ImageCubeMapTexture (§34.4.3) naming a DDS cube with the same colours must
match the composed cube.
"""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 10.0
TAN = math.tan(math.pi / 8)  # default fieldOfView

CUBE = '''<ComposedCubeMapTexture>
<PixelTexture containerField="frontTexture" image="1 1 3 0xFF0000"/>
<PixelTexture containerField="backTexture" image="1 1 3 0x00FF00"/>
<PixelTexture containerField="leftTexture" image="1 1 3 0x0000FF"/>
<PixelTexture containerField="rightTexture" image="1 1 3 0xFFFF00"/>
<PixelTexture containerField="topTexture" image="1 1 3 0xFF00FF"/>
<PixelTexture containerField="bottomTexture" image="1 1 3 0x00FFFF"/>
</ComposedCubeMapTexture>'''
FACES = {'F': (1, 0, 0), 'B': (0, 1, 0), 'L': (0, 0, 1),
         'R': (1, 1, 0), 'T': (1, 0, 1), 'D': (0, 1, 1)}
MATERIALS = {
    'unlit': '',
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}


def sphere(generator):
    rings, segments = 48, 96
    points, normals, index = [], [], []
    for i in range(rings + 1):
        phi = math.pi * i / rings
        for j in range(segments + 1):
            theta = 2 * math.pi * j / segments
            n = (math.sin(phi) * math.sin(theta), math.cos(phi),
                 math.sin(phi) * math.cos(theta))
            points.append('%.5f %.5f %.5f' % tuple(2 * c for c in n))
            normals.append('%.5f %.5f %.5f' % n)
    for i in range(rings):
        for j in range(segments):
            a = i * (segments + 1) + j
            b = a + segments + 1
            index.append(f'{a} {b} {b + 1} {a + 1} -1')
    return (f'<IndexedFaceSet coordIndex="{" ".join(index)}">'
            f'<Coordinate point="{" ".join(points)}"/>'
            f'<Normal vector="{" ".join(normals)}"/>{generator}</IndexedFaceSet>')


def dds_cube(path):
    """A 1x1 BGRA DDS cube map. DDS stores faces +X, -X, +Y, -Y, +Z, -Z for a
    left-handed space; +Z is the X3D front (DdsDecode.hpp)."""
    header = bytearray(128)
    header[0:4] = b'DDS '
    for offset, value in ((4, 124), (8, 0x1007), (12, 1), (16, 1), (76, 32),
                          (80, 0x41), (88, 32), (92, 0xFF0000), (96, 0xFF00),
                          (100, 0xFF), (104, 0xFF000000), (108, 0x1008),
                          (112, 0xFE00)):
        struct.pack_into('<I', header, offset, value)
    faces = 'RLTDFB'  # +X right, -X left, +Y top, -Y bottom, +Z front, -Z back
    pixels = b''.join(bytes((round(255 * FACES[f][2]), round(255 * FACES[f][1]),
                             round(255 * FACES[f][0]), 255)) for f in faces)
    path.write_bytes(bytes(header) + pixels)


def render(tmp, name, appearance, generator=''):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><Viewpoint position="0 0 {DISTANCE}"/>
<Shape><Appearance>{appearance}</Appearance>{sphere(generator)}</Shape>
</Scene></X3D>''')
    result = subprocess.run([
        'xvfb-run', '-a', 'env', 'LIBGL_ALWAYS_SOFTWARE=1',
        'GALLIUM_DRIVER=llvmpipe', '__GLX_VENDOR_LIBRARY_NAME=mesa',
        str(renderer), '--screenshot', str(output), str(scene),
    ], timeout=120, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    header, data = output.read_bytes().split(b'\n255\n', 1)
    width, height = map(int, header.split()[1:3])

    def pixel(x, y):
        """RGB where the sphere point (x, y, z) projects (PPM rows top-down)."""
        z = math.sqrt(max(0.0, 4 - x * x - y * y))
        column = width // 2 + round(x / (DISTANCE - z) / TAN * (height / 2))
        row = height // 2 - round(y / (DISTANCE - z) / TAN * (height / 2))
        offset = (row * width + column) * 3
        return tuple(data[offset:offset + 3])
    return pixel


def face(rgb):
    """The face whose colour this pixel shows (lighting scales, hue stays)."""
    peak = max(rgb)
    if peak < 30:
        return '?'
    norm = tuple(c / peak for c in rgb)
    best = min(FACES, key=lambda k: sum((a - b) ** 2 for a, b in zip(norm, FACES[k])))
    return best if sum((a - b) ** 2 for a, b in zip(norm, FACES[best])) < 0.1 else '?'


R45 = math.sqrt(2)                                # normal 45 degrees off +Z
S60 = 2 * math.sin(math.radians(60))              # normal 60 degrees off +Z
generator = '<TextureCoordinateGenerator mode="{}"/>'.format

with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    checked = 0
    for name, material in MATERIALS.items():
        cases = [
            ('', [((0, 0), 'B'), ((R45, 0), 'R'), ((-R45, 0), 'L'),
                  ((0, R45), 'T'), ((0, -R45), 'D')]),
            (generator('CAMERASPACEPOSITION'), [((0, 0), 'F'), ((R45, 0), 'F')]),
            (generator('CAMERASPACENORMAL'), [((0, 0), 'B'), ((S60, 0), 'R'),
                                             ((0, S60), 'T')]),
            (generator('CAMERASPACEREFLECTIONVECTOR'), [((R45, 0), 'R')]),
        ]
        for index, (gen, samples) in enumerate(cases):
            pixel = render(tmp, f'{name}-{index}', material + CUBE, gen)
            for (x, y), want in samples:
                got = pixel(x, y)
                assert face(got) == want, (name, gen, (x, y), want, got)
                checked += 1

    # ImageCubeMapTexture: the DDS cube decodes to the same faces.
    dds = tmp / 'cube.dds'
    dds_cube(dds)
    image = f'<ImageCubeMapTexture url=\'"{dds}"\'/>'
    for gen, samples in (('', [((0, 0), 'B'), ((R45, 0), 'R'), ((-R45, 0), 'L'),
                               ((0, R45), 'T'), ((0, -R45), 'D')]),
                         (generator('CAMERASPACEPOSITION'), [((0, 0), 'F')])):
        pixel = render(tmp, f'image{len(gen)}', image, gen)
        for (x, y), want in samples:
            got = pixel(x, y)
            assert face(got) == want, ('ImageCubeMapTexture', gen, (x, y), want, got)
            checked += 1

    # A cube stage inside MultiTexture: right face (yellow) x 50% grey.
    pixel = render(tmp, 'multi', '<MultiTexture mode=\'"MODULATE" "MODULATE"\'>'
                   + CUBE + '<PixelTexture image="1 1 1 0x80"/></MultiTexture>')
    r, g, b = pixel(R45, 0)
    assert abs(r - 128) < 12 and abs(g - 128) < 12 and b < 12, (r, g, b)
    print(f'GL cube maps: reflection, position and normal lookups hit the '
          f'Figure 34.1 faces in unlit, Phong and PBR ({checked} samples); '
          f'DDS ImageCubeMapTexture matches; cube MultiTexture stage combines')
