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
 * @class BoundedPhysicsModel
 * @brief BoundedPhysicsModel provides user-defined geometrical boundaries for
 * particle motion.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#BoundedPhysicsModel
 */
class BoundedPhysicsModel : public virtual X3DParticlePhysicsModelNode {
public:
  BoundedPhysicsModel() = default;

  ~BoundedPhysicsModel() = default;

  static SFNode getDefaultGeometry() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "physics"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained geometry node provides the geometry used for each
   * particle when the parent ParticleSystem node has geometryType=GEOMETRY.
   */
  const SFNode &getGeometry() const { return _geometry; }

  /**
   * @details Permitted X3D node types: X3DGeometryNode
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained geometry node provides the geometry used for each
   * particle when the parent ParticleSystem node has geometryType=GEOMETRY.
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

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
  SFNode _geometry{nullptr};
};

} // namespace x3d::nodes
