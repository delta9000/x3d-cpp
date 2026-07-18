// X3DEnvironmentalSensorNode.hpp
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
  /**
   * @brief Default constructor for X3DEnvironmentalSensorNode
   */
  X3DEnvironmentalSensorNode() = default;

  /**
   * @brief Virtual destructor for X3DEnvironmentalSensorNode
   */
  virtual ~X3DEnvironmentalSensorNode() = default;

  /**
   * @brief Get the default value for size
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultSize() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "EnvironmentalSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of size. AccessType: inputOutput
   * @details
   * @return SFVec3f The current value of size.
   */
  SFVec3f getSize() const { return _size; }

  /**
   * @brief Sets the value of size. AccessType: inputOutput
   * @details
   * @param value The new value for size.
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
   * @brief Non-validating write of size (runtime/reader ingest path).
   * @details Assigns without the range check setSize() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSize() stays the
   *          enforcement point for programmatic callers.
   */
  void setSizeUnchecked(const SFVec3f &value) { _size = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DEnvironmentalSensorNode").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
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

  /**
   * @brief Member variable for size.
   */

  SFVec3f _size{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
