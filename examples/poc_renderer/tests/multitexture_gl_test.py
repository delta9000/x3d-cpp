"""Headless GL regression for MultiTexture in the OpenGL PoC (REQ-MULTITEXTURE).

§18.4.3: each MultiTexture stage combines its texture (arg1) with the previous
result or the colour its source selects (arg2), using the UV set its
MultiTextureCoordinate channel names. A quad faces the camera with two UV sets:
set 0 samples the left texel of 2x1 textures, set 1 the right texel.

  * channels: stage 0 REPLACEs with red (set 0); stage 1 ADDs green from set 1.
    The result is yellow. Ignoring stage 1 leaves red; sampling it with set 0
    adds black, also red.
  * source: one white stage MODULATEs the FACTOR colour (blue) instead of the
    surface colour, so the quad is blue.

Each built-in program (unlit, Phong, PBR) runs both cases.
"""
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])

CHANNELS = '''<MultiTexture mode='"REPLACE" "ADD"'>
<PixelTexture image="2 1 3 0xFF0000 0x0000FF" repeatS="false" repeatT="false"/>
<PixelTexture image="2 1 3 0x0000FF 0x00FF00" repeatS="false" repeatT="false"/>
</MultiTexture>'''
FACTOR = '''<MultiTexture mode='"MODULATE"' source='"FACTOR"' color="0 0 1">
<PixelTexture image="1 1 3 0xFFFFFF"/>
</MultiTexture>'''

MATERIALS = {
    'unlit': '',
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}


def center(tmp, name, material, texture):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><Viewpoint position="0 0 3"/>
<Shape><Appearance>{material}{texture}</Appearance>
<IndexedFaceSet coordIndex="0 1 2 3 -1" texCoordIndex="0 0 0 0 -1">
<Coordinate point="-1 -1 0 1 -1 0 1 1 0 -1 1 0"/>
<MultiTextureCoordinate>
<TextureCoordinate point="0.25 0.5"/><TextureCoordinate point="0.75 0.5"/>
</MultiTextureCoordinate>
</IndexedFaceSet></Shape>
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
    offset = ((height // 2) * width + width // 2) * 3
    return tuple(data[offset:offset + 3])


def hue(pixel, rgb):
    """Each channel the expected colour lacks stays dark; the others are lit."""
    return all((v > 60) if on else (v < 30) for v, on in zip(pixel, rgb))


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    for name, material in MATERIALS.items():
        channels = center(tmp, f'{name}-channels', material, CHANNELS)
        assert hue(channels, (1, 1, 0)), (name, 'channels', channels)
        factor = center(tmp, f'{name}-factor', material, FACTOR)
        assert hue(factor, (0, 0, 1)), (name, 'factor', factor)
    print('GL MultiTexture: stage modes, UV channels and sources in every program')
