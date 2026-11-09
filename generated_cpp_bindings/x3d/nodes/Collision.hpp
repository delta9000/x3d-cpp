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

#include "x3d/nodes/X3DSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Collision
 * @brief Collision detects camera-to-object contact using current view and
 * NavigationInfo avatarSize.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#Collision
 */
class Collision : public virtual X3DGroupingNode, public virtual X3DSensorNode {
public:
  Collision() = default;

  ~Collision() = default;

  static SFNode getDefaultProxy() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 2; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time of collision between camera (avatar) and geometry.
   */
  const SFTime &getCollideTime() const { return _collideTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time of collision between camera (avatar) and geometry.
   */
  void emitCollideTime(const SFTime &value) { _collideTime = value; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The proxy node is used as a substitute for Collision children
   * during collision detection, simplifying collision-intersection
   * computations.
   */
  const SFNode &getProxy() const { return _proxy; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableProxyNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setProxyUnchecked(const SFNode &value) { _proxy = value; }

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
  SFTime _collideTime{};

  SFNode _proxy{nullptr};
};

} // namespace x3d::nodes
