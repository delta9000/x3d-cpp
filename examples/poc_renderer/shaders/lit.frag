#version 330 core
// lit.frag — PoC Phong fragment shader: Blinn-Phong + per-slot texture maps
// + optional tangent-space normal mapping + sRGB gamma output.
//
// LIGHTING MODEL:
//   Up to kMaxLights directional lights in eye space. Two-sided via
//   gl_FrontFacing. The NavigationInfo headlight (§23.4.4) is an additional
//   camera-space light whenever headlight=TRUE (default), independent of the
//   scene's lights.
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
// COLOUR SPACE (ADR-0027):
//   Phong shades in display space: colour textures are uploaded without sRGB
//   decode and main.cpp sets uGammaOutput = 0, so authored X3D 3.x colours keep
//   their look and match UnlitMaterial. The uGammaOutput switch remains for
//   callers that want the linear workflow PhysicalMaterial uses.

out vec4 FragColor;

const int kMaxLights = 8;

in vec3 vNormalEye;
in vec3 vPosEye;
in vec4 vColor;
in vec2 vTexCoord;

#include "multitexture.glsl"
#include "texgen.glsl"

uniform vec4 uDiffuse;       // rgb = diffuse/base, a = 1 - transparency.
uniform vec3 uEmissive;      // added unlit (augmented by emissive texture).
uniform vec3 uAmbientColor;  // material ambientIntensity (broadcast); §17 multiplies it by base.
uniform int  uHasColors;     // 1 => per-vertex vColor overrides uDiffuse.rgb.

uniform int uFillMode;       // bit 1: fill; bit 2: hatch (polygons only).
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
uniform int   uAlphaMode;    // 0=Opaque, 1=Mask, 2=Blend (matches ex::AlphaMode).
uniform float uAlphaCutoff;  // Mask threshold.

// ---- Lights -----------------------------------------------------------------
uniform int  uNumLights;
uniform vec3 uLightDirEye[kMaxLights]; // direction of TRAVEL, eye space.
uniform vec3 uLightColor[kMaxLights];  // rgb * intensity, premultiplied.
uniform float uLightAmbient[kMaxLights]; // §17.2.2.4 per-light ambientIntensity.
uniform int uLightType[kMaxLights]; // 0 directional, 1 point, 2 spot.
uniform vec3 uLightPosEye[kMaxLights];
uniform vec3 uLightAttenuation[kMaxLights];
uniform float uLightRadius[kMaxLights];
uniform vec2 uLightCone[kMaxLights]; // beamWidth, cutOffAngle in radians.

#include "shadow.glsl"
#include "projector.glsl"

bool resolveLight(int i, vec3 posEye, out vec3 L, out float atten) {
    if (uLightType[i] == 0) {
        L = normalize(-uLightDirEye[i]);
        atten = shadowVisibility(i, posEye);
        return true;
    }
    vec3 toLight = uLightPosEye[i] - posEye;
    float dist = length(toLight);
    if (dist > uLightRadius[i]) return false;
    L = dist > 1e-6 ? toLight / dist : vec3(0.0, 0.0, 1.0);
    vec3 a = uLightAttenuation[i];
    atten = 1.0 / max(a.x + a.y * dist + a.z * dist * dist, 1.0);
    if (uLightType[i] == 2) {
        float ang = acos(clamp(dot(normalize(uLightDirEye[i]), -L), -1.0, 1.0));
        if (ang >= uLightCone[i].y) return false;
        if (ang > uLightCone[i].x && uLightCone[i].y > uLightCone[i].x)
            atten *= (uLightCone[i].y - ang) / (uLightCone[i].y - uLightCone[i].x);
    }
    atten *= shadowVisibility(i, posEye);
    return true;
}


// ---- Output -----------------------------------------------------------------
uniform int uGammaOutput; // 1 => apply LINEARtoSRGB before writing FragColor.

// ---- Fog (§24.4.2 / §17 Table 17.5) -----------------------------------------
// visibilityRange is already world-scaled by the extractor; 0 disables fog.
uniform vec3  uFogColor;
uniform int   uFogType;            // 0 = LINEAR, 1 = EXPONENTIAL.
uniform float uFogVisibilityRange;

