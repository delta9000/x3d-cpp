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

#include "x3d/nodes/X3DGroupingNode.hpp"

#include "x3d/nodes/X3DViewportNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Viewport
 * @brief Viewport is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#Viewport
 */
class Viewport : public virtual X3DViewportNode {
public:
  Viewport() = default;

  ~Viewport() = default;

  static MFFloat getDefaultClipBoundary() {
    return std::vector<float>{0, 1, 0, 1};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "viewport"; }

  static std::string componentName() { return "Layering"; }

  static int componentLevel() { return 1; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details clipBoundary is specified in fractions of the normal render
   * surface in the sequence left/right/bottom/top.
   */
  const MFFloat &getClipBoundary() const { return _clipBoundary; }

  /**
   * Access: inputOutput.
   * @details clipBoundary is specified in fractions of the normal render
   * surface in the sequence left/right/bottom/top.
   */
  void setClipBoundary(const MFFloat &value) {

    validateClipBoundary(value);

    _clipBoundary = value;
  }

  void setClipBoundary(MFFloat &&value) {

    validateClipBoundary(value);

    _clipBoundary = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setClipBoundary() for range validation.
   */
  void setClipBoundaryUnchecked(const MFFloat &value) { _clipBoundary = value; }

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
  static void checkRangesClipBoundary(const MFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

private:
  static void validateClipBoundary(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("clipBoundary below minimum of 0");
      if (v > 1.0f)
        throw std::out_of_range("clipBoundary above maximum of 1");
    }
  }

  MFFloat _clipBoundary{std::vector<float>{0, 1, 0, 1}};
};

} // namespace x3d::nodes
