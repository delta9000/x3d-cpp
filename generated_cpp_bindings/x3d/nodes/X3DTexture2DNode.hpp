// X3DTexture2DNode.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DTextureNode.hpp"

#include "x3d/nodes/X3DSingleTextureNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DTexture2DNode
 * @brief Base type for all nodes which specify 2D sources for texture images.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#X3DTexture2DNode
 */
class X3DTexture2DNode : public virtual X3DSingleTextureNode {
public:
  /**
   * @brief Default constructor for X3DTexture2DNode
   */
  X3DTexture2DNode() = default;

  /**
   * @brief Virtual destructor for X3DTexture2DNode
   */
  virtual ~X3DTexture2DNode() = default;

  /**
   * @brief Get the default value for repeatS
   * @return SFBool The default value
   */
  static SFBool getDefaultRepeatS() { return true; }

  /**
   * @brief Get the default value for repeatT
   * @return SFBool The default value
   */
  static SFBool getDefaultRepeatT() { return true; }

  /**
   * @brief Get the default value for textureProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultTextureProperties() { return nullptr; }

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
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of repeatS. AccessType: initializeOnly
   * @details
   * @return SFBool The current value of repeatS.
   */
  SFBool getRepeatS() const { return _repeatS; }
  /**
   * @brief Data-layer write of repeatS (reader/init ingest path).
   * @details repeatS is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRepeatS().
   */
  void setRepeatSUnchecked(const SFBool &value) { _repeatS = value; }

  /**
   * @brief Gets the value of repeatT. AccessType: initializeOnly
   * @details
   * @return SFBool The current value of repeatT.
   */
  SFBool getRepeatT() const { return _repeatT; }
  /**
   * @brief Data-layer write of repeatT (reader/init ingest path).
   * @details repeatT is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRepeatT().
   */
  void setRepeatTUnchecked(const SFBool &value) { _repeatT = value; }

  /**
   * @brief Gets the value of textureProperties. AccessType: initializeOnly
   * @details
   * @return SFNode The current value of textureProperties.
   */
  SFNode getTextureProperties() const { return _textureProperties; }

  /**
   * @brief Acceptable node types for the textureProperties field.
   * @details Permitted X3D node types: TextureProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableTexturePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"TextureProperties"};
    return types;
  }
  /**
   * @brief Data-layer write of textureProperties (reader/init ingest path).
   * @details textureProperties is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setTextureProperties().
   */
  void setTexturePropertiesUnchecked(const SFNode &value) {
    _textureProperties = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "X3DTexture2DNode").
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
   * @brief Member variable for repeatS.
   */

  SFBool _repeatS{true};

  /**
   * @brief Member variable for repeatT.
   */

  SFBool _repeatT{true};

  /**
   * @brief Member variable for textureProperties.
   */

  SFNode _textureProperties{nullptr};
};

} // namespace x3d::nodes
