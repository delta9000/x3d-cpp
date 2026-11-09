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

#include "x3d/nodes/X3DInterpolatorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ColorInterpolator
 * @brief ColorInterpolator generates a range of color values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#ColorInterpolator
 */
class ColorInterpolator : public virtual X3DInterpolatorNode {
public:
  ColorInterpolator() = default;

  ~ColorInterpolator() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 2; }

  /**
   * MFColor; access: inputOutput. Borrows the stored value.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  const MFColor &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  void setKeyValue(const MFColor &value) {

    validateKeyValue(value);

    _keyValue = value;
  }

  void setKeyValue(MFColor &&value) {

    validateKeyValue(value);

    _keyValue = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setKeyValue() for range validation.
   */
  void setKeyValueUnchecked(const MFColor &value) { _keyValue = value; }

  /**
   * SFColor; access: outputOnly. Borrows the stored value.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   */
  const SFColor &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   */
  void emitValue_changed(const SFColor &value) { _value_changed = value; }

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
  static void checkRangesKeyValue(const MFColor &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateKeyValue(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0.0f)
        throw std::out_of_range("keyValue.r below minimum of 0");
      if (v.r > 1.0f)
        throw std::out_of_range("keyValue.r above maximum of 1");

      if (v.g < 0.0f)
        throw std::out_of_range("keyValue.g below minimum of 0");
      if (v.g > 1.0f)
        throw std::out_of_range("keyValue.g above maximum of 1");

      if (v.b < 0.0f)
        throw std::out_of_range("keyValue.b below minimum of 0");
      if (v.b > 1.0f)
        throw std::out_of_range("keyValue.b above maximum of 1");
    }
  }

  MFColor _keyValue{};

  SFColor _value_changed{};
};

} // namespace x3d::nodes
