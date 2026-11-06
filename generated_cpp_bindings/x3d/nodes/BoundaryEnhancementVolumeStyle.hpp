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
 * @class BoundaryEnhancementVolumeStyle
 * @brief BoundaryEnhancementVolumeStyle provides boundary enhancement for the
 * volume rendering style.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#BoundaryEnhancementVolumeStyle
 */
class BoundaryEnhancementVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  BoundaryEnhancementVolumeStyle() = default;

  ~BoundaryEnhancementVolumeStyle() = default;

  static SFFloat getDefaultBoundaryOpacity() { return 0.9; }

  static SFFloat getDefaultOpacityFactor() { return 2; }

  static SFFloat getDefaultRetainedOpacity() { return 0.2; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details boundaryOpacity k_gs is the factored amount of the gradient
   * enhancement to use.
   */
  const SFFloat &getBoundaryOpacity() const { return _boundaryOpacity; }

  /**
   * Access: inputOutput.
   * @details boundaryOpacity k_gs is the factored amount of the gradient
   * enhancement to use.
   */
  void setBoundaryOpacity(const SFFloat &value) {

    validateBoundaryOpacity(value);

    _boundaryOpacity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBoundaryOpacity() for range validation.
   */
  void setBoundaryOpacityUnchecked(const SFFloat &value) {
    _boundaryOpacity = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details opacityFactor k_ge is the power function to control the slope of
   * the opacity curve to highlight the set of data.
   */
  const SFFloat &getOpacityFactor() const { return _opacityFactor; }

  /**
   * Access: inputOutput.
   * @details opacityFactor k_ge is the power function to control the slope of
   * the opacity curve to highlight the set of data.
   */
  void setOpacityFactor(const SFFloat &value) {

    validateOpacityFactor(value);

    _opacityFactor = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setOpacityFactor() for range validation.
   */
  void setOpacityFactorUnchecked(const SFFloat &value) {
    _opacityFactor = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details retainedOpacity k_gc is the amount of initial opacity to mix into
   * the output.
   */
  const SFFloat &getRetainedOpacity() const { return _retainedOpacity; }

  /**
   * Access: inputOutput.
   * @details retainedOpacity k_gc is the amount of initial opacity to mix into
   * the output.
   */
  void setRetainedOpacity(const SFFloat &value) {

    validateRetainedOpacity(value);

    _retainedOpacity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRetainedOpacity() for range validation.
   */
  void setRetainedOpacityUnchecked(const SFFloat &value) {
    _retainedOpacity = value;
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
  static void checkRangesBoundaryOpacity(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesOpacityFactor(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRetainedOpacity(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateBoundaryOpacity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("boundaryOpacity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("boundaryOpacity above maximum of 1");
  }

  static void validateOpacityFactor(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("opacityFactor below minimum of 0");
  }

  static void validateRetainedOpacity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("retainedOpacity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("retainedOpacity above maximum of 1");
  }

  SFFloat _boundaryOpacity{0.9};

  SFFloat _opacityFactor{2};

  SFFloat _retainedOpacity{0.2};
};

} // namespace x3d::nodes
