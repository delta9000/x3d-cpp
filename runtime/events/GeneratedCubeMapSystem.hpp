// GeneratedCubeMapSystem.hpp — §34.4.2 GeneratedCubeMapTexture update modes.
//
// The consumer renders a generated cube map when its `update` is not NONE
// (TextureRef::generatedCube). §34.4.2: NEXT_FRAME_ONLY renders the next frame
// once, then `update` "is set to NONE at the start of the next frame". This
// System performs that reset as an input event to the node, so the field
// changes and update_changed reaches ROUTEs.
//
// "The next frame" is the frame the extractor emits after the tick in which
// the value took effect. A write during tick N is drawn after tick N and reset
// at the start of tick N + 1. A value present before the first tick (authored,
// or set between ticks) is drawn after the following tick and reset at the
// start of the one after that.
#ifndef X3D_RUNTIME_GENERATED_CUBE_MAP_SYSTEM_HPP
#define X3D_RUNTIME_GENERATED_CUBE_MAP_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "x3d/nodes/GeneratedCubeMapTexture.hpp"

#include <any>
#include <cstdint>
#include <unordered_map>

namespace x3d::runtime {

class GeneratedCubeMapSystem : public System {
public:
  ~GeneratedCubeMapSystem() override { retireCallbacksBeforeDestruction(); }

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *cube = dynamic_cast<x3d::nodes::GeneratedCubeMapTexture *>(node);
    if (!cube) return;
    nodes_[cube] = kUnarmed;
    arm(cube, ctx);
    ctx.addFieldWriteListener(
        node, ctx.guardCallback(*this, [this, cube, &ctx](const FieldAddress &a) {
          if (a.node == cube && a.field == "update") arm(cube, ctx);
        }));
  }

  void detach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *cube = dynamic_cast<x3d::nodes::GeneratedCubeMapTexture *>(node);
    if (cube && nodes_.erase(cube)) ctx.removeFieldWriteListeners(node);
  }

  void update(double, X3DExecutionContext &ctx) override {
    using Update = x3d::core::GeneratedCubeMapTextureUpdateChoices;
    for (auto &[cube, drawnAfter] : nodes_) {
      if (drawnAfter == kUnarmed || ctx.tickGeneration() <= drawnAfter) continue;
      drawnAfter = kUnarmed;
      if (cube->getUpdate() == Update::NEXT_FRAME_ONLY)
        ctx.postEvent(cube, "update", std::any(Update::NONE));
    }
  }

private:
  static constexpr std::uint64_t kUnarmed = ~std::uint64_t{0};

  // Record the tick whose frame first shows NEXT_FRAME_ONLY.
  void arm(x3d::nodes::GeneratedCubeMapTexture *cube, X3DExecutionContext &ctx) {
    using Update = x3d::core::GeneratedCubeMapTextureUpdateChoices;
    if (cube->getUpdate() != Update::NEXT_FRAME_ONLY) {
      nodes_[cube] = kUnarmed;
      return;
    }
    nodes_[cube] = ctx.ticking() ? ctx.tickGeneration() : ctx.tickGeneration() + 1;
  }

  std::unordered_map<x3d::nodes::GeneratedCubeMapTexture *, std::uint64_t> nodes_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_GENERATED_CUBE_MAP_SYSTEM_HPP
