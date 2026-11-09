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
  OrthoViewpoint() = default;

  ~OrthoViewpoint() = default;

  static SFVec3f getDefaultCenterOfRotation() { return SFVec3f{0, 0, 0}; }

  static MFFloat getDefaultFieldOfView() {
    return std::vector<float>{-1, -1, 1, 1};
  }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 10}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 3; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   */
  const SFVec3f &getCenterOfRotation() const { return _centerOfRotation; }

  /**
   * Access: inputOutput.
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   */
  void setCenterOfRotation(const SFVec3f &value) { _centerOfRotation = value; }

  void setCenterOfRotation(SFVec3f &&value) {

    _centerOfRotation = std::move(value);
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Minimum and maximum extents of view in units of local coordinate
   * system.
   */
  const MFFloat &getFieldOfView() const { return _fieldOfView; }

  /**
   * Access: inputOutput.
   * @details Minimum and maximum extents of view in units of local coordinate
   * system.
   */
  void setFieldOfView(const MFFloat &value) { _fieldOfView = value; }

  void setFieldOfView(MFFloat &&value) { _fieldOfView = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details position (x, y, z in meters) relative to local coordinate system.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details position (x, y, z in meters) relative to local coordinate system.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

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
  SFVec3f _centerOfRotation{SFVec3f{0, 0, 0}};

  MFFloat _fieldOfView{std::vector<float>{-1, -1, 1, 1}};

  SFVec3f _position{SFVec3f{0, 0, 10}};
};

} // namespace x3d::nodes
