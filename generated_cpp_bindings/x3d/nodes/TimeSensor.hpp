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

#include "x3d/nodes/X3DTimeDependentNode.hpp"

#include "x3d/nodes/X3DSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TimeSensor
 * @brief TimeSensor continuously generates events as time passes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html#TimeSensor
 */
class TimeSensor : public virtual X3DTimeDependentNode,
                   public virtual X3DSensorNode {
public:
  TimeSensor() = default;

  ~TimeSensor() = default;

  static SFTime getDefaultCycleInterval() { return 1.0; }

  static SFBool getDefaultLoop() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Time"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details cycleInterval is loop duration in seconds.
   */
  const SFTime &getCycleInterval() const { return _cycleInterval; }

  /**
   * Access: inputOutput.
   * @details cycleInterval is loop duration in seconds.
   */
  void setCycleInterval(const SFTime &value) {

    validateCycleInterval(value);

    _cycleInterval = value;
  }

  void setCycleInterval(SFTime &&value) {

    validateCycleInterval(value);

    _cycleInterval = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setCycleInterval() for range validation.
   */
  void setCycleIntervalUnchecked(const SFTime &value) {
    _cycleInterval = value;
  }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details cycleTime sends a time outputOnly at startTime, and also at the
   * beginning of each new cycle (useful for synchronization with other
   * time-based objects).
   */
  const SFTime &getCycleTime() const { return _cycleTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details cycleTime sends a time outputOnly at startTime, and also at the
   * beginning of each new cycle (useful for synchronization with other
   * time-based objects).
   */
  void emitCycleTime(const SFTime &value) { _cycleTime = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details fraction_changed continuously sends value in range [0,1] showing
   * time progress in the current cycle.
   */
  const SFFloat &getFraction_changed() const { return _fraction_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details fraction_changed continuously sends value in range [0,1] showing
   * time progress in the current cycle.
   */
  void emitFraction_changed(const SFFloat &value) { _fraction_changed = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  const SFBool &getLoop() const { return _loop; }

  /**
   * Access: inputOutput.
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time continuously sends the absolute time (value 0.
   */
  const SFTime &getTime() const { return _time; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time continuously sends the absolute time (value 0.
   */
  void emitTime(const SFTime &value) { _time = value; }

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
  static void checkRangesCycleInterval(const SFTime &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateCycleInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("cycleInterval below minimum of 0");
  }

  SFTime _cycleInterval{1.0};

  SFTime _cycleTime{};

  SFFloat _fraction_changed{};

  SFBool _loop{false};

  SFTime _time{};
};

} // namespace x3d::nodes
