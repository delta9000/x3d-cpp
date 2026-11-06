"""Headless GL regression for PointLight and SpotLight in Phong and PBR."""
import pathlib
from gl_test_helpers import render as render_gl, pixels as screenshot_pixels
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])


def render(tmp, name, light, material):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    scene.write_text(f'''<X3D profile="Immersive" version="4.0"><Scene>
<NavigationInfo headlight="false"/>
<Viewpoint position="0 0 8"/>
{light}
<Shape><Appearance>{material}</Appearance><Box size="3 3 1"/></Shape>
</Scene></X3D>''')
    render_gl(renderer, scene, output)
    width, height, data = screenshot_pixels(output)
    # The center lies on the box's front face. No background pixels enter.
    center = ((height // 2) * width + width // 2) * 3
    return sum(data[center:center + 3])


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    materials = ('<Material diffuseColor="0.6 0.5 0.4"/>',
                 '<PhysicalMaterial baseColor="0.6 0.5 0.4" roughness="1"/>')
    for index, material in enumerate(materials):
        point = render(tmp, f'point_{index}',
                       '<PointLight global="true" location="0 0 4" radius="10"/>', material)
        cutoff = render(tmp, f'cutoff_{index}',
                        '<PointLight global="true" location="0 0 4" radius="1"/>', material)
        attenuated = render(tmp, f'attenuated_{index}',
                            '<PointLight global="true" location="0 0 4" radius="10" attenuation="1 1 0"/>', material)
        spot = render(tmp, f'spot_{index}',
                      '<SpotLight global="true" location="0 0 4" direction="0 0 -1" beamWidth="0.2" cutOffAngle="0.4"/>', material)
        feather = render(tmp, f'feather_{index}',
                         '<SpotLight global="true" location="0 0 4" direction="0.3 0 -1" beamWidth="0.2" cutOffAngle="0.4"/>', material)
        away = render(tmp, f'away_{index}',
                      '<SpotLight global="true" location="0 0 4" direction="1 0 0" beamWidth="0.2" cutOffAngle="0.4"/>', material)
        assert point > cutoff + 80, (index, point, cutoff)
        assert point > attenuated + 30, (index, point, attenuated)
        assert spot > feather + 30, (index, spot, feather)
        assert feather > away + 30, (index, feather, away)
    print('positional GL lighting: Phong and PBR point, radius, attenuation, spot OK')
