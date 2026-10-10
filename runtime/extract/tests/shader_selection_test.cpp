// §31 programmable shaders through RuntimeSession: Appearance.shaders
// selection (§31.2.2.3), part source resolution (inline, data: and resolver
// urls), the host validator seam, isSelected/isValid events (§31.3.2),
// activate, live uniform values and source edits reaching RenderItem::
// shaderProgram through the incremental delta channel.
#include "RuntimeSession.hpp"
#include "ShaderExtract.hpp"
#include "X3DParse.hpp"
#include "doctest/doctest.h"

#include <algorithm>
#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
namespace ex = x3d::runtime::extract;

namespace {

const char *kVert = "void main(){ gl_Position = vec4(0.0); }";
const char *kFrag = "uniform vec3 uColor; void main(){ gl_FragColor = vec4(uColor, 1.0); }";

std::string part(const std::string &type, const std::string &source,
                 const std::string &def = "") {
  return "<ShaderPart " + (def.empty() ? "" : "DEF='" + def + "' ") + "type='" + type +
         "'><![CDATA[" + source + "]]></ShaderPart>";
}

std::string shape(const std::string &shaders) {
  return "<Shape><Appearance><Material diffuseColor='0 0 1'/>" + shaders +
         "</Appearance><Box/></Shape>";
}

struct Host {
  std::unique_ptr<RuntimeSession> session;
  double now = 0;
  std::map<std::string, std::vector<bool>> log; // "DEF.isValid" -> values

  explicit Host(const std::string &scene, SessionOptions options = {}) {
    session = RuntimeSession::create(
        x3d::codec::parseDocument("<X3D profile='Full' version='4.0'><Scene>" + scene +
                                  "</Scene></X3D>"),
        std::move(options));
    std::map<const X3DNode *, std::string> names;
    for (const auto &[name, node] : session->scene().defs) names[node.get()] = name;
    session->context().addFieldWriteListener([this, names](const FieldAddress &a) {
      auto found = names.find(a.node);
      if (found == names.end() || (a.field != "isValid" && a.field != "isSelected")) return;
      for (const auto &info : a.node->fields())
        if (info.x3dName == a.field && info.get)
          log[found->second + "." + a.field].push_back(std::any_cast<bool>(info.get(*a.node)));
    });
    session->fullSnapshot();
    tick();
  }

  X3DNode &node(const char *def) {
    auto n = session->scene().resolve(def);
    REQUIRE(n);
    return *n;
  }
  ex::RenderDelta tick() {
    session->tick(now += 0.1);
    return session->delta();
  }
  ex::RenderDelta post(const char *def, const char *field, std::any value) {
    session->context().postEvent(&node(def), field, std::move(value));
    return tick();
  }
  const ex::RenderItem &item() {
    REQUIRE(session->extractor().itemCount() == 1);
    return session->extractor().item(0);
  }
  std::vector<bool> events(const std::string &key) const {
    auto found = log.find(key);
    return found == log.end() ? std::vector<bool>{} : found->second;
  }
};

const ex::ShaderFieldBinding *field(const ex::ShaderProgramDesc &p, const char *name) {
  for (const auto &f : p.fields)
    if (f.name == name) return &f;
  return nullptr;
}

bool updatedMaterial(const ex::RenderDelta &d) {
  return std::find(d.updatedMaterial.begin(), d.updatedMaterial.end(), 0u) !=
         d.updatedMaterial.end();
}

} // namespace

