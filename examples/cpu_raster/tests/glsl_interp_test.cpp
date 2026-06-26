// glsl_interp_test.cpp — the GLSL-subset interpreter: parsing, expressions,
// swizzles, control flow, builtins, user functions, discard, and host binding of
// varyings/uniforms (the author ComposedShader path executed on the CPU).
#include "RenderItem.hpp"
#include "cpuraster/GlslInterpreter.hpp"
#include "cpuraster/Rasterizer.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace x3d::cpuraster;
namespace ex = x3d::runtime::extract;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static bool near(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }

static g::vec4 run(const std::string &src, FragmentInput f, bool &discarded,
                   std::vector<EyeLight> lights = {}, ex::MaterialDesc m = {}) {
  InterpretedProgram prog;
  std::string err;
  if (!prog.compile(src, &err)) {
    std::fprintf(stderr, "compile error: %s\n", err.c_str());
    discarded = false;
    return {-9, -9, -9, -9};
  }
  FragmentShader fs = makeInterpretedShader(prog, m, lights, /*hasColors=*/false);
  g::vec4 o;
  discarded = !fs(f, o);
  return o;
}

int main() {
  FragmentInput f;
  f.posEye = {0, 0, -5};
  f.normalEye = {0, 0, 1};
  f.frontFacing = true;
  bool disc = false;

  // ---- 1. literal vec4 ----
  {
    g::vec4 o = run("void main(){ FragColor = vec4(0.25, 0.5, 0.75, 1.0); }", f, disc);
    CHECK(!disc && near(o.x, 0.25f) && near(o.y, 0.5f) && near(o.z, 0.75f) && near(o.w, 1));
  }

  // ---- 2. swizzle reorder + scalar splat constructor ----
  {
    g::vec4 o = run("void main(){ vec4 c = vec4(1.0,2.0,3.0,4.0);"
                    " FragColor = vec4(c.zyx, c.w); }", f, disc);
    CHECK(near(o.x, 3) && near(o.y, 2) && near(o.z, 1) && near(o.w, 4));
  }

  // ---- 3. for-loop accumulation ----
  {
    g::vec4 o = run("void main(){ float s = 0.0;"
                    " for (int i = 0; i < 4; i++) { s += 1.0; }"
                    " FragColor = vec4(s, 0.0, 0.0, 1.0); }", f, disc);
    CHECK(near(o.x, 4.0f));
  }

  // ---- 4. user function + builtins (sq, clamp, max) ----
  {
    g::vec4 o = run("float sq(float x){ return x*x; }"
                    " void main(){ FragColor = vec4(sq(3.0),"
                    " clamp(5.0, 0.0, 1.0), max(2.0, 7.0), 1.0); }", f, disc);
    CHECK(near(o.x, 9) && near(o.y, 1) && near(o.z, 7));
  }

  // ---- 5. discard ----
  {
    g::vec4 o = run("void main(){ discard; }", f, disc);
    (void)o;
    CHECK(disc);
  }

  // ---- 6. read a host-bound varying (vNormalEye) ----
  {
    g::vec4 o = run("void main(){ FragColor = vec4(vNormalEye, 1.0); }", f, disc);
    CHECK(near(o.x, 0) && near(o.y, 0) && near(o.z, 1));
  }

  // ---- 7. light loop over host-bound uniform arrays ----
  {
    EyeLight L; L.dirEye = {0, 0, -1}; L.color = {1, 1, 1};
    g::vec4 o = run("void main(){ vec3 N = normalize(vNormalEye); float s = 0.0;"
                    " for (int i = 0; i < uNumLights; ++i) {"
                    "   vec3 L = normalize(-uLightDirEye[i]);"
                    "   s += max(dot(N, L), 0.0); }"
                    " FragColor = vec4(s, s, s, 1.0); }", f, disc, {L});
    CHECK(near(o.x, 1.0f, 1e-3f));
  }

  // ---- 8. conditional + gl_FrontFacing ----
  {
    FragmentInput back = f; back.frontFacing = false;
    g::vec4 o = run("void main(){ FragColor = gl_FrontFacing ?"
                    " vec4(1.0,0.0,0.0,1.0) : vec4(0.0,1.0,0.0,1.0); }", back, disc);
    CHECK(near(o.y, 1.0f) && near(o.x, 0.0f)); // back face -> green branch.
  }

  // ---- 9. parse error is reported, not crashed ----
  {
    InterpretedProgram prog;
    std::string err;
    CHECK(!prog.compile("void main( { FragColor = ; }", &err));
    CHECK(!err.empty());
  }

  // ---- 10. dFdx of the precomputed varying vPosEye is exact ----
  {
    FragmentInput d = f;
    d.dPosEyeDx = {2, 0, 0};
    g::vec4 o = run("void main(){ vec3 g = dFdx(vPosEye);"
                    " FragColor = vec4(g, 1.0); }", d, disc);
    CHECK(near(o.x, 2.0f));
  }

  if (failures) { std::fprintf(stderr, "glsl_interp_test: %d failure(s)\n", failures); return 1; }
  std::printf("glsl_interp_test: OK\n");
  return 0;
}
