#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class LineProperties
 * @brief LineProperties allows precise fine-grained control over the rendering
 * style of lines and edges for associated geometry nodes inside the same Shape.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#LineProperties
 */
class LineProperties : public virtual X3DAppearanceChildNode {
public:
  LineProperties() = default;

  ~LineProperties() = default;

  static SFBool getDefaultApplied() { return true; }

  static SFInt32 getDefaultLinetype() { return 1; }

  static SFFloat getDefaultLinewidthScaleFactor() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "lineProperties"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 2; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether or not LineProperties are applied to associated geometry.
   */
  const SFBool &getApplied() const { return _applied; }

  /**
   * Access: inputOutput.
   * @details Whether or not LineProperties are applied to associated geometry.
   */
  void setApplied(const SFBool &value) { _applied = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details linetype selects a line pattern, with solid default if defined
   * value isn't supported.
   */
  const SFInt32 &getLinetype() const { return _linetype; }

  /**
   * Access: inputOutput.
   * @details linetype selects a line pattern, with solid default if defined
   * value isn't supported.
   */
  void setLinetype(const SFInt32 &value) {

    validateLinetype(value);

    _linetype = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setLinetype() for range validation.
   */
  void setLinetypeUnchecked(const SFInt32 &value) { _linetype = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details linewidthScaleFactor is a scale factor multiplied by
   * browser-dependent nominal linewidth, mapped to nearest available line
   * width.
   */
  const SFFloat &getLinewidthScaleFactor() const {
    return _linewidthScaleFactor;
  }

  /**
   * Access: inputOutput.
   * @details linewidthScaleFactor is a scale factor multiplied by
   * browser-dependent nominal linewidth, mapped to nearest available line
   * width.
   */
  void setLinewidthScaleFactor(const SFFloat &value) {

    _linewidthScaleFactor = value;
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
  static void checkRangesLinetype(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateLinetype(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("linetype below minimum of 0");
  }

  SFBool _applied{true};

  SFInt32 _linetype{1};

  SFFloat _linewidthScaleFactor{0};
};

} // namespace x3d::nodes
