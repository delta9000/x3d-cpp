#version 330 core
// lit.frag — PoC Phong fragment shader: Blinn-Phong + per-slot texture maps
// + optional tangent-space normal mapping + sRGB gamma output.
//
// LIGHTING MODEL:
//   Up to kMaxLights directional lights in eye space. Two-sided via
//   gl_FrontFacing. NavigationInfo headlight is light[0] when no scene lights.
//
// TEXTURE SLOTS (units match the PoC bind order in main.cpp):
//   Unit 0 — diffuse / base-color (uTexture / uHasTexture)
//   Unit 1 — normal map (uNormalTex / uHasNormalTex)      — tangent-space RGB
//   Unit 2 — emissive texture (uEmissiveTex / uHasEmissiveTex)
//   Unit 3 — specular texture (uSpecularTex / uHasSpecularTex)
//
// NORMAL MAPPING: when uHasNormalTex != 0 the tangent-space normal from the
// map is decoded ([0,1]^3 -> [-1,1]^3), scaled by uNormalScale, and transformed
// into eye space via a TBN built from the geometric normal and a synthesized
// tangent frame (Gram-Schmidt from the screen-space derivatives of the position).
// This is a SCREEN-SPACE approximation (no pre-computed tangents in the vertex
// buffer); it suffices for the PoC and avoids an attribute-layout change.
//
// GAMMA / sRGB OUTPUT (Phase 5.5):
//   When uGammaOutput != 0 the final linear color is converted to sRGB before
//   writing FragColor. Phong enables this to match PBR output expectations.
//   The pow(3) approximation is fine for a PoC.

out vec4 FragColor;

const int kMaxLights = 8;

in vec3 vNormalEye;
in vec3 vPosEye;
in vec4 vColor;
in vec2 vTexCoord;

uniform vec4 uDiffuse;       // rgb = diffuse/base, a = 1 - transparency.
uniform vec3 uEmissive;      // added unlit (augmented by emissive texture).
uniform vec3 uAmbientColor;  // diffuse * ambientIntensity.
uniform int  uHasColors;     // 1 => per-vertex vColor overrides uDiffuse.rgb.

// ---- Texture slots (all optional — shader guards on Has* flags) -------------
uniform int       uHasTexture;   // 0: diffuse slot absent.
uniform sampler2D uTexture;      // unit 0: diffuse / base-color (or glyph atlas).
uniform int       uGlyphAtlas;   // 1: uTexture is a font coverage atlas (.r = alpha).

uniform int       uHasNormalTex; // 0: no normal map.
uniform sampler2D uNormalTex;    // unit 1: tangent-space normal map.
uniform float     uNormalScale;  // scales the XY perturbation (default 1.0).

uniform int       uHasEmissiveTex; // 0: no emissive texture.
uniform sampler2D uEmissiveTex;    // unit 2: emissive texture.

uniform int       uHasSpecularTex; // 0: no specular texture.
uniform sampler2D uSpecularTex;    // unit 3: specular texture.

// ---- Material params --------------------------------------------------------
uniform vec3  uSpecular;     // X3D specularColor (Blinn-Phong tint).
uniform float uShininess;    // X3D shininess [0,1] — exponent = *128.
uniform int   uAlphaMode;    // 0=Opaque, 1=Blend, 2=Mask.
uniform float uAlphaCutoff;  // Mask threshold.

// ---- Lights -----------------------------------------------------------------
uniform int  uNumLights;
uniform vec3 uLightDirEye[kMaxLights]; // direction of TRAVEL, eye space.
uniform vec3 uLightColor[kMaxLights];  // rgb * intensity, premultiplied.

// ---- Output -----------------------------------------------------------------
uniform int uGammaOutput; // 1 => apply LINEARtoSRGB before writing FragColor.

