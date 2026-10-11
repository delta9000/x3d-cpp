// author_shader_test.cpp — REQ-SHADER observable output: a scene-authored
// ComposedShader selected by the SDK (RenderItem::shaderProgram) runs in the
// CPU reference host, its author <field> uniforms (SF values, MF arrays and
// texture samplers) reach the pixels, a uniform
// event changes the next frame, and a program the host cannot compile falls
// through to the next shader or to the fixed-function material (§31.2.2.3).
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"
#include "cpuraster/AuthorShader.hpp"
#include "cpuraster/SceneRender.hpp"

#include <any>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace x3d::cpuraster;
namespace ex = x3d::runtime::extract;
namespace rt = x3d::runtime;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x);                     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static bool near(const g::vec4 &c, float r, float gr, float b) {
  return std::fabs(c.x - r) < 0.02f && std::fabs(c.y - gr) < 0.02f &&
         std::fabs(c.z - b) < 0.02f;
}

static const char *kVert = "void main(){ gl_Position = vec4(0.0); }";

static std::string composed(const std::string &def, const std::string &frag,
                            const std::string &fields = "") {
  return "<ComposedShader DEF='" + def + "' language='GLSL'>" + fields +
         "<ShaderPart type='VERTEX'><![CDATA[" + kVert + "]]></ShaderPart>"
         "<ShaderPart type='FRAGMENT'><![CDATA[" + frag + "]]></ShaderPart>"
         "</ComposedShader>";
}

struct Host {
  std::unique_ptr<rt::RuntimeSession> session;
  AuthorShaderCache cache;
  double now = 0;

  explicit Host(const std::string &shaders) {
    rt::SessionOptions options;
    options.meshOptions.shaders.validator = interpreterShaderValidator();
    session = rt::RuntimeSession::create(
        x3d::codec::parseDocument(
            "<X3D profile='Full' version='4.0'><Scene><Viewpoint position='0 0 5'/>"
            "<NavigationInfo headlight='false'/>"
            "<Shape><Appearance><Material emissiveColor='0 0 1' diffuseColor='0 0 0'/>" +
            shaders + "</Appearance><Box size='2 2 2'/></Shape></Scene></X3D>"),
        std::move(options));
    session->fullSnapshot();
    tick();
  }
  void tick() {
    session->tick(now += 0.1);
    session->delta();
  }
  g::vec4 center() {
    RenderOptions opt;
    opt.width = opt.height = 32;
    opt.authorShaderFor = cache.hook();
    return renderScene(session->context(), session->extractor(), opt).colorAt(16, 16);
  }
  bool flag(const char *def, const char *field) {
    auto n = session->scene().resolve(def);
    for (const auto &info : n->fields())
      if (info.x3dName == field) return std::any_cast<bool>(info.get(*n));
    return false;
  }
};

int main() {
  const std::string colorFrag =
      "uniform vec3 uColor; void main(){ FragColor = vec4(uColor, 1.0); }";
  const std::string colorField =
      "<field name='uColor' type='SFColor' accessType='inputOutput' value='1 0 0'/>";

  // The author program draws instead of the material (emissive blue).
  {
    Host h(composed("S", colorFrag, colorField));
    CHECK(near(h.center(), 1, 0, 0));
    CHECK(h.flag("S", "isSelected"));
    CHECK(h.flag("S", "isValid"));

    // A uniform event reaches the next frame through the delta channel.
    h.session->context().postEvent(h.session->scene().resolve("S").get(), "uColor",
                                   std::any(x3d::core::SFColor{0, 1, 0}));
    h.tick();
    CHECK(near(h.center(), 0, 1, 0));
  }

  // REQ-SHADER-2: texture (SFNode -> sampler2D, MFNode -> sampler2D[]) and
  // array (MF -> float[] / vec3[]) fields reach the program; an MF event too.
  {
    const std::string frag =
        "uniform sampler2D uTex; uniform sampler2D uLayers[2];"
        "uniform float uW[3]; uniform vec3 uP[2];"
        "void main(){ FragColor = vec4(texture(uTex, vec2(0.5)).g * uW[2],"
        " texture(uLayers[1], vec2(0.5)).b, uP[1].x, 1.0); }";
    const std::string fields =
        "<field name='uTex' type='SFNode' accessType='inputOutput'>"
        "<PixelTexture image='1 1 3 0x00FF00'/></field>"
        "<field name='uLayers' type='MFNode' accessType='inputOutput'>"
        "<PixelTexture image='1 1 3 0xFF0000'/><PixelTexture image='1 1 3 0x0000FF'/>"
        "</field>"
        "<field name='uW' type='MFFloat' accessType='inputOutput' value='0 0 0.5'/>"
        "<field name='uP' type='MFVec3f' accessType='inputOutput' value='0 0 0 0.25 0 0'/>";
    Host h(composed("T", frag, fields));
    CHECK(h.flag("T", "isSelected"));
    CHECK(near(h.center(), 0.5f, 1, 0.25f));
    h.session->context().postEvent(h.session->scene().resolve("T").get(), "uW",
                                   std::any(x3d::core::MFFloat{0, 0, 1}));
    h.tick();
    CHECK(near(h.center(), 1, 1, 0.25f));
  }

  // A program this host cannot compile is invalid: selection moves on.
  {
    Host h(composed("Bad", "void main(){ FragColor = ; }") +
           composed("Good", "void main(){ FragColor = vec4(1.0, 1.0, 0.0, 1.0); }"));
    CHECK(near(h.center(), 1, 1, 0));
    CHECK(!h.flag("Bad", "isValid"));
    CHECK(!h.flag("Bad", "isSelected"));
    CHECK(h.flag("Good", "isSelected"));
  }

  // No runnable shader: the fixed-function material draws.
  {
    Host h(composed("Bad", "void main(){ FragColor = ; }"));
    CHECK(near(h.center(), 0, 0, 1));
    CHECK(!h.flag("Bad", "isValid"));
  }

  if (failures) {
    std::fprintf(stderr, "author_shader_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("author_shader_test: OK\n");
  return 0;
}
