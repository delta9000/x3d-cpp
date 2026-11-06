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
  Color() = default;

  ~Color() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesColor";
  }

  static std::string getDefaultContainerField() { return "color"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 1; }

  /**
   * MFColor; access: inputOutput. Borrows the stored value.
   * @details The color field defines an array of 3-tuple RGB colors.
   */
  const MFColor &getColor() const { return _color; }

  /**
   * Access: inputOutput.
   * @details The color field defines an array of 3-tuple RGB colors.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setColor() for range validation.
   */
  void setColorUnchecked(const MFColor &value) { _color = value; }

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

  MFColor _color{};
};

} // namespace x3d::nodes
