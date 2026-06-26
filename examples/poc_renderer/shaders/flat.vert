#version 330 core
// flat.vert — PoC M1 unlit-flat vertex shader. Positions are in the geometry's
// LOCAL frame; the model matrix is the per-PATH world transform from the
// extractor (column-major Mat4 uploaded directly via m.m.data()). MVP order is
// projection * view * model.
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
