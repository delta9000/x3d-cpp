// BindingStack.hpp — one bindable category's stack. Top = bound node. Transitions
// are reported through an Emit callback (decoupled from the context). The X3D
// binding protocol (ISO/IEC 19775-1 7.2.2). namespace x3d::runtime.
#ifndef X3D_RUNTIME_BINDING_STACK_HPP
#define X3D_RUNTIME_BINDING_STACK_HPP

#include <algorithm>
#include <functional>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {
using x3d::nodes::X3DNode;

// BIND-09: kind of the latest viewpoint bind transition. Set by BindingSystem
// when a viewpoint stack mutation emits a bound/unbound transition; read by
// ViewpointBindSystem to distinguish §23.3.1 r5.1 (push: apply jump/retain)
// from r6.3 (pop: restore stored offset). Reset to None after each post-cascade.
enum class BindTransition { None, Push, Pop };

class BindingStack {
public:
  // emit(node, true)  => node became bound   (caller posts isBound TRUE  + bindTime)
  // emit(node, false) => node became unbound (caller posts isBound FALSE + bindTime)
  // §23.3.1: bindTime fires on every isBound transition, including unbind.
  using Emit = std::function<void(X3DNode *, bool)>;

  X3DNode *top() const { return stack_.empty() ? nullptr : stack_.back(); }

  // set_bind TRUE: move/push node to top.
  void bind(X3DNode *node, const Emit &emit) {
    if (!stack_.empty() && stack_.back() == node) return; // already bound
    X3DNode *prev = top();
    remove(node);
    stack_.push_back(node);
    if (prev && prev != node) emit(prev, false);
    emit(node, true);
  }

  // set_bind FALSE: pop if top (next becomes bound), else just remove.
  void unbind(X3DNode *node, const Emit &emit) {
    if (stack_.empty()) return;
    if (stack_.back() == node) {
      stack_.pop_back();
      emit(node, false);
      if (X3DNode *nt = top()) emit(nt, true);
    } else {
      remove(node); // non-top removal: no bound transition
    }
  }

  // Startup default-bind: push + announce bound (no previous-top semantics).
  void pushDefault(X3DNode *node, const Emit &emit) {
    stack_.push_back(node);
    emit(node, true);
  }

private:
  void remove(X3DNode *node) {
    stack_.erase(std::remove(stack_.begin(), stack_.end(), node), stack_.end());
  }
  std::vector<X3DNode *> stack_;
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BINDING_STACK_HPP
