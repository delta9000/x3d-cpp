// xml_composed_shader_test.cpp
// XML ComposedShader capture (Phase-3 author-shader plumbing).
//
// Proves the XML reader un-tables a file-authored ComposedShader the same way it
// already did for <Script>:
//   1. A <ComposedShader> with author <field name accessType type value> children
//      lands those fields in the per-node DynamicFieldStore (visible via
//      effectiveFields), so the renderer can bind them as uniforms.
//   2. Each child <ShaderPart>'s inline <![CDATA[...]]> GLSL body is captured into
//      ShaderPart.sourceCode (previously dropped — only <Script> was handled).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "DynamicField.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/ComposedShader.hpp"
#include "x3d/nodes/ShaderPart.hpp"
#include "x3d/nodes/X3DShapeNode.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d::core;
using namespace x3d::nodes;
namespace runtime = x3d::runtime;
namespace codec = x3d::codec;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

const char *sampleXml() {
  return R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <Shape>
    <Appearance>
      <ComposedShader DEF="CS" language="GLSL">
        <field accessType="inputOutput" name="tint" type="SFColor" value="0.95 0.45 0.10"/>
        <ShaderPart type="VERTEX"><![CDATA[
void main() { gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0)); }
]]></ShaderPart>
        <ShaderPart type="FRAGMENT"><![CDATA[
uniform vec4 tint;
void main() { gl_FragColor = tint; }
]]></ShaderPart>
      </ComposedShader>
    </Appearance>
    <Box/>
  </Shape>
</Scene></X3D>)X3D";
}

// Find the first ComposedShader under a parsed scene's Shapes.
std::shared_ptr<ComposedShader> findComposedShader(const runtime::Scene &scene) {
  for (const auto &n : scene.rootNodes) {
    auto shape = std::dynamic_pointer_cast<X3DShapeNode>(n);
    if (!shape) continue;
    auto app = std::dynamic_pointer_cast<Appearance>(shape->getAppearance());
    if (!app) continue;
    for (const auto &s : app->getShaders())
      if (auto cs = std::dynamic_pointer_cast<ComposedShader>(s))
        return cs;
  }
  return nullptr;
}

} // namespace

int main() {
  codec::XmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(sampleXml());

  auto cs = findComposedShader(doc.scene);
  check(cs != nullptr, "ComposedShader parsed under Shape.appearance.shaders");
  if (!cs) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }

  // (1) Author <field> landed in the DynamicFieldStore.
  bool hasTint = false;
  for (const FieldInfo &fi : runtime::dynamicFieldStore().authorFields(*cs))
    if (fi.x3dName == "tint" && fi.type == X3DFieldType::SFColor)
      hasTint = true;
  check(hasTint, "author <field> 'tint' (SFColor) captured into the store");

  // (2) ShaderPart CDATA captured into sourceCode, per stage.
  bool vsOk = false, fsOk = false;
  for (const auto &p : cs->getParts()) {
    auto part = std::dynamic_pointer_cast<ShaderPart>(p);
    if (!part) continue;
    const std::string type = part->getType();
    const bool hasSrc = part->getSourceCode().find("void main") != std::string::npos;
    if (type == "VERTEX")   vsOk = hasSrc;
    if (type == "FRAGMENT") fsOk = hasSrc;
  }
  check(vsOk, "VERTEX ShaderPart CDATA captured into sourceCode");
  check(fsOk, "FRAGMENT ShaderPart CDATA captured into sourceCode");

  runtime::dynamicFieldStore().clear();
  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all XML ComposedShader capture tests passed\n";
  return 0;
}
