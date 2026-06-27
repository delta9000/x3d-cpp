#include "MaterialSystem.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <memory>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("texture_mapping_test") {
  // PhysicalMaterial with baseTextureMapping="uv1"
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"u.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "baseTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    setF(mat, "baseTextureMapping", std::any(SFString{"uv1"}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((!m.textures.empty()));
    CHECK((m.textures[0].slot == TextureRef::Slot::BaseColor));
    CHECK((m.textures[0].texCoordMapping == "uv1"));
  }
  // Empty texCoordMapping => UV set 0
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"u.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "baseTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    CHECK((!m.textures.empty()));
    CHECK((m.textures[0].texCoordMapping.empty()));
  }
  return;
}
