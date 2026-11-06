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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Billboard
 * @brief Billboard is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#Billboard
 */
class Billboard : public virtual X3DGroupingNode {
public:
  Billboard() = default;

  ~Billboard() = default;

  static SFVec3f getDefaultAxisOfRotation() { return SFVec3f{0, 1, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details axisOfRotation direction is relative to local coordinate system.
   */
  const SFVec3f &getAxisOfRotation() const { return _axisOfRotation; }

  /**
   * Access: inputOutput.
   * @details axisOfRotation direction is relative to local coordinate system.
   */
  void setAxisOfRotation(const SFVec3f &value) { _axisOfRotation = value; }

  void setAxisOfRotation(SFVec3f &&value) {

    _axisOfRotation = std::move(value);
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

private:
  SFVec3f _axisOfRotation{SFVec3f{0, 1, 0}};
};

} // namespace x3d::nodes
