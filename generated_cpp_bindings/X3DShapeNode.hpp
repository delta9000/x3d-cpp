// X3DShapeNode.hpp
#ifndef X3DSHAPENODE_HPP
#define X3DSHAPENODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

/**
 * @class X3DShapeNode
 * @brief Base type for all Shape nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#X3DShapeNode
 */
class X3DShapeNode : public virtual X3DChildNode,
                     public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for X3DShapeNode
   */
  X3DShapeNode() = default;

  /**
   * @brief Virtual destructor for X3DShapeNode
   */
  virtual ~X3DShapeNode() = default;

  /**
   * @brief Get the default value for appearance
   * @return SFNode The default value
   */
  static SFNode getDefaultAppearance() { return nullptr; }

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
   * @brief Get the default value for castShadow
   * @return SFBool The default value
   */
  static SFBool getDefaultCastShadow() { return true; }

  /**
   * @brief Get the default value for geometry
   * @return SFNode The default value
   */
  static SFNode getDefaultGeometry() { return nullptr; }

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
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of appearance. AccessType: inputOutput
   * @details
   * @return SFNode The current value of appearance.
   */
  SFNode getAppearance() const { return _appearance; }

  /**
   * @brief Acceptable node types for the appearance field.
   * @details Permitted X3D node types: X3DAppearanceNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAppearanceNodeTypes() {
    static const std::vector<std::string> types = {"X3DAppearanceNode"};
    return types;
  }

  /**
   * @brief Sets the value of appearance. AccessType: inputOutput
   * @details
   * @param value The new value for appearance.
   */
  void setAppearance(const SFNode &value) { _appearance = value; }

  void setAppearance(SFNode &&value) { _appearance = std::move(value); }

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
   * @brief Gets the value of castShadow. AccessType: inputOutput
   * @details
   * @return SFBool The current value of castShadow.
   */
  SFBool getCastShadow() const { return _castShadow; }

  /**
   * @brief Sets the value of castShadow. AccessType: inputOutput
   * @details
   * @param value The new value for castShadow.
   */
  void setCastShadow(const SFBool &value) { _castShadow = value; }

  /**
   * @brief Gets the value of geometry. AccessType: inputOutput
   * @details
   * @return SFNode The current value of geometry.
   */
  SFNode getGeometry() const { return _geometry; }

  /**
   * @brief Acceptable node types for the geometry field.
   * @details Permitted X3D node types: X3DGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry. AccessType: inputOutput
   * @details
   * @param value The new value for geometry.
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

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
   * @brief The X3D type name of this node (e.g. "X3DShapeNode").
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
   * @brief Member variable for appearance.
   */

  SFNode _appearance{nullptr};

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
   * @brief Member variable for castShadow.
   */

  SFBool _castShadow{true};

  /**
   * @brief Member variable for geometry.
   */

  SFNode _geometry{nullptr};

  /**
   * @brief Member variable for visible.
   */

  SFBool _visible{true};
};

#endif // X3DSHAPENODE_HPP