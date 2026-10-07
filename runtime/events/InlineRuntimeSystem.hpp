#ifndef X3D_RUNTIME_INLINE_RUNTIME_SYSTEM_HPP
#define X3D_RUNTIME_INLINE_RUNTIME_SYSTEM_HPP

#include "InlineExpand.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "GeoNodes.hpp"
#include "ViewDependentSystem.hpp"
#include "x3d/nodes/Inline.hpp"
#include "x3d/nodes/GeoLOD.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::runtime {

// §9.4.2: load=FALSE removes the content; a changed URL replaces it.
class InlineRuntimeSystem : public System {
public:
  InlineRuntimeSystem(Scene &scene, InlineResolver resolver, std::string baseUrl)
      : scene_(scene), resolver_(std::move(resolver)), baseUrl_(std::move(baseUrl)) {
    for (const auto &[group, original] : scene_.expandedInlines) {
      X3DNode *parent = nullptr;
      auto owned = findGroup(group, parent);
      if (owned && original)
        loaded_[original.get()] = {original, owned, parent,
                                   dynamic_cast<x3d::nodes::Inline *>(original.get())->getUrl()};
    }
  }

  void attach(X3DNode *node, X3DExecutionContext &) override {
    if (dynamic_cast<x3d::nodes::Inline *>(node) && seen_.insert(node).second)
      inlines_.push_back(node);
    if (auto *lod = dynamic_cast<x3d::nodes::GeoLOD *>(node);
        lod && (!lod->getRootUrl().empty() || !lod->getChild1Url().empty() ||
                !lod->getChild2Url().empty() || !lod->getChild3Url().empty() ||
                !lod->getChild4Url().empty()))
      geoLods_.try_emplace(lod);
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    geoLods_.erase(node);
    if (!seen_.erase(node)) return;
    inlines_.erase(std::remove(inlines_.begin(), inlines_.end(), node), inlines_.end());
    attempted_.erase(node);
  }

