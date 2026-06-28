#include "x3d/sdk.hpp"

#include <iostream>

int main() {
  namespace sdk = x3d::sdk;

  constexpr const char *scene = R"X3D(
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Viewpoint position="0 0 6"/>
    <Transform translation="0 0 0">
      <Shape>
        <Appearance>
          <Material diffuseColor="0.1 0.5 0.9"/>
        </Appearance>
        <Box size="2 2 2"/>
      </Shape>
    </Transform>
  </Scene>
</X3D>
)X3D";

  sdk::X3DDocument doc = sdk::parseDocument(scene, sdk::Encoding::XML);

  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  sdk::BridgeResult bridge = ctx.buildFrom(doc.scene);
  if (!bridge.rejected.empty()) {
    std::cerr << "route bridge rejected " << bridge.rejected.size()
              << " route(s)\n";
    return 1;
  }

  sdk::SceneExtractor extractor(ctx, doc.scene);
  sdk::RenderDelta snapshot = extractor.fullSnapshot();

  std::cout << "x3d-cpp embedded: version=" << doc.version
            << " profile=Interchange"
            << " render_items=" << snapshot.added.size() << "\n";
  return snapshot.added.empty() ? 1 : 0;
}
