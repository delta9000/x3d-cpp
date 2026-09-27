#ifndef X3D_RUNTIME_INLINE_RUNTIME_SYSTEM_HPP
#define X3D_RUNTIME_INLINE_RUNTIME_SYSTEM_HPP

#include "InlineExpand.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "x3d/nodes/Inline.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::runtime {

// Loads an initially deferred Inline through the same resolver and expansion
// pass used by parseDocument. The caller supplies the resolver and base URL.
class InlineRuntimeSystem : public System {
public:
  InlineRuntimeSystem(Scene &scene, InlineResolver resolver, std::string baseUrl)
      : scene_(scene), resolver_(std::move(resolver)), baseUrl_(std::move(baseUrl)) {}

  void attach(X3DNode *node, X3DExecutionContext &) override {
    if (dynamic_cast<x3d::nodes::Inline *>(node) && seen_.insert(node).second)
      inlines_.push_back(node);
  }

  void update(double, X3DExecutionContext &ctx) override {
    bool needsExpansion = false;
    for (X3DNode *n : inlines_) {
      auto *inl = dynamic_cast<x3d::nodes::Inline *>(n);
      if (!inl || scene_.expandedInlineScenes.count(n)) continue;
      if (!inl->getLoad()) {
        attempted_.erase(n);
        continue;
      }
      if (auto it = attempted_.find(n);
          it != attempted_.end() && it->second == inl->getUrl())
        continue;
      attempted_[n] = inl->getUrl();
      needsExpansion = true;
    }
    if (!needsExpansion) return;

    struct Splice { X3DNode *parent; std::shared_ptr<X3DNode> group; };
    std::vector<Splice> splices;
    const std::size_t oldRoutes = scene_.resolvedInlineRoutes.size();
    const std::size_t oldPeers = scene_.protoPeerNodes.size();
    std::vector<bool> routeBound;
    for (const Route &r : scene_.routes)
      routeBound.push_back(!r.from.expired() && !r.to.expired());

    expandInlines(scene_, resolver_, baseUrl_, warnings_,
                  [&](X3DNode *parent, const std::shared_ptr<X3DNode> &group) {
                    splices.push_back({parent, group});
                  });
    if (splices.empty()) return;

    wireInlineImports(scene_);
    scene_.resolveRoutes();
    ctx.refreshSceneTopology(scene_);
    for (const Splice &s : splices) {
      ctx.attachNewSubtree(s.group.get());
      // §9.4.2: the newly loaded children now participate in rendering.
      ctx.markActiveChildChanged(s.parent ? s.parent : s.group.get());
    }
    for (std::size_t i = oldPeers; i < scene_.protoPeerNodes.size(); ++i)
      ctx.attachNewSubtree(scene_.protoPeerNodes[i].get());
    for (std::size_t i = oldRoutes; i < scene_.resolvedInlineRoutes.size(); ++i) {
      const auto &r = scene_.resolvedInlineRoutes[i];
      if (r.from && r.to)
        ctx.addRoute({r.from.get(), r.fromField}, {r.to.get(), r.toField});
    }
    for (std::size_t i = 0; i < scene_.routes.size(); ++i) {
      if (routeBound[i]) continue;
      const Route &r = scene_.routes[i];
      auto from = r.from.lock(), to = r.to.lock();
      if (from && to)
        ctx.addRoute({from.get(), r.fromField}, {to.get(), r.toField});
    }
  }

private:
  Scene &scene_;
  InlineResolver resolver_;
  std::string baseUrl_;
  std::vector<X3DNode *> inlines_;
  std::unordered_set<X3DNode *> seen_;
  std::unordered_map<X3DNode *, MFString> attempted_;
  std::vector<InlineWarning> warnings_;
};

} // namespace x3d::runtime

#endif
