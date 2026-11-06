#version 330 core
// Constant material color without lighting.
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
