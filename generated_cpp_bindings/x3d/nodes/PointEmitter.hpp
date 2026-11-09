#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DParticleEmitterNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PointEmitter
 * @brief PointEmitter generates particles from a specific point in space using
 * the specified direction and speed.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#PointEmitter
 */
class PointEmitter : public virtual X3DParticleEmitterNode {
public:
  PointEmitter() = default;

  ~PointEmitter() = default;

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 1, 0}; }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "emitter"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Initial direction from which particles emanate.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details Initial direction from which particles emanate.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Point from which particles emanate.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details Point from which particles emanate.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFVec3f _direction{SFVec3f{0, 1, 0}};

  SFVec3f _position{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
