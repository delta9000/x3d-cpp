// GeoLOD.hpp
#ifndef GEOLOD_HPP
#define GEOLOD_HPP

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

/**
 * @class GeoLOD
 * @brief Note that MFNode rootNode field can contain multiple nodes and has
 * accessType inputOutput. Meanwhile MFNode children field is outputOnly, unlike
 * other X3DGroupingNode exemplars.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoLOD
 */
class GeoLOD : public virtual X3DChildNode, public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for GeoLOD
   */
  GeoLOD() = default;

  /**
   * @brief Destructor for GeoLOD
   */
  ~GeoLOD() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultCenter() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for geoOrigin
   * @return SFNode The default value
   */
  static SFNode getDefaultGeoOrigin() { return nullptr; }

  /**
   * @brief Get the default value for geoSystem
   * @return MFString The default value
   */
  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  /**
   * @brief Get the default value for range
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRange() { return 10; }

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
  static std::string componentName() { return "Geospatial"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of center. AccessType: initializeOnly
   * @details Viewer range from geographic-coordinates center triggers quadtree
   * loading/unloading.
   * @return SFVec3d The current value of center.
   */
  SFVec3d getCenter() const { return _center; }
  /**
   * @brief Data-layer write of center (reader/init ingest path).
   * @details center is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCenter().
   */
  void setCenterUnchecked(const SFVec3d &value) { _center = value; }

  /**
   * @brief Gets the value of child1Url. AccessType: initializeOnly
   * @details quadtree geometry loaded when viewer is within range.
   * @return MFString The current value of child1Url.
   */
  MFString getChild1Url() const { return _child1Url; }
  /**
   * @brief Data-layer write of child1Url (reader/init ingest path).
   * @details child1Url is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setChild1Url().
   */
  void setChild1UrlUnchecked(const MFString &value) { _child1Url = value; }

  /**
   * @brief Gets the value of child2Url. AccessType: initializeOnly
   * @details quadtree geometry loaded when viewer is within range.
   * @return MFString The current value of child2Url.
   */
  MFString getChild2Url() const { return _child2Url; }
  /**
   * @brief Data-layer write of child2Url (reader/init ingest path).
   * @details child2Url is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setChild2Url().
   */
  void setChild2UrlUnchecked(const MFString &value) { _child2Url = value; }

  /**
   * @brief Gets the value of child3Url. AccessType: initializeOnly
   * @details quadtree geometry loaded when viewer is within range.
   * @return MFString The current value of child3Url.
   */
  MFString getChild3Url() const { return _child3Url; }
  /**
   * @brief Data-layer write of child3Url (reader/init ingest path).
   * @details child3Url is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setChild3Url().
   */
  void setChild3UrlUnchecked(const MFString &value) { _child3Url = value; }

  /**
   * @brief Gets the value of child4Url. AccessType: initializeOnly
   * @details quadtree geometry loaded when viewer is within range.
   * @return MFString The current value of child4Url.
   */
  MFString getChild4Url() const { return _child4Url; }
  /**
   * @brief Data-layer write of child4Url (reader/init ingest path).
   * @details child4Url is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setChild4Url().
   */
  void setChild4UrlUnchecked(const MFString &value) { _child4Url = value; }

  /**
   * @brief Gets the value of children. AccessType: outputOnly
   * @details The outputOnly children field exposes a portion of the scene graph
   * for the currently loaded set of nodes.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Emit an output value on children. AccessType: outputOnly
   * @details The outputOnly children field exposes a portion of the scene graph
   * for the currently loaded set of nodes. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitChildren(const MFNode &value) { _children = value; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * @brief Gets the value of geoOrigin. AccessType: initializeOnly
   * @details Single contained GeoOrigin node that can specify a local
   * coordinate frame for extended precision.
   * @return SFNode The current value of geoOrigin.
   */
  SFNode getGeoOrigin() const { return _geoOrigin; }

  /**
   * @brief Acceptable node types for the geoOrigin field.
   * @details Permitted X3D node types: GeoOrigin
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeoOriginNodeTypes() {
    static const std::vector<std::string> types = {"GeoOrigin"};
    return types;
  }
  /**
   * @brief Data-layer write of geoOrigin (reader/init ingest path).
   * @details geoOrigin is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoOrigin().
   */
  void setGeoOriginUnchecked(const SFNode &value) { _geoOrigin = value; }

  /**
   * @brief Gets the value of geoSystem. AccessType: initializeOnly
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   * @return MFString The current value of geoSystem.
   */
  MFString getGeoSystem() const { return _geoSystem; }
  /**
   * @brief Data-layer write of geoSystem (reader/init ingest path).
   * @details geoSystem is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoSystem().
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * @brief Gets the value of level_changed. AccessType: outputOnly
   * @details Output event that reports when the new children outputOnly event
   * is generated, with value 0 or 1, where 0 indicates the rootNode field and 1
   * indicates the nodes specified by the child1Url, child2Url, child3Url, and
   * child4Url fields.
   * @return SFInt32 The current value of level_changed.
   */
  SFInt32 getLevel_changed() const { return _level_changed; }

  /**
   * @brief Emit an output value on level_changed. AccessType: outputOnly
   * @details Output event that reports when the new children outputOnly event
   * is generated, with value 0 or 1, where 0 indicates the rootNode field and 1
   * indicates the nodes specified by the child1Url, child2Url, child3Url, and
   * child4Url fields. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitLevel_changed(const SFInt32 &value) { _level_changed = value; }

  /**
   * @brief Gets the value of range. AccessType: initializeOnly
   * @details Viewer range from geographic-coordinates center triggers quadtree
   * loading/unloading.
   * @return SFFloat The current value of range.
   */
  SFFloat getRange() const { return _range; }
  /**
   * @brief Data-layer write of range (reader/init ingest path).
   * @details range is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRange().
   */
  void setRangeUnchecked(const SFFloat &value) { _range = value; }

  /**
   * @brief Gets the value of rootNode. AccessType: initializeOnly
   * @details Geometry for the root tile.
   * @return MFNode The current value of rootNode.
   */
  MFNode getRootNode() const { return _rootNode; }

  /**
   * @brief Acceptable node types for the rootNode field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRootNodeNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }
  /**
   * @brief Data-layer write of rootNode (reader/init ingest path).
   * @details rootNode is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRootNode().
   */
  void setRootNodeUnchecked(const MFNode &value) { _rootNode = value; }

  /**
   * @brief Gets the value of rootUrl. AccessType: initializeOnly
   * @details url for scene providing geometry for the root tile.
   * @return MFString The current value of rootUrl.
   */
  MFString getRootUrl() const { return _rootUrl; }
  /**
   * @brief Data-layer write of rootUrl (reader/init ingest path).
   * @details rootUrl is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRootUrl().
   */
  void setRootUrlUnchecked(const MFString &value) { _rootUrl = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoLOD").
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
   * @brief Member variable for center.
   */

  SFVec3d _center{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for child1Url.
   */

  MFString _child1Url{};

  /**
   * @brief Member variable for child2Url.
   */

  MFString _child2Url{};

  /**
   * @brief Member variable for child3Url.
   */

  MFString _child3Url{};

  /**
   * @brief Member variable for child4Url.
   */

  MFString _child4Url{};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for level_changed.
   */

  SFInt32 _level_changed{};

  /**
   * @brief Member variable for range.
   */

  SFFloat _range{10};

  /**
   * @brief Member variable for rootNode.
   */

  MFNode _rootNode{};

  /**
   * @brief Member variable for rootUrl.
   */

  MFString _rootUrl{};
};

#endif // GEOLOD_HPP