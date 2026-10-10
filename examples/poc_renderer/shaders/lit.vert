#version 330 core
// lit.vert — PoC M3 (T12) lit vertex shader. Positions + normals are in the
// geometry's LOCAL frame; the model matrix is the per-PATH world transform from
// the extractor (column-major Mat4 uploaded directly via m.m.data()). MVP order
// is projection * view * model.
//
// We light in CAMERA (eye) space so the NavigationInfo headlight — a
// directional light fixed to the camera — is a trivial constant direction in
// the fragment shader, and world-space LightDesc directions are pre-transformed
// to eye space on the CPU (uLightDirEye). The normal matrix is the inverse-
// transpose of (view*model)'s upper-left 3x3 so non-uniform scale does not skew
// the shading normal.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord; // B8: X3D LOCAL (bottom-left = GL); no flip.

layout(location = 4) in uvec2 aSkinRange;
uniform bool uSkinEnabled;
uniform samplerBuffer uInfluences;
uniform samplerBuffer uPalette;

vec3 skinPosition(vec3 bindPos) {
    if (!uSkinEnabled || aSkinRange.y == 0u) return bindPos;
    vec3 result = vec3(0.0);
    for (uint k = 0u; k < aSkinRange.y; ++k) {
        vec2 influence = texelFetch(uInfluences, int(aSkinRange.x + k)).rg;
        int base = int(influence.x) * 7;
        mat4 matrix = mat4(texelFetch(uPalette, base), texelFetch(uPalette, base + 1),
                           texelFetch(uPalette, base + 2), texelFetch(uPalette, base + 3));
        result += influence.y * (matrix * vec4(bindPos, 1.0)).xyz;
    }
    return result;
}

vec3 skinNormal(vec3 bindNormal) {
    if (!uSkinEnabled || aSkinRange.y == 0u) return bindNormal;
    vec3 result = vec3(0.0);
    for (uint k = 0u; k < aSkinRange.y; ++k) {
        vec2 influence = texelFetch(uInfluences, int(aSkinRange.x + k)).rg;
        int base = int(influence.x) * 7 + 4;
        mat3 normal = mat3(texelFetch(uPalette, base).xyz,
                           texelFetch(uPalette, base + 1).xyz,
                           texelFetch(uPalette, base + 2).xyz);
        result += influence.y * normal * bindNormal;
    }
    return length(result) > 0.0 ? normalize(result) : bindNormal;
}

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
// REQ-CLIP (§11.4.1): the item's enabled ClipPlanes in EYE space. The host
// enables GL_CLIP_DISTANCE0..n-1; a point with a*x + b*y + c*z + d < 0 is cut.
uniform int uNumClipPlanes;
uniform vec4 uClipPlane[6];
out float gl_ClipDistance[6];
uniform mat3 uNormalMatrix; // inverse-transpose of (view*model) 3x3, eye space.
uniform float uPointSizeScale;
uniform vec3 uPointAttenuation;
uniform float uPointSizeMin;
uniform float uPointSizeMax;
// TXF-2: §18.4.8 TextureCoordinateGenerator mode. 0 = off (use the authored
// aTexCoord); 1 = SPHERE, 2 = CAMERASPACENORMAL, 3 = CAMERASPACEPOSITION
// (also COORD-EYE, which uses the same camera-space vertex coordinates),
// 4 = CAMERASPACEREFLECTIONVECTOR — the view-dependent modes, computed here from
// eye-space state (matches cpu_raster MaterialShader.hpp detail::texCoordGenUv).
uniform int  uTexCoordGenMode;

out vec3 vNormalEye;        // shading normal in eye space (not yet normalized).
out vec3 vPosEye;           // vertex position in eye space (for Blinn-Phong view dir).
out vec4 vColor;            // per-vertex Color (only consulted when uHasColors).
out vec2 vTexCoord;         // B8: passed through un-flipped for the sampler.

void main() {
    vec4 posEye = uView * uModel * vec4(skinPosition(aPos), 1.0);
    float d = length(posEye.xyz);
    gl_PointSize = clamp((uPointAttenuation.x + uPointAttenuation.y * d +
                          uPointAttenuation.z * d * d) * uPointSizeScale,
                         uPointSizeMin, uPointSizeMax);
    vNormalEye = uNormalMatrix * skinNormal(aNormal);
    vPosEye = posEye.xyz;
    vColor = aColor;
    vec3 Neye = normalize(vNormalEye);
    if (uTexCoordGenMode > 0) {
        if (uTexCoordGenMode == 1)       vTexCoord = Neye.xy * 0.5 + 0.5; // SPHERE
        else if (uTexCoordGenMode == 2)  vTexCoord = Neye.xy;             // CAMERASPACENORMAL
        else if (uTexCoordGenMode == 3)  vTexCoord = posEye.xy;           // CAMERASPACEPOSITION
        else {                                                            // REFLECTIONVECTOR
            // Table 18.6: E points from the position to the eye (the origin).
            vec3 E = normalize(-posEye.xyz);
            vec3 R = reflect(-E, Neye);       // = 2*dot(E,N)*N - E
            vTexCoord = R.xy;
        }
    } else {
        vTexCoord = aTexCoord;
    }
    gl_Position = uProjection * posEye;
    for (int i = 0; i < 6; ++i)
        gl_ClipDistance[i] = i < uNumClipPlanes ? dot(uClipPlane[i], posEye) : 1.0;
}
