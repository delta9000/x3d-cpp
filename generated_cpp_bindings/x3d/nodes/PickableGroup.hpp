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

#include "x3d/nodes/X3DPickableObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PickableGroup
 * @brief PickableGroup is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#PickableGroup
 */
class PickableGroup : public virtual X3DGroupingNode,
                      public virtual X3DPickableObject {
public:
  PickableGroup() = default;

  ~PickableGroup() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Author-provided prose that describes intended purpose of the node.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Author-provided prose that describes intended purpose of the node.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

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
  SFString _description{};
};

} // namespace x3d::nodes
