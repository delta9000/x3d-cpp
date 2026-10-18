#include <x3d/sdk.hpp>
#include <x3d/ext.hpp>

#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) return 1;
  const std::string text = R"(<X3D profile='Full' version='4.0'><Scene>
    <ExternProtoDeclare name='RoomGroup' url='"cave-fallbacks.x3d#RoomGroup"'>
      <field name='children' type='MFNode' accessType='inputOutput'/>
    </ExternProtoDeclare>
    <ProtoInstance name='RoomGroup'><fieldValue name='children'>
      <Shape><Box/></Shape>
    </fieldValue></ProtoInstance>
  </Scene></X3D>)";
  // Read the distributed fallback as a plain installed-SDK consumer.
  auto doc = x3d::sdk::parseDocument(text, x3d::sdk::Encoding::XML, argv[1]);
  if (!doc.protoWarnings.empty()) return 2;
  auto session = x3d::sdk::RuntimeSession::create(std::move(doc));
  auto input = x3d::ext::installCaveInput(session->scene(), session->context());
  if (!x3d::ext::install()({x3d::ext::kRoomGroupUrn}, "")) return 3;
  if (session->fullSnapshot().added.size() != 1) return 4;
  std::cout << "installed ext target, native resolver, and fallback: PASS\n";
}
