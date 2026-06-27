// ViewpointGroup.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ViewpointGroup
 * @brief ViewpointGroup can contain Viewpoint, OrthoViewpoint, GeoViewpoint and
 * other ViewpointGroup nodes for better user-navigation support with a shared
 * description on the viewpoint list.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#ViewpointGroup
 */
class ViewpointGroup : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for ViewpointGroup
   */
  ViewpointGroup() = default;

  /**
   * @brief Destructor for ViewpointGroup
   */
  ~ViewpointGroup() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for displayed
   * @return SFBool The default value
   */
  static SFBool getDefaultDisplayed() { return true; }

  /**
   * @brief Get the default value for retainUserOffsets
   * @return SFBool The default value
   */
  static SFBool getDefaultRetainUserOffsets() { return false; }

  /**
   * @brief Get the default value for size
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultSize() { return SFVec3f{0, 0, 0}; }

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
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details center specifies center point of proximity box within which
   * ViewpointGroup is usable and displayed on viewpoint list.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details center specifies center point of proximity box within which
   * ViewpointGroup is usable and displayed on viewpoint list.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details ViewpointGroup contains Viewpoint, OrthoViewpoint, GeoViewpoint
   * and other ViewpointGroup nodes that each have containerField='children'
   * default value.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: X3DViewpointNode, ViewpointGroup
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewpointNode",
                                                   "ViewpointGroup"};
    return types;
  }

  /**
   * @brief Sets the value of children. AccessType: inputOutput
   * @details ViewpointGroup contains Viewpoint, OrthoViewpoint, GeoViewpoint
   * and other ViewpointGroup nodes that each have containerField='children'
   * default value.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Text description or navigation hint to identify this
   * ViewpointGroup.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Text description or navigation hint to identify this
   * ViewpointGroup.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of displayed. AccessType: inputOutput
   * @details displayed determines whether this ViewpointGroup is displayed in
   * the current viewpoint list.
   * @return SFBool The current value of displayed.
   */
  SFBool getDisplayed() const { return _displayed; }

  /**
   * @brief Sets the value of displayed. AccessType: inputOutput
   * @details displayed determines whether this ViewpointGroup is displayed in
   * the current viewpoint list.
   * @param value The new value for displayed.
   */
  void setDisplayed(const SFBool &value) { _displayed = value; }

  /**
   * @brief Gets the value of retainUserOffsets. AccessType: inputOutput
   * @details Retain (true) or reset to zero (false) any prior user navigation
   * offsets from defined viewpoint position, orientation.
   * @return SFBool The current value of retainUserOffsets.
   */
  SFBool getRetainUserOffsets() const { return _retainUserOffsets; }

  /**
   * @brief Sets the value of retainUserOffsets. AccessType: inputOutput
   * @details Retain (true) or reset to zero (false) any prior user navigation
   * offsets from defined viewpoint position, orientation.
   * @param value The new value for retainUserOffsets.
   */
  void setRetainUserOffsets(const SFBool &value) { _retainUserOffsets = value; }

  /**
   * @brief Gets the value of size. AccessType: inputOutput
   * @details size of Proximity box around center location, oriented within
   * local transformation frame, within which ViewpointGroup is usable and
   * displayed on viewpoint list.
   * @return SFVec3f The current value of size.
   */
  SFVec3f getSize() const { return _size; }

  /**
   * @brief Sets the value of size. AccessType: inputOutput
   * @details size of Proximity box around center location, oriented within
   * local transformation frame, within which ViewpointGroup is usable and
   * displayed on viewpoint list.
   * @param value The new value for size.
   */
  void setSize(const SFVec3f &value) { _size = value; }

  void setSize(SFVec3f &&value) { _size = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ViewpointGroup").
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
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for displayed.
   */

  SFBool _displayed{true};

  /**
   * @brief Member variable for retainUserOffsets.
   */

  SFBool _retainUserOffsets{false};

  /**
   * @brief Member variable for size.
   */

  SFVec3f _size{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
