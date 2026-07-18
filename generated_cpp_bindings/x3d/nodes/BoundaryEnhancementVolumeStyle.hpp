// BoundaryEnhancementVolumeStyle.hpp
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
  /**
   * @brief Default constructor for BoundaryEnhancementVolumeStyle
   */
  BoundaryEnhancementVolumeStyle() = default;

  /**
   * @brief Destructor for BoundaryEnhancementVolumeStyle
   */
  ~BoundaryEnhancementVolumeStyle() = default;

  /**
   * @brief Get the default value for boundaryOpacity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBoundaryOpacity() { return 0.9; }

  /**
   * @brief Get the default value for opacityFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultOpacityFactor() { return 2; }

  /**
   * @brief Get the default value for retainedOpacity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRetainedOpacity() { return 0.2; }

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
   * @brief Gets the value of boundaryOpacity. AccessType: inputOutput
   * @details boundaryOpacity k_gs is the factored amount of the gradient
   * enhancement to use.
   * @return SFFloat The current value of boundaryOpacity.
   */
  SFFloat getBoundaryOpacity() const { return _boundaryOpacity; }

  /**
   * @brief Sets the value of boundaryOpacity. AccessType: inputOutput
   * @details boundaryOpacity k_gs is the factored amount of the gradient
   * enhancement to use.
   * @param value The new value for boundaryOpacity.
   */
  void setBoundaryOpacity(const SFFloat &value) {

    validateBoundaryOpacity(value);

    _boundaryOpacity = value;
  }

  /**
   * @brief Non-validating write of boundaryOpacity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBoundaryOpacity() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setBoundaryOpacity() stays the enforcement point for
   * programmatic callers.
   */
  void setBoundaryOpacityUnchecked(const SFFloat &value) {
    _boundaryOpacity = value;
  }

  /**
   * @brief Gets the value of opacityFactor. AccessType: inputOutput
   * @details opacityFactor k_ge is the power function to control the slope of
   * the opacity curve to highlight the set of data.
   * @return SFFloat The current value of opacityFactor.
   */
  SFFloat getOpacityFactor() const { return _opacityFactor; }

  /**
   * @brief Sets the value of opacityFactor. AccessType: inputOutput
   * @details opacityFactor k_ge is the power function to control the slope of
   * the opacity curve to highlight the set of data.
   * @param value The new value for opacityFactor.
   */
  void setOpacityFactor(const SFFloat &value) {

    validateOpacityFactor(value);

    _opacityFactor = value;
  }

  /**
   * @brief Non-validating write of opacityFactor (runtime/reader ingest path).
   * @details Assigns without the range check setOpacityFactor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setOpacityFactor() stays the
   *          enforcement point for programmatic callers.
   */
  void setOpacityFactorUnchecked(const SFFloat &value) {
    _opacityFactor = value;
  }

  /**
   * @brief Gets the value of retainedOpacity. AccessType: inputOutput
   * @details retainedOpacity k_gc is the amount of initial opacity to mix into
   * the output.
   * @return SFFloat The current value of retainedOpacity.
   */
  SFFloat getRetainedOpacity() const { return _retainedOpacity; }

  /**
   * @brief Sets the value of retainedOpacity. AccessType: inputOutput
   * @details retainedOpacity k_gc is the amount of initial opacity to mix into
   * the output.
   * @param value The new value for retainedOpacity.
   */
  void setRetainedOpacity(const SFFloat &value) {

    validateRetainedOpacity(value);

    _retainedOpacity = value;
  }

  /**
   * @brief Non-validating write of retainedOpacity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setRetainedOpacity() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setRetainedOpacity() stays the enforcement point for
   * programmatic callers.
   */
  void setRetainedOpacityUnchecked(const SFFloat &value) {
    _retainedOpacity = value;
  }

  /**
   * @brief The X3D type name of this node (e.g.
   * "BoundaryEnhancementVolumeStyle").
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

  /**
   * @brief Member variable for boundaryOpacity.
   */

  SFFloat _boundaryOpacity{0.9};

  /**
   * @brief Member variable for opacityFactor.
   */

  SFFloat _opacityFactor{2};

  /**
   * @brief Member variable for retainedOpacity.
   */

  SFFloat _retainedOpacity{0.2};
};

} // namespace x3d::nodes
