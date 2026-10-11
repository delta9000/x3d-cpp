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

REQ-MULTITEXTURE-2 (the PoC matches cpu_raster beyond four stages and sets):

  * eight stages over eight UV sets: stage 0 REPLACEs with red (set 0), six
    white MODULATE stages follow, and stage 7 ADDs green from set 7 (the right
    texel). Yellow. Dropping stages past the fourth leaves red; clamping the
    channel to set 3 (a left texel) adds blue instead.
  * per-stage generators: stage 0 REPLACEs with red through
    CAMERASPACEPOSITION (s = 0 at the centre), stage 1 ADDs green through COORD
    (s = 0.75 on a quad whose local centre is x = 0.75). Yellow; one shared
    generator gives a different colour.
  * per-slot coordinates (Phong and PBR): an emissive texture with
    emissiveTextureMapping naming UV set 1 shows its right texel (green) while
    the base slot uses set 0; sharing set 0 shows red.

Each built-in program (unlit, Phong, PBR) runs the cases.
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


LEFT = '<TextureCoordinate point="0.25 0.5"/>'
RIGHT = '<TextureCoordinate point="0.75 0.5"/>'
WHITE = '<PixelTexture image="1 1 3 0xFFFFFF"/>'
RED_BLUE = '<PixelTexture image="2 1 3 0xFF0000 0x0000FF" repeatS="false" repeatT="false"/>'
BLUE_GREEN = '<PixelTexture image="2 1 3 0x0000FF 0x00FF00" repeatS="false" repeatT="false"/>'
EIGHT_STAGES = (
    "<MultiTexture mode='\"REPLACE\" " + '"MODULATE" ' * 6 + "\"ADD\"'>"
    + RED_BLUE + WHITE * 6 + BLUE_GREEN + '</MultiTexture>')
EIGHT_SETS = LEFT * 7 + RIGHT
TWO_STAGES = f'''<MultiTexture mode='"REPLACE" "ADD"'>{RED_BLUE}{BLUE_GREEN}</MultiTexture>'''
GENERATORS = ('<TextureCoordinateGenerator mode="CAMERASPACEPOSITION"/>'
              '<TextureCoordinateGenerator mode="COORD"/>')
QUAD = '-1 -1 0 1 -1 0 1 1 0 -1 1 0'
# Local x from -0.25 to 1.75; the Transform centres it in view.
OFFSET_QUAD = '-0.25 -1 0 1.75 -1 0 1.75 1 0 -0.25 1 0'
EMISSIVE = ('emissiveColor="1 1 1" emissiveTextureMapping="e">'
            '<PixelTexture containerField="emissiveTexture" image="2 1 3 0xFF0000 0x00FF00" '
            'repeatS="false" repeatT="false"/>')
SLOT_MATERIALS = {
    'phong': f'<Material diffuseColor="0 0 0" {EMISSIVE}</Material>',
    'pbr': f'<PhysicalMaterial baseColor="0 0 0" metallic="0" roughness="1" {EMISSIVE}'
           '</PhysicalMaterial>',
}


def center(tmp, name, material, texture, sets=LEFT + RIGHT, points=QUAD, shift=0.0):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><Viewpoint position="0 0 3"/>
<Transform translation="{shift} 0 0">
<Shape><Appearance>{material}{texture}</Appearance>
<IndexedFaceSet coordIndex="0 1 2 3 -1" texCoordIndex="0 0 0 0 -1">
<Coordinate point="{points}"/>
<MultiTextureCoordinate>{sets}</MultiTextureCoordinate>
</IndexedFaceSet></Shape></Transform>
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
        stages = center(tmp, f'{name}-stages', material, EIGHT_STAGES, EIGHT_SETS)
        assert hue(stages, (1, 1, 0)), (name, 'eight stages', stages)
        generated = center(tmp, f'{name}-generators', material, TWO_STAGES, GENERATORS,
                           OFFSET_QUAD, -0.75)
        assert hue(generated, (1, 1, 0)), (name, 'per-stage generators', generated)
    for name, material in SLOT_MATERIALS.items():
        slot = center(tmp, f'{name}-slot', material, '',
                      LEFT + '<TextureCoordinate mapping="e" point="0.75 0.5"/>')
        assert hue(slot, (0, 1, 0)), (name, 'per-slot UV set', slot)
    print('GL MultiTexture: stage modes, UV channels and sources in every program; '
          'eight stages and UV sets, per-stage generators and per-slot coordinates')