// sRGB gamma encoding (approx pow(1/2.2) via piecewise; cleaner than raw pow).
vec3 linearToSRGB(vec3 lin) {
    bvec3 cutoff = lessThan(lin, vec3(0.0031308));
    vec3 lower   = lin * 12.92;
    vec3 upper   = pow(clamp(lin, 0.0, 1.0), vec3(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(upper, lower, cutoff);
}

// §17 fog: fogInterpolant(d) then blend with the fog colour. d = eye-space
// distance to the viewer, V = visibilityRange. LINEAR f = (V-d)/V (d<V else 0);
// EXPONENTIAL f = exp(-d/(V-d)) (d<V else 0). result = f*color + (1-f)*fogColor.
vec3 applyFog(vec3 color, float d) {
    float V = uFogVisibilityRange;
    if (V <= 0.0) return color;       // fog disabled.
    float f = 0.0;                    // d >= V => fully fogged.
    if (d < V)
        f = (uFogType == 1) ? exp(-d / (V - d)) : (V - d) / V;
    return color * f + uFogColor * (1.0 - f);
}

void main() {
    // §18.4.8: every texture slot samples the generated coordinates, if any.
    vec2 uv = texGenUv(vTexCoord, vPosEye, vNormalEye);
    bool hatch = (uFillMode & 2) != 0 && hatchPixel();
    if ((uFillMode & 1) == 0 && !hatch) discard;
    // ---- Base color from diffuse slot ± per-vertex Color -------------------
    vec3 base  = (uHasColors != 0) ? vColor.rgb : uDiffuse.rgb;
    float alpha = uDiffuse.a;
    if (uGlyphAtlas != 0) {
        // Font atlas: single-channel coverage in .r. Keep the material color and
        // alpha-test on coverage (no blending in the PoC's opaque depth pass).
        if (texture(uTexture, uv).r < 0.5) discard;
    } else if (uNumStages > 0) {
        // §18.4.3 MultiTexture: DIFFUSE is the material/vertex diffuse colour.
        vec4 c = applyMultiTexture(vec4(base, alpha), base, uSpecular, uv,
                                   envDirection(vPosEye, vNormalEye));
        base  = c.rgb;
        alpha = c.a;
    } else if (uHasTexture != 0) {
        vec4 texel = texture(uTexture, uv);
        base  *= texel.rgb;
        alpha *= texel.a;
    }

    // ---- Alpha cut / blend gate --------------------------------------------
    if (uAlphaMode == 1 && alpha < uAlphaCutoff) discard; // MASK (== ex::AlphaMode::Mask).

    // ---- Geometric normal in eye space (two-sided) -------------------------
    vec3 Ngeo = normalize(vNormalEye);
    if (!gl_FrontFacing) Ngeo = -Ngeo;

    // ---- Normal mapping (tangent-space, screen-space TBN approximation) ----
    vec3 N = Ngeo;
    if (uHasNormalTex != 0) {
        // Decode tangent-space normal: [0,1]^3 -> [-1,1]^3.
        vec3 tsN = texture(uNormalTex, uv).rgb * 2.0 - 1.0;
        tsN.xy  *= uNormalScale;
        tsN = normalize(tsN);

        // Build a TBN from derivatives of position and the geometric normal.
        // This is the "derivative TBN" (no precomputed tangent attribute needed).
        vec3 dPdx  = dFdx(vPosEye);
        vec3 dPdy  = dFdy(vPosEye);
        vec2 dUVdx = dFdx(uv);
        vec2 dUVdy = dFdy(uv);
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
        emissive *= texture(uEmissiveTex, uv).rgb;

    // ---- Specular color: material specular ± specular texture ---------------
    vec3 specCol = uSpecular;
    if (uHasSpecularTex != 0)
        specCol *= texture(uSpecularTex, uv).rgb;

    // ---- Lighting accumulation (Blinn-Phong) --------------------------------
    vec3 V       = normalize(-vPosEye);
    float expo   = max(uShininess * 128.0, 1.0);
    vec3 lit     = emissive;

    for (int i = 0; i < uNumLights && i < kMaxLights; ++i) {
        // §17 ambient: light.ambientIntensity × ambientParameter, with
        // ambientParameter = material ambientIntensity × diffuseParameter (the
        // textured/vertex-coloured base) — linear in diffuse (ADR-0027).
        // Normal-independent, applied before ndl.
        vec3 L;
        float atten;
        vec3 projected;
        if (!resolveLight(i, vPosEye, L, atten) || !projectorColor(i, vPosEye, projected))
            continue;
        vec3 lightColor = uLightColor[i] * atten * projected;
        lit += (uAmbientColor * base) * lightColor * uLightAmbient[i];
        float ndl = max(dot(N, L), 0.0);
        lit      += base * lightColor * ndl;
        if (ndl > 0.0) {
            vec3  H    = normalize(L + V);
            float ndh  = max(dot(N, H), 0.0);
            lit       += specCol * lightColor * pow(ndh, expo);
        }
    }

    // ---- sRGB output encoding (Phase 5.5) -----------------------------------
    if (uGammaOutput != 0)
        lit = linearToSRGB(lit);

    // §17: fog is the final step, applied to the output (display) colour.
    lit = applyFog(hatch ? uHatchColor : lit, length(vPosEye));

    FragColor = vec4(lit, alpha);
}
