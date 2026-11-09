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
  X3DTexture2DNode() = default;

  virtual ~X3DTexture2DNode() = default;

  static SFBool getDefaultRepeatS() { return true; }

  static SFBool getDefaultRepeatT() { return true; }

  static SFNode getDefaultTextureProperties() { return nullptr; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getRepeatS() const { return _repeatS; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRepeatSUnchecked(const SFBool &value) { _repeatS = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getRepeatT() const { return _repeatT; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRepeatTUnchecked(const SFBool &value) { _repeatT = value; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFNode &getTextureProperties() const { return _textureProperties; }

  /**
   * @details Permitted X3D node types: TextureProperties
   */
  static const std::vector<std::string> &
  acceptableTexturePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"TextureProperties"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexturePropertiesUnchecked(const SFNode &value) {
    _textureProperties = value;
  }

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
  SFBool _repeatS{true};

  SFBool _repeatT{true};

  SFNode _textureProperties{nullptr};
};

} // namespace x3d::nodes
