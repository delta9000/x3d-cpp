// X3DActiveNode.hpp
// The per-tick update protocol for time-driven node behaviors.
#ifndef X3D_RUNTIME_ACTIVE_NODE_HPP
#define X3D_RUNTIME_ACTIVE_NODE_HPP

namespace x3d::runtime {

class X3DExecutionContext;

/**
 * @brief A node behavior that needs to run every frame (e.g. TimeSensor).
 * @details The browser/runtime registers active nodes with an
 *          X3DExecutionContext; each `tick(now)` calls `update`, in which the
 *          behavior reads its node and the current time and emits output events
 *          into the context (which then cascades them). Purely event-driven
 *          behaviors (interpolators, filters) do NOT implement this — they
 *          register an inputOnly handler instead and react to incoming events.
 *
 *          This is the extension seam for the browser's active-node families;
 *          the foundation ships the protocol, concrete behaviors layer on top.
 */
class ActiveNode {
public:
  virtual ~ActiveNode() = default;

  /**
   * @brief Advance this behavior to time `now`, emitting events into `ctx`.
   */
  virtual void update(double now, X3DExecutionContext &ctx) = 0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_ACTIVE_NODE_HPP
