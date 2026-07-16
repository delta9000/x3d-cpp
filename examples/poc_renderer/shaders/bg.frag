#version 330 core
// bg.frag — X3D Background sky/ground gradient (§Background, issue #39),
// painted as an infinite dome behind the scene. Each pixel's view ray is
// unprojected to a world direction (mirrors the perspective() matrix built
// on the C++ side: xn/proj[0][0], yn/proj[1][1], -1, then rotated by the
// inverse view). The angle from +Y (0 = zenith, pi = nadir) selects the sky
// band; groundColor only covers [0, last groundAngle] measured from the
// nadir and does NOT extend past it — beyond that (when the last
// groundAngle < pi/2) the sky shows through, per the NIST
// verify_lastground.x3d conformance case.
//
// This is an independent reimplementation of examples/cpu_raster's
// skyGroundColor()/colorRamp() (same §Background semantics, no shared code —
// each renderer backend owns its own interpretation of the seam's
// BackgroundDesc; that independence is the point of the Consumer/RenderDelta
// seam-genericity proof).
in vec2 vNdc;
out vec4 FragColor;

uniform mat4 uInvView;
uniform float uInvP0; // 1 / proj[0][0]
uniform float uInvP5; // 1 / proj[1][1]

const int kMaxBands = 8;
uniform vec3  uSkyColor[kMaxBands];
uniform float uSkyAngle[kMaxBands - 1];
uniform int   uSkyColorCount;
uniform int   uSkyAngleCount;
uniform vec3  uGroundColor[kMaxBands];
uniform float uGroundAngle[kMaxBands - 1];
uniform int   uGroundColorCount;
uniform int   uGroundAngleCount;

const float kPi = 3.14159265358979323846;

vec3 colorRamp(float ang, vec3 cols[kMaxBands], int colorCount,
               float angs[kMaxBands - 1], int angleCount) {
    if (colorCount <= 1 || angleCount <= 0) return cols[0];
    if (ang <= angs[0]) {
        float t = angs[0] > 1e-6 ? ang / angs[0] : 0.0;
        return mix(cols[0], cols[1], t);
    }
    for (int i = 1; i < kMaxBands - 1; ++i) {
        if (i >= angleCount || i + 1 >= colorCount) break;
        if (ang <= angs[i]) {
            float span = angs[i] - angs[i - 1];
            float t = span > 1e-6 ? (ang - angs[i - 1]) / span : 0.0;
            return mix(cols[i], cols[i + 1], t);
        }
    }
    return cols[colorCount - 1];
}

void main() {
    vec3 dirView = vec3(vNdc.x * uInvP0, vNdc.y * uInvP5, -1.0);
    vec3 dir = normalize((uInvView * vec4(dirView, 0.0)).xyz);
    float ang = acos(clamp(dir.y, -1.0, 1.0));

    vec3 result = uSkyColorCount > 0
        ? colorRamp(ang, uSkyColor, uSkyColorCount, uSkyAngle, uSkyAngleCount)
        : vec3(0.0);

    if (uGroundColorCount > 0) {
        float fromNadir = kPi - ang;
        float maxGround = uGroundAngleCount > 0 ? uGroundAngle[uGroundAngleCount - 1] : 0.0;
        if (fromNadir <= maxGround) {
            result = colorRamp(fromNadir, uGroundColor, uGroundColorCount,
                                uGroundAngle, uGroundAngleCount);
        }
    }
    FragColor = vec4(result, 1.0);
}
