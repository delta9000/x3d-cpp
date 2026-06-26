// InterpolatorRegistration.hpp
// One call wires every interpolator System into an execution context. This is
// the registry-of-behavior seam that replaces 8 separate per-type files.
// makeInterpolatorSystems() is the single source of truth for the system list
// (the 8 linear interpolators + the 5 non-linear ones: 3 spline + squad + ease);
// registerInterpolatorSystems() add-only-registers them, and X3DSceneBridge's
// attachInterpolators() (the PIV-1 production caller) attaches them to every node.
#ifndef X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP
#define X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP

#include "InterpolatorSystem.hpp"
#include "SplineInterpolatorSystem.hpp"
#include "X3DExecutionContext.hpp"

#include "ColorInterpolator.hpp"
#include "CoordinateInterpolator.hpp"
#include "CoordinateInterpolator2D.hpp"
#include "EaseInEaseOut.hpp"
#include "NormalInterpolator.hpp"
#include "OrientationInterpolator.hpp"
#include "PositionInterpolator.hpp"
#include "PositionInterpolator2D.hpp"
#include "ScalarInterpolator.hpp"
#include "SplinePositionInterpolator.hpp"
#include "SplinePositionInterpolator2D.hpp"
#include "SplineScalarInterpolator.hpp"
#include "SquadOrientationInterpolator.hpp"

#include <memory>
#include <vector>

namespace x3d::runtime {

/// Build the full interpolator System set (single source of truth for the list).
inline std::vector<std::shared_ptr<System>> makeInterpolatorSystems() {
  std::vector<std::shared_ptr<System>> systems;
  // Linear interpolators (§19.4.1-9).
  systems.push_back(std::make_shared<InterpolatorSystem<ScalarInterpolator, float>>(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); }));
  systems.push_back(std::make_shared<InterpolatorSystem<PositionInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); }));
  systems.push_back(std::make_shared<InterpolatorSystem<PositionInterpolator2D, SFVec2f>>(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); }));
  systems.push_back(std::make_shared<InterpolatorSystem<ColorInterpolator, SFColor>>(
      [](const SFColor &a, const SFColor &b, float t) { return lerpColorHsv(a, b, t); }));
  systems.push_back(std::make_shared<InterpolatorSystem<OrientationInterpolator, SFRotation>>(
      [](const SFRotation &a, const SFRotation &b, float t) { return slerpRotation(a, b, t); }));
  systems.push_back(std::make_shared<MultiInterpolatorSystem<CoordinateInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); }));
  systems.push_back(std::make_shared<MultiInterpolatorSystem<CoordinateInterpolator2D, SFVec2f>>(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); }));
  systems.push_back(std::make_shared<MultiInterpolatorSystem<NormalInterpolator, SFVec3f>>(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return slerpNormal(a, b, t); }));
  // Non-linear interpolators (§19.2.4 Hermite spline, §19.4.13 Squad) + the
  // §19.4.4 EaseInEaseOut fraction modifier (INTERP-01).
  systems.push_back(std::make_shared<SplineInterpolatorSystem<SplinePositionInterpolator, SFVec3f>>());
  systems.push_back(std::make_shared<SplineInterpolatorSystem<SplinePositionInterpolator2D, SFVec2f>>());
  systems.push_back(std::make_shared<SplineInterpolatorSystem<SplineScalarInterpolator, float>>());
  systems.push_back(std::make_shared<SquadOrientationInterpolatorSystem>());
  systems.push_back(std::make_shared<EaseInEaseOutSystem>());
  return systems;
}

/// Register every interpolator System (add-only; attach per node separately, or
/// use attachInterpolators() for the production scene-walk wiring).
inline void registerInterpolatorSystems(X3DExecutionContext &ctx) {
  for (auto &s : makeInterpolatorSystems()) ctx.addSystem(std::move(s));
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INTERPOLATOR_REGISTRATION_HPP
