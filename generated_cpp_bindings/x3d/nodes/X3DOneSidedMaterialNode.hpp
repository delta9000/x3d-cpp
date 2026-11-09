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

#include "x3d/nodes/X3DMaterialNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DOneSidedMaterialNode
 * @brief Base type for material nodes that describe how the shape looks like
 * from one side. A different number of contanied texture nodes are allowed by
 * each of the implementing nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#X3DOneSidedMaterialNode
 */
class X3DOneSidedMaterialNode : public virtual X3DMaterialNode {
public:
  X3DOneSidedMaterialNode() = default;

  virtual ~X3DOneSidedMaterialNode() = default;

  static SFFloat getDefaultNormalScale() { return 1; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 4; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getEmissiveTextureMapping() const {
    return _emissiveTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setEmissiveTextureMapping(const SFString &value) {

    _emissiveTextureMapping = value;
  }

  void setEmissiveTextureMapping(SFString &&value) {

    _emissiveTextureMapping = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getNormalScale() const { return _normalScale; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setNormalScale(const SFFloat &value) {

    validateNormalScale(value);

    _normalScale = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setNormalScale() for range validation.
   */
  void setNormalScaleUnchecked(const SFFloat &value) { _normalScale = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getNormalTextureMapping() const {
    return _normalTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setNormalTextureMapping(const SFString &value) {

    _normalTextureMapping = value;
  }

  void setNormalTextureMapping(SFString &&value) {

    _normalTextureMapping = std::move(value);
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
  static void checkRangesNormalScale(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateNormalScale(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("normalScale below minimum of 0");
  }

  SFString _emissiveTextureMapping{};

  SFFloat _normalScale{1};

  SFString _normalTextureMapping{};
};

} // namespace x3d::nodes
