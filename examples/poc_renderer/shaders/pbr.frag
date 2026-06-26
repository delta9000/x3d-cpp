#version 330 core
// pbr.frag — PoC PhysicalMaterial (metallic-roughness) fragment shader.
//
// Implements the glTF 2.0 §3.9 metallic-roughness BRDF using analytic
// directional lights only. Phase 4 (IBL / environment lighting) is deferred;
// there is NO envDiffuse / envSpecular / brdfLUT term here.
//
// BRDF summary:
//   f = f_diffuse + f_specular
//   f_diffuse  = (1 - metallic) * baseColor / pi
//   f_specular = D(h) * G(l,v) * F(v,h) / (4 * NdotL * NdotV)
//   D = GGX / Trowbridge-Reitz: alpha^2 / (pi*(NdotH^2*(alpha^2-1)+1)^2)
//   G = Schlick-Smith GGX: separate NdotV and NdotL terms with k=alpha/2
//   F = Schlick: F0 + (1-F0)*(1-VdotH)^5; F0 = mix(0.04, baseColor, metallic)
//
// TEXTURE SLOTS (units match the PoC bind order in main.cpp):
//   Unit 0 — base-color (uBaseColorTex / uHasBaseColorTex)   — sRGB
//   Unit 1 — normal map (uNormalTex / uHasNormalTex)         — linear
//   Unit 2 — emissive   (uEmissiveTex / uHasEmissiveTex)     — sRGB
//   Unit 3 — metallic-roughness ORM (uMetallicRoughnessTex)  — linear
//             glTF packing: R=occlusion, G=roughness, B=metallic
//   Unit 4 — occlusion  (uOcclusionTex / uHasOcclusionTex)   — linear (R only)
//             (if slot 3 already packs occlusion this may be redundant)
//
// OUTPUT: always sRGB (linearToSRGB applied at the end).
// TWO-SIDED: N flipped via gl_FrontFacing, same as lit.frag.
// NORMAL MAPPING: same screen-space derivative TBN as lit.frag.

out vec4 FragColor;

const int kMaxLights = 8;
const float PI = 3.14159265358979323846;

in vec3 vNormalEye;
in vec3 vPosEye;
in vec4 vColor;
in vec2 vTexCoord;

// ---- Material params (physical model) --------------------------------------
uniform vec4  uBaseColor;    // rgb = baseColor, a = 1 - transparency.
uniform float uMetallic;     // [0,1].
uniform float uRoughness;    // [0,1] — remapped to alpha = roughness^2.
uniform vec3  uEmissive;     // emissive contribution (pre-multiplied).
uniform float uNormalScale;  // normal map strength.
uniform float uOcclusionStrength; // AO strength.

// ---- Alpha ------------------------------------------------------------------
uniform int   uAlphaMode;    // 0=Opaque, 1=Blend, 2=Mask.
uniform float uAlphaCutoff;

// ---- Vertex color override --------------------------------------------------
uniform int uHasColors;

// ---- Texture slots ----------------------------------------------------------
uniform int       uHasBaseColorTex;
uniform sampler2D uBaseColorTex;    // unit 0.

uniform int       uHasNormalTex;
uniform sampler2D uNormalTex;       // unit 1.

uniform int       uHasEmissiveTex;
uniform sampler2D uEmissiveTex;     // unit 2.

uniform int       uHasMetallicRoughnessTex;
uniform sampler2D uMetallicRoughnessTex; // unit 3: ORM (R=AO, G=rough, B=metal).

uniform int       uHasOcclusionTex;
uniform sampler2D uOcclusionTex;    // unit 4: separate occlusion (R channel).

// ---- Lights -----------------------------------------------------------------
uniform int  uNumLights;
uniform vec3 uLightDirEye[kMaxLights]; // direction of TRAVEL, eye space.
uniform vec3 uLightColor[kMaxLights];  // rgb * intensity.

// ---- GGX BRDF helpers -------------------------------------------------------

// GGX normal distribution function (Trowbridge-Reitz).
float D_GGX(float NdotH, float alpha2) {
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

// Schlick-Smith visibility function (combined G/4NL*NV).
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alpha2) {
    float GV = NdotL * sqrt(NdotV * NdotV * (1.0 - alpha2) + alpha2);
    float GL = NdotV * sqrt(NdotL * NdotL * (1.0 - alpha2) + alpha2);
    return 0.5 / max(GV + GL, 1e-5);
}

// Schlick Fresnel approximation.
vec3 F_Schlick(float VdotH, vec3 F0) {
    float f = 1.0 - VdotH;
    float f2 = f * f;
    return F0 + (1.0 - F0) * (f2 * f2 * f);
}