TEST_SUITE("Shader selection (§31)") {

TEST_CASE("ComposedShader reaches RenderItem::shaderProgram with its uniforms") {
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>"
               "<field name='uColor' type='SFColor' accessType='inputOutput' value='1 0 0'/>"
               "<field name='uScale' type='SFFloat' accessType='inputOutput' value='2'/>" +
               part("VERTEX", kVert) + part("FRAGMENT", kFrag) + "</ComposedShader>"));
  const auto &p = h.item().shaderProgram;
  REQUIRE(p.has_value());
  CHECK(p->isSelected);
  CHECK(p->isValid);
  CHECK(p->language == "GLSL");
  REQUIRE(p->stages.size() == 2);
  CHECK(p->stages[0].stage == ex::ShaderStageDesc::Stage::Vertex);
  CHECK(p->stages[0].source == kVert);
  CHECK(p->stages[1].stage == ex::ShaderStageDesc::Stage::Fragment);
  CHECK(p->stages[1].source == kFrag);
  const auto *color = field(*p, "uColor");
  REQUIRE(color);
  CHECK(std::get<SFColor>(color->value.value) == SFColor{1, 0, 0});
  CHECK(std::get<float>(field(*p, "uScale")->value.value) == doctest::Approx(2));
  // §31.3.2: the selected shader reports itself selected and valid, once.
  CHECK(h.events("S.isValid") == std::vector<bool>{true});
  CHECK(h.events("S.isSelected") == std::vector<bool>{true});
  // The fixed-function material is still extracted for fallback.
  CHECK(h.item().material.phong.diffuse.b == doctest::Approx(1));
}

TEST_CASE("Selection takes the first supported, valid shader in order") {
  Host h(shape("<ComposedShader DEF='Hlsl' language='HLSL'>" + part("VERTEX", kVert) +
               part("FRAGMENT", kFrag) + "</ComposedShader>" +
               "<PackagedShader DEF='Pack' language='GLSL'/>" +
               "<ComposedShader DEF='NoVertex' language='GLSL'>" + part("FRAGMENT", kFrag) +
               "</ComposedShader>" +
               "<ComposedShader DEF='Good' language='glsl'>" + part("VERTEX", kVert) +
               part("FRAGMENT", kFrag) + "</ComposedShader>" +
               "<ComposedShader DEF='Later' language='GLSL'>" + part("VERTEX", kVert) +
               part("FRAGMENT", kFrag) + "</ComposedShader>"));
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(h.events("Good.isSelected") == std::vector<bool>{true});
  CHECK(h.events("Good.isValid") == std::vector<bool>{true});
  CHECK(h.events("NoVertex.isValid") == std::vector<bool>{false});
  CHECK(h.events("NoVertex.isSelected").empty());
  // Unsupported languages and node types stay inert; later entries are not tried.
  CHECK(h.events("Hlsl.isValid").empty());
  CHECK(h.events("Pack.isValid").empty());
  CHECK(h.events("Later.isValid").empty());
  CHECK(h.events("Later.isSelected").empty());
}

TEST_CASE("No valid shader keeps the fixed-function path") {
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>" + part("FRAGMENT", kFrag) +
               "</ComposedShader>"));
  CHECK_FALSE(h.item().shaderProgram.has_value());
  CHECK(h.events("S.isValid") == std::vector<bool>{false});
  CHECK(h.events("S.isSelected").empty());
}

TEST_CASE("A host validator decides validity and selection falls through") {
  SessionOptions options;
  options.meshOptions.shaders.validator = [](const ex::ShaderProgramDesc &p) {
    for (const auto &s : p.stages)
      if (s.source.find("broken") != std::string::npos)
        return ex::ShaderValidation{false, "0:1: syntax error"};
    return ex::ShaderValidation{true, {}};
  };
  const std::string scene =
      shape("<ComposedShader DEF='Bad' language='GLSL'>" + part("VERTEX", kVert) +
            part("FRAGMENT", "void main(){ broken }") + "</ComposedShader>" +
            "<ComposedShader DEF='Good' language='GLSL'>" + part("VERTEX", kVert) +
            part("FRAGMENT", kFrag) + "</ComposedShader>");
  Host h(scene, options);
  CHECK(h.events("Bad.isValid") == std::vector<bool>{false});
  CHECK(h.events("Good.isSelected") == std::vector<bool>{true});
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(h.item().shaderProgram->stages[1].source == kFrag);

  // The candidate list carries the host's error for diagnostics.
  const auto appearance = geombounds::getNode(*h.item().path.back(), "appearance");
  const auto sel = ex::selectShader(appearance.get(), &h.session->context().authorFields(),
                                    options.meshOptions.shaders);
  REQUIRE(sel.candidates.size() == 2);
  CHECK(sel.candidates[0].error == "0:1: syntax error");
  CHECK(sel.selected == &h.node("Good"));
}

