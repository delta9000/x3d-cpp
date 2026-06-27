// BindingSystem.hpp — owns one BindingStack per bindable category, enrols bindable
// nodes from a Scene, wires each node's set_bind handler to drive its stack, and
// default-binds the first of each category. Decoupled from X3DExecutionContext via
// a Poster (post an event) + Clock (current time) callback — so there is NO include
// cycle with the context. Context-owned, purely event-driven. namespace x3d::runtime.
#ifndef X3D_RUNTIME_BINDING_SYSTEM_HPP
#define X3D_RUNTIME_BINDING_SYSTEM_HPP

#include "BindingStack.hpp"
#include "x3d/nodes/X3DBindableNode.hpp"
#include "x3d/nodes/X3DInterfaceRegistry.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;
using namespace x3d::nodes;

class BindingSystem {
public:
  using Poster = std::function<void(X3DNode *, const std::string &, std::any)>;
  using Clock = std::function<double()>;
  using TransitionSink = std::function<void(BindTransition)>;

  // Enrol every bindable node; wire its set_bind handler to post via `poster`.
  void enrollScene(const Scene &scene, Poster poster, Clock clock,
                   TransitionSink sink = {}) {
    poster_ = std::move(poster);
    clock_ = std::move(clock);
    sink_ = std::move(sink);
    for (const auto &root : scene.rootNodes)
      if (root) walk(root.get());
  }

  // Bind the first enrolled node of each category that has no current top.
  // Runs at build time (no active cascade) -> set isBound + bindTime directly
  // via reflection (§23.3.1 case 1: "during loading" bindTime must be emitted).
  void bindDefaults() {
    const double t = clock_();
    BindingStack::Emit emit = [t](X3DNode *n, bool bound) {
      setIsBound(n, bound);
      setBindTime(n, t);
    };
    for (auto &kv : enrolled_) {
      const std::string &cat = kv.first;
      if (!kv.second.empty() && !stacks_[cat].top()) {
        if (sink_ && cat == "Viewpoint") sink_(BindTransition::Push);
        stacks_[cat].pushDefault(kv.second.front(), emit);
      }
    }
  }

  X3DNode *bound(const std::string &category) const {
    auto it = stacks_.find(category);
    return it == stacks_.end() ? nullptr : it->second.top();
  }

  // BIND-06 §7.2.2 r9 / §23.3.1 r9: a deleted bound node behaves as set_bind FALSE
  // — pop it from its stack so the next entry becomes bound (isBound/bindTime).
  void removeNode(X3DNode *node) {
    if (!node || !poster_) return;
    auto it = stacks_.find(category(node));
    if (it == stacks_.end()) return;
    BindingStack::Emit emit = [this](X3DNode *t, bool bound) {
      poster_(t, "isBound", std::any(SFBool(bound)));
      poster_(t, "bindTime", std::any(SFTime(clock_())));
    };
    // BIND-09: signal Pop before unbinding (after, the top has changed).
    if (sink_ && it->first == "Viewpoint" && it->second.top() == node)
      sink_(BindTransition::Pop);
    it->second.unbind(node, emit);
  }

  // Category of a bindable node: viewpoint family / background family / own type.
  static std::string category(X3DNode *n) {
    if (X3DInterfaceRegistry::nodeImplements(n, InterfaceId::X3DViewpointNode))
      return "Viewpoint";
    if (X3DInterfaceRegistry::nodeImplements(n, InterfaceId::X3DBackgroundNode))
      return "Background";
    return n->nodeTypeName();
  }

private:
  // Write an output field directly via reflection (bypasses the event cascade).
  template <typename T>
  static void setField(X3DNode *n, const char *name, T value) {
    for (const auto &f : n->fields())
      if (f.x3dName == name && f.set) { f.set(*n, std::any(T(value))); return; }
  }
  static void setIsBound(X3DNode *n, bool bound) { setField(n, "isBound", SFBool(bound)); }
  static void setBindTime(X3DNode *n, double t)  { setField(n, "bindTime", SFTime(t)); }

  void enroll(X3DNode *node) {
    auto *b = dynamic_cast<X3DBindableNode *>(node);
    if (!b) return;
    const std::string cat = category(node);
    enrolled_[cat].push_back(node);
    b->setOnSet_bindHandler([this, node, cat](const SFBool &v) {
      BindingStack::Emit emit = [this](X3DNode *t, bool bound) {
        // §23.3.1: bindTime fires on EVERY isBound transition (bind AND unbind).
        poster_(t, "isBound", std::any(SFBool(bound)));
        poster_(t, "bindTime", std::any(SFTime(clock_())));
      };
      if (v) {
        if (sink_ && cat == "Viewpoint") sink_(BindTransition::Push);
        stacks_[cat].bind(node, emit);
      } else {
        if (sink_ && cat == "Viewpoint" && stacks_[cat].top() == node)
          sink_(BindTransition::Pop);
        stacks_[cat].unbind(node, emit);
      }
    });
  }

  void walk(X3DNode *n) {
    enroll(n);
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) walk(c.get());
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get());
      }
    }
  }

  Poster poster_;
  Clock clock_;
  TransitionSink sink_;
  std::unordered_map<std::string, BindingStack> stacks_;
  std::unordered_map<std::string, std::vector<X3DNode *>> enrolled_;
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BINDING_SYSTEM_HPP
