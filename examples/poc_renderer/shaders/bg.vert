#version 330 core
// bg.vert — fullscreen triangle (no VBO) covering the whole viewport, so
// bg.frag can paint every pixel of the §Background sky/ground gradient
// before scene geometry draws. gl_VertexID selects one of 3 over-sized
// corners; the two beyond [-1,1] get clipped away, leaving exactly the
// viewport. NDC z is irrelevant — the pass runs with depth test disabled.
out vec2 vNdc;

void main() {
    vec2 corners[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vNdc = corners[gl_VertexID];
    gl_Position = vec4(vNdc, 1.0, 1.0);
}
