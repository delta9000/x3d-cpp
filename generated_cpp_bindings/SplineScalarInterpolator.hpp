// SplineScalarInterpolator.hpp
#ifndef SPLINESCALARINTERPOLATOR_HPP
#define SPLINESCALARINTERPOLATOR_HPP

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
 * @class SplineScalarInterpolator
 * @brief SplineScalarInterpolator performs non-linear interpolation among
 * paired lists of float values and velocities to produce an SFFloat
 * value_changed output event.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#SplineScalarInterpolator
 */
class SplineScalarInterpolator : public virtual X3DInterpolatorNode {
public:
  /**
   * @brief Default constructor for SplineScalarInterpolator
   */
  SplineScalarInterpolator() = default;

  /**
   * @brief Destructor for SplineScalarInterpolator
   */
  ~SplineScalarInterpolator() = default;

  /**
   * @brief Get the default value for closed
   * @return SFBool The default value
   */
  static SFBool getDefaultClosed() { return false; }

  /**
   * @brief Get the default value for normalizeVelocity
   * @return SFBool The default value
   */
  static SFBool getDefaultNormalizeVelocity() { return false; }

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
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of closed. AccessType: inputOutput
   * @details Whether or not the curve is closed (i.
   * @return SFBool The current value of closed.
   */
  SFBool getClosed() const { return _closed; }

  /**
   * @brief Sets the value of closed. AccessType: inputOutput
   * @details Whether or not the curve is closed (i.
   * @param value The new value for closed.
   */
  void setClosed(const SFBool &value) { _closed = value; }

  /**
   * @brief Gets the value of keyValue. AccessType: inputOutput
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   * @return MFFloat The current value of keyValue.
   */
  MFFloat getKeyValue() const { return _keyValue; }

  /**
   * @brief Sets the value of keyValue. AccessType: inputOutput
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   * @param value The new value for keyValue.
   */
  void setKeyValue(const MFFloat &value) { _keyValue = value; }

  void setKeyValue(MFFloat &&value) { _keyValue = std::move(value); }

  /**
   * @brief Gets the value of keyVelocity. AccessType: inputOutput
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   * @return MFFloat The current value of keyVelocity.
   */
  MFFloat getKeyVelocity() const { return _keyVelocity; }

  /**
   * @brief Sets the value of keyVelocity. AccessType: inputOutput
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   * @param value The new value for keyVelocity.
   */
  void setKeyVelocity(const MFFloat &value) { _keyVelocity = value; }

  void setKeyVelocity(MFFloat &&value) { _keyVelocity = std::move(value); }

  /**
   * @brief Gets the value of normalizeVelocity. AccessType: inputOutput
   * @details normalizeVelocity field specifies whether the velocity vectors are
   * normalized to produce smooth speed transitions, or transformed into
   * tangency vectors.
   * @return SFBool The current value of normalizeVelocity.
   */
  SFBool getNormalizeVelocity() const { return _normalizeVelocity; }

  /**
   * @brief Sets the value of normalizeVelocity. AccessType: inputOutput
   * @details normalizeVelocity field specifies whether the velocity vectors are
   * normalized to produce smooth speed transitions, or transformed into
   * tangency vectors.
   * @param value The new value for normalizeVelocity.
   */
  void setNormalizeVelocity(const SFBool &value) { _normalizeVelocity = value; }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values.
   * @return SFFloat The current value of value_changed.
   */
  SFFloat getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFFloat &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "SplineScalarInterpolator").
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
   * @brief Member variable for closed.
   */

  SFBool _closed{false};

  /**
   * @brief Member variable for keyValue.
   */

  MFFloat _keyValue{};

  /**
   * @brief Member variable for keyVelocity.
   */

  MFFloat _keyVelocity{};

  /**
   * @brief Member variable for normalizeVelocity.
   */

  SFBool _normalizeVelocity{false};

  /**
   * @brief Member variable for value_changed.
   */

  SFFloat _value_changed{};
};

#endif // SPLINESCALARINTERPOLATOR_HPP