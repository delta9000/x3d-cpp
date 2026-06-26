// X3DPointingDeviceSensorNode.hpp
#ifndef X3DPOINTINGDEVICESENSORNODE_HPP
#define X3DPOINTINGDEVICESENSORNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

/**
 * @class X3DPointingDeviceSensorNode
 * @brief Base type for all pointing device sensors.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#X3DPointingDeviceSensorNode
 */
class X3DPointingDeviceSensorNode : public virtual X3DSensorNode {
public:
  /**
   * @brief Default constructor for X3DPointingDeviceSensorNode
   */
  X3DPointingDeviceSensorNode() = default;

  /**
   * @brief Virtual destructor for X3DPointingDeviceSensorNode
   */
  virtual ~X3DPointingDeviceSensorNode() = default;

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
  static std::string componentName() { return "PointingDeviceSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of isOver. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isOver.
   */
  SFBool getIsOver() const { return _isOver; }

  /**
   * @brief Emit an output value on isOver. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsOver(const SFBool &value) { _isOver = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DPointingDeviceSensorNode").
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
   * @brief Member variable for isOver.
   */

  SFBool _isOver{};
};

#endif // X3DPOINTINGDEVICESENSORNODE_HPP