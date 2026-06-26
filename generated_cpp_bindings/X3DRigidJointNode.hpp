// X3DRigidJointNode.hpp
#ifndef X3DRIGIDJOINTNODE_HPP
#define X3DRIGIDJOINTNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

/**
 * @class X3DRigidJointNode
 * @brief The X3DRigidJointNode abstract node type is the base type for all
 * joint types.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#X3DRigidJointNode
 */
class X3DRigidJointNode : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for X3DRigidJointNode
   */
  X3DRigidJointNode() = default;

  /**
   * @brief Virtual destructor for X3DRigidJointNode
   */
  virtual ~X3DRigidJointNode() = default;

  /**
   * @brief Get the default value for body1
   * @return SFNode The default value
   */
  static SFNode getDefaultBody1() { return nullptr; }

  /**
   * @brief Get the default value for body2
   * @return SFNode The default value
   */
  static SFNode getDefaultBody2() { return nullptr; }

  /**
   * @brief Get the default value for forceOutput
   * @return std::vector<ForceOutputValues> The default value
   */
  static std::vector<ForceOutputValues> getDefaultForceOutput() {
    return std::vector<ForceOutputValues>{ForceOutputValues::NONE};
  }

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
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of body1. AccessType: inputOutput
   * @details
   * @return SFNode The current value of body1.
   */
  SFNode getBody1() const { return _body1; }

  /**
   * @brief Acceptable node types for the body1 field.
   * @details Permitted X3D node types: RigidBody
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBody1NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * @brief Sets the value of body1. AccessType: inputOutput
   * @details
   * @param value The new value for body1.
   */
  void setBody1(const SFNode &value) { _body1 = value; }

  void setBody1(SFNode &&value) { _body1 = std::move(value); }

  /**
   * @brief Gets the value of body2. AccessType: inputOutput
   * @details
   * @return SFNode The current value of body2.
   */
  SFNode getBody2() const { return _body2; }

  /**
   * @brief Acceptable node types for the body2 field.
   * @details Permitted X3D node types: RigidBody
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBody2NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * @brief Sets the value of body2. AccessType: inputOutput
   * @details
   * @param value The new value for body2.
   */
  void setBody2(const SFNode &value) { _body2 = value; }

  void setBody2(SFNode &&value) { _body2 = std::move(value); }

  /**
   * @brief Gets the value of forceOutput. AccessType: inputOutput
   * @details
   * @return std::vector<ForceOutputValues> The current value of forceOutput.
   */
  std::vector<ForceOutputValues> getForceOutput() const { return _forceOutput; }

  /**
   * @brief Sets the value of forceOutput. AccessType: inputOutput
   * @details
   * @param value The new value for forceOutput.
   */
  void setForceOutput(const std::vector<ForceOutputValues> &value) {

    _forceOutput = value;
  }

  void setForceOutput(std::vector<ForceOutputValues> &&value) {

    _forceOutput = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "X3DRigidJointNode").
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
   * @brief Member variable for body1.
   */

  SFNode _body1{nullptr};

  /**
   * @brief Member variable for body2.
   */

  SFNode _body2{nullptr};

  /**
   * @brief Member variable for forceOutput.
   */

  std::vector<ForceOutputValues> _forceOutput{
      std::vector<ForceOutputValues>{ForceOutputValues::NONE}};
};

#endif // X3DRIGIDJOINTNODE_HPP