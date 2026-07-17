// X3DChaserNode.hpp
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

#include "x3d/nodes/X3DFollowerNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DChaserNode
 * @brief The X3DChaserNode abstract node type calculates the output on
 * value_changed as a finite impulse response (FIR) based on the events received
 * on set_destination field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#X3DChaserNode
 */
class X3DChaserNode : public virtual X3DFollowerNode {
public:
  /**
   * @brief Default constructor for X3DChaserNode
   */
  X3DChaserNode() = default;

  /**
   * @brief Virtual destructor for X3DChaserNode
   */
  virtual ~X3DChaserNode() = default;

  /**
   * @brief Get the default value for duration
   * @return SFTime The default value
   */
  static SFTime getDefaultDuration() { return 1; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Followers"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of duration. AccessType: initializeOnly
   * @details
   * @return SFTime The current value of duration.
   */
  SFTime getDuration() const { return _duration; }
  /**
   * @brief Data-layer write of duration (reader/init ingest path).
   * @details duration is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setDuration().
   */
  void setDurationUnchecked(const SFTime &value) { _duration = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DChaserNode").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDuration(const SFTime &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for duration.
   */

  SFTime _duration{1};
};

} // namespace x3d::nodes
