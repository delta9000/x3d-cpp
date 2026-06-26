// SegmentedVolumeData.hpp
#ifndef SEGMENTEDVOLUMEDATA_HPP
#define SEGMENTEDVOLUMEDATA_HPP

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

#include "X3DVolumeDataNode.hpp"

/**
 * @class SegmentedVolumeData
 * @brief SegmentedVolumeData displays a segmented voxel dataset with different
 * RenderStyle nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#SegmentedVolumeData
 */
class SegmentedVolumeData : public virtual X3DVolumeDataNode {
public:
  /**
   * @brief Default constructor for SegmentedVolumeData
   */
  SegmentedVolumeData() = default;

  /**
   * @brief Destructor for SegmentedVolumeData
   */
  ~SegmentedVolumeData() = default;

  /**
   * @brief Get the default value for segmentIdentifiers
   * @return SFNode The default value
   */
  static SFNode getDefaultSegmentIdentifiers() { return nullptr; }

  /**
   * @brief Get the default value for voxels
   * @return SFNode The default value
   */
  static SFNode getDefaultVoxels() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of renderStyle. AccessType: inputOutput
   * @details Multiple contained X3DVolumeRenderStyleNode nodes corresponding to
   * each isosurface that define specific rendering technique for this
   * volumetric object.
   * @return MFNode The current value of renderStyle.
   */
  MFNode getRenderStyle() const { return _renderStyle; }

  /**
   * @brief Acceptable node types for the renderStyle field.
   * @details Permitted X3D node types: X3DVolumeRenderStyleNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {"X3DVolumeRenderStyleNode"};
    return types;
  }

  /**
   * @brief Sets the value of renderStyle. AccessType: inputOutput
   * @details Multiple contained X3DVolumeRenderStyleNode nodes corresponding to
   * each isosurface that define specific rendering technique for this
   * volumetric object.
   * @param value The new value for renderStyle.
   */
  void setRenderStyle(const MFNode &value) { _renderStyle = value; }

  void setRenderStyle(MFNode &&value) { _renderStyle = std::move(value); }

  /**
   * @brief Gets the value of segmentEnabled. AccessType: inputOutput
   * @details Array of boolean values that indicates whether to draw each
   * segment, with indices corresponding to the segment identifier.
   * @return MFBool The current value of segmentEnabled.
   */
  MFBool getSegmentEnabled() const { return _segmentEnabled; }

  /**
   * @brief Sets the value of segmentEnabled. AccessType: inputOutput
   * @details Array of boolean values that indicates whether to draw each
   * segment, with indices corresponding to the segment identifier.
   * @param value The new value for segmentEnabled.
   */
  void setSegmentEnabled(const MFBool &value) { _segmentEnabled = value; }

  void setSegmentEnabled(MFBool &&value) { _segmentEnabled = std::move(value); }

  /**
   * @brief Gets the value of segmentIdentifiers. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) holds component texture that provides
   * corresponding segment identifier.
   * @return SFNode The current value of segmentIdentifiers.
   */
  SFNode getSegmentIdentifiers() const { return _segmentIdentifiers; }

  /**
   * @brief Acceptable node types for the segmentIdentifiers field.
   * @details Permitted X3D node types: X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableSegmentIdentifiersNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of segmentIdentifiers. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) holds component texture that provides
   * corresponding segment identifier.
   * @param value The new value for segmentIdentifiers.
   */
  void setSegmentIdentifiers(const SFNode &value) {

    _segmentIdentifiers = value;
  }

  void setSegmentIdentifiers(SFNode &&value) {

    _segmentIdentifiers = std::move(value);
  }

  /**
   * @brief Gets the value of voxels. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides raw voxel information
   * utilized by corresponding rendering styles.
   * @return SFNode The current value of voxels.
   */
  SFNode getVoxels() const { return _voxels; }

  /**
   * @brief Acceptable node types for the voxels field.
   * @details Permitted X3D node types: X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableVoxelsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of voxels. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides raw voxel information
   * utilized by corresponding rendering styles.
   * @param value The new value for voxels.
   */
  void setVoxels(const SFNode &value) { _voxels = value; }

  void setVoxels(SFNode &&value) { _voxels = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "SegmentedVolumeData").
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
   * @brief Member variable for renderStyle.
   */

  MFNode _renderStyle{};

  /**
   * @brief Member variable for segmentEnabled.
   */

  MFBool _segmentEnabled{};

  /**
   * @brief Member variable for segmentIdentifiers.
   */

  SFNode _segmentIdentifiers{nullptr};

  /**
   * @brief Member variable for voxels.
   */

  SFNode _voxels{nullptr};
};

#endif // SEGMENTEDVOLUMEDATA_HPP