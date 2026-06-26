#version 330 core
// unlit.frag — PoC B4 unlit fragment shader. Lines and points (and any
// normal-less mesh) are ALWAYS unlit per the B4 consumer contract: there is no
// lighting, no normal, no cull. The color is the per-vertex Color when present
// (uHasColors != 0), else the material baseColor (uBaseColor.rgb), with alpha =
// 1 - transparency carried on uBaseColor.a.
out vec4 FragColor;

in vec4 vColor;

uniform vec4 uBaseColor; // rgb = baseColor/diffuse, a = 1 - transparency.
uniform int  uHasColors; // 1 => per-vertex vColor overrides uBaseColor.rgb.

void main() {
    vec3 rgb = (uHasColors != 0) ? vColor.rgb : uBaseColor.rgb;
    float a  = (uHasColors != 0) ? vColor.a   : uBaseColor.a;
    FragColor = vec4(rgb, a);
}
