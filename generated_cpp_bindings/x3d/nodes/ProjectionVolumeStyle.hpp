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
  ProjectionVolumeStyle() = default;

  ~ProjectionVolumeStyle() = default;

  static SFFloat getDefaultIntensityThreshold() { return 0; }

  static ProjectionVolumeStyleTypeChoices getDefaultType() {
    return ProjectionVolumeStyleTypeChoices::MAX;
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Threshold value used when type=MIN (LMIP) or type=MAX (MIP).
   */
  const SFFloat &getIntensityThreshold() const { return _intensityThreshold; }

  /**
   * Access: inputOutput.
   * @details Threshold value used when type=MIN (LMIP) or type=MAX (MIP).
   */
  void setIntensityThreshold(const SFFloat &value) {

    validateIntensityThreshold(value);

    _intensityThreshold = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setIntensityThreshold() for range validation.
   */
  void setIntensityThresholdUnchecked(const SFFloat &value) {
    _intensityThreshold = value;
  }

  /**
   * ProjectionVolumeStyleTypeChoices; access: inputOutput. Borrows the stored
   * value.
   * @details If type=MAX then Maximum Intensity Projection (MIP) or Least MIP
   * (LMIP) algorithm is used to generate output color.
   */
  const ProjectionVolumeStyleTypeChoices &getType() const { return _type; }

  /**
   * Access: inputOutput.
   * @details If type=MAX then Maximum Intensity Projection (MIP) or Least MIP
   * (LMIP) algorithm is used to generate output color.
   */
  void setType(const ProjectionVolumeStyleTypeChoices &value) { _type = value; }

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
  static void checkRangesIntensityThreshold(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out);

private:
  static void validateIntensityThreshold(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("intensityThreshold below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("intensityThreshold above maximum of 1");
  }

  SFFloat _intensityThreshold{0};

  ProjectionVolumeStyleTypeChoices _type{ProjectionVolumeStyleTypeChoices::MAX};
};

} // namespace x3d::nodes
