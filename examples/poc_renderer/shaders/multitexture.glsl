// multitexture.glsl — §18.4.3 MultiTexture stage combiner, shared by lit.frag,
// pbr.frag and unlit.frag (main.cpp expands the #include line).
//
// The host sets uNumStages > 0 only when the base-colour slot holds a
// MultiTexture (or a stage with non-default controls); otherwise the shader's
// single-texture multiply applies unchanged. Stage i samples unit 8+i with the
// UV set its channel selects. The mode names are the OpenGL / D3D
// fixed-function texture-environment operators: arg1 = the stage texture,
// arg2 = the previous result (seeded with the surface colour) or the colour the
// stage's source selects. Each stage output is clamped to [0,1]. This mirrors
// examples/cpu_raster/cpuraster/MaterialShader.hpp detail::combineBaseStages.

const int kMaxStages = 4;

in vec2 vTexSet[kMaxStages]; // UV sets 0..3 (texture transforms already baked).

uniform int       uNumStages;
uniform sampler2D uStageTex[kMaxStages];
uniform int       uStageHasTex[kMaxStages];  // 0 => sample white.
uniform ivec2     uStageMode[kMaxStages];    // (rgb, alpha) mode codes, below.
uniform int       uStageSource[kMaxStages];  // 0 previous, 1 DIFFUSE, 2 SPECULAR, 3 FACTOR.
uniform int       uStageFunction[kMaxStages]; // 0 none, 1 COMPLEMENT, 2 ALPHAREPLICATE.
uniform vec4      uStageFactor[kMaxStages];  // MultiTexture color.rgb + alpha.
uniform int       uStageChannel[kMaxStages]; // UV set; -1 = the generated coordinates (texgen.glsl).

// Mode codes (main.cpp multiTextureModeCode):
//  0 MODULATE  1 REPLACE/SELECTARG1  2 SELECTARG2  3 MODULATE2X  4 MODULATE4X
//  5 ADD  6 ADDSIGNED  7 ADDSIGNED2X  8 SUBTRACT  9 ADDSMOOTH
// 10 BLENDDIFFUSEALPHA  11 BLENDTEXTUREALPHA  12 BLENDFACTORALPHA
// 13 BLENDCURRENTALPHA  14 MODULATEALPHA_ADDCOLOR  15 MODULATEINVALPHA_ADDCOLOR
// 16 MODULATEINVCOLOR_ADDALPHA  17 DOTPRODUCT3  18 OFF
vec4 mtCombine(int mode, int func, vec4 a1, vec4 a2, float diffuseAlpha,
               float factorAlpha) {
    vec4 r;
    if (mode == 18)      r = a2;
    else if (mode == 1)  r = a1;
    else if (mode == 2)  r = a2;
    else if (mode == 3)  r = a1 * a2 * 2.0;
    else if (mode == 4)  r = a1 * a2 * 4.0;
    else if (mode == 5)  r = a1 + a2;
    else if (mode == 6)  r = a1 + a2 - vec4(0.5);
    else if (mode == 7)  r = (a1 + a2 - vec4(0.5)) * 2.0;
    else if (mode == 8)  r = a1 - a2;
    else if (mode == 9)  r = a1 + a2 - a1 * a2;
    else if (mode >= 10 && mode <= 13) {
        float t = mode == 10 ? diffuseAlpha : mode == 11 ? a1.a
                : mode == 12 ? factorAlpha : a2.a;
        r = a1 * t + a2 * (1.0 - t);
    }
    else if (mode == 14) r = vec4(a1.rgb + a2.rgb * a1.a, a1.a * a2.a);
    else if (mode == 15) r = vec4(a1.rgb + a2.rgb * (1.0 - a1.a), a1.a * a2.a);
    else if (mode == 16) r = vec4((vec3(1.0) - a1.rgb) * a2.rgb + vec3(a1.a), a1.a * a2.a);
    else if (mode == 17) r = vec4(dot(a1.rgb * 2.0 - 1.0, a2.rgb * 2.0 - 1.0));
    else                 r = a1 * a2; // MODULATE, the default.
    if (func == 1)      r = vec4(1.0) - r;
    else if (func == 2) r = vec4(r.a);
    return clamp(r, 0.0, 1.0);
}

vec2 mtUv(int i, vec2 generated) {
    int c = uStageChannel[i];
    return c < 0 ? generated : vTexSet[min(c, kMaxStages - 1)];
}

vec4 mtStage(int i, vec4 texel, vec4 acc, vec3 diffuse, vec3 specular,
             float alpha0) {
    if (uStageHasTex[i] == 0) texel = vec4(1.0);
    int s = uStageSource[i];
    vec4 arg2 = s == 3 ? uStageFactor[i]
              : s == 2 ? vec4(specular, alpha0)
              : s == 1 ? vec4(diffuse, alpha0) : acc;
    float fa = uStageFactor[i].a;
    vec4 rgb = mtCombine(uStageMode[i].x, uStageFunction[i], texel, arg2, alpha0, fa);
    vec4 a = mtCombine(uStageMode[i].y, uStageFunction[i], texel, arg2, alpha0, fa);
    return vec4(rgb.rgb, a.a);
}

// Fold the stages into `initial` (the surface colour). Samplers are indexed
// with constants only, as GLSL 3.30 requires.
vec4 applyMultiTexture(vec4 initial, vec3 diffuse, vec3 specular, vec2 generated) {
    vec4 acc = initial;
    float a0 = initial.a;
    if (uNumStages > 0)
        acc = mtStage(0, texture(uStageTex[0], mtUv(0, generated)), acc, diffuse, specular, a0);
    if (uNumStages > 1)
        acc = mtStage(1, texture(uStageTex[1], mtUv(1, generated)), acc, diffuse, specular, a0);
    if (uNumStages > 2)
        acc = mtStage(2, texture(uStageTex[2], mtUv(2, generated)), acc, diffuse, specular, a0);
    if (uNumStages > 3)
        acc = mtStage(3, texture(uStageTex[3], mtUv(3, generated)), acc, diffuse, specular, a0);
    return acc;
}
