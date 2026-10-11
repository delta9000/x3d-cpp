// TextureProjectorSystem.hpp — §42.3.1 X3DTextureProjectorNode.aspectRatio.
//
// aspectRatio is an output: the width / height of the image the projector
// projects. The runtime knows that ratio only for a PixelTexture (its image
// field); a url texture is decoded by the consumer's TextureResolver, which
// the runtime never calls, so no aspectRatio is emitted for it (the extractor
// still uses the decoded ratio for ProjectorDesc). The event is sent when the
// System attaches and whenever the projector's texture, or that PixelTexture's
// image, changes to a different ratio.
#ifndef X3D_RUNTIME_TEXTURE_PROJECTOR_SYSTEM_HPP
#define X3D_RUNTIME_TEXTURE_PROJECTOR_SYSTEM_HPP

#include "GeometryBounds.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "x3d/nodes/X3DTextureProjectorNode.hpp"

#include <any>
#include <unordered_map>

namespace x3d::runtime {

class TextureProjectorSystem : public System {
public:
  ~TextureProjectorSystem() override { retireCallbacksBeforeDestruction(); }

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (!dynamic_cast<x3d::nodes::X3DTextureProjectorNode *>(node)) return;
    projectors_[node] = 0.0f;
    report(node, ctx);
    ctx.addFieldWriteListener(
        node, ctx.guardCallback(*this, [this, node, &ctx](const FieldAddress &a) {
          if (a.node == node && a.field == "texture") report(node, ctx);
          else if (a.field == "image" && a.node == texture(node)) report(node, ctx);
        }));
  }

  void detach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (projectors_.erase(node)) ctx.removeFieldWriteListeners(node);
  }

  void update(double, X3DExecutionContext &) override {}

private:
  static const X3DNode *texture(const X3DNode *projector) {
    return geombounds::getNode(*projector, "texture").get();
  }

  void report(X3DNode *projector, X3DExecutionContext &ctx) {
    const X3DNode *t = texture(projector);
    if (!t || t->nodeTypeName() != "PixelTexture") return;
    const auto image = geombounds::getField<x3d::core::SFImage>(*t, "image", {});
    if (image.width <= 0 || image.height <= 0) return;
    const float ratio = static_cast<float>(image.width) / static_cast<float>(image.height);
    float &last = projectors_[projector];
    if (ratio == last) return;
    last = ratio;
    ctx.postEvent(projector, "aspectRatio", std::any(x3d::core::SFFloat(ratio)));
  }

  std::unordered_map<X3DNode *, float> projectors_; // last ratio sent (0: none).
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TEXTURE_PROJECTOR_SYSTEM_HPP
