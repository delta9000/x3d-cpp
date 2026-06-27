// ProjectionVolumeStyle.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ProjectionVolumeStyle
 * @brief ProjectionVolumeStyle uses voxel data to directly generate output
 * color.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#ProjectionVolumeStyle
 */
class ProjectionVolumeStyle : public virtual X3DVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for ProjectionVolumeStyle
   */
  ProjectionVolumeStyle() = default;

  /**
   * @brief Destructor for ProjectionVolumeStyle
   */
  ~ProjectionVolumeStyle() = default;

  /**
   * @brief Get the default value for intensityThreshold
   * @return SFFloat The default value
   */
  static SFFloat getDefaultIntensityThreshold() { return 0; }

  /**
   * @brief Get the default value for type
   * @return ProjectionVolumeStyleTypeChoices The default value
   */
  static ProjectionVolumeStyleTypeChoices getDefaultType() {
    return ProjectionVolumeStyleTypeChoices::MAX;
  }

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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of intensityThreshold. AccessType: inputOutput
   * @details Threshold value used when type=MIN (LMIP) or type=MAX (MIP).
   * @return SFFloat The current value of intensityThreshold.
   */
  SFFloat getIntensityThreshold() const { return _intensityThreshold; }

  /**
   * @brief Sets the value of intensityThreshold. AccessType: inputOutput
   * @details Threshold value used when type=MIN (LMIP) or type=MAX (MIP).
   * @param value The new value for intensityThreshold.
   */
  void setIntensityThreshold(const SFFloat &value) {

    validateIntensityThreshold(value);

    _intensityThreshold = value;
  }

  /**
   * @brief Non-validating write of intensityThreshold (runtime/reader ingest
   * path).
   * @details Assigns without the range check setIntensityThreshold() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setIntensityThreshold() stays the
   *          enforcement point for programmatic callers.
   */
  void setIntensityThresholdUnchecked(const SFFloat &value) {
    _intensityThreshold = value;
  }

  /**
   * @brief Gets the value of type. AccessType: inputOutput
   * @details If type=MAX then Maximum Intensity Projection (MIP) or Least MIP
   * (LMIP) algorithm is used to generate output color.
   * @return ProjectionVolumeStyleTypeChoices The current value of type.
   */
  ProjectionVolumeStyleTypeChoices getType() const { return _type; }

  /**
   * @brief Sets the value of type. AccessType: inputOutput
   * @details If type=MAX then Maximum Intensity Projection (MIP) or Least MIP
   * (LMIP) algorithm is used to generate output color.
   * @param value The new value for type.
   */
  void setType(const ProjectionVolumeStyleTypeChoices &value) { _type = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ProjectionVolumeStyle").
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
  static void checkRangesIntensityThreshold(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out);

private:
  static void validateIntensityThreshold(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("intensityThreshold below minimum of 0");
    if (value > 1)
      throw std::out_of_range("intensityThreshold above maximum of 1");
  }

  /**
   * @brief Member variable for intensityThreshold.
   */

  SFFloat _intensityThreshold{0};

  /**
   * @brief Member variable for type.
   */

  ProjectionVolumeStyleTypeChoices _type{ProjectionVolumeStyleTypeChoices::MAX};
};

} // namespace x3d::nodes
