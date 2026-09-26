// AnchorSystem.hpp — Anchor activation (ISO/IEC 19775-1 §9.4.1).
//
// An Anchor's children are pointer-sensitive: a click (button press and release
// over the same Anchor's geometry) activates it. Its url is tried in order:
//   * "#Name"  — bind the Viewpoint DEF'd Name in this scene (the in-scene case);
//   * anything else — handed to the embedder's AnchorHandler, which loads a
//     replacement world, opens a separate window (§9.4.1 cases b/c, steered by
//     `parameter`, e.g. "target=_blank"), or ignores it. The runtime is headless,
//     so with no handler installed a non-fragment url does nothing.
// A pointing-device sensor that grabbed the pointer this tick wins (§20.2.1);
// while a press on an Anchor is held the Anchor owns the pointer, so navigation
// does not also drag or LOOKAT on the same click.
#ifndef X3D_RUNTIME_ANCHOR_SYSTEM_HPP
#define X3D_RUNTIME_ANCHOR_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/Anchor.hpp"
#include "x3d/nodes/X3DInterfaceRegistry.hpp"

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

namespace x3d::runtime {
using namespace x3d::core;

class AnchorSystem : public System {
public:
  /// Called for an activated Anchor whose url is not an in-scene "#Name":
  /// the Anchor, its url list and its parameter list (§9.4.1).
  using AnchorHandler = std::function<void(X3DNode *anchor, const MFString &url,
                                           const MFString &parameter)>;
  void setAnchorHandler(AnchorHandler h) { handler_ = std::move(h); }

  // Inventory: Anchors present (else the per-click pick is skipped) and the
  // scene's viewpoints by DEF name, for "#Name" urls.
  void attach(X3DNode *node, X3DExecutionContext & /*ctx*/) override {
    if (dynamic_cast<x3d::nodes::Anchor *>(node)) ++anchors_;
    if (x3d::nodes::X3DInterfaceRegistry::nodeImplements(
            node, x3d::nodes::InterfaceId::X3DViewpointNode) &&
        !node->getDEF().empty())
      viewpointsByDef_.emplace(node->getDEF(), node);
  }

  void update(double /*now*/, X3DExecutionContext &ctx) override {
    const PointerState &ps = ctx.pointerState();
    const bool down = ps.present && ps.buttonDown;
    const bool pressEdge = down && !wasDown_;
    const bool releaseEdge = !down && wasDown_;
    wasDown_ = down;
    if (anchors_ == 0) return;

    if (pressEdge) {
      pressed_ = ctx.pointerConsumedBySensor() ? nullptr : anchorUnder(ctx, ps);
    }
    if (pressed_ && down) ctx.setPointerConsumedBySensor(true); // hold the pointer
    if (releaseEdge && pressed_) {
      X3DNode *a = pressed_;
      pressed_ = nullptr;
      if (ps.present && anchorUnder(ctx, ps) == a) activate(a, ctx);
    }
  }

private:
  // The innermost Anchor on the pick path under the pointer, if any.
  static X3DNode *anchorUnder(X3DExecutionContext &ctx, const PointerState &ps) {
    PickResult pick = ctx.pick(ps.ray);
    if (!pick.hit) return nullptr;
    for (auto it = pick.path.rbegin(); it != pick.path.rend(); ++it)
      if (dynamic_cast<const x3d::nodes::Anchor *>(*it))
        return const_cast<X3DNode *>(*it);
    return nullptr;
  }

  void activate(X3DNode *node, X3DExecutionContext &ctx) {
    auto *a = dynamic_cast<x3d::nodes::Anchor *>(node);
    if (!a) return;
    for (const std::string &u : a->getUrl()) {
      if (u.size() > 1 && u[0] == '#') {
        auto it = viewpointsByDef_.find(u.substr(1));
        if (it == viewpointsByDef_.end()) continue; // try the next url
        ctx.postEvent(it->second, "set_bind", std::any(SFBool{true}));
        return;
      }
      if (u.empty()) continue;
      if (handler_) handler_(node, a->getUrl(), a->getParameter());
      return;
    }
  }

  AnchorHandler handler_;
  std::unordered_map<std::string, X3DNode *> viewpointsByDef_;
  std::size_t anchors_ = 0;
  X3DNode *pressed_ = nullptr;
  bool wasDown_ = false;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_ANCHOR_SYSTEM_HPP
