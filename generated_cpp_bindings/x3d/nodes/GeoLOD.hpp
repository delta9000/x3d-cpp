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
 * @class GeoLOD
 * @brief Note that MFNode rootNode field can contain multiple nodes and has
 * accessType inputOutput. Meanwhile MFNode children field is outputOnly, unlike
 * other X3DGroupingNode exemplars.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoLOD
 */
class GeoLOD : public virtual X3DChildNode, public virtual X3DBoundedObject {
public:
  GeoLOD() = default;

  ~GeoLOD() = default;

  static SFVec3d getDefaultCenter() { return SFVec3d{0, 0, 0}; }

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFFloat getDefaultRange() { return 10; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3d; access: initializeOnly. Borrows the stored value.
   * @details Viewer range from geographic-coordinates center triggers quadtree
   * loading/unloading.
   */
  const SFVec3d &getCenter() const { return _center; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCenterUnchecked(const SFVec3d &value) { _center = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details quadtree geometry loaded when viewer is within range.
   */
  const MFString &getChild1Url() const { return _child1Url; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setChild1UrlUnchecked(const MFString &value) { _child1Url = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details quadtree geometry loaded when viewer is within range.
   */
  const MFString &getChild2Url() const { return _child2Url; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setChild2UrlUnchecked(const MFString &value) { _child2Url = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details quadtree geometry loaded when viewer is within range.
   */
  const MFString &getChild3Url() const { return _child3Url; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setChild3UrlUnchecked(const MFString &value) { _child3Url = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details quadtree geometry loaded when viewer is within range.
   */
  const MFString &getChild4Url() const { return _child4Url; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setChild4UrlUnchecked(const MFString &value) { _child4Url = value; }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details The outputOnly children field exposes a portion of the scene graph
   * for the currently loaded set of nodes.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details The outputOnly children field exposes a portion of the scene graph
   * for the currently loaded set of nodes.
   */
  void emitChildren(const MFNode &value) { _children = value; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details Single contained GeoOrigin node that can specify a local
   * coordinate frame for extended precision.
   */
  const SFNode &getGeoOrigin() const { return _geoOrigin; }

  /**
   * @details Permitted X3D node types: GeoOrigin
   */
  static const std::vector<std::string> &acceptableGeoOriginNodeTypes() {
    static const std::vector<std::string> types = {"GeoOrigin"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoOriginUnchecked(const SFNode &value) { _geoOrigin = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   */
  const MFString &getGeoSystem() const { return _geoSystem; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details Output event that reports when the new children outputOnly event
   * is generated, with value 0 or 1, where 0 indicates the rootNode field and 1
   * indicates the nodes specified by the child1Url, child2Url, child3Url, and
   * child4Url fields.
   */
  const SFInt32 &getLevel_changed() const { return _level_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event that reports when the new children outputOnly event
   * is generated, with value 0 or 1, where 0 indicates the rootNode field and 1
   * indicates the nodes specified by the child1Url, child2Url, child3Url, and
   * child4Url fields.
   */
  void emitLevel_changed(const SFInt32 &value) { _level_changed = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Viewer range from geographic-coordinates center triggers quadtree
   * loading/unloading.
   */
  const SFFloat &getRange() const { return _range; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRangeUnchecked(const SFFloat &value) { _range = value; }

  /**
   * MFNode; access: initializeOnly. Borrows the stored value.
   * @details Geometry for the root tile.
   */
  const MFNode &getRootNode() const { return _rootNode; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableRootNodeNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRootNodeUnchecked(const MFNode &value) { _rootNode = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details url for scene providing geometry for the root tile.
   */
  const MFString &getRootUrl() const { return _rootUrl; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRootUrlUnchecked(const MFString &value) { _rootUrl = value; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRange(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  SFVec3d _center{SFVec3d{0, 0, 0}};

  MFString _child1Url{};

  MFString _child2Url{};

  MFString _child3Url{};

  MFString _child4Url{};

  MFNode _children{};

  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFInt32 _level_changed{};

  SFFloat _range{10};

  MFNode _rootNode{};

  MFString _rootUrl{};
};

} // namespace x3d::nodes
