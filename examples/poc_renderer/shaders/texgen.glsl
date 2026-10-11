// texgen.glsl — §18.4.8 TextureCoordinateGenerator (Table 18.6) and the
// per-texture coordinate mappings, evaluated per fragment; shared by lit.frag,
// pbr.frag and unlit.frag (main.cpp expands the #include line, before
// multitexture.glsl). Mirrors examples/cpu_raster/cpuraster/MaterialShader.hpp
// detail::textureCoordinates and detail::texCoordGenUv, including the
// deterministic Perlin noise and the generated-coordinate TextureTransform.
//
// Every texture the programs sample has its own mapping (REQ-MULTITEXTURE-2):
// mapping k < 5 is the material slot on texture unit k (base colour, normal,
// emissive, specular or metallic-roughness, occlusion); mapping 5 + i is
// MultiTexture stage i. A mapping samples UV set uMapInfo.x (clamped to the
// last set, as cpu_raster does; the host repeats the last authored set into
// the unused attributes) unless it has a generator. Generator modes are
// TexCoordGenMode + 1 (0 = none):
//  1 SPHERE  2 CAMERASPACENORMAL  3 CAMERASPACEPOSITION
//  4 CAMERASPACEREFLECTIONVECTOR  5 SPHERE-LOCAL  6 COORD  7 COORD-EYE
//  8 NOISE  9 NOISE-EYE  10 SPHERE-REFLECT  11 SPHERE-REFLECT-LOCAL
// The vertex shaders still write vTexCoord from the material's first generator
// for modes 1-4 and 7 (the fallback a fragment shader without this file uses).

// The host prepends "#define POC_MAX_STAGES 4" when the context has too few
// texture image units for eight stages (units 8-11 and 16-19).
#ifndef POC_MAX_STAGES
#define POC_MAX_STAGES 8
#endif
const int kMaxUvSets = 8;
const int kMaxMappings = 5 + POC_MAX_STAGES;

in vec4 vUvPairs[kMaxUvSets / 2]; // UV sets 0..7, two per vector (transforms baked).

uniform ivec3 uMapInfo[kMaxMappings];  // (UV set, generator mode, parameter count).
uniform vec3  uMapParamA[kMaxMappings]; // generator parameter[0..2].
uniform vec3  uMapParamB[kMaxMappings]; // generator parameter[3..5].
uniform mat3  uMapTransform[kMaxMappings]; // generated-coordinate TextureTransform.

vec2 uvSet(int c) {
    c = clamp(c, 0, kMaxUvSets - 1);
    vec4 p = vUvPairs[c / 2];
    return (c % 2) == 0 ? p.xy : p.zw;
}

in vec3 vPosLocal;    // pre-transform (skinned) vertex position.
in vec3 vNormalLocal; // pre-transform normal.

float texGenParameter(int k, int i, float fallback) {
    if (i >= uMapInfo[k].z) return fallback;
    return i < 3 ? uMapParamA[k][i] : uMapParamB[k][i - 3];
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

// The full (s, t, r) vector; 2D textures use (s, t), §34.2.2 environment
// textures look up the direction (s, t, r).
vec3 generatedVec(int k, int mode, vec3 posEye, vec3 normalEye) {
    vec3 n = normalize(normalEye);
    vec3 nl = normalize(vNormalLocal);
    if (!gl_FrontFacing) { n = -n; nl = -nl; }
    if (mode == 1) return n * 0.5 + 0.5;
    if (mode == 2) return n;
    if (mode == 3 || mode == 7) return posEye;
    if (mode == 4) return reflect(normalize(posEye), n); // E points to the eye.
    if (mode == 5) return nl * 0.5 + 0.5;
    if (mode == 6) return vPosLocal;
    if (mode == 8 || mode == 9) {
        vec3 p = (mode == 8 ? vPosLocal : posEye) *
                     vec3(texGenParameter(k, 0, 1.0), texGenParameter(k, 1, 1.0),
                          texGenParameter(k, 2, 1.0)) +
                 vec3(texGenParameter(k, 3, 0.0), texGenParameter(k, 4, 0.0),
                      texGenParameter(k, 5, 0.0));
        return vec3(perlinNoise(p, 0u), perlinNoise(p, 1013u), perlinNoise(p, 2026u));
    }
    if (mode == 10 || mode == 11) {
        bool local = mode == 11;
        vec3 eye = vec3(texGenParameter(k, 1, 0.0), texGenParameter(k, 2, 0.0),
                        texGenParameter(k, 3, 0.0));
        vec3 incident = normalize(local ? vPosLocal - eye : posEye);
        vec3 normal = -(local ? nl : n);
        float eta = texGenParameter(k, 0, 1.0);
        float d = dot(normal, incident);
        float k = 1.0 - eta * eta * (1.0 - d * d);
        return k < 0.0 ? vec3(0.0) : incident * eta - normal * (eta * d + sqrt(k));
    }
    return n * 0.5 + 0.5; // SPHERE, the default.
}

// §34.2.2: the direction an environment (cube map) texture is sampled with:
// its mapping's generator (s, t, r), or with no generator the camera-space
// reflection vector. Generated-coordinate TextureTransforms are 2D and do not
// apply. The cube is fixed to the geometry's local frame (ADR-0060): eye-space
// modes (CAMERASPACENORMAL/POSITION/REFLECTIONVECTOR, COORD-EYE, NOISE-EYE,
// SPHERE-REFLECT) are carried there by uEyeToLocal, the inverse of the upper
// 3x3 of view * model; local modes and the biased SPHERE are used as is.
uniform mat3 uEyeToLocal;
vec3 envDirection(int k, vec3 posEye, vec3 normalEye) {
    int mode = uMapInfo[k].y > 0 ? uMapInfo[k].y : 4;
    vec3 v = generatedVec(k, mode, posEye, normalEye);
    bool eyeSpace = mode == 2 || mode == 3 || mode == 4 || mode == 7 ||
                    mode == 9 || mode == 10;
    return eyeSpace ? uEyeToLocal * v : v;
}

// The coordinates mapping k samples.
vec2 mapUv(int k, vec3 posEye, vec3 normalEye) {
    int mode = uMapInfo[k].y;
    if (mode <= 0) return uvSet(uMapInfo[k].x);
    vec3 t = uMapTransform[k] * vec3(generatedVec(k, mode, posEye, normalEye).xy, 1.0);
    return t.xy / (t.z != 0.0 ? t.z : 1.0);
}
