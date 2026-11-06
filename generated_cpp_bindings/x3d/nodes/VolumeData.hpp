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

#include "x3d/nodes/X3DVolumeDataNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class VolumeData
 * @brief VolumeData displays a simple non-segmented voxel dataset with a single
 * RenderStyle node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#VolumeData
 */
class VolumeData : public virtual X3DVolumeDataNode {
public:
  VolumeData() = default;

  ~VolumeData() = default;

  static SFNode getDefaultRenderStyle() { return nullptr; }

  static SFNode getDefaultVoxels() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DVolumeRenderStyleNode node that defines
   * specific rendering technique for this volumetric object.
   */
  const SFNode &getRenderStyle() const { return _renderStyle; }

  /**
   * @details Permitted X3D node types: X3DVolumeRenderStyleNode
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {"X3DVolumeRenderStyleNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained X3DVolumeRenderStyleNode node that defines
   * specific rendering technique for this volumetric object.
   */
  void setRenderStyle(const SFNode &value) { _renderStyle = value; }

  void setRenderStyle(SFNode &&value) { _renderStyle = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides raw voxel information
   * utilized by corresponding rendering styles.
   */
  const SFNode &getVoxels() const { return _voxels; }

  /**
   * @details Permitted X3D node types: X3DTexture3DNode
   */
  static const std::vector<std::string> &acceptableVoxelsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides raw voxel information
   * utilized by corresponding rendering styles.
   */
  void setVoxels(const SFNode &value) { _voxels = value; }

  void setVoxels(SFNode &&value) { _voxels = std::move(value); }

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
  SFNode _renderStyle{nullptr};

  SFNode _voxels{nullptr};
};

} // namespace x3d::nodes
