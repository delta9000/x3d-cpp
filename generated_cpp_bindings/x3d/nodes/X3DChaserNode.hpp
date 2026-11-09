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
  X3DChaserNode() = default;

  virtual ~X3DChaserNode() = default;

  static SFTime getDefaultDuration() { return 1; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFTime &getDuration() const { return _duration; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setDurationUnchecked(const SFTime &value) { _duration = value; }

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
  static void checkRangesDuration(const SFTime &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  SFTime _duration{1};
};

} // namespace x3d::nodes
