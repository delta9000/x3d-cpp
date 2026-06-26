#version 330 core
// author_lambert.frag — a sample "ComposedShader" fragment shader run by the CPU
// GLSL interpreter (GlslInterpreter.hpp) via `--frag`. It reads the host-seeded
// varyings (vNormalEye, vPosEye) and uniforms (uNumLights, uLightDirEye[],
// uLightColor[], uDiffuse) — the same names the PoC's lit.vert emits / the
// interpreter binds — and writes FragColor. Exercises for-loops, swizzles,
// builtins (normalize/dot/max/pow/mix) and a user-defined helper function.

in vec3 vNormalEye;
in vec3 vPosEye;
in vec4 vColor;
in vec2 vTexCoord;

uniform vec4 uDiffuse;        // rgb = diffuse, a = 1 - transparency
uniform int  uNumLights;
uniform vec3 uLightDirEye[8]; // direction of travel, eye space
uniform vec3 uLightColor[8];  // rgb * intensity

out vec4 FragColor;

vec3 toSRGB(vec3 c) {
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main() {
    vec3 N = normalize(vNormalEye);
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(-vPosEye);

    vec3 base = uDiffuse.rgb;
    vec3 lit = 0.15 * base; // ambient

    for (int i = 0; i < uNumLights; ++i) {
        vec3 L = normalize(-uLightDirEye[i]);
        float ndl = max(dot(N, L), 0.0);
        lit += base * uLightColor[i] * ndl;
    }

    // A cyan rim term so the interpreter path is visually distinct from the
    // built-in Phong port (proves the author shader actually executed).
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    lit = mix(lit, vec3(0.1, 0.9, 1.0), rim * 0.6);

    FragColor = vec4(toSRGB(lit), uDiffuse.a);
}
