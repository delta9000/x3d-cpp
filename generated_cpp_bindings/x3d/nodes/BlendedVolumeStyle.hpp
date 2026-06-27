// BlendedVolumeStyle.hpp
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
 * @class BlendedVolumeStyle
 * @brief BlendedVolumeStyle combines rendering of two voxel data sets into one
 * by blending voxel values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#BlendedVolumeStyle
 */
class BlendedVolumeStyle : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for BlendedVolumeStyle
   */
  BlendedVolumeStyle() = default;

  /**
   * @brief Destructor for BlendedVolumeStyle
   */
  ~BlendedVolumeStyle() = default;

  /**
   * @brief Get the default value for renderStyle
   * @return SFNode The default value
   */
  static SFNode getDefaultRenderStyle() { return nullptr; }

  /**
   * @brief Get the default value for voxels
   * @return SFNode The default value
   */
  static SFNode getDefaultVoxels() { return nullptr; }

  /**
   * @brief Get the default value for weightConstant1
   * @return SFFloat The default value
   */
  static SFFloat getDefaultWeightConstant1() { return 0.5; }

  /**
   * @brief Get the default value for weightConstant2
   * @return SFFloat The default value
   */
  static SFFloat getDefaultWeightConstant2() { return 0.5; }

  /**
   * @brief Get the default value for weightFunction1
   * @return SFString The default value
   */
  static SFString getDefaultWeightFunction1() { return "CONSTANT"; }

  /**
   * @brief Get the default value for weightFunction2
   * @return SFString The default value
   */
  static SFString getDefaultWeightFunction2() { return "CONSTANT"; }

  /**
   * @brief Get the default value for weightTransferFunction1
   * @return SFNode The default value
   */
  static SFNode getDefaultWeightTransferFunction1() { return nullptr; }

  /**
   * @brief Get the default value for weightTransferFunction2
   * @return SFNode The default value
   */
  static SFNode getDefaultWeightTransferFunction2() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "renderStyle"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "VolumeRendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of renderStyle. AccessType: inputOutput
   * @details Single contained X3DComposableVolumeRenderStyleNode node that
   * defines specific rendering technique for data in the voxels field, and the
   * result is blended with parent VolumeData or SegmentedVoliumeData node.
   * @return SFNode The current value of renderStyle.
   */
  SFNode getRenderStyle() const { return _renderStyle; }

  /**
   * @brief Acceptable node types for the renderStyle field.
   * @details Permitted X3D node types: X3DComposableVolumeRenderStyleNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DComposableVolumeRenderStyleNode"};
    return types;
  }

  /**
   * @brief Sets the value of renderStyle. AccessType: inputOutput
   * @details Single contained X3DComposableVolumeRenderStyleNode node that
   * defines specific rendering technique for data in the voxels field, and the
   * result is blended with parent VolumeData or SegmentedVoliumeData node.
   * @param value The new value for renderStyle.
   */
  void setRenderStyle(const SFNode &value) { _renderStyle = value; }

  void setRenderStyle(SFNode &&value) { _renderStyle = std::move(value); }

  /**
   * @brief Gets the value of voxels. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides second set of raw voxel
   * information utilized by corresponding rendering styles.
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
   * ImageTexture3D, PixelTexture3D) that provides second set of raw voxel
   * information utilized by corresponding rendering styles.
   * @param value The new value for voxels.
   */
  void setVoxels(const SFNode &value) { _voxels = value; }

  void setVoxels(SFNode &&value) { _voxels = std::move(value); }

  /**
   * @brief Gets the value of weightConstant1. AccessType: inputOutput
   * @details weightConstant1 is used when weightFunction1=CONSTANT.
   * @return SFFloat The current value of weightConstant1.
   */
  SFFloat getWeightConstant1() const { return _weightConstant1; }

  /**
   * @brief Sets the value of weightConstant1. AccessType: inputOutput
   * @details weightConstant1 is used when weightFunction1=CONSTANT.
   * @param value The new value for weightConstant1.
   */
  void setWeightConstant1(const SFFloat &value) {

    validateWeightConstant1(value);

    _weightConstant1 = value;
  }

  /**
   * @brief Non-validating write of weightConstant1 (runtime/reader ingest
   * path).
   * @details Assigns without the range check setWeightConstant1() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setWeightConstant1() stays the enforcement point for
   * programmatic callers.
   */
  void setWeightConstant1Unchecked(const SFFloat &value) {
    _weightConstant1 = value;
  }

  /**
   * @brief Gets the value of weightConstant2. AccessType: inputOutput
   * @details weightConstant2 is used when weightFunction2=CONSTANT.
   * @return SFFloat The current value of weightConstant2.
   */
  SFFloat getWeightConstant2() const { return _weightConstant2; }

  /**
   * @brief Sets the value of weightConstant2. AccessType: inputOutput
   * @details weightConstant2 is used when weightFunction2=CONSTANT.
   * @param value The new value for weightConstant2.
   */
  void setWeightConstant2(const SFFloat &value) {

    validateWeightConstant2(value);

    _weightConstant2 = value;
  }

  /**
   * @brief Non-validating write of weightConstant2 (runtime/reader ingest
   * path).
   * @details Assigns without the range check setWeightConstant2() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setWeightConstant2() stays the enforcement point for
   * programmatic callers.
   */
  void setWeightConstant2Unchecked(const SFFloat &value) {
    _weightConstant2 = value;
  }

  /**
   * @brief Gets the value of weightFunction1. AccessType: inputOutput
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   * @return SFString The current value of weightFunction1.
   */
  SFString getWeightFunction1() const { return _weightFunction1; }

  /**
   * @brief Sets the value of weightFunction1. AccessType: inputOutput
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   * @param value The new value for weightFunction1.
   */
  void setWeightFunction1(const SFString &value) { _weightFunction1 = value; }

  void setWeightFunction1(SFString &&value) {

    _weightFunction1 = std::move(value);
  }

  /**
   * @brief Gets the value of weightFunction2. AccessType: inputOutput
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   * @return SFString The current value of weightFunction2.
   */
  SFString getWeightFunction2() const { return _weightFunction2; }

  /**
   * @brief Sets the value of weightFunction2. AccessType: inputOutput
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   * @param value The new value for weightFunction2.
   */
  void setWeightFunction2(const SFString &value) { _weightFunction2 = value; }

  void setWeightFunction2(SFString &&value) {

    _weightFunction2 = std::move(value);
  }

  /**
   * @brief Gets the value of weightTransferFunction1. AccessType: inputOutput
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   * @return SFNode The current value of weightTransferFunction1.
   */
  SFNode getWeightTransferFunction1() const { return _weightTransferFunction1; }

  /**
   * @brief Acceptable node types for the weightTransferFunction1 field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableWeightTransferFunction1NodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of weightTransferFunction1. AccessType: inputOutput
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   * @param value The new value for weightTransferFunction1.
   */
  void setWeightTransferFunction1(const SFNode &value) {

    _weightTransferFunction1 = value;
  }

  void setWeightTransferFunction1(SFNode &&value) {

    _weightTransferFunction1 = std::move(value);
  }

  /**
   * @brief Gets the value of weightTransferFunction2. AccessType: inputOutput
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   * @return SFNode The current value of weightTransferFunction2.
   */
  SFNode getWeightTransferFunction2() const { return _weightTransferFunction2; }

  /**
   * @brief Acceptable node types for the weightTransferFunction2 field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableWeightTransferFunction2NodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of weightTransferFunction2. AccessType: inputOutput
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   * @param value The new value for weightTransferFunction2.
   */
  void setWeightTransferFunction2(const SFNode &value) {

    _weightTransferFunction2 = value;
  }

  void setWeightTransferFunction2(SFNode &&value) {

    _weightTransferFunction2 = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "BlendedVolumeStyle").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesWeightConstant1(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesWeightConstant2(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateWeightConstant1(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("weightConstant1 below minimum of 0");
    if (value > 1)
      throw std::out_of_range("weightConstant1 above maximum of 1");
  }

  static void validateWeightConstant2(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("weightConstant2 below minimum of 0");
    if (value > 1)
      throw std::out_of_range("weightConstant2 above maximum of 1");
  }

  /**
   * @brief Member variable for renderStyle.
   */

  SFNode _renderStyle{nullptr};

  /**
   * @brief Member variable for voxels.
   */

  SFNode _voxels{nullptr};

  /**
   * @brief Member variable for weightConstant1.
   */

  SFFloat _weightConstant1{0.5};

  /**
   * @brief Member variable for weightConstant2.
   */

  SFFloat _weightConstant2{0.5};

  /**
   * @brief Member variable for weightFunction1.
   */

  SFString _weightFunction1{"CONSTANT"};

  /**
   * @brief Member variable for weightFunction2.
   */

  SFString _weightFunction2{"CONSTANT"};

  /**
   * @brief Member variable for weightTransferFunction1.
   */

  SFNode _weightTransferFunction1{nullptr};

  /**
   * @brief Member variable for weightTransferFunction2.
   */

  SFNode _weightTransferFunction2{nullptr};
};

} // namespace x3d::nodes
