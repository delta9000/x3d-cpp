#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DVolumeRenderStyleNode.hpp"

#include "x3d/nodes/X3DComposableVolumeRenderStyleNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class OpacityMapVolumeStyle
 * @brief OpacityMapVolumeStyle specifies that volumetric data is rendered using
 * opacity mapped to a transfer function texture.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#OpacityMapVolumeStyle
 */
class OpacityMapVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  OpacityMapVolumeStyle() = default;

  ~OpacityMapVolumeStyle() = default;

  static SFNode getDefaultTransferFunction() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The transferFunction field holds a single texture representation
   * in either two or three dimensions that maps the voxel data values to a
   * specific colour output.
   */
  const SFNode &getTransferFunction() const { return _transferFunction; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, X3DTexture3DNode
   */
  static const std::vector<std::string> &acceptableTransferFunctionNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The transferFunction field holds a single texture representation
   * in either two or three dimensions that maps the voxel data values to a
   * specific colour output.
   */
  void setTransferFunction(const SFNode &value) { _transferFunction = value; }

  void setTransferFunction(SFNode &&value) {

    _transferFunction = std::move(value);
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
  SFNode _transferFunction{nullptr};
};

} // namespace x3d::nodes
