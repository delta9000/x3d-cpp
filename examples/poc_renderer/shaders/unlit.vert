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

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_PointSize = 3.0; // PoC: visible points for PointSet (GL_PROGRAM_POINT_SIZE).
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
