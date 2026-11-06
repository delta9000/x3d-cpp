#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DEnvironmentalSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class VisibilitySensor
 * @brief VisibilitySensor detects when user can see a specific object or region
 * as they navigate the world.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html#VisibilitySensor
 */
class VisibilitySensor : public virtual X3DEnvironmentalSensorNode {
public:
  VisibilitySensor() = default;

  ~VisibilitySensor() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalSensor"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when user's camera enters visibility region
   * for sensor.
   */
  const SFTime &getEnterTime() const { return _enterTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when user's camera enters visibility region
   * for sensor.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when user's camera exits visibility region
   * for sensor.
   */
  const SFTime &getExitTime() const { return _exitTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when user's camera exits visibility region
   * for sensor.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

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
  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFTime _enterTime{};

  SFTime _exitTime{};
};

} // namespace x3d::nodes
