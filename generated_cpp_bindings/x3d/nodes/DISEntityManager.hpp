// DISEntityManager.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class DISEntityManager
 * @brief DISEntityManager notifies a scene when new DIS ESPDU entities arrive
 * or current entities leave.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html#DISEntityManager
 */
class DISEntityManager : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for DISEntityManager
   */
  DISEntityManager() = default;

  /**
   * @brief Destructor for DISEntityManager
   */
  ~DISEntityManager() = default;

  /**
   * @brief Get the default value for address
   * @return SFString The default value
   */
  static SFString getDefaultAddress() { return "localhost"; }

  /**
   * @brief Get the default value for applicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultApplicationID() { return 0; }

  /**
   * @brief Get the default value for port
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultPort() { return 0; }

  /**
   * @brief Get the default value for siteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSiteID() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesDISEntityTypeMapping";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "DIS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of addedEntities. AccessType: outputOnly
   * @details addedEntities array contains any new entities added during the
   * last frame.
   * @return MFNode The current value of addedEntities.
   */
  MFNode getAddedEntities() const { return _addedEntities; }

  /**
   * @brief Emit an output value on addedEntities. AccessType: outputOnly
   * @details addedEntities array contains any new entities added during the
   * last frame. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitAddedEntities(const MFNode &value) { _addedEntities = value; }

  /**
   * @brief Acceptable node types for the addedEntities field.
   * @details Permitted X3D node types: EspduTransform
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAddedEntitiesNodeTypes() {
    static const std::vector<std::string> types = {"EspduTransform"};
    return types;
  }

  /**
   * @brief Gets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'.
   * @return SFString The current value of address.
   */
  SFString getAddress() const { return _address; }

  /**
   * @brief Sets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'.
   * @param value The new value for address.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * @brief Gets the value of applicationID. AccessType: inputOutput
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   * @return SFInt32 The current value of applicationID.
   */
  SFInt32 getApplicationID() const { return _applicationID; }

  /**
   * @brief Sets the value of applicationID. AccessType: inputOutput
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   * @param value The new value for applicationID.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details mapping field provides a mechanism for automatically creating an
   * X3D model when a new entity arrives over the network.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: DISEntityTypeMapping
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"DISEntityTypeMapping"};
    return types;
  }

  /**
   * @brief Sets the value of children. AccessType: inputOutput
   * @details mapping field provides a mechanism for automatically creating an
   * X3D model when a new entity arrives over the network.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of port. AccessType: inputOutput
   * @details Multicast network port, for example: 3000.
   * @return SFInt32 The current value of port.
   */
  SFInt32 getPort() const { return _port; }

  /**
   * @brief Sets the value of port. AccessType: inputOutput
   * @details Multicast network port, for example: 3000.
   * @param value The new value for port.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * @brief Gets the value of removedEntities. AccessType: outputOnly
   * @details removedEntities output array provides EspduTransform references to
   * any entities removed during last frame, either due to a timeout or from an
   * explicit RemoveEntityPDU action.
   * @return MFNode The current value of removedEntities.
   */
  MFNode getRemovedEntities() const { return _removedEntities; }

  /**
   * @brief Emit an output value on removedEntities. AccessType: outputOnly
   * @details removedEntities output array provides EspduTransform references to
   * any entities removed during last frame, either due to a timeout or from an
   * explicit RemoveEntityPDU action. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitRemovedEntities(const MFNode &value) { _removedEntities = value; }

  /**
   * @brief Acceptable node types for the removedEntities field.
   * @details Permitted X3D node types: EspduTransform
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRemovedEntitiesNodeTypes() {
    static const std::vector<std::string> types = {"EspduTransform"};
    return types;
  }

  /**
   * @brief Gets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @return SFInt32 The current value of siteID.
   */
  SFInt32 getSiteID() const { return _siteID; }

  /**
   * @brief Sets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @param value The new value for siteID.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

  /**
   * @brief The X3D type name of this node (e.g. "DISEntityManager").
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
   * @brief Member variable for addedEntities.
   */

  MFNode _addedEntities{};

  /**
   * @brief Member variable for address.
   */

  SFString _address{"localhost"};

  /**
   * @brief Member variable for applicationID.
   */

  SFInt32 _applicationID{0};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for port.
   */

  SFInt32 _port{0};

  /**
   * @brief Member variable for removedEntities.
   */

  MFNode _removedEntities{};

  /**
   * @brief Member variable for siteID.
   */

  SFInt32 _siteID{0};
};

} // namespace x3d::nodes
