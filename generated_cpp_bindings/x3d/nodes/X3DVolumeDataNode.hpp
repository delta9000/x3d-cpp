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

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DVolumeDataNode
 * @brief The X3DVolumeDataNode abstract node type is the base type for all node
 * types that describe volumetric data to be rendered.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#X3DVolumeDataNode
 */
class X3DVolumeDataNode : public virtual X3DChildNode,
                          public virtual X3DBoundedObject {
public:
  X3DVolumeDataNode() = default;

  virtual ~X3DVolumeDataNode() = default;

  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultBboxDisplay() { return false; }

  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  static SFVec3f getDefaultDimensions() { return SFVec3f{1, 1, 1}; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxCenter() const { return _bboxCenter; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getBboxDisplay() const { return _bboxDisplay; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxSize() const { return _bboxSize; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFVec3f &getDimensions() const { return _dimensions; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDimensions(const SFVec3f &value) { _dimensions = value; }

  void setDimensions(SFVec3f &&value) { _dimensions = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getVisible() const { return _visible; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVisible(const SFBool &value) { _visible = value; }

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
  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  SFBool _bboxDisplay{false};

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  SFVec3f _dimensions{SFVec3f{1, 1, 1}};

  SFBool _visible{true};
};

} // namespace x3d::nodes
