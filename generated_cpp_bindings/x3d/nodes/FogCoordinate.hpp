#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class FogCoordinate
 * @brief FogCoordinate defines a set of explicit fog depths on a per-vertex
 * basis, overriding Fog visibilityRange.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#FogCoordinate
 */
class FogCoordinate : public virtual X3DGeometricPropertyNode {
public:
  FogCoordinate() = default;

  ~FogCoordinate() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "fogCoord"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 4; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details depth contains a set of 3D coordinate (triplet) point values.
   */
  const MFFloat &getDepth() const { return _depth; }

  /**
   * Access: inputOutput.
   * @details depth contains a set of 3D coordinate (triplet) point values.
   */
  void setDepth(const MFFloat &value) {

    validateDepth(value);

    _depth = value;
  }

  void setDepth(MFFloat &&value) {

    validateDepth(value);

    _depth = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDepth() for range validation.
   */
  void setDepthUnchecked(const MFFloat &value) { _depth = value; }

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
  static void checkRangesDepth(const MFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateDepth(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("depth below minimum of 0");
      if (v > 1.0f)
        throw std::out_of_range("depth above maximum of 1");
    }
  }

  MFFloat _depth{};
};

} // namespace x3d::nodes
