// HAnimDisplacer.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class HAnimDisplacer
 * @brief HAnimDisplacer nodes alter the shape of coordinate-based geometry
 * within parent HAnimJoint or HAnimSegment nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/hanim.html#HAnimDisplacer
 */
class HAnimDisplacer : public virtual X3DGeometricPropertyNode {
public:
  /**
   * @brief Default constructor for HAnimDisplacer
   */
  HAnimDisplacer() = default;

  /**
   * @brief Destructor for HAnimDisplacer
   */
  ~HAnimDisplacer() = default;

  /**
   * @brief Get the default value for weight
   * @return SFFloat The default value
   */
  static SFFloat getDefaultWeight() { return 0.0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "displacers"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "HAnim"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of coordIndex. AccessType: inputOutput
   * @details Defines index values into the parent HAnimSegment or
   * HAnimBody/HAnimHumanoid coordinate array for the mesh of vertices affected
   * by this HAnimDisplacer.
   * @return MFInt32 The current value of coordIndex.
   */
  MFInt32 getCoordIndex() const { return _coordIndex; }

  /**
   * @brief Sets the value of coordIndex. AccessType: inputOutput
   * @details Defines index values into the parent HAnimSegment or
   * HAnimBody/HAnimHumanoid coordinate array for the mesh of vertices affected
   * by this HAnimDisplacer.
   * @param value The new value for coordIndex.
   */
  void setCoordIndex(const MFInt32 &value) { _coordIndex = value; }

  void setCoordIndex(MFInt32 &&value) { _coordIndex = std::move(value); }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of this
   * node.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of displacements. AccessType: inputOutput
   * @details displacements are a set of SFVec3f values added to neutral/resting
   * position of each of the corresponding HAnimSegment vertices (or
   * HAnimJoint/HAnimHumanoid vertices) referenced by coordIndex field.
   * @return MFVec3f The current value of displacements.
   */
  MFVec3f getDisplacements() const { return _displacements; }

  /**
   * @brief Sets the value of displacements. AccessType: inputOutput
   * @details displacements are a set of SFVec3f values added to neutral/resting
   * position of each of the corresponding HAnimSegment vertices (or
   * HAnimJoint/HAnimHumanoid vertices) referenced by coordIndex field.
   * @param value The new value for displacements.
   */
  void setDisplacements(const MFVec3f &value) { _displacements = value; }

  void setDisplacements(MFVec3f &&value) { _displacements = std::move(value); }

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimDisplacer node
   * can be identified at run time for animation purposes.
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details Unique name attribute must be defined so that HAnimDisplacer node
   * can be identified at run time for animation purposes.
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details The weigh factor has typical range [0,1] and defines the scale
   * factor applied to displacement values before adding them to neutral vertex
   * positions.
   * @return SFFloat The current value of weight.
   */
  SFFloat getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details The weigh factor has typical range [0,1] and defines the scale
   * factor applied to displacement values before adding them to neutral vertex
   * positions.
   * @param value The new value for weight.
   */
  void setWeight(const SFFloat &value) { _weight = value; }

  /**
   * @brief The X3D type name of this node (e.g. "HAnimDisplacer").
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
   * @brief Member variable for coordIndex.
   */

  MFInt32 _coordIndex{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for displacements.
   */

  MFVec3f _displacements{};

  /**
   * @brief Member variable for name.
   */

  SFString _name{};

  /**
   * @brief Member variable for weight.
   */

  SFFloat _weight{0.0};
};

} // namespace x3d::nodes