// sRGB gamma encoding (approx pow(1/2.2) via piecewise; cleaner than raw pow).
vec3 linearToSRGB(vec3 lin) {
    bvec3 cutoff = lessThan(lin, vec3(0.0031308));
    vec3 lower   = lin * 12.92;
    vec3 upper   = pow(clamp(lin, 0.0, 1.0), vec3(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(upper, lower, cutoff);
}

void main() {
    // ---- Base color from diffuse slot ± per-vertex Color -------------------
    vec3 base  = (uHasColors != 0) ? vColor.rgb : uDiffuse.rgb;
    float alpha = uDiffuse.a;
    if (uGlyphAtlas != 0) {
        // Font atlas: single-channel coverage in .r. Keep the material color and
        // alpha-test on coverage (no blending in the PoC's opaque depth pass).
        if (texture(uTexture, vTexCoord).r < 0.5) discard;
    } else if (uHasTexture != 0) {
        vec4 texel = texture(uTexture, vTexCoord);
        base  *= texel.rgb;
        alpha *= texel.a;
    }

    // ---- Alpha cut / blend gate --------------------------------------------
    if (uAlphaMode == 2 && alpha < uAlphaCutoff) discard; // MASK.

    // ---- Geometric normal in eye space (two-sided) -------------------------
    vec3 Ngeo = normalize(vNormalEye);
    if (!gl_FrontFacing) Ngeo = -Ngeo;

    // ---- Normal mapping (tangent-space, screen-space TBN approximation) ----
    vec3 N = Ngeo;
    if (uHasNormalTex != 0) {
        // Decode tangent-space normal: [0,1]^3 -> [-1,1]^3.
        vec3 tsN = texture(uNormalTex, vTexCoord).rgb * 2.0 - 1.0;
        tsN.xy  *= uNormalScale;
        tsN = normalize(tsN);

        // Build a TBN from derivatives of position and the geometric normal.
        // This is the "derivative TBN" (no precomputed tangent attribute needed).
        vec3 dPdx  = dFdx(vPosEye);
        vec3 dPdy  = dFdy(vPosEye);
        vec2 dUVdx = dFdx(vTexCoord);
        vec2 dUVdy = dFdy(vTexCoord);
        float det  = dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x;
        if (abs(det) > 1e-6) {
            vec3 T = normalize((dPdx * dUVdy.y - dPdy * dUVdx.y) / det);
            // Re-orthogonalise T against Ngeo (Gram-Schmidt).
            T = normalize(T - dot(T, Ngeo) * Ngeo);
            vec3 B = cross(Ngeo, T);
            // Transform tsN into eye space.
            N = normalize(T * tsN.x + B * tsN.y + Ngeo * tsN.z);
        }
    }

    // ---- Emissive: material emissive modulated by emissive texture ----------
    vec3 emissive = uEmissive;
    if (uHasEmissiveTex != 0)
        emissive *= texture(uEmissiveTex, vTexCoord).rgb;

    // ---- Specular color: material specular ± specular texture ---------------
    vec3 specCol = uSpecular;
    if (uHasSpecularTex != 0)
        specCol *= texture(uSpecularTex, vTexCoord).rgb;

    // ---- Lighting accumulation (Blinn-Phong) --------------------------------
    vec3 V       = normalize(-vPosEye);
    float expo   = max(uShininess * 128.0, 1.0);
    vec3 lit     = uAmbientColor * base + emissive;

    for (int i = 0; i < uNumLights && i < kMaxLights; ++i) {
        vec3 L    = normalize(-uLightDirEye[i]);
        float ndl = max(dot(N, L), 0.0);
        lit      += base * uLightColor[i] * ndl;
        if (ndl > 0.0) {
            vec3  H    = normalize(L + V);
            float ndh  = max(dot(N, H), 0.0);
            lit       += specCol * uLightColor[i] * pow(ndh, expo);
        }
    }

    // ---- sRGB output encoding (Phase 5.5) -----------------------------------
    if (uGammaOutput != 0)
        lit = linearToSRGB(lit);

    FragColor = vec4(lit, alpha);
}
