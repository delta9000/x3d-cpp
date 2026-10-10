"""Headless GL regression for LocalFog in the OpenGL PoC (REQ-LOCALFOG, §24.4.3).

A LocalFog fogs only the geometry of its enclosing grouping node, in place of
the bound global Fog. Two boxes stand side by side, both well past a short
visibilityRange: the left one shares a Group with a green LocalFog, the right
one sees only the blue global Fog. Each built-in program (Phong, PBR, unlit)
must draw the left box green and the right one blue, and a disabled LocalFog
must leave the global Fog in charge of both.
"""
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 6.0
FRONT = 1.0  # half the box size: the front faces sit at z = 1.


def render(tmp, name, material, local_enabled):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    box = f'<Shape><Appearance>{material}</Appearance><Box size="2 2 2"/></Shape>'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/>
<Viewpoint position="0 0 {DISTANCE}"/><NavigationInfo headlight="false"/>
<Fog color="0 0 1" visibilityRange="2"/>
<Transform translation="-1.5 0 0"><Group>
<LocalFog color="0 1 0" visibilityRange="2" enabled="{local_enabled}"/>{box}
</Group></Transform>
<Transform translation="1.5 0 0">{box}</Transform>
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

    def pixel(eye_x):
        scale = (height / 2) / ((DISTANCE - FRONT) * 0.41421356)
        offset = ((height // 2) * width + width // 2 + round(eye_x * scale)) * 3
        return tuple(data[offset:offset + 3])
    return pixel


def near(pixel, rgb):
    return all(abs(a - b) < 40 for a, b in zip(pixel, rgb))


MATERIALS = {
    'phong': '<Material emissiveColor="1 0 0" diffuseColor="0 0 0"/>',
    'pbr': '<PhysicalMaterial emissiveColor="1 0 0" baseColor="0 0 0"/>',
    'unlit': '<UnlitMaterial emissiveColor="1 0 0"/>',
}

with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    for name, material in MATERIALS.items():
        scoped = render(tmp, f'{name}-local', material, 'true')
        assert near(scoped(-1.5), (0, 255, 0)), (name, scoped(-1.5))
        assert near(scoped(1.5), (0, 0, 255)), (name, scoped(1.5))
        disabled = render(tmp, f'{name}-disabled', material, 'false')
        assert near(disabled(-1.5), (0, 0, 255)), (name, disabled(-1.5))
        assert near(disabled(1.5), (0, 0, 255)), (name, disabled(1.5))
    print('GL LocalFog: Phong, PBR and unlit fog in scope; global Fog elsewhere')
