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
  ViewpointGroup() = default;

  ~ViewpointGroup() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultDisplayed() { return true; }

  static SFBool getDefaultRetainUserOffsets() { return false; }

  static SFVec3f getDefaultSize() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 3; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details center specifies center point of proximity box within which
   * ViewpointGroup is usable and displayed on viewpoint list.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details center specifies center point of proximity box within which
   * ViewpointGroup is usable and displayed on viewpoint list.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details ViewpointGroup contains Viewpoint, OrthoViewpoint, GeoViewpoint
   * and other ViewpointGroup nodes that each have containerField='children'
   * default value.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: X3DViewpointNode, ViewpointGroup
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewpointNode",
                                                   "ViewpointGroup"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details ViewpointGroup contains Viewpoint, OrthoViewpoint, GeoViewpoint
   * and other ViewpointGroup nodes that each have containerField='children'
   * default value.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Text description or navigation hint to identify this
   * ViewpointGroup.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Text description or navigation hint to identify this
   * ViewpointGroup.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details displayed determines whether this ViewpointGroup is displayed in
   * the current viewpoint list.
   */
  const SFBool &getDisplayed() const { return _displayed; }

  /**
   * Access: inputOutput.
   * @details displayed determines whether this ViewpointGroup is displayed in
   * the current viewpoint list.
   */
  void setDisplayed(const SFBool &value) { _displayed = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Retain (true) or reset to zero (false) any prior user navigation
   * offsets from defined viewpoint position, orientation.
   */
  const SFBool &getRetainUserOffsets() const { return _retainUserOffsets; }

  /**
   * Access: inputOutput.
   * @details Retain (true) or reset to zero (false) any prior user navigation
   * offsets from defined viewpoint position, orientation.
   */
  void setRetainUserOffsets(const SFBool &value) { _retainUserOffsets = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details size of Proximity box around center location, oriented within
   * local transformation frame, within which ViewpointGroup is usable and
   * displayed on viewpoint list.
   */
  const SFVec3f &getSize() const { return _size; }

  /**
   * Access: inputOutput.
   * @details size of Proximity box around center location, oriented within
   * local transformation frame, within which ViewpointGroup is usable and
   * displayed on viewpoint list.
   */
  void setSize(const SFVec3f &value) { _size = value; }

  void setSize(SFVec3f &&value) { _size = std::move(value); }

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
  SFVec3f _center{SFVec3f{0, 0, 0}};

  MFNode _children{};

  SFString _description{};

  SFBool _displayed{true};

  SFBool _retainUserOffsets{false};

  SFVec3f _size{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
