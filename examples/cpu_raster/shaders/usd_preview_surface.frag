#version 330 core
// usd_preview_surface.frag — the PORTABLE UsdPreviewSurface reference shader.
//
// This is the frozen contract of the USD→realtime material seam: ONE fragment
// shader that runs BOTH in the CPU rasterizer's GlslInterpreter (--frag) AND on
// desktop OpenGL (examples/poc_renderer). It implements the metallic-roughness
// subset of the UsdPreviewSurface reference BRDF (Pixar Storm previewSurface.glslfx
// / glTF 2.0 Appendix B): GGX(Trowbridge-Reitz) D + Schlick-GGX Smith G +
// Schlick F, alpha = roughness^2, F0 = mix(0.04, baseColor, metallic).
//
// Interpreter-legality (see GlslInterpreter.hpp envelope): uses ONLY the shared
// varying/uniform contract and the interpreter's builtin set. Deliberately avoids
// vector-relational builtins (lessThan/bvec3) — the sRGB encode uses the scalar
// pow() form, which is bit-portable to desktop GL too. Texture slots are guarded
// by uHas*Tex so an unbound sampler is never sampled.
//
// TEXTURE SLOTS (host binds these; interpreter seeds uHas*Tex + samplers):
//   uBaseColorTex          sRGB   base color / albedo
//   uNormalTex             linear tangent-space normal (xyz*2-1)
//   uEmissiveTex           sRGB   emissive
//   uMetallicRoughnessTex  linear glTF ORM: R=occlusion, G=roughness, B=metallic
//   uOcclusionTex          linear separate AO (R)

in vec3 vNormalEye;
in vec3 vPosEye;
in vec4 vColor;
in vec2 vTexCoord;

out vec4 FragColor;

// ---- Material params (UsdPreviewSurface metallic workflow) ------------------
uniform vec4  uBaseColor;         // rgb = diffuseColor, a = opacity
uniform float uMetallic;
uniform float uRoughness;
uniform vec3  uEmissive;
uniform float uNormalScale;
uniform float uOcclusionStrength;
uniform int   uAlphaMode;         // 0=Opaque, 1=Blend, 2=Mask
uniform float uAlphaCutoff;
uniform int   uHasColors;

// ---- UsdPreviewSurface fidelity params (X3D PhysicalMaterial cannot carry
// these; seedUniforms defaults them to the spec fallbacks, so the cpuraster
// direct path stays metallic-workflow/ior=1.5/no-clearcoat. The --emit-glsl
// path bakes real values here.) ----------------------------------------------
uniform int   uUseSpecularWorkflow; // 0=metallic (use uMetallic), 1=specular
uniform vec3  uSpecularColor;       // specular-workflow F0
uniform float uIor;                 // dielectric index of refraction (drives F0)
uniform float uClearcoat;           // weight of a second white specular lobe
uniform float uClearcoatRoughness;
uniform int   uOpacityMode;         // 0=transparent (diffuse-only), 1=presence
uniform float uOpacityThreshold;

// ---- Texture slots ----------------------------------------------------------
uniform int       uHasBaseColorTex;
uniform sampler2D uBaseColorTex;
uniform int       uHasNormalTex;
uniform sampler2D uNormalTex;
uniform int       uHasEmissiveTex;
uniform sampler2D uEmissiveTex;
uniform int       uHasMetallicRoughnessTex;
uniform sampler2D uMetallicRoughnessTex;
uniform int       uHasOcclusionTex;
uniform sampler2D uOcclusionTex;

// ---- Lights -----------------------------------------------------------------
uniform int  uNumLights;
uniform vec3 uLightDirEye[8];     // direction of travel, eye space
uniform vec3 uLightColor[8];      // rgb * intensity

const float PI = 3.14159265358979;

// GGX / Trowbridge-Reitz normal distribution.
float D_GGX(float NdotH, float alpha2) {
    float d = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * d * d);
}

// Smith-GGX correlated visibility (G / (4 NdotL NdotV)).
float V_Smith(float NdotL, float NdotV, float alpha2) {
    float gv = NdotL * sqrt(NdotV * NdotV * (1.0 - alpha2) + alpha2);
    float gl = NdotV * sqrt(NdotL * NdotL * (1.0 - alpha2) + alpha2);
    return 0.5 / max(gv + gl, 1e-5);
}

// Schlick Fresnel.
vec3 F_Schlick(float VdotH, vec3 F0) {
    float f = 1.0 - VdotH;
    float f2 = f * f;
    return F0 + (1.0 - F0) * (f2 * f2 * f);
}

