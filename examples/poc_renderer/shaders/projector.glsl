// §42 texture projectors (ADR-0061). A projector is uploaded as a light
// (positional for TextureProjector, directional for TextureProjectorParallel);
// uLightProjector[i] names its projector slot. projectorColor() filters that
// light by the projected texel (rgb × alpha) inside the projection volume and
// drops it outside, before the near distance or past the far distance, and on
// items outside a scoped projector's group (uProjectorMask, per item).
const int kMaxProjectors = 3;
uniform int uLightProjector[kMaxLights];            // slot + 1; 0 = ordinary light.
uniform int uProjectorMask;                         // bit per slot that lights this item.
uniform mat4 uProjectorMatrix[kMaxProjectors];      // eye -> projector eye.
uniform mat4 uProjectorProjection[kMaxProjectors];
uniform vec2 uProjectorRange[kMaxProjectors];       // near, far; <= 0 means none.
uniform int uProjectorParallel[kMaxProjectors];
uniform int uProjectorHasTex[kMaxProjectors];
uniform sampler2D uProjectorTex0;                   // units 13-15.
uniform sampler2D uProjectorTex1;
uniform sampler2D uProjectorTex2;

bool projectorColor(int i, vec3 posEye, out vec3 rgb) {
    rgb = vec3(1.0);
    int slot = uLightProjector[i] - 1;
    if (slot < 0) return true;
    if ((uProjectorMask & (1 << slot)) == 0) return false;
    vec4 e = uProjectorMatrix[slot] * vec4(posEye, 1.0);
    float d = -e.z;
    if (uProjectorParallel[slot] != 0 ? d < 0.0 : d <= 0.0) return false;
    if (uProjectorRange[slot].x > 0.0 && d < uProjectorRange[slot].x) return false;
    if (uProjectorRange[slot].y > 0.0 && d > uProjectorRange[slot].y) return false;
    vec4 c = uProjectorProjection[slot] * e;
    if (c.w <= 0.0 || any(greaterThan(abs(c.xy), vec2(c.w)))) return false;
    if (uProjectorHasTex[slot] == 0) return true;
    vec2 st = c.xy / c.w * 0.5 + 0.5;
    vec4 t = slot == 0 ? textureLod(uProjectorTex0, st, 0.0)
           : slot == 1 ? textureLod(uProjectorTex1, st, 0.0)
                       : textureLod(uProjectorTex2, st, 0.0);
    rgb = t.rgb * t.a;
    return true;
}
