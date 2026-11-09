#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DRigidJointNode
 * @brief The X3DRigidJointNode abstract node type is the base type for all
 * joint types.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#X3DRigidJointNode
 */
class X3DRigidJointNode : public virtual X3DNode {
public:
  X3DRigidJointNode() = default;

  virtual ~X3DRigidJointNode() = default;

  static SFNode getDefaultBody1() { return nullptr; }

  static SFNode getDefaultBody2() { return nullptr; }

  static MFString getDefaultForceOutput() {
    return std::vector<std::string>{"NONE"};
  }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getBody1() const { return _body1; }

  /**
   * @details Permitted X3D node types: RigidBody
   */
  static const std::vector<std::string> &acceptableBody1NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBody1(const SFNode &value) { _body1 = value; }

  void setBody1(SFNode &&value) { _body1 = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getBody2() const { return _body2; }

  /**
   * @details Permitted X3D node types: RigidBody
   */
  static const std::vector<std::string> &acceptableBody2NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBody2(const SFNode &value) { _body2 = value; }

  void setBody2(SFNode &&value) { _body2 = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFString &getForceOutput() const { return _forceOutput; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setForceOutput(const MFString &value) { _forceOutput = value; }

  void setForceOutput(MFString &&value) { _forceOutput = std::move(value); }

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

private:
  SFNode _body1{nullptr};

  SFNode _body2{nullptr};

  MFString _forceOutput{std::vector<std::string>{"NONE"}};
};

} // namespace x3d::nodes
