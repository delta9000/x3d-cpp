// shadow.glsl — §17.2.2.8 / §17.3.1 light shadows for lit.frag and pbr.frag
// (main.cpp expands the #include line; kMaxLights comes from the includer).
//
// main.cpp renders a depth map per shadow-enabled light into one depth array
// texture before the scene pass: one layer for a DirectionalLight (an
// orthographic view over the scene bounds) or a SpotLight (a perspective view
// over its cone), six for a PointLight (a cube of 90-degree views). Each
// layer's matrix maps EYE-space positions to that light's clip space. A
// fragment the light's depth map shows to be occluded receives the light
// scaled by 1 - shadowIntensity, as the CPU host's ray test does. Fragments
// outside every layer of a light are unshadowed.

const int kMaxShadowLayers = 24;

uniform sampler2DArrayShadow uShadowMaps;          // unit 5.
uniform mat4  uShadowMatrix[kMaxShadowLayers];     // eye -> light clip.
uniform int   uLightShadowBase[kMaxLights];        // first layer, -1 = no shadow.
uniform int   uLightShadowCount[kMaxLights];       // 1, or 6 for a PointLight.
uniform float uLightShadowIntensity[kMaxLights];

float shadowVisibility(int i, vec3 posEye) {
    int base = uLightShadowBase[i];
    if (base < 0) return 1.0;
    for (int k = 0; k < 6; ++k) {
        if (k >= uLightShadowCount[i]) break;
        vec4 c = uShadowMatrix[base + k] * vec4(posEye, 1.0);
        if (c.w <= 0.0) continue;
        vec3 n = c.xyz / c.w;
        if (abs(n.x) > 1.0 || abs(n.y) > 1.0 || n.z > 1.0) continue;
        vec3 s = n * 0.5 + 0.5;
        float lit = texture(uShadowMaps, vec4(s.xy, float(base + k), s.z));
        return 1.0 - uLightShadowIntensity[i] * (1.0 - lit);
    }
    return 1.0;
}
