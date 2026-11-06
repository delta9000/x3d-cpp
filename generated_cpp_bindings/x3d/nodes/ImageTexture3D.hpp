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

#include "x3d/nodes/X3DTexture3DNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ImageTexture3D
 * @brief ImageTexture3D defines a 3D image-based texture map by specifying a
 * single image file that contains complete 3D data.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#ImageTexture3D
 */
class ImageTexture3D : public virtual X3DTexture3DNode,
                       public virtual X3DUrlObject {
public:
  ImageTexture3D() = default;

  ~ImageTexture3D() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture3DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing3D"; }

  static int componentLevel() { return 2; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
};

} // namespace x3d::nodes
