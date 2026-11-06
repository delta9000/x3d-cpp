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
  BlendedVolumeStyle() = default;

  ~BlendedVolumeStyle() = default;

  static SFNode getDefaultRenderStyle() { return nullptr; }

  static SFNode getDefaultVoxels() { return nullptr; }

  static SFFloat getDefaultWeightConstant1() { return 0.5; }

  static SFFloat getDefaultWeightConstant2() { return 0.5; }

  static SFString getDefaultWeightFunction1() { return "CONSTANT"; }

  static SFString getDefaultWeightFunction2() { return "CONSTANT"; }

  static SFNode getDefaultWeightTransferFunction1() { return nullptr; }

  static SFNode getDefaultWeightTransferFunction2() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 3; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DComposableVolumeRenderStyleNode node that
   * defines specific rendering technique for data in the voxels field, and the
   * result is blended with parent VolumeData or SegmentedVoliumeData node.
   */
  const SFNode &getRenderStyle() const { return _renderStyle; }

  /**
   * @details Permitted X3D node types: X3DComposableVolumeRenderStyleNode
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DComposableVolumeRenderStyleNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained X3DComposableVolumeRenderStyleNode node that
   * defines specific rendering technique for data in the voxels field, and the
   * result is blended with parent VolumeData or SegmentedVoliumeData node.
   */
  void setRenderStyle(const SFNode &value) { _renderStyle = value; }

  void setRenderStyle(SFNode &&value) { _renderStyle = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides second set of raw voxel
   * information utilized by corresponding rendering styles.
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
   * ImageTexture3D, PixelTexture3D) that provides second set of raw voxel
   * information utilized by corresponding rendering styles.
   */
  void setVoxels(const SFNode &value) { _voxels = value; }

  void setVoxels(SFNode &&value) { _voxels = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details weightConstant1 is used when weightFunction1=CONSTANT.
   */
  const SFFloat &getWeightConstant1() const { return _weightConstant1; }

  /**
   * Access: inputOutput.
   * @details weightConstant1 is used when weightFunction1=CONSTANT.
   */
  void setWeightConstant1(const SFFloat &value) {

    validateWeightConstant1(value);

    _weightConstant1 = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWeightConstant1() for range validation.
   */
  void setWeightConstant1Unchecked(const SFFloat &value) {
    _weightConstant1 = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details weightConstant2 is used when weightFunction2=CONSTANT.
   */
  const SFFloat &getWeightConstant2() const { return _weightConstant2; }

  /**
   * Access: inputOutput.
   * @details weightConstant2 is used when weightFunction2=CONSTANT.
   */
  void setWeightConstant2(const SFFloat &value) {

    validateWeightConstant2(value);

    _weightConstant2 = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWeightConstant2() for range validation.
   */
  void setWeightConstant2Unchecked(const SFFloat &value) {
    _weightConstant2 = value;
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   */
  const SFString &getWeightFunction1() const { return _weightFunction1; }

  /**
   * Access: inputOutput.
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   */
  void setWeightFunction1(const SFString &value) { _weightFunction1 = value; }

  void setWeightFunction1(SFString &&value) {

    _weightFunction1 = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   */
  const SFString &getWeightFunction2() const { return _weightFunction2; }

  /**
   * Access: inputOutput.
   * @details specifies 2D textures used to determine weight values when weight
   * function is set to TABLE.
   */
  void setWeightFunction2(const SFString &value) { _weightFunction2 = value; }

  void setWeightFunction2(SFString &&value) {

    _weightFunction2 = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   */
  const SFNode &getWeightTransferFunction1() const {
    return _weightTransferFunction1;
  }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &
  acceptableWeightTransferFunction1NodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   */
  void setWeightTransferFunction1(const SFNode &value) {

    _weightTransferFunction1 = value;
  }

  void setWeightTransferFunction1(SFNode &&value) {

    _weightTransferFunction1 = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   */
  const SFNode &getWeightTransferFunction2() const {
    return _weightTransferFunction2;
  }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &
  acceptableWeightTransferFunction2NodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The weightTransferFunction1 and weightTransferFunction2 fields
   * specify two-dimensional textures that are used to determine the weight
   * values when the weight function is set to "TABLE".
   */
  void setWeightTransferFunction2(const SFNode &value) {

    _weightTransferFunction2 = value;
  }

  void setWeightTransferFunction2(SFNode &&value) {

    _weightTransferFunction2 = std::move(value);
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

    if (value < 0.0f)
      throw std::out_of_range("weightConstant1 below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("weightConstant1 above maximum of 1");
  }

  static void validateWeightConstant2(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("weightConstant2 below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("weightConstant2 above maximum of 1");
  }

  SFNode _renderStyle{nullptr};

  SFNode _voxels{nullptr};

  SFFloat _weightConstant1{0.5};

  SFFloat _weightConstant2{0.5};

  SFString _weightFunction1{"CONSTANT"};

  SFString _weightFunction2{"CONSTANT"};

  SFNode _weightTransferFunction1{nullptr};

  SFNode _weightTransferFunction2{nullptr};
};

} // namespace x3d::nodes
