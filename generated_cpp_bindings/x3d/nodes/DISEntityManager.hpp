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
  DISEntityManager() = default;

  ~DISEntityManager() = default;

  static SFString getDefaultAddress() { return "localhost"; }

  static SFInt32 getDefaultApplicationID() { return 0; }

  static SFInt32 getDefaultPort() { return 0; }

  static SFInt32 getDefaultSiteID() { return 0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesDISEntityTypeMapping";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "DIS"; }

  static int componentLevel() { return 2; }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details addedEntities array contains any new entities added during the
   * last frame.
   */
  const MFNode &getAddedEntities() const { return _addedEntities; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details addedEntities array contains any new entities added during the
   * last frame.
   */
  void emitAddedEntities(const MFNode &value) { _addedEntities = value; }

  /**
   * @details Permitted X3D node types: EspduTransform
   */
  static const std::vector<std::string> &acceptableAddedEntitiesNodeTypes() {
    static const std::vector<std::string> types = {"EspduTransform"};
    return types;
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Multicast network address, or else 'localhost'.
   */
  const SFString &getAddress() const { return _address; }

  /**
   * Access: inputOutput.
   * @details Multicast network address, or else 'localhost'.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   */
  const SFInt32 &getApplicationID() const { return _applicationID; }

  /**
   * Access: inputOutput.
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details mapping field provides a mechanism for automatically creating an
   * X3D model when a new entity arrives over the network.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: DISEntityTypeMapping
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"DISEntityTypeMapping"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details mapping field provides a mechanism for automatically creating an
   * X3D model when a new entity arrives over the network.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Multicast network port, for example: 3000.
   */
  const SFInt32 &getPort() const { return _port; }

  /**
   * Access: inputOutput.
   * @details Multicast network port, for example: 3000.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details removedEntities output array provides EspduTransform references to
   * any entities removed during last frame, either due to a timeout or from an
   * explicit RemoveEntityPDU action.
   */
  const MFNode &getRemovedEntities() const { return _removedEntities; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details removedEntities output array provides EspduTransform references to
   * any entities removed during last frame, either due to a timeout or from an
   * explicit RemoveEntityPDU action.
   */
  void emitRemovedEntities(const MFNode &value) { _removedEntities = value; }

  /**
   * @details Permitted X3D node types: EspduTransform
   */
  static const std::vector<std::string> &acceptableRemovedEntitiesNodeTypes() {
    static const std::vector<std::string> types = {"EspduTransform"};
    return types;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  const SFInt32 &getSiteID() const { return _siteID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

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
  MFNode _addedEntities{};

  SFString _address{"localhost"};

  SFInt32 _applicationID{0};

  MFNode _children{};

  SFInt32 _port{0};

  MFNode _removedEntities{};

  SFInt32 _siteID{0};
};

} // namespace x3d::nodes
