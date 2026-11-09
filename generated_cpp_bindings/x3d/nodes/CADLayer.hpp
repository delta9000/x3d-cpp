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
 * @class CADLayer
 * @brief CADLayer nodes define a hierarchy that shows layer structure for a
 * Computer-Aided Design (CAD) model.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html#CADLayer
 */
class CADLayer : public virtual X3DGroupingNode {
public:
  CADLayer() = default;

  ~CADLayer() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "CADGeometry"; }

  static int componentLevel() { return 2; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Optional name for this particular CAD node.
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details Optional name for this particular CAD node.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

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
  SFString _name{};
};

} // namespace x3d::nodes