TEST_CASE("Author field events update the uniform values on the next delta") {
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>"
               "<field name='uColor' type='SFColor' accessType='inputOutput' value='1 0 0'/>" +
               part("VERTEX", kVert) + part("FRAGMENT", kFrag) + "</ComposedShader>"));
  const auto d = h.post("S", "uColor", std::any(SFColor{0, 1, 0}));
  CHECK(updatedMaterial(d));
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(std::get<SFColor>(field(*h.item().shaderProgram, "uColor")->value.value) ==
        SFColor{0, 1, 0});
  // A uniform write is not a re-validation: no new isValid/isSelected events.
  CHECK(h.events("S.isValid") == std::vector<bool>{true});
  CHECK(h.events("S.isSelected") == std::vector<bool>{true});
}

TEST_CASE("Part sources resolve from data: urls and the host resolver") {
  SessionOptions options;
  std::vector<std::string> asked;
  options.meshOptions.shaders.resolver = [&asked](const std::string &url, ex::AssetKind kind) {
    CHECK(kind == ex::AssetKind::Shader);
    asked.push_back(url);
    if (url == "frag.glsl")
      return ex::AssetResult::makeReady(std::vector<std::uint8_t>(kFrag, kFrag + std::string(kFrag).size()));
    return ex::AssetResult::makeFailed();
  };
  // "void main(){}" percent-encoded, and the fragment fetched after a failed url.
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>"
               "<ShaderPart type='VERTEX' url='\"data:text/plain,void%20main()%7B%7D\"'/>"
               "<ShaderPart type='FRAGMENT' url='\"missing.glsl\" \"frag.glsl\"'/>"
               "</ComposedShader>"),
         options);
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(h.item().shaderProgram->stages[0].source == "void main(){}");
  CHECK(h.item().shaderProgram->stages[1].source == kFrag);
  CHECK(std::find(asked.begin(), asked.end(), "missing.glsl") != asked.end());

  CHECK(ex::shader_detail::decodeDataUrl("data:;base64,dm9pZCBtYWluKCl7fQ==") ==
        std::string("void main(){}"));
}

TEST_CASE("A pending url becomes valid when it arrives") {
  SessionOptions options;
  auto ready = std::make_shared<bool>(false);
  options.meshOptions.shaders.resolver = [ready](const std::string &, ex::AssetKind) {
    if (!*ready) return ex::AssetResult::makePending();
    return ex::AssetResult::makeReady(std::vector<std::uint8_t>(kFrag, kFrag + std::string(kFrag).size()));
  };
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>" + part("VERTEX", kVert) +
               "<ShaderPart type='FRAGMENT' url='\"frag.glsl\"'/></ComposedShader>"),
         options);
  CHECK_FALSE(h.item().shaderProgram.has_value());
  CHECK(h.events("S.isValid").empty()); // nothing reported while loading
  h.tick();
  CHECK(h.events("S.isValid").empty());
  *ready = true;
  const auto d = h.tick();
  CHECK(h.events("S.isValid") == std::vector<bool>{true});
  CHECK(h.events("S.isSelected") == std::vector<bool>{true});
  // The emitted outputs dirty the shader, so the extractor re-selects.
  CHECK(updatedMaterial(d));
  CHECK(h.item().shaderProgram.has_value());
}

