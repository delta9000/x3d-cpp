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
  Switch() = default;

  ~Switch() = default;

  static SFInt32 getDefaultWhichChoice() { return -1; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesGroupLODShapeTransformSwitch";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Grouping"; }

  static int componentLevel() { return 2; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Index of active child choice, counting from 0.
   */
  const SFInt32 &getWhichChoice() const { return _whichChoice; }

  /**
   * Access: inputOutput.
   * @details Index of active child choice, counting from 0.
   */
  void setWhichChoice(const SFInt32 &value) {

    validateWhichChoice(value);

    _whichChoice = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWhichChoice() for range validation.
   */
  void setWhichChoiceUnchecked(const SFInt32 &value) { _whichChoice = value; }

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
  static void checkRangesWhichChoice(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateWhichChoice(const SFInt32 &value) {

    if (value < -1)
      throw std::out_of_range("whichChoice below minimum of -1");
  }

  SFInt32 _whichChoice{-1};
};

} // namespace x3d::nodes
