"""Headless GL regression for scene-authored shaders (REQ-SHADER, §31).

The SDK selects each Appearance's shader against the PoC's GL compiler and
hands the program over on RenderItem::shaderProgram. A selected program draws
with its author <field> uniforms (SF values, MF arrays and texture
samplers); one that does not compile is skipped for the
next shader, or for the fixed-function material when none is left.
"""
import pathlib
import subprocess
import sys
import tempfile

renderer = pathlib.Path(sys.argv[1])

VERT = '''#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
void main() { gl_Position = projectionMatrix * modelViewMatrix * vec4(aPos, 1.0); }'''


def frag(body):
    return f'''#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {{ {body} }}'''


def composed(body, color='1 0 0'):
    return f'''<ComposedShader language="GLSL">
<field name="uColor" type="SFColor" accessType="inputOutput" value="{color}"/>
<ShaderPart type="VERTEX"><![CDATA[{VERT}]]></ShaderPart>
<ShaderPart type="FRAGMENT"><![CDATA[{frag(body)}]]></ShaderPart>
</ComposedShader>'''


def center(tmp, name, shaders):
    scene = tmp / f'{name}.x3d'
    output = tmp / f'{name}.ppm'
    # The material alone draws blue (emissive, unlit by the absent headlight).
    scene.write_text(f'''<X3D profile="Full" version="4.0"><Scene>
<Viewpoint position="0 0 6"/><NavigationInfo headlight="false"/>
<Shape><Appearance><Material emissiveColor="0 0 1" diffuseColor="0 0 0"/>
{shaders}</Appearance><Box size="2 2 2"/></Shape>
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


def near(pixel, rgb):
    return all(abs(a - b) < 40 for a, b in zip(pixel, rgb))


with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    material = center(tmp, 'material', '')
    assert near(material, (0, 0, 255)), material

    selected = center(tmp, 'selected', composed('FragColor = vec4(uColor, 1.0);'))
    assert near(selected, (255, 0, 0)), selected

    fallthrough = center(tmp, 'fallthrough',
                         composed('FragColor = vec4(uColor, 1.0) +;') +
                         composed('FragColor = vec4(uColor, 1.0);', '0 1 0'))
    assert near(fallthrough, (0, 255, 0)), fallthrough

    invalid = center(tmp, 'invalid', composed('FragColor = vec4(uColor, 1.0) +;'))
    assert near(invalid, (0, 0, 255)), invalid

    # REQ-SHADER-2: SFNode -> sampler2D, MFNode -> sampler2D[], MF -> arrays.
    arrays = center(tmp, 'arrays', f'''<ComposedShader language="GLSL">
<field name="uTex" type="SFNode" accessType="inputOutput">
<PixelTexture image="1 1 3 0x00FF00"/></field>
<field name="uLayers" type="MFNode" accessType="inputOutput">
<PixelTexture image="1 1 3 0xFF0000"/><PixelTexture image="1 1 3 0x0000FF"/></field>
<field name="uW" type="MFFloat" accessType="inputOutput" value="0 0 0.5"/>
<field name="uP" type="MFVec3f" accessType="inputOutput" value="0 0 0 0.25 0 0"/>
<ShaderPart type="VERTEX"><![CDATA[{VERT}]]></ShaderPart>
<ShaderPart type="FRAGMENT"><![CDATA[#version 330 core
uniform sampler2D uTex;
uniform sampler2D uLayers[2];
uniform float uW[3];
uniform vec3 uP[2];
out vec4 FragColor;
void main() {{
  FragColor = vec4(texture(uTex, vec2(0.5)).g * uW[2],
                   texture(uLayers[1], vec2(0.5)).b, uP[1].x, 1.0);
}}]]></ShaderPart>
</ComposedShader>''')
    assert near(arrays, (128, 255, 64)), arrays
    print('GL author shaders: selection, uniforms, texture and array fields, and '
          'fallback OK')