// sRGB gamma encoding (piecewise, same as lit.frag).
vec3 linearToSRGB(vec3 lin) {
    bvec3 cutoff = lessThan(lin, vec3(0.0031308));
    vec3 lower   = lin * 12.92;
    vec3 upper   = pow(clamp(lin, 0.0, 1.0), vec3(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(upper, lower, cutoff);
}

void main() {
    // ---- Base color ---------------------------------------------------------
    vec4 baseCol = uBaseColor;
    if (uHasColors != 0) baseCol.rgb = vColor.rgb;
    if (uHasBaseColorTex != 0) {
        // Base-color textures are uploaded as GL_SRGB8_ALPHA8, so the GPU already
        // linearizes on sample — do NOT pow(2.2) again here (that double-decodes
        // and darkens). Matches lit.frag, which also trusts the hardware decode.
        baseCol *= texture(uBaseColorTex, vTexCoord);
    }
    float alpha = baseCol.a;
    if (uAlphaMode == 2 && alpha < uAlphaCutoff) discard;

    // ---- Metallic / roughness from ORM texture or uniforms ------------------
    float metallic  = uMetallic;
    float roughness = uRoughness;
    if (uHasMetallicRoughnessTex != 0) {
        vec3 orm = texture(uMetallicRoughnessTex, vTexCoord).rgb;
        // glTF: G = roughness, B = metallic. R = occlusion (handled below).
        roughness *= orm.g;
        metallic  *= orm.b;
    }
    float alpha2 = max(roughness * roughness, 0.001);
    alpha2 = alpha2 * alpha2; // perceptual roughness -> linear alpha^2

    // ---- Occlusion ----------------------------------------------------------
    float ao = 1.0;
    if (uHasMetallicRoughnessTex != 0) {
        ao = texture(uMetallicRoughnessTex, vTexCoord).r; // ORM R channel.
    } else if (uHasOcclusionTex != 0) {
        ao = texture(uOcclusionTex, vTexCoord).r;
    }
    ao = mix(1.0, ao, uOcclusionStrength);

    // ---- Geometric normal (two-sided) ---------------------------------------
    vec3 Ngeo = normalize(vNormalEye);
    if (!gl_FrontFacing) Ngeo = -Ngeo;

    // ---- Normal mapping (same derivative-TBN approach as lit.frag) ----------
    vec3 N = Ngeo;
    if (uHasNormalTex != 0) {
        vec3 tsN = texture(uNormalTex, vTexCoord).rgb * 2.0 - 1.0;
        tsN.xy *= uNormalScale;
        tsN = normalize(tsN);
        vec3 dPdx  = dFdx(vPosEye);
        vec3 dPdy  = dFdy(vPosEye);
        vec2 dUVdx = dFdx(vTexCoord);
        vec2 dUVdy = dFdy(vTexCoord);
        float det  = dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x;
        if (abs(det) > 1e-6) {
            vec3 T = normalize((dPdx * dUVdy.y - dPdy * dUVdx.y) / det);
            T = normalize(T - dot(T, Ngeo) * Ngeo);
            vec3 B = cross(Ngeo, T);
            N = normalize(T * tsN.x + B * tsN.y + Ngeo * tsN.z);
        }
    }

    // ---- PBR setup ----------------------------------------------------------
    vec3 V    = normalize(-vPosEye);
    float NdV = max(dot(N, V), 0.0);
    // Dielectric base reflectance 0.04; metals absorb diffuse.
    vec3 F0  = mix(vec3(0.04), baseCol.rgb, metallic);
    // Diffuse color for non-metals.
    vec3 diffColor = (1.0 - metallic) * baseCol.rgb;

    // ---- Emissive -----------------------------------------------------------
    vec3 emissive = uEmissive;
    if (uHasEmissiveTex != 0) {
        // Emissive textures are uploaded sRGB (GL_SRGB8_ALPHA8) → GPU linearizes
        // on sample; no shader pow(2.2) (would double-decode). Matches lit.frag.
        vec3 et = texture(uEmissiveTex, vTexCoord).rgb;
        emissive *= et;
    }

    // ---- Light accumulation (analytic GGX BRDF, directional lights only) ----
    // NOTE: Phase 4 IBL (envDiffuse SH + prefiltered specular + brdfLUT) is
    // DEFERRED; there is no environment term here.
    vec3 color = emissive;

    for (int i = 0; i < uNumLights && i < kMaxLights; ++i) {
        vec3 L    = normalize(-uLightDirEye[i]); // toward light
        float NdL = max(dot(N, L), 0.0);
        if (NdL <= 0.0) continue;

        vec3  H    = normalize(L + V);
        float NdH  = max(dot(N, H), 0.0);
        float VdH  = max(dot(V, H), 0.0);

        // Specular: Cook-Torrance GGX.
        float D   = D_GGX(NdH, alpha2);
        float Vis = V_SmithGGXCorrelated(NdL, NdV, alpha2);
        vec3  F   = F_Schlick(VdH, F0);
        vec3 spec = D * Vis * F;

        // Diffuse: Lambertian.
        vec3 kD = (1.0 - F) * (1.0 - metallic);

        color += (kD * diffColor / PI + spec) * uLightColor[i] * NdL;
    }

    // ---- Apply AO on the diffuse component (ambient occlusion) -------------
    // A simple ambient term to make unlit areas not pitch-black.
    vec3 ambient = 0.03 * diffColor * ao;
    color += ambient;

    // ---- sRGB output --------------------------------------------------------
    color = linearToSRGB(color);

    FragColor = vec4(color, alpha);
}
