#version 330 core
// flat.frag — PoC M1 unlit-flat fragment shader. One constant color from
// MaterialDesc.toRGBA() (the T7a Unlit-white stub at this stage). Lighting and
// per-vertex Color land at M3 (T12).
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
