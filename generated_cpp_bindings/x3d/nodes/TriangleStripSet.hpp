#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

#include "x3d/nodes/X3DComposedGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TriangleStripSet
 * @brief TriangleStripSet is a geometry node containing a
 * Coordinate|CoordinateDouble node, and can also contain Color|ColorRGBA,
 * Normal and TextureCoordinate nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#TriangleStripSet
 */
class TriangleStripSet : public virtual X3DComposedGeometryNode {
public:
  TriangleStripSet() = default;

  ~TriangleStripSet() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 3; }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details stripCount array provides number of vertices in each strip.
   */
  const MFInt32 &getStripCount() const { return _stripCount; }

  /**
   * Access: inputOutput.
   * @details stripCount array provides number of vertices in each strip.
   */
  void setStripCount(const MFInt32 &value) {

    validateStripCount(value);

    _stripCount = value;
  }

  void setStripCount(MFInt32 &&value) {

    validateStripCount(value);

    _stripCount = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStripCount() for range validation.
   */
  void setStripCountUnchecked(const MFInt32 &value) { _stripCount = value; }

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
  static void checkRangesStripCount(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateStripCount(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 3)
        throw std::out_of_range("stripCount below minimum of 3");
    }
  }

  MFInt32 _stripCount{};
};

} // namespace x3d::nodes
