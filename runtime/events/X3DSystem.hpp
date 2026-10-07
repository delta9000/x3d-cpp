// X3DSystem.hpp
// The System abstraction: a behavior family operating over a *collection* of
// nodes of one kind. Generalizes the per-node `ActiveNode` (see
// X3DActiveNode.hpp) so a browser registers one System per behavior family
// rather than one object per node.
//
// Two styles, both supported by the same base:
//   - Time-driven systems (e.g. TimeSensor) do their work in `update(now,
//   ctx)`,
//     which the context calls every tick before draining the cascade.
//   - Event-driven systems (the interpolator family) wire their inputOnly
//     handler in `attach(node, ctx)` and leave `update` a no-op; they react to
//     incoming events instead of the clock.
//
// A System owning its nodes contiguously is the seam for future data-oriented
// batching (iterate the node array in update()) without touching the node
// model.
#ifndef X3D_RUNTIME_SYSTEM_HPP
#define X3D_RUNTIME_SYSTEM_HPP

#include "CallbackLifetime.hpp"

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {
using x3d::nodes::X3DNode;

class X3DExecutionContext;

/**
 * @brief A behavior family operating over a collection of nodes of one kind.
 * @details The unit a browser registers with an X3DExecutionContext (via
 *          `addSystem`). `attach` enrolls one node — event-driven systems
 *          register their inputOnly handler there; time-driven systems record
 *          the node for per-tick processing. `update` is called every tick for
 *          time-driven systems and defaults to a no-op for event-driven ones.
 */
class System {
public:
  System() = default;
  System(const System &) = delete;
  System &operator=(const System &) = delete;
  System(System &&) = delete;
  System &operator=(System &&) = delete;
  virtual ~System() { retireCallbacksBeforeDestruction(); }

protected:
  /// A callback-owning most-derived destructor MUST call this before its
  /// members start destruction. Base destruction alone is too late if member
  /// captures/destructors synchronously invoke a retained node handler.
  /// Standard callback-owning systems do so; custom subclasses must do likewise.
  void retireCallbacksBeforeDestruction() noexcept {
    if (!callbacks_.retire()) std::terminate();
  }

private:
  friend class X3DExecutionContext;
  CallbackLifetime callbacks_;

public:

  /**
   * @brief Enroll a node in this system (call after routes are built).
   * @details Event-driven systems register their inputOnly handler here;
   *          time-driven systems stash the node for `update`.
   */
  virtual void attach(X3DNode *node, X3DExecutionContext &ctx) = 0;

  /// Release per-node state before a live Inline removes this node (§9.4.2).
  virtual void detach(X3DNode *, X3DExecutionContext &) {}

  /**
   * @brief Advance time-driven nodes to `now`, emitting events into `ctx`.
   * @details Default no-op: event-driven systems do all their work from the
   *          handler wired in `attach`.
   */
  virtual void update(double now, X3DExecutionContext &ctx) {
    (void)now;
    (void)ctx;
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SYSTEM_HPP
