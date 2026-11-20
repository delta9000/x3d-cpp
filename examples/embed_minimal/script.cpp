// Run an inline ECMAScript Script node through the installed x3d_cpp::ecmascript
// target: parse, attach every Script, tick, and read the ROUTEd result back.
#include <x3d/sdk.hpp>
#include <EcmaScriptBackend.hpp>
#include <ScriptSystem.hpp>
#include <x3d/nodes/Script.hpp>
#include <x3d/nodes/Transform.hpp>

#include <iostream>
#include <memory>

int main() {
  const std::string text = R"(<X3D profile='Full' version='4.0'><Scene>
    <Transform DEF='T'/>
    <Script DEF='S'>
      <field name='position' type='SFVec3f' accessType='outputOnly'/>
      <![CDATA[ecmascript:
        function initialize() { position = new SFVec3f(1, 2, 3); }
      ]]>
    </Script>
    <ROUTE fromNode='S' fromField='position' toNode='T' toField='translation'/>
  </Scene></X3D>)";
  auto session = x3d::sdk::RuntimeSession::create(
      x3d::sdk::parseDocument(text, x3d::sdk::Encoding::XML));
  auto scripts = std::make_shared<x3d::runtime::ScriptSystem>(
      std::make_shared<x3d::runtime::EcmaScriptBackend>(), "x3d_embed_script", "4.0");
  auto *script = dynamic_cast<x3d::nodes::Script *>(session->scene().resolve("S").get());
  if (!script) return 2;
  session->context().addScriptSystem(scripts);
  scripts->attach(script, session->context());
  session->tick(1.0);
  auto *t = dynamic_cast<x3d::nodes::Transform *>(session->scene().resolve("T").get());
  if (!t) return 3;
  const auto p = t->getTranslation();
  if (p.x != 1 || p.y != 2 || p.z != 3) {
    std::cerr << "translation " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    return 4;
  }
  std::cout << "installed ecmascript target ran an inline Script: PASS\n";
}
