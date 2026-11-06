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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class StaticGroup
 * @brief StaticGroup is similar to Group node but does not allow access to
 * children after creation time.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#StaticGroup
 */
class StaticGroup : public virtual X3DChildNode,
                    public virtual X3DBoundedObject {
public:
  StaticGroup() = default;

  ~StaticGroup() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Grouping"; }

  static int componentLevel() { return 3; }

  /**
   * MFNode; access: initializeOnly. Borrows the stored value.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setChildrenUnchecked(const MFNode &value) { _children = value; }

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
  MFNode _children{};
};

} // namespace x3d::nodes
