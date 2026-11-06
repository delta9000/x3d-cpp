#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DParticlePhysicsModelNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ForcePhysicsModel
 * @brief ForcePhysicsModel applies a constant force value to the particles.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#ForcePhysicsModel
 */
class ForcePhysicsModel : public virtual X3DParticlePhysicsModelNode {
public:
  ForcePhysicsModel() = default;

  ~ForcePhysicsModel() = default;

  static SFVec3f getDefaultForce() { return SFVec3f{0, -9.8, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "physics"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details force field indicates strength and direction of the propelling
   * force on the particles (for example, default is Earth's gravity).
   */
  const SFVec3f &getForce() const { return _force; }

  /**
   * Access: inputOutput.
   * @details force field indicates strength and direction of the propelling
   * force on the particles (for example, default is Earth's gravity).
   */
  void setForce(const SFVec3f &value) { _force = value; }

  void setForce(SFVec3f &&value) { _force = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  SFVec3f _force{SFVec3f{0, -9.8, 0}};
};

} // namespace x3d::nodes
