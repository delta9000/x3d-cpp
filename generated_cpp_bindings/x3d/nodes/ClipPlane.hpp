// ClipPlane.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ClipPlane
 * @brief ClipPlane specifies a single plane equation used to clip (i.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#ClipPlane
 */
class ClipPlane : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for ClipPlane
   */
  ClipPlane() = default;

  /**
   * @brief Destructor for ClipPlane
   */
  ~ClipPlane() = default;

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for plane
   * @return SFVec4f The default value
   */
  static SFVec4f getDefaultPlane() { return SFVec4f{0, 1, 0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 5; }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of plane. AccessType: inputOutput
   * @details If (a,b,c,d) is the plane, with the first three components being a
   * normalized vector describing the plane's normal direction (and thus the
   * fourth component d being distance from the origin), a point (x,y,z) is
   * visible to the user, with regards to the clipping plane, if a*x+b*y+c*z+d
   * is greater than 0.
   * @return SFVec4f The current value of plane.
   */
  SFVec4f getPlane() const { return _plane; }

  /**
   * @brief Sets the value of plane. AccessType: inputOutput
   * @details If (a,b,c,d) is the plane, with the first three components being a
   * normalized vector describing the plane's normal direction (and thus the
   * fourth component d being distance from the origin), a point (x,y,z) is
   * visible to the user, with regards to the clipping plane, if a*x+b*y+c*z+d
   * is greater than 0.
   * @param value The new value for plane.
   */
  void setPlane(const SFVec4f &value) {

    validatePlane(value);

    _plane = value;
  }

  void setPlane(SFVec4f &&value) {

    validatePlane(value);

    _plane = std::move(value);
  }

  /**
   * @brief Non-validating write of plane (runtime/reader ingest path).
   * @details Assigns without the range check setPlane() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setPlane() stays the
   *          enforcement point for programmatic callers.
   */
  void setPlaneUnchecked(const SFVec4f &value) { _plane = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ClipPlane").
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
  static void checkRangesPlane(const SFVec4f &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validatePlane(const SFVec4f &value) {

    if (value.x < -1)
      throw std::out_of_range("plane.x below minimum of -1");
    if (value.x > 1)
      throw std::out_of_range("plane.x above maximum of 1");

    if (value.y < -1)
      throw std::out_of_range("plane.y below minimum of -1");
    if (value.y > 1)
      throw std::out_of_range("plane.y above maximum of 1");

    if (value.z < -1)
      throw std::out_of_range("plane.z below minimum of -1");
    if (value.z > 1)
      throw std::out_of_range("plane.z above maximum of 1");

    if (value.w < -1)
      throw std::out_of_range("plane.w below minimum of -1");
    if (value.w > 1)
      throw std::out_of_range("plane.w above maximum of 1");
  }

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for plane.
   */

  SFVec4f _plane{SFVec4f{0, 1, 0, 0}};
};

} // namespace x3d::nodes
