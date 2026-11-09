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
 * @class SegmentedVolumeData
 * @brief SegmentedVolumeData displays a segmented voxel dataset with different
 * RenderStyle nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#SegmentedVolumeData
 */
class SegmentedVolumeData : public virtual X3DVolumeDataNode {
public:
  SegmentedVolumeData() = default;

  ~SegmentedVolumeData() = default;

  static SFNode getDefaultSegmentIdentifiers() { return nullptr; }

  static SFNode getDefaultVoxels() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Multiple contained X3DVolumeRenderStyleNode nodes corresponding to
   * each isosurface that define specific rendering technique for this
   * volumetric object.
   */
  const MFNode &getRenderStyle() const { return _renderStyle; }

  /**
   * @details Permitted X3D node types: X3DVolumeRenderStyleNode
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {"X3DVolumeRenderStyleNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Multiple contained X3DVolumeRenderStyleNode nodes corresponding to
   * each isosurface that define specific rendering technique for this
   * volumetric object.
   */
  void setRenderStyle(const MFNode &value) { _renderStyle = value; }

  void setRenderStyle(MFNode &&value) { _renderStyle = std::move(value); }

  /**
   * MFBool; access: inputOutput. Borrows the stored value.
   * @details Array of boolean values that indicates whether to draw each
   * segment, with indices corresponding to the segment identifier.
   */
  const MFBool &getSegmentEnabled() const { return _segmentEnabled; }

  /**
   * Access: inputOutput.
   * @details Array of boolean values that indicates whether to draw each
   * segment, with indices corresponding to the segment identifier.
   */
  void setSegmentEnabled(const MFBool &value) { _segmentEnabled = value; }

  void setSegmentEnabled(MFBool &&value) { _segmentEnabled = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) holds component texture that provides
   * corresponding segment identifier.
   */
  const SFNode &getSegmentIdentifiers() const { return _segmentIdentifiers; }

  /**
   * @details Permitted X3D node types: X3DTexture3DNode
   */
  static const std::vector<std::string> &
  acceptableSegmentIdentifiersNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) holds component texture that provides
   * corresponding segment identifier.
   */
  void setSegmentIdentifiers(const SFNode &value) {

    _segmentIdentifiers = value;
  }

  void setSegmentIdentifiers(SFNode &&value) {

    _segmentIdentifiers = std::move(value);
  }

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
  MFNode _renderStyle{};

  MFBool _segmentEnabled{};

  SFNode _segmentIdentifiers{nullptr};

  SFNode _voxels{nullptr};
};

} // namespace x3d::nodes
