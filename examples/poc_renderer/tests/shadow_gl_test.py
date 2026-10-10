"""Headless GL regression for light shadows in the OpenGL PoC (REQ-SHADOW).

§17.2.2.8 / §17.3.1: a light with shadows TRUE does not reach a fragment that a
Shape with castShadow TRUE hides from it; shadowIntensity sets how dark the
occluded fragment gets (the light's contribution scales by 1 - intensity).

A camera looks straight down at a white floor; a small slab floats 2 m above
it at x in [-3, -1]. Each light sits up and to the left, so the slab's shadow
falls on the floor around x = 0, which the camera sees past the slab. The
floor at x = 3 is lit in every case. The headlight is off and no light has
ambient, so a full shadow matches the floor with the light turned off (black
for Phong; PBR keeps its small constant ambient term). PBR writes sRGB, so
its levels are compared after decoding to linear.
"""
import math
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])
HEIGHT = 10.0
HALF_HEIGHT = (HEIGHT - 0.05) * math.tan(math.pi / 8)  # floor top at y = 0.05

LIGHTS = {
    'directional': '<DirectionalLight direction="1 -1 0" {extra}/>',
    'point': '<PointLight location="-4 4 0" {extra}/>',
    'spot': '<SpotLight location="-4 4 0" direction="1 -1 0" beamWidth="1.2" '
            'cutOffAngle="1.2" {extra}/>',
}
GAMMA = {'phong': False, 'pbr': True}
MATERIALS = {
    'phong': '<Material diffuseColor="1 1 1"/>',
    'pbr': '<PhysicalMaterial baseColor="1 1 1" metallic="0" roughness="1"/>',
}


def render(tmp, name, light, material, cast='true'):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Background skyColor="0 0 0"/><NavigationInfo headlight="false"/>
<Viewpoint position="0 {HEIGHT} 0" orientation="1 0 0 -1.5707963"/>
{light}
<Shape><Appearance>{material}</Appearance><Box size="12 0.1 12"/></Shape>
<Transform translation="-2 2 0"><Shape castShadow="{cast}">
<Appearance>{material}</Appearance><Box size="2 0.2 2"/></Shape></Transform>
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

    def floor(x):
        column = width // 2 + round(x / HALF_HEIGHT * (height / 2))
        offset = ((height // 2) * width + column) * 3
        return sum(data[offset:offset + 3]) / 3
    return floor


def linear(value, srgb):
    c = value / 255
    if not srgb:
        return c
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    for material_name, material in MATERIALS.items():
        srgb = GAMMA[material_name]
        level = lambda floor, x: linear(floor(x), srgb)
        dark = level(render(tmp, f'{material_name}-dark', LIGHTS['directional'].format(
            extra='on="false"'), material), 0.0)
        for light_name, light in LIGHTS.items():
            tag = f'{material_name}-{light_name}'
            floor = render(tmp, tag, light.format(extra='shadows="true"'), material)
            lit, shadowed = level(floor, 3.0), level(floor, 0.0)
            assert lit - dark > 0.1 and shadowed - dark < 0.1 * (lit - dark), \
                (tag, 'shadow', lit, shadowed, dark)

            # The floor at x = 0 gets at least as much light as at x = 3.
            for extra, cast, why in (('', 'true', 'shadows FALSE'),
                                     ('shadows="true"', 'false', 'castShadow FALSE')):
                floor = render(tmp, f'{tag}-{cast}', light.format(extra=extra),
                               material, cast=cast)
                unshadowed = level(floor, 0.0) - dark
                assert unshadowed > 0.8 * (level(floor, 3.0) - dark), (tag, why, unshadowed)

        # Directional light: ndl is the same at both points, so a half-intensity
        # shadow halves the light.
        floor = render(tmp, f'{material_name}-half', LIGHTS['directional'].format(
            extra='shadows="true" shadowIntensity="0.5"'), material)
        lit, half = level(floor, 3.0) - dark, level(floor, 0.0) - dark
        assert abs(half / lit - 0.5) < 0.08, (material_name, 'shadowIntensity', lit, half)
    print('GL shadows: directional, point and spot lights in Phong and PBR; '
          'shadowIntensity, castShadow and shadows FALSE honoured')
