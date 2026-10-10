"""Headless GL regression for eye-space TextureCoordinateGenerator modes (#140)."""
import math
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
DISTANCE = 8.0
HALF_HEIGHT = DISTANCE * math.tan(math.pi / 8)  # default fieldOfView, 720p window


def render(tmp, name, texcoord):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    # u < 0.5 samples red, u in (0.5, 1) samples blue. Authored UVs are all
    # u = 0.75, so a mode that falls back to them reads blue.
    scene.write_text(f'''<X3D profile="Interactive" version="4.0"><Scene>
<Viewpoint position="0 0 {DISTANCE}"/>
<Shape><Appearance><Material diffuseColor="1 1 1"/>
<PixelTexture image="4 1 3 0xFF0000 0xFF0000 0x0000FF 0x0000FF"/></Appearance>
<IndexedFaceSet coordIndex="0 1 2 3 -1" texCoordIndex="0 0 0 0 -1">
<Coordinate point="-6 -6 0 6 -6 0 6 6 0 -6 6 0"/>{texcoord}</IndexedFaceSet></Shape>
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
        # Column whose eye-space surface point has x = eye_x on the y = 0 row.
        column = width // 2 + round(eye_x / HALF_HEIGHT * (height / 2))
        offset = ((height // 2) * width + column) * 3
        return data[offset], data[offset + 2]  # red, blue
    return pixel


def red(sample):
    r, b = sample
    return r > b + 40


def blue(sample):
    r, b = sample
    return b > r + 40


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    authored = '<TextureCoordinate point="0.75 0.5"/>'
    fallback = render(tmp, 'authored', authored)
    assert blue(fallback(0.25)), fallback(0.25)
    for mode in ('CAMERASPACEPOSITION', 'COORD-EYE'):
        # u = eye x = 0.25 -> red. COORD-EYE used to fall back to authored UVs.
        pixel = render(tmp, mode, f'<TextureCoordinateGenerator mode="{mode}"/>')
        assert red(pixel(0.25)), (mode, pixel(0.25))
    # Facing quad, N = (0, 0, 1): at eye x = 2, E = normalize(-2, 0, 8) and
    # R = 2 dot(E, N) N - E has R.x = +0.243 -> red. The reversed eye vector
    # gave -0.243, which repeats to u = 0.757 -> blue.
    pixel = render(tmp, 'reflection',
                   '<TextureCoordinateGenerator mode="CAMERASPACEREFLECTIONVECTOR"/>')
    assert red(pixel(2.0)), pixel(2.0)
    print('GL texgen: COORD-EYE and CAMERASPACEREFLECTIONVECTOR OK')
