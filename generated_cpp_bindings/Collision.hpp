// Collision.hpp
#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

#include "X3DSensorNode.hpp"

/**
 * @class Collision
 * @brief Collision detects camera-to-object contact using current view and
 * NavigationInfo avatarSize.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#Collision
 */
class Collision : public virtual X3DGroupingNode, public virtual X3DSensorNode {
public:
  /**
   * @brief Default constructor for Collision
   */
  Collision() = default;

  /**
   * @brief Destructor for Collision
   */
  ~Collision() = default;

  /**
   * @brief Get the default value for proxy
   * @return SFNode The default value
   */
  static SFNode getDefaultProxy() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of collideTime. AccessType: outputOnly
   * @details Time of collision between camera (avatar) and geometry.
   * @return SFTime The current value of collideTime.
   */
  SFTime getCollideTime() const { return _collideTime; }

  /**
   * @brief Emit an output value on collideTime. AccessType: outputOnly
   * @details Time of collision between camera (avatar) and geometry.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitCollideTime(const SFTime &value) { _collideTime = value; }

  /**
   * @brief Gets the value of proxy. AccessType: initializeOnly
   * @details The proxy node is used as a substitute for Collision children
   * during collision detection, simplifying collision-intersection
   * computations.
   * @return SFNode The current value of proxy.
   */
  SFNode getProxy() const { return _proxy; }

  /**
   * @brief Acceptable node types for the proxy field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableProxyNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }
  /**
   * @brief Data-layer write of proxy (reader/init ingest path).
   * @details proxy is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setProxy().
   */
  void setProxyUnchecked(const SFNode &value) { _proxy = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Collision").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

private:
  /**
   * @brief Member variable for collideTime.
   */

  SFTime _collideTime{};

  /**
   * @brief Member variable for proxy.
   */

  SFNode _proxy{nullptr};
};

#endif // COLLISION_HPP