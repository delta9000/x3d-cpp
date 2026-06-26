// SplineInterpolatorSystem.hpp
// Event-driven Systems for the non-linear interpolator family (INTERP-01):
//   - SplineInterpolatorSystem<NodeT, ValueT>: the §19.2.4 Hermite spline,
//     shared by SplinePositionInterpolator (SFVec3f), SplinePositionInterpolator2D
//     (SFVec2f), and SplineScalarInterpolator (float). Reads the node's closed /
//     keyVelocity / normalizeVelocity fields directly.
//   - SquadOrientationInterpolatorSystem: §19.4.13 Squad on rotations.
//   - EaseInEaseOutSystem: §19.4.4 fraction modifier (emits modifiedFraction_changed).
// Each wires the node's inputOnly handler in attach(), mirroring InterpolatorSystem.
// Empty-key emits no event for the interpolator nodes (§19.3.1, INTERP-02).
#ifndef X3D_RUNTIME_SPLINE_INTERPOLATOR_SYSTEM_HPP
#define X3D_RUNTIME_SPLINE_INTERPOLATOR_SYSTEM_HPP

#include "SplineInterpolation.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "EaseInEaseOut.hpp"
#include "SquadOrientationInterpolator.hpp"

#include <any>

namespace x3d::runtime {

/// Hermite spline interpolator (keyValue is std::vector<ValueT>, output ValueT).
template <typename NodeT, typename ValueT>
class SplineInterpolatorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<NodeT *>(node);
    if (!interp) return;
    interp->setOnSet_fractionHandler([&ctx, interp](const SFFloat &fraction) {
      if (interp->getKey().empty()) return; // §19.3.1 (INTERP-02)
      ctx.postEvent(interp, "value_changed",
                    std::any(hermiteSpline<ValueT>(
                        interp->getKey(), interp->getKeyValue(),
                        interp->getKeyVelocity(), interp->getClosed(),
                        interp->getNormalizeVelocity(), fraction)));
    });
  }
};

/// §19.4.13 Squad orientation interpolator.
class SquadOrientationInterpolatorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *interp = dynamic_cast<SquadOrientationInterpolator *>(node);
    if (!interp) return;
    interp->setOnSet_fractionHandler([&ctx, interp](const SFFloat &fraction) {
      if (interp->getKey().empty()) return; // §19.3.1 (INTERP-02)
      ctx.postEvent(
          interp, "value_changed",
          std::any(squadOrientation(interp->getKey(), interp->getKeyValue(),
                                    fraction)));
    });
  }
};

/// §19.4.4 EaseInEaseOut fraction modifier.
class EaseInEaseOutSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *ease = dynamic_cast<EaseInEaseOut *>(node);
    if (!ease) return;
    ease->setOnSet_fractionHandler([&ctx, ease](const SFFloat &fraction) {
      ctx.postEvent(ease, "modifiedFraction_changed",
                    std::any(SFFloat{easeInEaseOut(
                        ease->getKey(), ease->getEaseInEaseOut(), fraction)}));
    });
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SPLINE_INTERPOLATOR_SYSTEM_HPP
