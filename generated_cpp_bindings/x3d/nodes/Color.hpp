// Color.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

#include "x3d/nodes/X3DColorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Color
 * @brief Color node defines a set of RGB color values that apply either to a
 * sibling Coordinate|CoordinateDouble node, or else to a parent ElevationGrid
 * node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#Color
 */
class Color : public virtual X3DColorNode {
public:
  /**
   * @brief Default constructor for Color
   */
  Color() = default;

  /**
   * @brief Destructor for Color
   */
  ~Color() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesColor";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "color"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of color. AccessType: inputOutput
   * @details The color field defines an array of 3-tuple RGB colors.
   * @return MFColor The current value of color.
   */
  MFColor getColor() const { return _color; }

  /**
   * @brief Sets the value of color. AccessType: inputOutput
   * @details The color field defines an array of 3-tuple RGB colors.
   * @param value The new value for color.
   */
  void setColor(const MFColor &value) {

    validateColor(value);

    _color = value;
  }

  void setColor(MFColor &&value) {

    validateColor(value);

    _color = std::move(value);
  }

  /**
   * @brief Non-validating write of color (runtime/reader ingest path).
   * @details Assigns without the range check setColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setColorUnchecked(const MFColor &value) { _color = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Color").
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
  static void checkRangesColor(const MFColor &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateColor(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0.0f)
        throw std::out_of_range("color.r below minimum of 0");
      if (v.r > 1.0f)
        throw std::out_of_range("color.r above maximum of 1");

      if (v.g < 0.0f)
        throw std::out_of_range("color.g below minimum of 0");
      if (v.g > 1.0f)
        throw std::out_of_range("color.g above maximum of 1");

      if (v.b < 0.0f)
        throw std::out_of_range("color.b below minimum of 0");
      if (v.b > 1.0f)
        throw std::out_of_range("color.b above maximum of 1");
    }
  }

  /**
   * @brief Member variable for color.
   */

  MFColor _color{};
};

} // namespace x3d::nodes
