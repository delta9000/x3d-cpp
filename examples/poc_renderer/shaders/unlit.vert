#version 330 core
// unlit.vert — PoC B4 unlit vertex shader for the topology!=Triangles OR
// !hasNormals shading path (lines, points, and any normal-less mesh). Positions
// are in the geometry's LOCAL frame; uModel is the per-PATH world transform.
// No normal attribute / no light uniforms are consulted — this program is the
// explicit unlit selector the B4 consumer contract names. The per-vertex Color
// (aColor) is forwarded; the fragment shader picks vColor vs uBaseColor.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal; // present in the shared layout; unused here.
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord; // X3D LOCAL (bottom-left = GL); no flip.

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
// TXF-2/SEAM-LINEPOINT: §12.4.8 PointProperties. Point size = (A + B·d + C·d²)·
// scale clamped to [min,max] with d the eye-space distance; defaults (scale 1,
// atten (1,0,0), min=max=1) reproduce the historic fixed size.
uniform float uPointSizeScale;
uniform vec3  uPointAttenuation;
uniform float uPointSizeMin;
uniform float uPointSizeMax;

out vec4 vColor;
out vec2 vTexCoord;        // passed through un-flipped for the sampler.
out vec3 vPosEye;          // eye-space position (for the §17 fog distance).

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vec4 posEye = uView * uModel * vec4(aPos, 1.0);
    vPosEye = posEye.xyz;
    float d = length(posEye.xyz);
    float size = (uPointAttenuation.x + uPointAttenuation.y * d +
                  uPointAttenuation.z * d * d) * uPointSizeScale;
    gl_PointSize = clamp(size, uPointSizeMin, uPointSizeMax);
    gl_Position = uProjection * posEye;
}
