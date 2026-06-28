#version 330 core
// unlit.frag — PoC B4 unlit fragment shader. Lines and points (and any
// normal-less mesh) are ALWAYS unlit per the B4 consumer contract: there is no
// lighting, no normal, no cull. The color is the per-vertex Color when present
// (uHasColors != 0), else the material baseColor (uBaseColor.rgb), with alpha =
// 1 - transparency carried on uBaseColor.a.
//
// A textured Appearance with NO Material is unlit per spec (§12.2.5): the
// extractor surfaces it as MaterialModel::Unlit with the image on the Emissive
// slot. When uHasTexture != 0 the texture modulates the (white) surface color so
// the unlit textured surface shows the image — without it the sphere/texture-style
// scenes (Appearance + ImageTexture, no Material) render flat white.
out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform vec4 uBaseColor; // rgb = baseColor/diffuse, a = 1 - transparency.
uniform int  uHasColors; // 1 => per-vertex vColor overrides uBaseColor.rgb.
uniform sampler2D uTexture;
uniform int  uHasTexture; // 1 => modulate the surface color by the texture.

void main() {
    vec3 rgb = (uHasColors != 0) ? vColor.rgb : uBaseColor.rgb;
    float a  = (uHasColors != 0) ? vColor.a   : uBaseColor.a;
    if (uHasTexture != 0) {
        vec4 tx = texture(uTexture, vTexCoord);
        rgb *= tx.rgb;
        a   *= tx.a;
    }
    FragColor = vec4(rgb, a);
}
