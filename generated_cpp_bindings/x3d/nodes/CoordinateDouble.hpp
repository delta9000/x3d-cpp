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

#include "x3d/nodes/X3DCoordinateNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CoordinateDouble
 * @brief CoordinateDouble builds geometry by defining a set of 3D coordinate
 * (triplet) point values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#CoordinateDouble
 */
class CoordinateDouble : public virtual X3DCoordinateNode {
public:
  CoordinateDouble() = default;

  ~CoordinateDouble() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DCoordinateNode";
  }

  static std::string getDefaultContainerField() { return "coord"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3d; access: inputOutput. Borrows the stored value.
   * @details point contains a set of 3D coordinate (triplet) point values.
   */
  const MFVec3d &getPoint() const { return _point; }

  /**
   * Access: inputOutput.
   * @details point contains a set of 3D coordinate (triplet) point values.
   */
  void setPoint(const MFVec3d &value) { _point = value; }

  void setPoint(MFVec3d &&value) { _point = std::move(value); }

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

private:
  MFVec3d _point{};
};

} // namespace x3d::nodes
