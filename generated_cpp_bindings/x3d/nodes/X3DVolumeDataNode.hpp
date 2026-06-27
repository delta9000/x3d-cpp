// X3DVolumeDataNode.hpp
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
  /**
   * @brief Default constructor for X3DVolumeDataNode
   */
  X3DVolumeDataNode() = default;

  /**
   * @brief Virtual destructor for X3DVolumeDataNode
   */
  virtual ~X3DVolumeDataNode() = default;

  /**
   * @brief Get the default value for bboxCenter
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for bboxDisplay
   * @return SFBool The default value
   */
  static SFBool getDefaultBboxDisplay() { return false; }

  /**
   * @brief Get the default value for bboxSize
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  /**
   * @brief Get the default value for dimensions
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDimensions() { return SFVec3f{1, 1, 1}; }

  /**
   * @brief Get the default value for visible
   * @return SFBool The default value
   */
  static SFBool getDefaultVisible() { return true; }

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
  static std::string componentName() { return "VolumeRendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of bboxCenter. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxCenter.
   */
  SFVec3f getBboxCenter() const { return _bboxCenter; }
  /**
   * @brief Data-layer write of bboxCenter (reader/init ingest path).
   * @details bboxCenter is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxCenter().
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * @brief Gets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @return SFBool The current value of bboxDisplay.
   */
  SFBool getBboxDisplay() const { return _bboxDisplay; }

  /**
   * @brief Sets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @param value The new value for bboxDisplay.
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * @brief Gets the value of bboxSize. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxSize.
   */
  SFVec3f getBboxSize() const { return _bboxSize; }
  /**
   * @brief Data-layer write of bboxSize (reader/init ingest path).
   * @details bboxSize is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxSize().
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * @brief Gets the value of dimensions. AccessType: inputOutput
   * @details
   * @return SFVec3f The current value of dimensions.
   */
  SFVec3f getDimensions() const { return _dimensions; }

  /**
   * @brief Sets the value of dimensions. AccessType: inputOutput
   * @details
   * @param value The new value for dimensions.
   */
  void setDimensions(const SFVec3f &value) { _dimensions = value; }

  void setDimensions(SFVec3f &&value) { _dimensions = std::move(value); }

  /**
   * @brief Gets the value of visible. AccessType: inputOutput
   * @details
   * @return SFBool The current value of visible.
   */
  SFBool getVisible() const { return _visible; }

  /**
   * @brief Sets the value of visible. AccessType: inputOutput
   * @details
   * @param value The new value for visible.
   */
  void setVisible(const SFBool &value) { _visible = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DVolumeDataNode").
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
   * @brief Member variable for bboxCenter.
   */

  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for bboxDisplay.
   */

  SFBool _bboxDisplay{false};

  /**
   * @brief Member variable for bboxSize.
   */

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  /**
   * @brief Member variable for dimensions.
   */

  SFVec3f _dimensions{SFVec3f{1, 1, 1}};

  /**
   * @brief Member variable for visible.
   */

  SFBool _visible{true};
};

} // namespace x3d::nodes
