// FollowerRegistration.hpp
// Factory: builds all 14 follower systems (7 types × Damper + Chaser) and
// provides the production wiring entry-point attachFollowers(), mirroring
// attachInterpolators() in X3DSceneBridge.hpp.
#ifndef X3D_RUNTIME_FOLLOWER_REGISTRATION_HPP
#define X3D_RUNTIME_FOLLOWER_REGISTRATION_HPP

#include "FollowerSystem.hpp"
#include "X3DExecutionContext.hpp"

// node headers — 14 follower nodes
#include "x3d/nodes/ColorChaser.hpp"
#include "x3d/nodes/ColorDamper.hpp"
#include "x3d/nodes/CoordinateChaser.hpp"
#include "x3d/nodes/CoordinateDamper.hpp"
#include "x3d/nodes/OrientationChaser.hpp"
#include "x3d/nodes/OrientationDamper.hpp"
#include "x3d/nodes/PositionChaser.hpp"
#include "x3d/nodes/PositionChaser2D.hpp"
#include "x3d/nodes/PositionDamper.hpp"
#include "x3d/nodes/PositionDamper2D.hpp"
#include "x3d/nodes/ScalarChaser.hpp"
#include "x3d/nodes/ScalarDamper.hpp"
#include "x3d/nodes/TexCoordChaser2D.hpp"
#include "x3d/nodes/TexCoordDamper2D.hpp"

#include <memory>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Build one System per follower node type (14 total: 7 types × Damper + Chaser).
 *
 * Value-type mapping (verified against each header's getInitialValue() return type):
 *   ScalarDamper / ScalarChaser         → float
 *   PositionDamper / PositionChaser     → SFVec3f
 *   PositionDamper2D / PositionChaser2D → SFVec2f
 *   ColorDamper / ColorChaser           → SFColor
 *   OrientationDamper / OrientationChaser → SFRotation
 *   CoordinateDamper / CoordinateChaser → MFVec3f
 *   TexCoordDamper2D / TexCoordChaser2D → MFVec2f
 */
inline std::vector<std::shared_ptr<System>> makeFollowerSystems() {
  std::vector<std::shared_ptr<System>> s;
  s.push_back(std::make_shared<DamperSystem<ScalarDamper, float>>());
  s.push_back(std::make_shared<ChaserSystem<ScalarChaser, float>>());
  s.push_back(std::make_shared<DamperSystem<PositionDamper, SFVec3f>>());
  s.push_back(std::make_shared<ChaserSystem<PositionChaser, SFVec3f>>());
  s.push_back(std::make_shared<DamperSystem<PositionDamper2D, SFVec2f>>());
  s.push_back(std::make_shared<ChaserSystem<PositionChaser2D, SFVec2f>>());
  s.push_back(std::make_shared<DamperSystem<ColorDamper, SFColor>>());
  s.push_back(std::make_shared<ChaserSystem<ColorChaser, SFColor>>());
  s.push_back(std::make_shared<DamperSystem<OrientationDamper, SFRotation>>());
  s.push_back(std::make_shared<ChaserSystem<OrientationChaser, SFRotation>>());
  s.push_back(std::make_shared<DamperSystem<CoordinateDamper, MFVec3f>>());
  s.push_back(std::make_shared<ChaserSystem<CoordinateChaser, MFVec3f>>());
  s.push_back(std::make_shared<DamperSystem<TexCoordDamper2D, MFVec2f>>());
  s.push_back(std::make_shared<ChaserSystem<TexCoordChaser2D, MFVec2f>>());
  return s;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_FOLLOWER_REGISTRATION_HPP
