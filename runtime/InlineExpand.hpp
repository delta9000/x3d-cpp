// InlineExpand.hpp
// Tier-1 parse-time Inline expansion. For each Inline node with load=TRUE and a
// resolvable url, replace it in its parent slot with a synthetic Group holding
// the loaded child scene's root nodes. The original Inline is preserved in
// Scene::expandedInlines so writers re-emit <Inline url=.../>. The child's
// internal ROUTEs are pre-resolved in the child's DEF scope into
// Scene::resolvedInlineRoutes. Node-agnostic: reflection + X3DNodeFactory only.
//
// Mirrors runtime/X3DProtoExpand.hpp (expandScene) and the AUD-B expandedSources
// writer redirect. DEF isolation per ISO 19775-1 §9.4.2: a child's DEFs are NOT
// exposed to the parent (only IMPORT would, which is out of Tier-1 scope).
#ifndef X3D_RUNTIME_INLINE_EXPAND_HPP
#define X3D_RUNTIME_INLINE_EXPAND_HPP

#include "X3DScene.hpp"
#include "X3DProtoExpand.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

// Re-export findField at the x3d::runtime level so callers (including the test)
// can use ::x3d::runtime::findField without needing to know about proto_detail.
inline const FieldInfo *findField(const X3DNode &n, const std::string &name) {
  return proto_detail::findField(n, name);
}

/// Resolve an Inline's url list to a parsed sub-scene (null on failure/cycle).
using InlineResolver = std::function<std::shared_ptr<Scene>(
    const std::vector<std::string> &urls, const std::string &baseUrl)>;

namespace inline_detail {

inline const FieldInfo *field(const X3DNode &n, const std::string &name) {
  return ::x3d::runtime::findField(n, name);
}

// Read an Inline's `url` (MFString) via reflection; empty if absent.
inline std::vector<std::string> readUrl(const X3DNode &inl) {
  const FieldInfo *u = field(inl, "url");
  if (!u || !u->get) return {};
  try {
    return std::any_cast<std::vector<std::string>>(u->get(inl));
  } catch (...) { return {}; }
}

// Read an Inline's `load` (SFBool); default TRUE if absent.
inline bool readLoad(const X3DNode &inl) {
  const FieldInfo *l = field(inl, "load");
  if (!l || !l->get) return true;
  try {
    return std::any_cast<bool>(l->get(inl));
  } catch (...) { return true; }
}

// Build a synthetic Group whose "children" are `content`.
inline std::shared_ptr<X3DNode>
makeGroup(const std::vector<std::shared_ptr<X3DNode>> &content) {
  auto g = X3DNodeFactory::create("Group");
  const FieldInfo *ch = field(*g, "children");
  if (ch && ch->set)
    ch->set(*g, std::any(content));
  return g;
}

// Pre-resolve the child scene's own ROUTEs (against the child's defs) and append
// to dst, plus hoist any routes the child already pre-resolved (nested inlines /
// protos). All become endpoint-concrete, parent-DEF-independent.
inline void hoistChildRoutes(Scene &child,
                             std::vector<ResolvedProtoRoute> &dst) {
  child.resolveRoutes(); // fill Route::from/to weak_ptrs from child.defs
  for (const Route &r : child.routes) {
    auto from = r.from.lock();
    auto to = r.to.lock();
    if (from && to)
      dst.push_back({from, r.fromField, to, r.toField});
  }
  for (const auto &pr : child.resolvedProtoRoutes) dst.push_back(pr);
  for (const auto &ir : child.resolvedInlineRoutes) dst.push_back(ir);
}

// Replace `target` with `replacement` inside an MFNode/SFNode field of `parent`.
// Returns true if a slot was found and rewritten.
inline bool replaceInParent(X3DNode &parent, const X3DNode *target,
                            const std::shared_ptr<X3DNode> &replacement) {
  for (const auto &f : parent.fields()) {
    if (!f.get || !f.set) continue;
    if (f.type == X3DFieldType::SFNode) {
      try {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(parent));
        if (c.get() == target) {
          f.set(parent, std::any(replacement));
          return true;
        }
      } catch (...) {}
    } else if (f.type == X3DFieldType::MFNode) {
      try {
        auto kids =
            std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(parent));
        for (auto &k : kids)
          if (k.get() == target) {
            k = replacement;
            f.set(parent, std::any(std::move(kids)));
            return true;
          }
      } catch (...) {}
    }
  }
  return false;
}

} // namespace inline_detail

/// Expand every load=TRUE Inline in `scene`. Lenient: failures become warnings.
inline void expandInlines(Scene &scene, const InlineResolver &resolver,
                          const std::string &baseUrl,
                          std::vector<InlineWarning> &warnings) {
  using namespace inline_detail;

  // Collect (parent, inlineNode) pairs first, so we don't mutate fields mid-walk.
  // Walk root-level + every node-typed field, recording the containing node.
  struct Site { X3DNode *parent; std::shared_ptr<X3DNode> inl; };
  std::vector<Site> sites;
  std::vector<std::shared_ptr<X3DNode>> rootInlines; // Inlines that ARE scene roots

  std::unordered_set<const X3DNode *> walked;
  std::function<void(const std::shared_ptr<X3DNode> &)> walk =
      [&](const std::shared_ptr<X3DNode> &n) {
        if (!n) return;
        if (!walked.insert(n.get()).second) return; // cycle/USE-sharing guard
        for (const auto &f : n->fields()) {
          if (!f.get) continue;
          if (f.type == X3DFieldType::SFNode) {
            try {
              auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
              if (!c) continue;
              if (c->nodeTypeName() == "Inline") sites.push_back({n.get(), c});
              else walk(c);
            } catch (...) {}
          } else if (f.type == X3DFieldType::MFNode) {
            try {
              auto cs = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n));
              for (auto &c : cs) {
                if (!c) continue;
                if (c->nodeTypeName() == "Inline") sites.push_back({n.get(), c});
                else walk(c);
              }
            } catch (...) {}
          }
        }
      };
  for (auto &r : scene.rootNodes) {
    if (r && r->nodeTypeName() == "Inline") rootInlines.push_back(r);
    else walk(r);
  }

  auto expandOne = [&](const std::shared_ptr<X3DNode> &inl,
                       X3DNode *parent /* null => scene root */) {
    if (!readLoad(*inl)) return; // load=FALSE: leave node un-expanded (Tier 3)
    std::vector<std::string> urls = readUrl(*inl);
    std::shared_ptr<Scene> child = resolver(urls, baseUrl);
    if (!child) {
      warnings.push_back({InlineWarning::Kind::UnresolvedUrl, inl->getDEF(),
                          urls.empty() ? "no url" : urls.front()});
      return; // node stays un-expanded; round-trips normally (no map entry)
    }
    // Build the Group from the child's root nodes; isolate child DEFs (we never
    // copy child.defs into scene.defs).
    auto group = makeGroup(child->rootNodes);
    hoistChildRoutes(*child, scene.resolvedInlineRoutes);
    scene.expandedInlines[group.get()] = inl; // preserve for writer round-trip
    if (parent) {
      replaceInParent(*parent, inl.get(), group);
    } else {
      for (auto &r : scene.rootNodes)
        if (r.get() == inl.get()) { r = group; break; }
    }
  };

  for (auto &site : sites) expandOne(site.inl, site.parent);
  for (auto &r : rootInlines) expandOne(r, nullptr);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INLINE_EXPAND_HPP
