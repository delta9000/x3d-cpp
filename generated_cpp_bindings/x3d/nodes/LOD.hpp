// LOD.hpp
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
 * @class LOD
 * @brief LOD (Level Of Detail) uses camera-to-object distance to switch among
 * contained child levels.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#LOD
 */
class LOD : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for LOD
   */
  LOD() = default;

  /**
   * @brief Destructor for LOD
   */
  ~LOD() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for forceTransitions
   * @return SFBool The default value
   */
  static SFBool getDefaultForceTransitions() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesGroupLODShapeTransformSwitch";
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
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of center. AccessType: initializeOnly
   * @details Viewpoint distance-measurement offset from origin of local
   * coordinate system, used for LOD node distance calculations.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }
  /**
   * @brief Data-layer write of center (reader/init ingest path).
   * @details center is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCenter().
   */
  void setCenterUnchecked(const SFVec3f &value) { _center = value; }

  /**
   * @brief Gets the value of forceTransitions. AccessType: initializeOnly
   * @details Whether to perform every range-based transition, regardless of
   * browser optimizations that might otherwise occur.
   * @return SFBool The current value of forceTransitions.
   */
  SFBool getForceTransitions() const { return _forceTransitions; }
  /**
   * @brief Data-layer write of forceTransitions (reader/init ingest path).
   * @details forceTransitions is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setForceTransitions().
   */
  void setForceTransitionsUnchecked(const SFBool &value) {
    _forceTransitions = value;
  }

  /**
   * @brief Gets the value of level_changed. AccessType: outputOnly
   * @details Output event that reports current level of LOD children whenever
   * switching occurs.
   * @return SFInt32 The current value of level_changed.
   */
  SFInt32 getLevel_changed() const { return _level_changed; }

  /**
   * @brief Emit an output value on level_changed. AccessType: outputOnly
   * @details Output event that reports current level of LOD children whenever
   * switching occurs. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitLevel_changed(const SFInt32 &value) { _level_changed = value; }

  /**
   * @brief Gets the value of range. AccessType: initializeOnly
   * @details Specifies ideal distances at which to switch between levels.
   * @return MFFloat The current value of range.
   */
  MFFloat getRange() const { return _range; }
  /**
   * @brief Data-layer write of range (reader/init ingest path).
   * @details range is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRange().
   */
  void setRangeUnchecked(const MFFloat &value) { _range = value; }

  /**
   * @brief The X3D type name of this node (e.g. "LOD").
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

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for forceTransitions.
   */

  SFBool _forceTransitions{false};

  /**
   * @brief Member variable for level_changed.
   */

  SFInt32 _level_changed{};

  /**
   * @brief Member variable for range.
   */

  MFFloat _range{};
};

} // namespace x3d::nodes