TEST_CASE("Source edits re-select; losing validity falls back and deselects") {
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>" + part("VERTEX", kVert) +
               "<ShaderPart DEF='F' type='FRAGMENT' url='\"data:,void%20main()%7B%7D\"'/>"
               "</ComposedShader>"));
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(h.item().shaderProgram->stages[1].source == "void main(){}");

  auto d = h.post("F", "url", std::any(MFString{"data:,void%20main()%7Bdiscard;%7D"}));
  CHECK(updatedMaterial(d));
  REQUIRE(h.item().shaderProgram.has_value());
  CHECK(h.item().shaderProgram->stages[1].source == "void main(){discard;}");

  d = h.post("F", "url", std::any(MFString{"missing.glsl"}));
  CHECK(updatedMaterial(d));
  CHECK_FALSE(h.item().shaderProgram.has_value());
  CHECK(h.events("S.isValid") == std::vector<bool>{true, false});
  CHECK(h.events("S.isSelected") == std::vector<bool>{true, false});
}

TEST_CASE("activate re-resolves urls and re-reports validity") {
  SessionOptions options;
  auto source = std::make_shared<std::string>(kFrag);
  options.meshOptions.shaders.resolver = [source](const std::string &, ex::AssetKind) {
    return ex::AssetResult::makeReady(std::vector<std::uint8_t>(source->begin(), source->end()));
  };
  Host h(shape("<ComposedShader DEF='S' language='GLSL'>" + part("VERTEX", kVert) +
               "<ShaderPart type='FRAGMENT' url='\"frag.glsl\"'/></ComposedShader>"),
         options);
  CHECK(h.events("S.isValid") == std::vector<bool>{true});
  *source = "void main(){ gl_FragColor = vec4(1.0); }";
  h.tick();
  CHECK(h.item().shaderProgram->stages[1].source == kFrag); // nothing reloaded yet

  const auto d = h.post("S", "activate", std::any(SFBool{true}));
  CHECK(updatedMaterial(d));
  CHECK(h.item().shaderProgram->stages[1].source == *source);
  CHECK(h.events("S.isValid") == std::vector<bool>{true, true});
  CHECK(h.events("S.isSelected") == std::vector<bool>{true});

  h.post("S", "activate", std::any(SFBool{false})); // FALSE does nothing
  CHECK(h.events("S.isValid") == std::vector<bool>{true, true});
}

TEST_CASE("ProgramShader assembles its programs and their fields") {
  Host h(shape("<ProgramShader DEF='P' language='GLSL'>"
               "<ShaderProgram type='VERTEX'>"
               "<field name='uScale' type='SFFloat' accessType='inputOutput' value='3'/>"
               "<![CDATA[" + std::string(kVert) + "]]></ShaderProgram>"
               "<ShaderProgram type='FRAGMENT'>"
               "<field name='uColor' type='SFVec3f' accessType='inputOutput' value='0 0 1'/>"
               "<![CDATA[" + std::string(kFrag) + "]]></ShaderProgram>"
               "</ProgramShader>"));
  const auto &p = h.item().shaderProgram;
  REQUIRE(p.has_value());
  REQUIRE(p->stages.size() == 2);
  CHECK(p->stages[0].source == kVert);
  CHECK(p->stages[1].source == kFrag);
  REQUIRE(field(*p, "uScale"));
  CHECK(std::get<float>(field(*p, "uScale")->value.value) == doctest::Approx(3));
  CHECK(std::get<SFVec3f>(field(*p, "uColor")->value.value) == SFVec3f{0, 0, 1});
  CHECK(h.events("P.isSelected") == std::vector<bool>{true});
}

TEST_CASE("A shader shared by two Appearances is selected once") {
  Host h("<Shape><Appearance><ComposedShader DEF='S' language='GLSL'>" + part("VERTEX", kVert) +
         part("FRAGMENT", kFrag) + "</ComposedShader></Appearance><Box/></Shape>" +
         "<Transform translation='3 0 0'><Shape><Appearance><ComposedShader USE='S'/>"
         "</Appearance><Sphere/></Shape></Transform>");
  REQUIRE(h.session->extractor().itemCount() == 2);
  CHECK(h.session->extractor().item(0).shaderProgram.has_value());
  CHECK(h.session->extractor().item(1).shaderProgram.has_value());
  CHECK(h.events("S.isSelected") == std::vector<bool>{true});
  CHECK(h.events("S.isValid") == std::vector<bool>{true});
}

} // TEST_SUITE
