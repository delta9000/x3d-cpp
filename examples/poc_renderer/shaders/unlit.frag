#version 330 core
// unlit.frag — PoC unlit fragment shader. Normal-less geometry and
// UnlitMaterial use the per-vertex Color when present (uHasColors != 0),
// otherwise uBaseColor.rgb. For lines and points, the caller supplies the
// material emissiveColor (§11.2.2.5). Alpha is uBaseColor.a.
//
// A textured Appearance with NO Material is unlit per spec (§12.2.5): the
// extractor surfaces it as MaterialModel::Unlit with the image on the Emissive
// slot. When uHasTexture != 0 the texture modulates the (white) surface color so
// the unlit textured surface shows the image — without it the sphere/texture-style
// scenes (Appearance + ImageTexture, no Material) render flat white.
out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vPosEye;

uniform vec4 uBaseColor; // rgb = unlit surface color, a = 1 - transparency.
uniform int  uHasColors; // 1 => per-vertex vColor overrides uBaseColor.rgb.
uniform sampler2D uTexture;
uniform int  uHasTexture; // 1 => modulate the surface color by the texture.

// FillProperties applies to polygons only; the draw path sends mode 1 for
// lines, points, and appearances without FillProperties. Bit 1 fills, bit 2
// hatches. The hatch grid is anchored to the window, in 8-pixel cells.
uniform int uFillMode;
uniform int uHatchStyle;
uniform vec3 uHatchColor;

bool hatchPixel() {
    vec2 p = floor(gl_FragCoord.xy);
    int style = (uHatchStyle >= 1 && uHatchStyle <= 6) ? uHatchStyle : 1;
    bool horizontal = mod(p.y, 8.0) < 1.0;
    bool vertical = mod(p.x, 8.0) < 1.0;
    bool positive = mod(p.x - p.y, 8.0) < 1.0;
    bool negative = mod(p.x + p.y, 8.0) < 1.0;
    if (style == 1) return horizontal;
    if (style == 2) return vertical;
    if (style == 3) return positive;
    if (style == 4) return negative;
    if (style == 5) return horizontal || vertical;
    return positive || negative;
}

// ---- Fog (§24.4.2 / §17 Table 17.5) -----------------------------------------
// visibilityRange is already world-scaled by the extractor; 0 disables fog.
uniform vec3  uFogColor;
uniform int   uFogType;            // 0 = LINEAR, 1 = EXPONENTIAL.
uniform float uFogVisibilityRange;

// §17 fog: d = eye-space distance, V = visibilityRange. LINEAR f = (V-d)/V
// (d<V else 0); EXPONENTIAL f = exp(-d/(V-d)) (d<V else 0).
vec3 applyFog(vec3 color, float d) {
    float V = uFogVisibilityRange;
    if (V <= 0.0) return color;       // fog disabled.
    float f = 0.0;                    // d >= V => fully fogged.
    if (d < V)
        f = (uFogType == 1) ? exp(-d / (V - d)) : (V - d) / V;
    return color * f + uFogColor * (1.0 - f);
}

void main() {
    bool hatch = (uFillMode & 2) != 0 && hatchPixel();
    if ((uFillMode & 1) == 0 && !hatch) discard;
    vec3 rgb = (uHasColors != 0) ? vColor.rgb : uBaseColor.rgb;
    float a  = (uHasColors != 0) ? vColor.a   : uBaseColor.a;
    if (uHasTexture != 0) {
        vec4 tx = texture(uTexture, vTexCoord);
        rgb *= tx.rgb;
        a   *= tx.a;
    }
    // §17: fog applies to the unlit equation too, as the final step.
    rgb = applyFog(hatch ? uHatchColor : rgb, length(vPosEye));
    FragColor = vec4(rgb, a);
}
