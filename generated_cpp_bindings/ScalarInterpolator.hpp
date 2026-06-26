// ScalarInterpolator.hpp
#ifndef SCALARINTERPOLATOR_HPP
#define SCALARINTERPOLATOR_HPP

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
 * @class ScalarInterpolator
 * @brief ScalarInterpolator generates piecewise-linear SFFloat values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#ScalarInterpolator
 */
class ScalarInterpolator : public virtual X3DInterpolatorNode {
public:
  /**
   * @brief Default constructor for ScalarInterpolator
   */
  ScalarInterpolator() = default;

  /**
   * @brief Destructor for ScalarInterpolator
   */
  ~ScalarInterpolator() = default;

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @return MFFloat The current value of keyValue.
   */
  MFFloat getKeyValue() const { return _keyValue; }

  /**
   * @brief Sets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @param value The new value for keyValue.
   */
  void setKeyValue(const MFFloat &value) { _keyValue = value; }

  void setKeyValue(MFFloat &&value) { _keyValue = std::move(value); }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   * @return SFFloat The current value of value_changed.
   */
  SFFloat getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFFloat &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ScalarInterpolator").
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

private:
  /**
   * @brief Member variable for keyValue.
   */

  MFFloat _keyValue{};

  /**
   * @brief Member variable for value_changed.
   */

  SFFloat _value_changed{};
};

#endif // SCALARINTERPOLATOR_HPP