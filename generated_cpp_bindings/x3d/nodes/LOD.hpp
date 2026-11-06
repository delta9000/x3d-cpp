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
  LOD() = default;

  ~LOD() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultForceTransitions() { return false; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesGroupLODShapeTransformSwitch";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details Viewpoint distance-measurement offset from origin of local
   * coordinate system, used for LOD node distance calculations.
   */
  const SFVec3f &getCenter() const { return _center; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCenterUnchecked(const SFVec3f &value) { _center = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether to perform every range-based transition, regardless of
   * browser optimizations that might otherwise occur.
   */
  const SFBool &getForceTransitions() const { return _forceTransitions; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setForceTransitionsUnchecked(const SFBool &value) {
    _forceTransitions = value;
  }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details Output event that reports current level of LOD children whenever
   * switching occurs.
   */
  const SFInt32 &getLevel_changed() const { return _level_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event that reports current level of LOD children whenever
   * switching occurs.
   */
  void emitLevel_changed(const SFInt32 &value) { _level_changed = value; }

  /**
   * MFFloat; access: initializeOnly. Borrows the stored value.
   * @details Specifies ideal distances at which to switch between levels.
   */
  const MFFloat &getRange() const { return _range; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRangeUnchecked(const MFFloat &value) { _range = value; }

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
  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFBool _forceTransitions{false};

  SFInt32 _level_changed{};

  MFFloat _range{};
};

} // namespace x3d::nodes
