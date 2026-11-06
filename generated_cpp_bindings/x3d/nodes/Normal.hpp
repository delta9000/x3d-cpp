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

#include "x3d/nodes/X3DNormalNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Normal
 * @brief Normal defines a set of 3D surface-normal vectors that apply either to
 * a sibling Coordinate|CoordinateDouble node, or else to a parent ElevationGrid
 * node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#Normal
 */
class Normal : public virtual X3DNormalNode {
public:
  Normal() = default;

  ~Normal() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DNormalNode";
  }

  static std::string getDefaultContainerField() { return "normal"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 2; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details set of unit-length normal vectors, corresponding to indexed
   * polygons or vertices.
   */
  const MFVec3f &getVector() const { return _vector; }

  /**
   * Access: inputOutput.
   * @details set of unit-length normal vectors, corresponding to indexed
   * polygons or vertices.
   */
  void setVector(const MFVec3f &value) {

    validateVector(value);

    _vector = value;
  }

  void setVector(MFVec3f &&value) {

    validateVector(value);

    _vector = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setVector() for range validation.
   */
  void setVectorUnchecked(const MFVec3f &value) { _vector = value; }

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
  static void checkRangesVector(const MFVec3f &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateVector(const MFVec3f &value) {

    for (const auto &v : value) {

      if (v.x < -1.0f)
        throw std::out_of_range("vector.x below minimum of -1");
      if (v.x > 1.0f)
        throw std::out_of_range("vector.x above maximum of 1");

      if (v.y < -1.0f)
        throw std::out_of_range("vector.y below minimum of -1");
      if (v.y > 1.0f)
        throw std::out_of_range("vector.y above maximum of 1");

      if (v.z < -1.0f)
        throw std::out_of_range("vector.z below minimum of -1");
      if (v.z > 1.0f)
        throw std::out_of_range("vector.z above maximum of 1");
    }
  }

  MFVec3f _vector{};
};

} // namespace x3d::nodes
