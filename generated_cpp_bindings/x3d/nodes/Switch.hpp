// Switch.hpp
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
 * @class Switch
 * @brief Switch is a Grouping node that only renders one (or zero) child at a
 * time.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#Switch
 */
class Switch : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for Switch
   */
  Switch() = default;

  /**
   * @brief Destructor for Switch
   */
  ~Switch() = default;

  /**
   * @brief Get the default value for whichChoice
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultWhichChoice() { return -1; }

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
  static std::string componentName() { return "Grouping"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of whichChoice. AccessType: inputOutput
   * @details Index of active child choice, counting from 0.
   * @return SFInt32 The current value of whichChoice.
   */
  SFInt32 getWhichChoice() const { return _whichChoice; }

  /**
   * @brief Sets the value of whichChoice. AccessType: inputOutput
   * @details Index of active child choice, counting from 0.
   * @param value The new value for whichChoice.
   */
  void setWhichChoice(const SFInt32 &value) {

    validateWhichChoice(value);

    _whichChoice = value;
  }

  /**
   * @brief Non-validating write of whichChoice (runtime/reader ingest path).
   * @details Assigns without the range check setWhichChoice() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setWhichChoice() stays the
   *          enforcement point for programmatic callers.
   */
  void setWhichChoiceUnchecked(const SFInt32 &value) { _whichChoice = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Switch").
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
  static void checkRangesWhichChoice(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateWhichChoice(const SFInt32 &value) {

    if (value < -1)
      throw std::out_of_range("whichChoice below minimum of -1");
  }

  /**
   * @brief Member variable for whichChoice.
   */

  SFInt32 _whichChoice{-1};
};

} // namespace x3d::nodes
