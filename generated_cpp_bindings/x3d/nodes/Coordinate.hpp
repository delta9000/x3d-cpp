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
 * @class Coordinate
 * @brief Coordinate builds geometry by defining a set of 3D coordinate
 * (triplet) point values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#Coordinate
 */
class Coordinate : public virtual X3DCoordinateNode {
public:
  Coordinate() = default;

  ~Coordinate() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DCoordinateNode";
  }

  static std::string getDefaultContainerField() { return "coord"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details point contains a set of 3D coordinate (triplet) point values.
   */
  const MFVec3f &getPoint() const { return _point; }

  /**
   * Access: inputOutput.
   * @details point contains a set of 3D coordinate (triplet) point values.
   */
  void setPoint(const MFVec3f &value) { _point = value; }

  void setPoint(MFVec3f &&value) { _point = std::move(value); }

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
  MFVec3f _point{};
};

} // namespace x3d::nodes
