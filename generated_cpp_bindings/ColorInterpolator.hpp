// ColorInterpolator.hpp
#ifndef COLORINTERPOLATOR_HPP
#define COLORINTERPOLATOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DInterpolatorNode.hpp"

/**
 * @class ColorInterpolator
 * @brief ColorInterpolator generates a range of color values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#ColorInterpolator
 */
class ColorInterpolator : public virtual X3DInterpolatorNode {
public:
  /**
   * @brief Default constructor for ColorInterpolator
   */
  ColorInterpolator() = default;

  /**
   * @brief Destructor for ColorInterpolator
   */
  ~ColorInterpolator() = default;

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
  static std::string componentName() { return "Interpolation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @return MFColor The current value of keyValue.
   */
  MFColor getKeyValue() const { return _keyValue; }

  /**
   * @brief Sets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @param value The new value for keyValue.
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
   * @brief Non-validating write of keyValue (runtime/reader ingest path).
   * @details Assigns without the range check setKeyValue() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setKeyValue() stays the
   *          enforcement point for programmatic callers.
   */
  void setKeyValueUnchecked(const MFColor &value) { _keyValue = value; }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   * @return SFColor The current value of value_changed.
   */
  SFColor getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFColor &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ColorInterpolator").
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
  static void checkRangesKeyValue(const MFColor &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateKeyValue(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0)
        throw std::out_of_range("keyValue.r below minimum of 0");
      if (v.r > 1)
        throw std::out_of_range("keyValue.r above maximum of 1");

      if (v.g < 0)
        throw std::out_of_range("keyValue.g below minimum of 0");
      if (v.g > 1)
        throw std::out_of_range("keyValue.g above maximum of 1");

      if (v.b < 0)
        throw std::out_of_range("keyValue.b below minimum of 0");
      if (v.b > 1)
        throw std::out_of_range("keyValue.b above maximum of 1");
    }
  }

  /**
   * @brief Member variable for keyValue.
   */

  MFColor _keyValue{};

  /**
   * @brief Member variable for value_changed.
   */

  SFColor _value_changed{};
};

#endif // COLORINTERPOLATOR_HPP