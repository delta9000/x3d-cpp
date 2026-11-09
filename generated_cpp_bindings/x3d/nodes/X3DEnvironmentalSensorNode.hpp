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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DEnvironmentalSensorNode
 * @brief Base type for the environmental sensor nodes ProximitySensor,
 * TransformSensor and VisibilitySensor.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html#X3DEnvironmentalSensorNode
 */
class X3DEnvironmentalSensorNode : public virtual X3DSensorNode {
public:
  X3DEnvironmentalSensorNode() = default;

  virtual ~X3DEnvironmentalSensorNode() = default;

  static SFVec3f getDefaultSize() { return SFVec3f{0, 0, 0}; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFVec3f &getSize() const { return _size; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setSize(const SFVec3f &value) {

    validateSize(value);

    _size = value;
  }

  void setSize(SFVec3f &&value) {

    validateSize(value);

    _size = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSize() for range validation.
   */
  void setSizeUnchecked(const SFVec3f &value) { _size = value; }

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

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSize(const SFVec3f &value, const std::string &nodeType,
                              const std::string &defName,
                              std::vector<RangeDiagnostic> &out);

private:
  static void validateSize(const SFVec3f &value) {

    if (value.x < 0.0f)
      throw std::out_of_range("size.x below minimum of 0");

    if (value.y < 0.0f)
      throw std::out_of_range("size.y below minimum of 0");

    if (value.z < 0.0f)
      throw std::out_of_range("size.z below minimum of 0");
  }

  SFVec3f _size{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
