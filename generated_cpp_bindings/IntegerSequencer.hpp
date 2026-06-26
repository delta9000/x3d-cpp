// IntegerSequencer.hpp
#ifndef INTEGERSEQUENCER_HPP
#define INTEGERSEQUENCER_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSequencerNode.hpp"

/**
 * @class IntegerSequencer
 * @brief IntegerSequencer generates periodic discrete integer values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#IntegerSequencer
 */
class IntegerSequencer : public virtual X3DSequencerNode {
public:
  /**
   * @brief Default constructor for IntegerSequencer
   */
  IntegerSequencer() = default;

  /**
   * @brief Destructor for IntegerSequencer
   */
  ~IntegerSequencer() = default;

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
  static std::string componentName() { return "EventUtilities"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear sequencing, each corresponding to an
   * input-fraction value in the key array.
   * @return MFInt32 The current value of keyValue.
   */
  MFInt32 getKeyValue() const { return _keyValue; }

  /**
   * @brief Sets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear sequencing, each corresponding to an
   * input-fraction value in the key array.
   * @param value The new value for keyValue.
   */
  void setKeyValue(const MFInt32 &value) { _keyValue = value; }

  void setKeyValue(MFInt32 &&value) { _keyValue = std::move(value); }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Single intermittent output value determined by current key time
   * and corresponding keyValue entry.
   * @return SFInt32 The current value of value_changed.
   */
  SFInt32 getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Single intermittent output value determined by current key time
   * and corresponding keyValue entry. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFInt32 &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "IntegerSequencer").
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

  MFInt32 _keyValue{};

  /**
   * @brief Member variable for value_changed.
   */

  SFInt32 _value_changed{};
};

#endif // INTEGERSEQUENCER_HPP