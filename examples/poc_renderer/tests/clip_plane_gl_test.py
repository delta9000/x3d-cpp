"""Headless GL regression for ClipPlane in the OpenGL PoC (REQ-CLIP, §11.4.1).

A ClipPlane keeps the half-space a*x + b*y + c*z + d >= 0 of its siblings.
Each built-in program (Phong, PBR, unlit) must cut a box at x = 0 when the
plane is enabled and draw it whole when it is not.
"""
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 6.0


def render(tmp, name, material, plane):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/>
<Viewpoint position="0 0 {DISTANCE}"/><NavigationInfo headlight="false"/>
<Group>{plane}<Shape><Appearance>{material}</Appearance><Box size="3 3 3"/></Shape></Group>
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
        # The box's front face sits at z = 1.5, 4.5 from the eye.
        column = width // 2 + round(eye_x / ((DISTANCE - 1.5) * 0.41421356) * (height / 2))
        offset = ((height // 2) * width + column) * 3
        return sum(data[offset:offset + 3])
    return pixel


MATERIALS = {
    'phong': '<Material emissiveColor="1 0 0" diffuseColor="0 0 0"/>',
    'pbr': '<PhysicalMaterial emissiveColor="1 0 0" baseColor="0 0 0"/>',
    'unlit': '<UnlitMaterial emissiveColor="1 0 0"/>',
}

with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    for name, material in MATERIALS.items():
        cut = render(tmp, f'{name}-cut', material, '<ClipPlane plane="1 0 0 0"/>')
        assert cut(1.0) > 150 and cut(-1.0) < 30, (name, cut(1.0), cut(-1.0))
        whole = render(tmp, f'{name}-off', material,
                       '<ClipPlane plane="1 0 0 0" enabled="false"/>')
        assert whole(1.0) > 150 and whole(-1.0) > 150, (name, whole(1.0), whole(-1.0))
    print('GL ClipPlane: Phong, PBR and unlit programs clip; disabled planes do not')
