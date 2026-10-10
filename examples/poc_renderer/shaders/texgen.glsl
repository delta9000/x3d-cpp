// texgen.glsl — §18.4.8 TextureCoordinateGenerator (Table 18.6), evaluated
// per fragment; shared by lit.frag, pbr.frag and unlit.frag (main.cpp expands
// the #include line). Mirrors examples/cpu_raster/cpuraster/MaterialShader.hpp
// detail::texCoordGenUv, including its deterministic Perlin noise and the
// generated-coordinate TextureTransform.
//
// uTexCoordGenMode is TexCoordGenMode + 1 (0 = no generator, use vTexCoord):
//  1 SPHERE  2 CAMERASPACENORMAL  3 CAMERASPACEPOSITION
//  4 CAMERASPACEREFLECTIONVECTOR  5 SPHERE-LOCAL  6 COORD  7 COORD-EYE
//  8 NOISE  9 NOISE-EYE  10 SPHERE-REFLECT  11 SPHERE-REFLECT-LOCAL
// The vertex shaders still write vTexCoord for modes 1-4 and 7 (the fallback a
// fragment shader without this file uses); this file recomputes every mode.

in vec3 vPosLocal;    // pre-transform (skinned) vertex position.
in vec3 vNormalLocal; // pre-transform normal.

uniform int   uTexCoordGenMode;
uniform float uTexGenParam[6];
uniform int   uTexGenParamCount;
uniform mat3  uTexGenTransform; // generated-coordinate TextureTransform.

float texGenParameter(int i, float fallback) {
    return i < uTexGenParamCount ? uTexGenParam[i] : fallback;
}

// Deterministic 3D gradient noise with Perlin's quintic interpolation; the
// same lattice hash and gradients as cpu_raster detail::perlinNoise.
float texGenGrad(int a, int b, int c, float dx, float dy, float dz, uint seed) {
    uint h = (uint(a) & 255u) * 374761393u ^ (uint(b) & 255u) * 668265263u ^
             (uint(c) & 255u) * 2246822519u ^ seed;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = (h ^ (h >> 16)) & 15u;
    float u = h < 8u ? dx : dy;
    float v = h < 4u ? dy : ((h == 12u || h == 14u) ? dx : dz);
    return (((h & 1u) != 0u) ? -u : u) + (((h & 2u) != 0u) ? -v : v);
}

float texGenFade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

float perlinNoise(vec3 p, uint seed) {
    int ix = int(floor(mod(p.x, 256.0)));
    int iy = int(floor(mod(p.y, 256.0)));
    int iz = int(floor(mod(p.z, 256.0)));
    vec3 f = p - floor(p);
    float a[2];
    for (int k = 0; k < 2; ++k) {
        float b[2];
        for (int j = 0; j < 2; ++j)
            b[j] = mix(texGenGrad(ix, iy + j, iz + k, f.x, f.y - float(j), f.z - float(k), seed),
                       texGenGrad(ix + 1, iy + j, iz + k, f.x - 1.0, f.y - float(j),
                                  f.z - float(k), seed),
                       texGenFade(f.x));
        a[k] = mix(b[0], b[1], texGenFade(f.y));
    }
    return mix(a[0], a[1], texGenFade(f.z));
}

vec2 generatedUv(int mode, vec3 posEye, vec3 normalEye) {
    vec3 n = normalize(normalEye);
    vec3 nl = normalize(vNormalLocal);
    if (!gl_FrontFacing) { n = -n; nl = -nl; }
    if (mode == 1) return n.xy * 0.5 + 0.5;
    if (mode == 2) return n.xy;
    if (mode == 3 || mode == 7) return posEye.xy;
    if (mode == 4) return reflect(normalize(posEye), n).xy; // E points to the eye.
    if (mode == 5) return nl.xy * 0.5 + 0.5;
    if (mode == 6) return vPosLocal.xy;
    if (mode == 8 || mode == 9) {
        vec3 p = (mode == 8 ? vPosLocal : posEye) *
                     vec3(texGenParameter(0, 1.0), texGenParameter(1, 1.0),
                          texGenParameter(2, 1.0)) +
                 vec3(texGenParameter(3, 0.0), texGenParameter(4, 0.0),
                      texGenParameter(5, 0.0));
        return vec2(perlinNoise(p, 0u), perlinNoise(p, 1013u));
    }
    if (mode == 10 || mode == 11) {
        bool local = mode == 11;
        vec3 eye = vec3(texGenParameter(1, 0.0), texGenParameter(2, 0.0),
                        texGenParameter(3, 0.0));
        vec3 incident = normalize(local ? vPosLocal - eye : posEye);
        vec3 normal = -(local ? nl : n);
        float eta = texGenParameter(0, 1.0);
        float d = dot(normal, incident);
        float k = 1.0 - eta * eta * (1.0 - d * d);
        return k < 0.0 ? vec2(0.0) : (incident * eta - normal * (eta * d + sqrt(k))).xy;
    }
    return n.xy * 0.5 + 0.5; // SPHERE, the default.
}

// The coordinates the material's texture slots sample.
vec2 texGenUv(vec2 authored, vec3 posEye, vec3 normalEye) {
    if (uTexCoordGenMode <= 0) return authored;
    vec3 t = uTexGenTransform * vec3(generatedUv(uTexCoordGenMode, posEye, normalEye), 1.0);
    return t.xy / (t.z != 0.0 ? t.z : 1.0);
}
