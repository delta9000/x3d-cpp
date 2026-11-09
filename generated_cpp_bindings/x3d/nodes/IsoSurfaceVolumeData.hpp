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
 * @class IsoSurfaceVolumeData
 * @brief IsoSurfaceVolumeData displays one or more surfaces extracted from a
 * voxel dataset.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#IsoSurfaceVolumeData
 */
class IsoSurfaceVolumeData : public virtual X3DVolumeDataNode {
public:
  IsoSurfaceVolumeData() = default;

  ~IsoSurfaceVolumeData() = default;

  static SFFloat getDefaultContourStepSize() { return 0; }

  static SFNode getDefaultGradients() { return nullptr; }

  static SFFloat getDefaultSurfaceTolerance() { return 0; }

  static SFNode getDefaultVoxels() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details If contourStepSize is non-zero, also render all isosurfaces that
   * are multiples of that step size from initial surface value.
   */
  const SFFloat &getContourStepSize() const { return _contourStepSize; }

  /**
   * Access: inputOutput.
   * @details If contourStepSize is non-zero, also render all isosurfaces that
   * are multiples of that step size from initial surface value.
   */
  void setContourStepSize(const SFFloat &value) { _contourStepSize = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides explicit per-voxel gradient
   * direction information for determining surface boundaries, rather than
   * having it implicitly calculated by the implementation.
   */
  const SFNode &getGradients() const { return _gradients; }

  /**
   * @details Permitted X3D node types: X3DTexture3DNode
   */
  static const std::vector<std::string> &acceptableGradientsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides explicit per-voxel gradient
   * direction information for determining surface boundaries, rather than
   * having it implicitly calculated by the implementation.
   */
  void setGradients(const SFNode &value) { _gradients = value; }

  void setGradients(SFNode &&value) { _gradients = std::move(value); }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Threshold for gradient magnitude for voxel inolusion in
   * isosurface.
   */
  const SFFloat &getSurfaceTolerance() const { return _surfaceTolerance; }

  /**
   * Access: inputOutput.
   * @details Threshold for gradient magnitude for voxel inolusion in
   * isosurface.
   */
  void setSurfaceTolerance(const SFFloat &value) {

    validateSurfaceTolerance(value);

    _surfaceTolerance = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSurfaceTolerance() for range validation.
   */
  void setSurfaceToleranceUnchecked(const SFFloat &value) {
    _surfaceTolerance = value;
  }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details If surfaceValues has one value defined, render corresponding
   * isosurface plus any isosurfaces based on contourStepSize.
   */
  const MFFloat &getSurfaceValues() const { return _surfaceValues; }

  /**
   * Access: inputOutput.
   * @details If surfaceValues has one value defined, render corresponding
   * isosurface plus any isosurfaces based on contourStepSize.
   */
  void setSurfaceValues(const MFFloat &value) { _surfaceValues = value; }

  void setSurfaceValues(MFFloat &&value) { _surfaceValues = std::move(value); }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSurfaceTolerance(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

private:
  static void validateSurfaceTolerance(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("surfaceTolerance below minimum of 0");
  }

  SFFloat _contourStepSize{0};

  SFNode _gradients{nullptr};

  MFNode _renderStyle{};

  SFFloat _surfaceTolerance{0};

  MFFloat _surfaceValues{};

  SFNode _voxels{nullptr};
};

} // namespace x3d::nodes
