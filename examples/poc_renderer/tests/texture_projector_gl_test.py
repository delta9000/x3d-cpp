"""Headless GL regression for §42 texture projectors in the OpenGL PoC
(REQ-PROJECTION, ADR-0061).

A white plane at z = 0 faces a camera on +Z with the headlight off and no
other light. A projector 5 in front of it casts a 2 x 2 PixelTexture (red,
green bottom; blue, yellow top), so each quadrant of the projected square
takes that texel's colour and the plane stays unlit outside the volume.
  * TextureProjector (perspective, fieldOfView 0.6) and
    TextureProjectorParallel (fieldOfView -1 -1 1 1), in Phong and PBR.
  * A scoped projector (global FALSE) in another group does not light it.
  * nearDistance beyond the plane leaves it unlit.
Colours are compared after subtracting the unlit corner, which PBR's constant
ambient floor keeps above black. Before this change the plane stayed unlit.
"""
import math
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
HALF = 10 * math.tan(math.pi / 8)  # half the view height on the plane
TEXTURE = ('<PixelTexture containerField="texture" '
           'image="2 2 3 0xFF0000 0x00FF00 0x0000FF 0xFFFF00"/>')
PERSPECTIVE = ('<TextureProjector location="0 0 5" direction="0 0 -1" upVector="0 1 0" '
               'fieldOfView="0.6" {extra}>' + TEXTURE + '</TextureProjector>')
PARALLEL = ('<TextureProjectorParallel location="0 0 5" direction="0 0 -1">' + TEXTURE +
            '</TextureProjectorParallel>')
MATERIALS = {
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}
HUES = {'R': (1, 0, 0), 'G': (0, 1, 0), 'B': (0, 0, 1), 'Y': (1, 1, 0)}


def render(tmp, name, projector, material):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><NavigationInfo headlight="false"/>
<Viewpoint position="0 0 10"/>
{projector}
<Shape><Appearance>{material}</Appearance><IndexedFaceSet coordIndex="0 1 2 3 -1">
<Coordinate point="-10 -10 0 10 -10 0 10 10 0 -10 10 0"/></IndexedFaceSet></Shape>
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

    def pixel(column, row):
        offset = (row * width + column) * 3
        return [c / 255 for c in data[offset:offset + 3]]
    base = pixel(2, 2)

    def hue(x, y):
        """The hue the plane point (x, y) adds over the unlit corner."""
        column = width // 2 + round(x / HALF * (height / 2))
        row = height // 2 - round(y / HALF * (height / 2))
        c = [a - b for a, b in zip(pixel(column, row), base)]
        peak = max(c)
        if peak < 0.05:
            return '.'
        n = [v / peak for v in c]
        for name, rgb in HUES.items():
            if sum((a - b) ** 2 for a, b in zip(n, rgb)) < 0.05:
                return name
        return f'?{c}'
    return hue


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    half = 5 * math.tan(0.3)  # perspective half-extent on the plane
    for material_name, material in MATERIALS.items():
        hue = render(tmp, f'{material_name}-perspective', PERSPECTIVE.format(extra=''), material)
        q, out = half / 2, half * 1.5
        got = [hue(-q, -q), hue(q, -q), hue(-q, q), hue(q, q), hue(out, 0), hue(0, out)]
        assert got == ['R', 'G', 'B', 'Y', '.', '.'], (material_name, 'perspective', got)

        hue = render(tmp, f'{material_name}-parallel', PARALLEL, material)
        got = [hue(-0.5, -0.5), hue(0.5, 0.5), hue(-1.5, 0)]
        assert got == ['R', 'Y', '.'], (material_name, 'parallel', got)

    material = MATERIALS['phong']
    hue = render(tmp, 'scoped', '<Group>' + PERSPECTIVE.format(extra='global="false"') +
                 '</Group>', material)
    assert hue(0.1, 0.1) == '.', ('scope', hue(0.1, 0.1))
    hue = render(tmp, 'near', PERSPECTIVE.format(extra='nearDistance="6"'), material)
    assert hue(half / 2, half / 2) == '.', ('nearDistance', hue(half / 2, half / 2))
    print('GL texture projectors: perspective and parallel quadrants in Phong and PBR, '
          'scope and near range OK')
