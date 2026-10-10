// InterpolatorSystem.hpp
// Two templated event-driven Systems replacing the 8 per-type interpolator
// systems. Single-value family (Scalar/Position/Position2D/Color/Orientation):
// InterpolatorSystem<NodeT, ValueT>. Multi-value family
// (Coordinate/Coordinate2D/Normal): MultiInterpolatorSystem<NodeT, ElemT>.
// Each is constructed with the per-type lerp function. The wiring
// (set_fraction handler -> value_changed via the cascade) is identical across
// all of them, which is exactly why they collapse.
#ifndef X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP
#define X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP

#include "Interpolation.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include <any>
#include <functional>
#include <utility>

namespace x3d::runtime {

/// Single-value interpolator: keyValue is std::vector<ValueT>, output ValueT.
template <typename NodeT, typename ValueT>
class InterpolatorSystem : public System {
public:
  ~InterpolatorSystem() override { retireCallbacksBeforeDestruction(); }

  using LerpFn = std::function<ValueT(const ValueT &, const ValueT &, float)>;
  explicit InterpolatorSystem(LerpFn lerp) : lerp_(std::move(lerp)) {}

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<NodeT *>(node);
    if (!interp) return;
    // §19.3.1: readback before set_fraction starts at keyValue[0].
    if (!interp->getKeyValue().empty())
      interp->emitValue_changed(interp->getKeyValue().front());
    LerpFn lerp = lerp_;
    interp->setOnSet_fractionHandler(
        ctx.guardCallback(*this, [&ctx, interp, lerp](const SFFloat &fraction) {
          // §19.3.1: an interpolator with no keys shall produce no events
          // (INTERP-02). Checked live so a later non-empty key re-enables it.
          if (interp->getKey().empty()) return;
          ctx.postOutputEvent(interp, "value_changed",
                        std::any(interpolateValue(interp->getKey(),
                                                  interp->getKeyValue(),
                                                  fraction, lerp)));
        }));
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *n = dynamic_cast<NodeT *>(node)) n->setOnSet_fractionHandler({});
  }

private:
  LerpFn lerp_;
};

/// Multi-value interpolator: keyValue is a flat std::vector<ElemT> reshaped by
/// numKeys; output std::vector<ElemT>.
template <typename NodeT, typename ElemT>
class MultiInterpolatorSystem : public System {
public:
  ~MultiInterpolatorSystem() override { retireCallbacksBeforeDestruction(); }

  using LerpFn = std::function<ElemT(const ElemT &, const ElemT &, float)>;
  explicit MultiInterpolatorSystem(LerpFn lerp) : lerp_(std::move(lerp)) {}

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<NodeT *>(node);
    if (!interp) return;
    // §19.3.1: initialize the first value_changed row without an event.
    const auto &values = interp->getKeyValue();
    if (!values.empty()) {
      const std::size_t width = interp->getKey().empty()
                                    ? 1 : values.size() / interp->getKey().size();
      interp->emitValue_changed(std::vector<ElemT>(values.begin(), values.begin() + width));
    }
    LerpFn lerp = lerp_;
    interp->setOnSet_fractionHandler(
        ctx.guardCallback(*this, [&ctx, interp, lerp](const SFFloat &fraction) {
          // §19.3.1: no keys -> no events (INTERP-02).
          if (interp->getKey().empty()) return;
          ctx.postOutputEvent(interp, "value_changed",
                        std::any(interpolateMulti(interp->getKey(),
                                                  interp->getKeyValue(),
                                                  fraction, lerp)));
        }));
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *n = dynamic_cast<NodeT *>(node)) n->setOnSet_fractionHandler({});
  }

private:
  LerpFn lerp_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INTERPOLATOR_SYSTEM_HPP
