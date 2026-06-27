// OrthoViewpoint.hpp
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

#include "x3d/nodes/X3DBindableNode.hpp"

#include "x3d/nodes/X3DViewpointNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class OrthoViewpoint
 * @brief OrthoViewpoint provides an orthographic perspective-free view of a
 * scene from a specific location and direction.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#OrthoViewpoint
 */
class OrthoViewpoint : public virtual X3DViewpointNode {
public:
  /**
   * @brief Default constructor for OrthoViewpoint
   */
  OrthoViewpoint() = default;

  /**
   * @brief Destructor for OrthoViewpoint
   */
  ~OrthoViewpoint() = default;

  /**
   * @brief Get the default value for centerOfRotation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenterOfRotation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for fieldOfView
   * @return MFFloat The default value
   */
  static MFFloat getDefaultFieldOfView() {
    return std::vector<float>{-1, -1, 1, 1};
  }

  /**
   * @brief Get the default value for position
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 10}; }

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
   * @brief Gets the value of centerOfRotation. AccessType: inputOutput
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   * @return SFVec3f The current value of centerOfRotation.
   */
  SFVec3f getCenterOfRotation() const { return _centerOfRotation; }

  /**
   * @brief Sets the value of centerOfRotation. AccessType: inputOutput
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   * @param value The new value for centerOfRotation.
   */
  void setCenterOfRotation(const SFVec3f &value) { _centerOfRotation = value; }

  void setCenterOfRotation(SFVec3f &&value) {

    _centerOfRotation = std::move(value);
  }

  /**
   * @brief Gets the value of fieldOfView. AccessType: inputOutput
   * @details Minimum and maximum extents of view in units of local coordinate
   * system.
   * @return MFFloat The current value of fieldOfView.
   */
  MFFloat getFieldOfView() const { return _fieldOfView; }

  /**
   * @brief Sets the value of fieldOfView. AccessType: inputOutput
   * @details Minimum and maximum extents of view in units of local coordinate
   * system.
   * @param value The new value for fieldOfView.
   */
  void setFieldOfView(const MFFloat &value) { _fieldOfView = value; }

  void setFieldOfView(MFFloat &&value) { _fieldOfView = std::move(value); }

  /**
   * @brief Gets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) relative to local coordinate system.
   * @return SFVec3f The current value of position.
   */
  SFVec3f getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) relative to local coordinate system.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "OrthoViewpoint").
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
   * @brief Member variable for centerOfRotation.
   */

  SFVec3f _centerOfRotation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for fieldOfView.
   */

  MFFloat _fieldOfView{std::vector<float>{-1, -1, 1, 1}};

  /**
   * @brief Member variable for position.
   */

  SFVec3f _position{SFVec3f{0, 0, 10}};
};

} // namespace x3d::nodes
