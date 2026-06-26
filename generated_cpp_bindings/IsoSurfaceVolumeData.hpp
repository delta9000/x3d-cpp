// IsoSurfaceVolumeData.hpp
#ifndef ISOSURFACEVOLUMEDATA_HPP
#define ISOSURFACEVOLUMEDATA_HPP

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
 * @class IsoSurfaceVolumeData
 * @brief IsoSurfaceVolumeData displays one or more surfaces extracted from a
 * voxel dataset.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#IsoSurfaceVolumeData
 */
class IsoSurfaceVolumeData : public virtual X3DVolumeDataNode {
public:
  /**
   * @brief Default constructor for IsoSurfaceVolumeData
   */
  IsoSurfaceVolumeData() = default;

  /**
   * @brief Destructor for IsoSurfaceVolumeData
   */
  ~IsoSurfaceVolumeData() = default;

  /**
   * @brief Get the default value for contourStepSize
   * @return SFFloat The default value
   */
  static SFFloat getDefaultContourStepSize() { return 0; }

  /**
   * @brief Get the default value for gradients
   * @return SFNode The default value
   */
  static SFNode getDefaultGradients() { return nullptr; }

  /**
   * @brief Get the default value for surfaceTolerance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSurfaceTolerance() { return 0; }

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
   * @brief Gets the value of contourStepSize. AccessType: inputOutput
   * @details If contourStepSize is non-zero, also render all isosurfaces that
   * are multiples of that step size from initial surface value.
   * @return SFFloat The current value of contourStepSize.
   */
  SFFloat getContourStepSize() const { return _contourStepSize; }

  /**
   * @brief Sets the value of contourStepSize. AccessType: inputOutput
   * @details If contourStepSize is non-zero, also render all isosurfaces that
   * are multiples of that step size from initial surface value.
   * @param value The new value for contourStepSize.
   */
  void setContourStepSize(const SFFloat &value) { _contourStepSize = value; }

  /**
   * @brief Gets the value of gradients. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides explicit per-voxel gradient
   * direction information for determining surface boundaries, rather than
   * having it implicitly calculated by the implementation.
   * @return SFNode The current value of gradients.
   */
  SFNode getGradients() const { return _gradients; }

  /**
   * @brief Acceptable node types for the gradients field.
   * @details Permitted X3D node types: X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGradientsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of gradients. AccessType: inputOutput
   * @details Single contained X3DTexture3DNode (ComposedTexture3D,
   * ImageTexture3D, PixelTexture3D) that provides explicit per-voxel gradient
   * direction information for determining surface boundaries, rather than
   * having it implicitly calculated by the implementation.
   * @param value The new value for gradients.
   */
  void setGradients(const SFNode &value) { _gradients = value; }

  void setGradients(SFNode &&value) { _gradients = std::move(value); }

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
   * @brief Gets the value of surfaceTolerance. AccessType: inputOutput
   * @details Threshold for gradient magnitude for voxel inolusion in
   * isosurface.
   * @return SFFloat The current value of surfaceTolerance.
   */
  SFFloat getSurfaceTolerance() const { return _surfaceTolerance; }

  /**
   * @brief Sets the value of surfaceTolerance. AccessType: inputOutput
   * @details Threshold for gradient magnitude for voxel inolusion in
   * isosurface.
   * @param value The new value for surfaceTolerance.
   */
  void setSurfaceTolerance(const SFFloat &value) {

    validateSurfaceTolerance(value);

    _surfaceTolerance = value;
  }

  /**
   * @brief Non-validating write of surfaceTolerance (runtime/reader ingest
   * path).
   * @details Assigns without the range check setSurfaceTolerance() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setSurfaceTolerance() stays the enforcement point for
   * programmatic callers.
   */
  void setSurfaceToleranceUnchecked(const SFFloat &value) {
    _surfaceTolerance = value;
  }

  /**
   * @brief Gets the value of surfaceValues. AccessType: inputOutput
   * @details If surfaceValues has one value defined, render corresponding
   * isosurface plus any isosurfaces based on contourStepSize.
   * @return MFFloat The current value of surfaceValues.
   */
  MFFloat getSurfaceValues() const { return _surfaceValues; }

  /**
   * @brief Sets the value of surfaceValues. AccessType: inputOutput
   * @details If surfaceValues has one value defined, render corresponding
   * isosurface plus any isosurfaces based on contourStepSize.
   * @param value The new value for surfaceValues.
   */
  void setSurfaceValues(const MFFloat &value) { _surfaceValues = value; }

  void setSurfaceValues(MFFloat &&value) { _surfaceValues = std::move(value); }

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
   * @brief The X3D type name of this node (e.g. "IsoSurfaceVolumeData").
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
  static void checkRangesSurfaceTolerance(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

private:
  static void validateSurfaceTolerance(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("surfaceTolerance below minimum of 0");
  }

  /**
   * @brief Member variable for contourStepSize.
   */

  SFFloat _contourStepSize{0};

  /**
   * @brief Member variable for gradients.
   */

  SFNode _gradients{nullptr};

  /**
   * @brief Member variable for renderStyle.
   */

  MFNode _renderStyle{};

  /**
   * @brief Member variable for surfaceTolerance.
   */

  SFFloat _surfaceTolerance{0};

  /**
   * @brief Member variable for surfaceValues.
   */

  MFFloat _surfaceValues{};

  /**
   * @brief Member variable for voxels.
   */

  SFNode _voxels{nullptr};
};

#endif // ISOSURFACEVOLUMEDATA_HPP