// Scalar-safe sRGB encode (interpreter has no vector-relational builtins; the
// pow() approximation is bit-portable across both backends).
vec3 toSRGB(vec3 c) {
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main() {
    // ---- Base color ---------------------------------------------------------
    vec4 baseCol = uBaseColor;
    if (uHasColors != 0) baseCol.rgb = vColor.rgb;
    if (uHasBaseColorTex != 0) baseCol *= texture(uBaseColorTex, vTexCoord);
    float alpha = baseCol.a;
    if (uAlphaMode == 2 && alpha < uAlphaCutoff) discard;

    // ---- Metallic / roughness (glTF ORM packing) ----------------------------
    float metallic  = uMetallic;
    float roughness = uRoughness;
    if (uHasMetallicRoughnessTex != 0) {
        vec3 orm = texture(uMetallicRoughnessTex, vTexCoord).rgb;
        roughness = roughness * orm.g;
        metallic  = metallic * orm.b;
    }
    float alpha2 = max(roughness * roughness, 0.001);
    alpha2 = alpha2 * alpha2;

    // ---- Occlusion ----------------------------------------------------------
    float ao = 1.0;
    if (uHasMetallicRoughnessTex != 0) {
        ao = texture(uMetallicRoughnessTex, vTexCoord).r;
    } else if (uHasOcclusionTex != 0) {
        ao = texture(uOcclusionTex, vTexCoord).r;
    }
    ao = mix(1.0, ao, uOcclusionStrength);

    // ---- Geometric normal (two-sided) ---------------------------------------
    vec3 Ngeo = normalize(vNormalEye);
    if (!gl_FrontFacing) Ngeo = -Ngeo;

    // ---- Normal mapping (derivative TBN; dFdx exact for vPosEye/vTexCoord) ---
    vec3 N = Ngeo;
    if (uHasNormalTex != 0) {
        vec3 tsN = texture(uNormalTex, vTexCoord).rgb * 2.0 - 1.0;
        tsN.x = tsN.x * uNormalScale;
        tsN.y = tsN.y * uNormalScale;
        tsN = normalize(tsN);
        vec3 dPx = dFdx(vPosEye);
        vec3 dPy = dFdy(vPosEye);
        vec2 dUx = dFdx(vTexCoord);
        vec2 dUy = dFdy(vTexCoord);
        float det = dUx.x * dUy.y - dUx.y * dUy.x;
        if (abs(det) > 1e-6) {
            vec3 T = normalize((dPx * dUy.y - dPy * dUx.y) / det);
            T = normalize(T - dot(T, Ngeo) * Ngeo);
            vec3 B = cross(Ngeo, T);
            N = normalize(T * tsN.x + B * tsN.y + Ngeo * tsN.z);
        }
    }

    // ---- PBR setup: workflow-dependent F0 / diffuse -------------------------
    // At the spec-default uniforms (metallic workflow, ior 1.5, no clearcoat,
    // opacityMode transparent) every branch below collapses to the plain glTF
    // metallic-roughness path — i.e. bit-parity with the native fixed-function
    // PBR. The extra terms only engage when the emitter bakes real UsdPreviewSurface
    // values in.
    vec3 V = normalize(-vPosEye);
    float NdV = max(dot(N, V), 0.0);

    // Dielectric F0 from ior: ((1-ior)/(1+ior))^2; ior=1.5 -> 0.04.
    float rr = (1.0 - uIor) / (1.0 + uIor);
    float f0Dielectric = rr * rr;

    vec3 F0;
    vec3 diffColor;
    if (uUseSpecularWorkflow != 0) {
        F0 = uSpecularColor;               // specular workflow: specularColor IS F0
        diffColor = baseCol.rgb;
    } else {
        F0 = mix(vec3(f0Dielectric), baseCol.rgb, metallic);
        diffColor = (1.0 - metallic) * baseCol.rgb;
    }

    // opacityMode "transparent" (0) attenuates only the diffuse contribution.
    float opacity = alpha;
    if (uOpacityMode == 0) diffColor = diffColor * opacity;

    // ---- Emissive -----------------------------------------------------------
    vec3 emissive = uEmissive;
    if (uHasEmissiveTex != 0) emissive = emissive * texture(uEmissiveTex, vTexCoord).rgb;

    // Clearcoat: a second white dielectric (F0=0.04) GGX lobe over the base.
    float ccAlpha = max(uClearcoatRoughness * uClearcoatRoughness, 0.001);
    ccAlpha = ccAlpha * ccAlpha;

    // ---- Analytic GGX accumulation (directional lights) ---------------------
    vec3 color = emissive;
    for (int i = 0; i < uNumLights; ++i) {
        vec3 L = normalize(-uLightDirEye[i]);
        float NdL = max(dot(N, L), 0.0);
        if (NdL <= 0.0) continue;
        vec3 H = normalize(L + V);
        float NdH = max(dot(N, H), 0.0);
        float VdH = max(dot(V, H), 0.0);
        float D = D_GGX(NdH, alpha2);
        float Vis = V_Smith(NdL, NdV, alpha2);
        vec3 F = F_Schlick(VdH, F0);
        vec3 spec = D * Vis * F;
        vec3 kD = (uUseSpecularWorkflow != 0) ? (1.0 - F) : (1.0 - F) * (1.0 - metallic);
        vec3 lobe = kD * diffColor / PI + spec;
        // Clearcoat lobe (default weight 0 → no-op). Its Fresnel also attenuates
        // the base lobe underneath (energy-conserving layering).
        if (uClearcoat > 0.0) {
            float Dc = D_GGX(NdH, ccAlpha);
            float Vc = V_Smith(NdL, NdV, ccAlpha);
            vec3 Fc = F_Schlick(VdH, vec3(0.04));
            vec3 cc = Dc * Vc * Fc * uClearcoat;
            lobe = lobe * (1.0 - uClearcoat * Fc) + cc;
        }
        color += lobe * uLightColor[i] * NdL;
    }

    // ---- Ambient (AO-modulated), opacityMode "presence", sRGB out -----------
    color += 0.03 * diffColor * ao;
    if (uOpacityMode == 1) color = color * opacity; // presence scales everything
    FragColor = vec4(toSRGB(color), opacity);
}
