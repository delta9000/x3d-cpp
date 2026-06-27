#include "MaterialSystem.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <memory>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("texture_orm_test") {
  // PhysicalMaterial with metallicRoughnessTexture → single slot, Linear.
  {
    auto tex = createX3DNode("ImageTexture");
    setF(tex, "url", std::any(MFString{"orm.png"}));
    auto mat = createX3DNode("PhysicalMaterial");
    setF(mat, "metallicRoughnessTexture", std::any(std::shared_ptr<X3DNode>(tex)));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(mat)));

    MaterialDesc m = materialOf(app.get());
    // Exactly one MetallicRoughness slot (not separate Metallic + Roughness).
    int mrCount = 0;
    for (const auto &t : m.textures)
      if (t.slot == TextureRef::Slot::MetallicRoughness) ++mrCount;
    CHECK((mrCount == 1));
  }
  return;
}
