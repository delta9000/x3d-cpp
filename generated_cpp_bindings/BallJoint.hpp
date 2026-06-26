// BallJoint.hpp
#ifndef BALLJOINT_HPP
#define BALLJOINT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DRigidJointNode.hpp"

/**
 * @class BallJoint
 * @brief BallJoint represents an unconstrained joint between two bodies that
 * pivot about a common anchor point.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#BallJoint
 */
class BallJoint : public virtual X3DRigidJointNode {
public:
  /**
   * @brief Default constructor for BallJoint
   */
  BallJoint() = default;

  /**
   * @brief Destructor for BallJoint
   */
  ~BallJoint() = default;

  /**
   * @brief Get the default value for anchorPoint
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "joints"; }

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
   * @brief Gets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @return SFVec3f The current value of anchorPoint.
   */
  SFVec3f getAnchorPoint() const { return _anchorPoint; }

  /**
   * @brief Sets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @param value The new value for anchorPoint.
   */
  void setAnchorPoint(const SFVec3f &value) { _anchorPoint = value; }

  void setAnchorPoint(SFVec3f &&value) { _anchorPoint = std::move(value); }

  /**
   * @brief Gets the value of body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body1AnchorPoint.
   */
  SFVec3f getBody1AnchorPoint() const { return _body1AnchorPoint; }

  /**
   * @brief Emit an output value on body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody1AnchorPoint(const SFVec3f &value) { _body1AnchorPoint = value; }

  /**
   * @brief Gets the value of body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body2AnchorPoint.
   */
  SFVec3f getBody2AnchorPoint() const { return _body2AnchorPoint; }

  /**
   * @brief Emit an output value on body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody2AnchorPoint(const SFVec3f &value) { _body2AnchorPoint = value; }

  /**
   * @brief The X3D type name of this node (e.g. "BallJoint").
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
   * @brief Member variable for anchorPoint.
   */

  SFVec3f _anchorPoint{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for body1AnchorPoint.
   */

  SFVec3f _body1AnchorPoint{};

  /**
   * @brief Member variable for body2AnchorPoint.
   */

  SFVec3f _body2AnchorPoint{};
};

#endif // BALLJOINT_HPP