  void update(double, X3DExecutionContext &ctx) override {
    std::vector<X3DNode *> lods;
    for (const auto &[node, _] : geoLods_) lods.push_back(node);
    for (auto *node : lods)
      if (auto it = geoLods_.find(node); it != geoLods_.end())
        updateGeoLod(node, it->second, ctx);
    // A parent unload can remove nested Inlines during this pass.
    for (X3DNode *n : std::vector<X3DNode *>(inlines_)) {
      if (!seen_.count(n)) continue;
      auto *inl = dynamic_cast<x3d::nodes::Inline *>(n);
      auto it = loaded_.find(n);
      if (it != loaded_.end() && (!inl->getLoad() || it->second.url != inl->getUrl()))
        unload(n, ctx);
    }

    bool needsExpansion = false;
    for (X3DNode *n : inlines_) {
      auto *inl = dynamic_cast<x3d::nodes::Inline *>(n);
      if (loaded_.count(n)) continue;
      if (!inl->getLoad()) { attempted_.erase(n); continue; }
      if (auto it = attempted_.find(n); it != attempted_.end() && it->second == inl->getUrl())
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
    ctx.markSceneTopologyChanged();
    for (const Splice &s : splices) {
      auto original = scene_.expandedInlines.at(s.group.get());
      auto *inl = dynamic_cast<x3d::nodes::Inline *>(original.get());
      loaded_[inl] = {original, s.group, s.parent, inl->getUrl()};
      ctx.attachNewSubtree(s.group.get());
    }
    for (const auto &[group, original] : scene_.expandedInlines) {
      if (!original || seen_.count(original.get())) continue;
      X3DNode *parent = nullptr;
      if (auto owned = findGroup(group, parent)) {
        auto *inl = dynamic_cast<x3d::nodes::Inline *>(original.get());
        loaded_[original.get()] = {original, owned, parent, inl->getUrl()};
        attach(original.get(), ctx);
      }
    }
    for (std::size_t i = oldPeers; i < scene_.protoPeerNodes.size(); ++i)
      ctx.attachNewSubtree(scene_.protoPeerNodes[i].get());
    for (std::size_t i = oldRoutes; i < scene_.resolvedInlineRoutes.size(); ++i) {
      const auto &r = scene_.resolvedInlineRoutes[i];
      if (r.from && r.to) ctx.addRoute({r.from.get(), r.fromField}, {r.to.get(), r.toField});
    }
    for (std::size_t i = 0; i < scene_.routes.size(); ++i) {
      if (routeBound[i]) continue;
      const Route &r = scene_.routes[i];
      auto from = r.from.lock(), to = r.to.lock();
      if (from && to) ctx.addRoute({from.get(), r.fromField}, {to.get(), r.toField});
    }
  }

private:
  struct Tile {
    std::shared_ptr<Scene> scene;
    std::shared_ptr<X3DNode> group;
    bool everAttached = false;
  };
  struct GeoLodState {
    // Authored rootNode content has no child Scene to retain its field entries.
    // Keep only that subgraph's entries while a URL tile is displayed.
    DynamicFieldStore authoredRootFields;
    Tile root;
    Tile children[4];
    int displayed = -1;
  };

  Tile loadTile(const MFString &urls) {
    if (urls.empty()) return {};
    auto inl = std::make_shared<x3d::nodes::Inline>();
    inl->setUrl(urls);
    Scene tile;
    tile.rootNodes.push_back(inl);
    std::vector<InlineWarning> tileWarnings;
    expandInlines(tile, resolver_, baseUrl_, tileWarnings);
    auto child = tile.expandedInlineScenes.find(inl.get());
    if (child == tile.expandedInlineScenes.end()) return {};
    return {child->second, tile.rootNodes.front()};
  }

  void retireTile(Tile &tile, X3DExecutionContext &ctx) {
    if (!tile.group) return;
    if (!tile.everAttached) { tile = {}; return; }
    std::unordered_set<const X3DNode *> nodes;
    std::function<void(X3DNode *)> collect = [&](X3DNode *n) {
      if (!n || !nodes.insert(n).second) return;
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        collect(c.get());
      });
    };
    collect(tile.group.get());
    for (const auto &peer : tile.scene->protoPeerNodes) collect(peer.get());
    std::vector<X3DNode *> nested;
    for (const auto &[n, entry] : loaded_)
      if (entry.parent && nodes.count(entry.parent)) nested.push_back(n);
    for (auto *n : nested) unload(n, ctx);
    nodes.clear();
    collect(tile.group.get());
    for (const auto &peer : tile.scene->protoPeerNodes) collect(peer.get());
    ctx.detachNodes(nodes);
    scene_.resolvedInlineRoutes.erase(std::remove_if(scene_.resolvedInlineRoutes.begin(),
        scene_.resolvedInlineRoutes.end(), [&](const ResolvedProtoRoute &r) {
          return nodes.count(r.from.get()) || nodes.count(r.to.get());
        }), scene_.resolvedInlineRoutes.end());
    tile = {};
  }

  void showGeoLod(x3d::nodes::GeoLOD *lod, GeoLodState &state, int level,
                  X3DExecutionContext &ctx) {
    if (state.displayed >= 0) {
      std::unordered_set<const X3DNode *> oldNodes;
      std::function<void(X3DNode *)> collect = [&](X3DNode *n) {
        if (!n || !oldNodes.insert(n).second) return;
        forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
          collect(c.get());
        });
      };
      for (const auto &n : lod->getChildren()) collect(n.get());
      auto collectPeers = [&](const Tile &tile) {
        if (tile.scene)
          for (const auto &peer : tile.scene->protoPeerNodes) collect(peer.get());
      };
      if (state.displayed == 0) collectPeers(state.root);
      else for (const auto &tile : state.children) collectPeers(tile);
      if (state.displayed == 0 && !lod->getRootNode().empty())
        state.authoredRootFields.importFrom(*scene_.authorFields, &oldNodes);
      ctx.detachNodes(oldNodes);
      scene_.resolvedInlineRoutes.erase(std::remove_if(scene_.resolvedInlineRoutes.begin(),
          scene_.resolvedInlineRoutes.end(), [&](const ResolvedProtoRoute &r) {
            return oldNodes.count(r.from.get()) || oldNodes.count(r.to.get());
          }), scene_.resolvedInlineRoutes.end());
    }
    MFNode displayed;
    if (level == 0) {
      scene_.authorFields->importFrom(state.authoredRootFields);
      state.authoredRootFields.clear(); // the active Scene owns the entries again
      displayed = lod->getRootNode();
      if (displayed.empty() && state.root.group) displayed.push_back(state.root.group);
    } else {
      for (const Tile &tile : state.children)
        if (tile.group) displayed.push_back(tile.group);
    }
    lod->emitChildren(displayed);
    lod->emitLevel_changed(level);
    ctx.postEvent(lod, "children", std::any(displayed));
    ctx.postEvent(lod, "level_changed", std::any(static_cast<SFInt32>(level)));
    ctx.markActiveChildChanged(lod);
    ctx.refreshSceneTopology(scene_);
    ctx.markSceneTopologyChanged();
    auto addRoutes = [&](Tile &tile) {
      if (!tile.scene) return;
      scene_.authorFields->importFrom(*tile.scene->authorFields);
      const auto first = scene_.resolvedInlineRoutes.size();
      inline_detail::hoistChildRoutes(*tile.scene, scene_.resolvedInlineRoutes);
      for (std::size_t i = first; i < scene_.resolvedInlineRoutes.size(); ++i) {
        const auto &r = scene_.resolvedInlineRoutes[i];
        if (r.from && r.to)
          ctx.addRoute({r.from.get(), r.fromField}, {r.to.get(), r.toField});
      }
    };
    if (level == 0) addRoutes(state.root);
    else for (auto &tile : state.children) addRoutes(tile);
    for (const auto &node : displayed) {
      if (!node) continue;
      ctx.attachNewSubtree(node.get());
    }
    auto attachPeers = [&](const Tile &tile) {
      if (tile.scene)
        for (const auto &peer : tile.scene->protoPeerNodes)
          ctx.attachNewSubtree(peer.get());
    };
    if (level == 0) attachPeers(state.root);
    else for (const auto &tile : state.children) attachPeers(tile);
    if (level == 0) state.root.everAttached = static_cast<bool>(state.root.group);
    else for (auto &tile : state.children)
      if (tile.group) tile.everAttached = true;
    if (level == 0 && !lod->getRootNode().empty()) {
      std::unordered_set<const X3DNode *> active;
      std::function<void(X3DNode *)> collect = [&](X3DNode *n) {
        if (!n || !active.insert(n).second) return;
        forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
          collect(c.get());
        });
      };
      for (const auto &node : displayed) collect(node.get());
      for (const Route &r : scene_.routes) {
        auto from = r.from.lock(), to = r.to.lock();
        if (from && to && (active.count(from.get()) || active.count(to.get())))
          ctx.addRoute({from.get(), r.fromField}, {to.get(), r.toField});
      }
    }
    state.displayed = level;
  }

  void updateGeoLod(X3DNode *node, GeoLodState &state, X3DExecutionContext &ctx) {
    auto *lod = dynamic_cast<x3d::nodes::GeoLOD *>(node);
    if (!lod) return;
    SFVec3f center;
    if (!geo::toWorld(*lod, lod->getCenter(), center)) return;
    const SFVec3f eye = ctx.worldTransform(lod).inverse().transformPoint(ctx.cameraWorldPosition());
    const bool near = viewdep::len(viewdep::sub(eye, center)) < lod->getRange();
    const int wasDisplayed = state.displayed;
    const bool rootPending = lod->getRootNode().empty() && !state.root.group;
    if (rootPending)
      state.root = loadTile(lod->getRootUrl());
    bool childrenReady = false;
    if (near) {
      const MFString urls[4] = {lod->getChild1Url(), lod->getChild2Url(),
                                lod->getChild3Url(), lod->getChild4Url()};
      bool ready = true;
      bool requested = false;
      for (int i = 0; i < 4; ++i) {
        if (urls[i].empty()) continue;
        requested = true;
        if (!state.children[i].group) state.children[i] = loadTile(urls[i]);
        ready &= static_cast<bool>(state.children[i].group);
      }
      childrenReady = requested && ready;
    } else {
      if (state.displayed == 1 &&
          (!lod->getRootNode().empty() || state.root.group))
        showGeoLod(lod, state, 0, ctx);
      if (state.displayed != 1)
        for (auto &tile : state.children) retireTile(tile, ctx);
    }
    if (state.displayed < 0)
      showGeoLod(lod, state, childrenReady ? 1 : 0, ctx);
    else if (childrenReady && state.displayed != 1)
      showGeoLod(lod, state, 1, ctx);
    else if (rootPending && state.root.group && wasDisplayed == 0)
      showGeoLod(lod, state, 0, ctx);
  }

  struct Loaded {
    std::shared_ptr<X3DNode> original, group;
    X3DNode *parent;
    MFString url;
  };

  std::shared_ptr<X3DNode> findGroup(X3DNode *target, X3DNode *&parent) const {
    std::unordered_set<const X3DNode *> seen;
    std::function<std::shared_ptr<X3DNode>(const std::shared_ptr<X3DNode> &, X3DNode *)> walk =
        [&](const std::shared_ptr<X3DNode> &n, X3DNode *p) -> std::shared_ptr<X3DNode> {
          if (!n || !seen.insert(n.get()).second) return {};
          if (n.get() == target) { parent = p; return n; }
          std::shared_ptr<X3DNode> found;
          forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
            if (!found) found = walk(c, n.get());
          });
          return found;
        };
    for (const auto &root : scene_.rootNodes)
      if (auto found = walk(root, nullptr)) return found;
    return {};
  }

  void unload(X3DNode *inlineNode, X3DExecutionContext &ctx) {
    auto it = loaded_.find(inlineNode);
    if (it == loaded_.end()) return;
    Loaded old = it->second; // retain ownership throughout detach
    std::unordered_set<const X3DNode *> nodes;
    std::function<void(X3DNode *)> collect = [&](X3DNode *n) {
      if (!n || !nodes.insert(n).second) return;
      forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
        collect(c.get());
      });
    };
    collect(old.group.get());
    std::vector<X3DNode *> nested;
    for (const auto &[n, entry] : loaded_)
      if (n != inlineNode && entry.parent && nodes.count(entry.parent)) nested.push_back(n);
    for (X3DNode *n : nested) unload(n, ctx);
    nodes.clear();
    collect(old.group.get());
    if (auto child = scene_.expandedInlineScenes.find(inlineNode);
        child != scene_.expandedInlineScenes.end())
      for (const auto &peer : child->second->protoPeerNodes) collect(peer.get());

    ctx.detachNodes(nodes);
    auto touches = [&](const ResolvedProtoRoute &r) {
      return nodes.count(r.from.get()) || nodes.count(r.to.get());
    };
    scene_.resolvedInlineRoutes.erase(std::remove_if(scene_.resolvedInlineRoutes.begin(),
        scene_.resolvedInlineRoutes.end(), touches), scene_.resolvedInlineRoutes.end());
    scene_.resolvedProtoRoutes.erase(std::remove_if(scene_.resolvedProtoRoutes.begin(),
        scene_.resolvedProtoRoutes.end(), touches), scene_.resolvedProtoRoutes.end());
    scene_.protoPeerNodes.erase(std::remove_if(scene_.protoPeerNodes.begin(),
        scene_.protoPeerNodes.end(), [&](const auto &p) { return nodes.count(p.get()); }),
        scene_.protoPeerNodes.end());
    for (auto def = scene_.defs.begin(); def != scene_.defs.end();) {
      bool imported = false;
      for (const Import &imp : scene_.imports)
        if (scene_.resolve(imp.inlineDEF).get() == inlineNode &&
            def->first == (imp.as.empty() ? imp.importedDEF : imp.as)) imported = true;
      if (imported && nodes.count(def->second.get())) def = scene_.defs.erase(def);
      else ++def;
    }
    for (auto p = scene_.protoRedirects.begin(); p != scene_.protoRedirects.end();)
      if (nodes.count(p->first)) p = scene_.protoRedirects.erase(p); else ++p;
    for (auto p = scene_.expandedSources.begin(); p != scene_.expandedSources.end();)
      if (nodes.count(p->first)) p = scene_.expandedSources.erase(p); else ++p;
    scene_.expandedInlineScenes.erase(inlineNode);
    scene_.expandedInlines.erase(old.group.get());
    if (old.parent) inline_detail::replaceInParent(*old.parent, old.group.get(), old.original);
    else for (auto &root : scene_.rootNodes)
      if (root.get() == old.group.get()) { root = old.original; break; }
    loaded_.erase(it);
    attempted_.erase(inlineNode);
    scene_.resolveRoutes();
    ctx.refreshSceneTopology(scene_);
    ctx.markSceneTopologyChanged();
  }

  Scene &scene_;
  InlineResolver resolver_;
  std::string baseUrl_;
  std::vector<X3DNode *> inlines_;
  std::unordered_set<X3DNode *> seen_;
  std::unordered_map<X3DNode *, MFString> attempted_;
  std::unordered_map<X3DNode *, Loaded> loaded_;
  std::unordered_map<X3DNode *, GeoLodState> geoLods_;
  std::vector<InlineWarning> warnings_;
};

} // namespace x3d::runtime

#endif
