"""Headless GL regression for GeneratedCubeMapTexture (REQ-CUBE, §34.4.2).

A radius-2 sphere (solid FALSE, so visible from inside) at the origin carries
a GeneratedCubeMapTexture and is surrounded by six unlit boxes, one per axis,
in the Figure 34.1 face colours. The camera is on +Z. The default lookup is the
reflection vector, so the sphere's centre shows the box behind the camera and
the points whose normal leans 45 degrees show the box along that axis. That
only holds when the faces are rendered from the sphere's origin along its axes
without the sphere itself. Also checked:
  * update NONE with nothing rendered before samples white;
  * NEXT_FRAME_ONLY keeps the faces it rendered after the system resets it to
    NONE (--screenshot captures a later frame);
  * under a Transform rotated 90 degrees about +Y the faces and the lookup
    both follow the local axes, so the reflection shows the same boxes.
The same scene is pinned for the CPU reference host by
examples/cpu_raster/tests/generated_cube_map_test.cpp.
"""
import math
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 10.0
TAN = math.tan(math.pi / 8)  # default fieldOfView

FACES = {'F': (1, 0, 0), 'B': (0, 1, 0), 'L': (0, 0, 1),
         'R': (1, 1, 0), 'T': (1, 0, 1), 'D': (0, 1, 1), 'W': (1, 1, 1)}
BOXES = {'F': '0 0 -20', 'B': '0 0 20', 'L': '-20 0 0', 'R': '20 0 0',
         'T': '0 20 0', 'D': '0 -20 0'}
MATERIALS = {
    'unlit': '',
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}


def render(tmp, name, material, update, rotation='0 1 0 0'):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    boxes = ''.join(
        f'<Transform translation="{at}"><Shape><Appearance><UnlitMaterial '
        f'emissiveColor="{" ".join(map(str, FACES[k]))}"/></Appearance>'
        f'<Box size="16 16 16"/></Shape></Transform>' for k, at in BOXES.items())
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><Viewpoint position="0 0 {DISTANCE}"/>
<Transform rotation="{rotation}"><Shape><Appearance>{material}
<GeneratedCubeMapTexture size="32" update="{update}"/></Appearance>
<Sphere radius="2" solid="false"/></Shape></Transform>{boxes}
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
    """The box whose colour this pixel shows (lighting scales, hue stays)."""
    peak = max(rgb)
    if peak < 30:
        return '?'
    norm = tuple(c / peak for c in rgb)
    best = min(FACES, key=lambda k: sum((a - b) ** 2 for a, b in zip(norm, FACES[k])))
    return best if sum((a - b) ** 2 for a, b in zip(norm, FACES[best])) < 0.1 else '?'


R45 = math.sqrt(2)  # normal 45 degrees off +Z
AROUND = [((0, 0), 'B'), ((R45, 0), 'R'), ((-R45, 0), 'L'),
          ((0, R45), 'T'), ((0, -R45), 'D')]

with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    checked = 0
    cases = []
    for name, material in MATERIALS.items():
        cases += [
            (f'{name}-always', material, 'ALWAYS', '0 1 0 0', AROUND),
            (f'{name}-rotated', material, 'ALWAYS', '0 1 0 1.5707963',
             [((0, 0), 'B'), ((R45, 0), 'R'), ((0, R45), 'T')]),
        ]
    cases += [
        ('next-frame', '', 'NEXT_FRAME_ONLY', '0 1 0 0', AROUND),
        ('none', '', 'NONE', '0 1 0 0', [((0, 0), 'W')]),
    ]
    for name, material, update, rotation, samples in cases:
        pixel = render(tmp, name, material, update, rotation)
        for (x, y), want in samples:
            got = pixel(x, y)
            assert face(got) == want, (name, (x, y), want, got)
            checked += 1
    print(f'GL generated cube maps: faces seen from the local origin without the '
          f'sphere in unlit, Phong and PBR, local frame under rotation, '
          f'NEXT_FRAME_ONLY kept and NONE white ({checked} samples)')
