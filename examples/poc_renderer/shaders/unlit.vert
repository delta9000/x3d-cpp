#version 330 core
// unlit.vert — PoC B4 unlit vertex shader for the topology!=Triangles OR
// !hasNormals shading path (lines, points, and any normal-less mesh). Positions
// are in the geometry's LOCAL frame; uModel is the per-PATH world transform.
// No normal attribute / no light uniforms are consulted — this program is the
// explicit unlit selector the B4 consumer contract names. The per-vertex Color
// (aColor) is forwarded; the fragment shader picks vColor vs uBaseColor.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal; // only the §18.4.8 generators read it.
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord; // X3D LOCAL (bottom-left = GL); no flip.
// §18.4.3 MultiTexture: UV sets 1..3 (set 0 is aTexCoord); stages pick by channel.
layout(location = 5) in vec2 aTexCoord1;
layout(location = 6) in vec2 aTexCoord2;
layout(location = 7) in vec2 aTexCoord3;

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

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
// REQ-CLIP (§11.4.1): the item's enabled ClipPlanes in EYE space. The host
// enables GL_CLIP_DISTANCE0..n-1; a point with a*x + b*y + c*z + d < 0 is cut.
uniform int uNumClipPlanes;
uniform vec4 uClipPlane[6];
out float gl_ClipDistance[6];
// TXF-2/SEAM-LINEPOINT: §12.4.8 PointProperties. Point size = (A + B·d + C·d²)·
// scale clamped to [min,max] with d the eye-space distance; defaults (scale 1,
// atten (1,0,0), min=max=1) reproduce the historic fixed size.
uniform float uPointSizeScale;
uniform vec3  uPointAttenuation;
uniform float uPointSizeMin;
uniform float uPointSizeMax;

out vec4 vColor;
out vec2 vTexCoord;        // passed through un-flipped for the sampler.
out vec2 vTexSet[4];       // §18.4.3 UV sets for MultiTexture stages.
out vec3 vPosLocal;        // §18.4.8 generator inputs (texgen.glsl).
out vec3 vNormalLocal;
out vec3 vNormalEye;
out vec3 vPosEye;          // eye-space position (for the §17 fog distance).

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vTexSet[0] = aTexCoord;
    vTexSet[1] = aTexCoord1;
    vTexSet[2] = aTexCoord2;
    vTexSet[3] = aTexCoord3;
    vPosLocal = skinPosition(aPos);
    vNormalLocal = aNormal;
    vNormalEye = transpose(inverse(mat3(uView * uModel))) * aNormal;
    vec4 posEye = uView * uModel * vec4(vPosLocal, 1.0);
    vPosEye = posEye.xyz;
    float d = length(posEye.xyz);
    float size = (uPointAttenuation.x + uPointAttenuation.y * d +
                  uPointAttenuation.z * d * d) * uPointSizeScale;
    gl_PointSize = clamp(size, uPointSizeMin, uPointSizeMax);
    gl_Position = uProjection * posEye;
    for (int i = 0; i < 6; ++i)
        gl_ClipDistance[i] = i < uNumClipPlanes ? dot(uClipPlane[i], posEye) : 1.0;
}
