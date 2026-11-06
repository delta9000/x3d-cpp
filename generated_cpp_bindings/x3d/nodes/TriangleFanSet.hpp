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
 * @class TriangleFanSet
 * @brief TriangleFanSet is a geometry node containing a
 * Coordinate|CoordinateDouble node, and can also contain Color|ColorRGBA,
 * Normal and TextureCoordinate nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#TriangleFanSet
 */
class TriangleFanSet : public virtual X3DComposedGeometryNode {
public:
  TriangleFanSet() = default;

  ~TriangleFanSet() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 3; }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details fanCount array provides number of vertices in each fan.
   */
  const MFInt32 &getFanCount() const { return _fanCount; }

  /**
   * Access: inputOutput.
   * @details fanCount array provides number of vertices in each fan.
   */
  void setFanCount(const MFInt32 &value) {

    validateFanCount(value);

    _fanCount = value;
  }

  void setFanCount(MFInt32 &&value) {

    validateFanCount(value);

    _fanCount = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFanCount() for range validation.
   */
  void setFanCountUnchecked(const MFInt32 &value) { _fanCount = value; }

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
  static void checkRangesFanCount(const MFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateFanCount(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 3)
        throw std::out_of_range("fanCount below minimum of 3");
    }
  }

  MFInt32 _fanCount{};
};

} // namespace x3d::nodes
