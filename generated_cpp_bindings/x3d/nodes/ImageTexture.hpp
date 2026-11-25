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

#include "x3d/nodes/X3DTexture2DNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ImageTexture
 * @brief ImageTexture maps a 2D-image file onto a geometric shape.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/texturing.html#ImageTexture
 */
class ImageTexture : public virtual X3DTexture2DNode,
                     public virtual X3DUrlObject {
public:
  ImageTexture() = default;

  ~ImageTexture() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3dUrlObjectTexture";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getColorDepth() const { return _colorDepth; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitColorDepth(const SFInt32 &value) { _colorDepth = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getHeight() const { return _height; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHeight(const SFInt32 &value) { _height = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getWidth() const { return _width; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitWidth(const SFInt32 &value) { _width = value; }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFInt32 _colorDepth{};

  SFInt32 _height{};

  SFInt32 _width{};
};

} // namespace x3d::nodes
