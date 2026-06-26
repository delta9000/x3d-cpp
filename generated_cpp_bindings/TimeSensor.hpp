// TimeSensor.hpp
#ifndef TIMESENSOR_HPP
#define TIMESENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DTimeDependentNode.hpp"

#include "X3DSensorNode.hpp"

/**
 * @class TimeSensor
 * @brief TimeSensor continuously generates events as time passes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html#TimeSensor
 */
class TimeSensor : public virtual X3DTimeDependentNode,
                   public virtual X3DSensorNode {
public:
  /**
   * @brief Default constructor for TimeSensor
   */
  TimeSensor() = default;

  /**
   * @brief Destructor for TimeSensor
   */
  ~TimeSensor() = default;

  /**
   * @brief Get the default value for cycleInterval
   * @return SFTime The default value
   */
  static SFTime getDefaultCycleInterval() { return 1.0; }

  /**
   * @brief Get the default value for loop
   * @return SFBool The default value
   */
  static SFBool getDefaultLoop() { return false; }

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
  static std::string componentName() { return "Time"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of cycleInterval. AccessType: inputOutput
   * @details cycleInterval is loop duration in seconds.
   * @return SFTime The current value of cycleInterval.
   */
  SFTime getCycleInterval() const { return _cycleInterval; }

  /**
   * @brief Sets the value of cycleInterval. AccessType: inputOutput
   * @details cycleInterval is loop duration in seconds.
   * @param value The new value for cycleInterval.
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
   * @brief Non-validating write of cycleInterval (runtime/reader ingest path).
   * @details Assigns without the range check setCycleInterval() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setCycleInterval() stays the
   *          enforcement point for programmatic callers.
   */
  void setCycleIntervalUnchecked(const SFTime &value) {
    _cycleInterval = value;
  }

  /**
   * @brief Gets the value of cycleTime. AccessType: outputOnly
   * @details cycleTime sends a time outputOnly at startTime, and also at the
   * beginning of each new cycle (useful for synchronization with other
   * time-based objects).
   * @return SFTime The current value of cycleTime.
   */
  SFTime getCycleTime() const { return _cycleTime; }

  /**
   * @brief Emit an output value on cycleTime. AccessType: outputOnly
   * @details cycleTime sends a time outputOnly at startTime, and also at the
   * beginning of each new cycle (useful for synchronization with other
   * time-based objects). outputOnly fields have no author-facing setter; a
   * node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitCycleTime(const SFTime &value) { _cycleTime = value; }

  /**
   * @brief Gets the value of fraction_changed. AccessType: outputOnly
   * @details fraction_changed continuously sends value in range [0,1] showing
   * time progress in the current cycle.
   * @return SFFloat The current value of fraction_changed.
   */
  SFFloat getFraction_changed() const { return _fraction_changed; }

  /**
   * @brief Emit an output value on fraction_changed. AccessType: outputOnly
   * @details fraction_changed continuously sends value in range [0,1] showing
   * time progress in the current cycle. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitFraction_changed(const SFFloat &value) { _fraction_changed = value; }

  /**
   * @brief Gets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @return SFBool The current value of loop.
   */
  SFBool getLoop() const { return _loop; }

  /**
   * @brief Sets the value of loop. AccessType: inputOutput
   * @details Repeat indefinitely when loop=true, repeat only once when
   * loop=false.
   * @param value The new value for loop.
   */
  void setLoop(const SFBool &value) { _loop = value; }

  /**
   * @brief Gets the value of time. AccessType: outputOnly
   * @details Time continuously sends the absolute time (value 0.
   * @return SFTime The current value of time.
   */
  SFTime getTime() const { return _time; }

  /**
   * @brief Emit an output value on time. AccessType: outputOnly
   * @details Time continuously sends the absolute time (value 0.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTime(const SFTime &value) { _time = value; }

  /**
   * @brief The X3D type name of this node (e.g. "TimeSensor").
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
  static void checkRangesCycleInterval(const SFTime &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateCycleInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("cycleInterval below minimum of 0");
  }

  /**
   * @brief Member variable for cycleInterval.
   */

  SFTime _cycleInterval{1.0};

  /**
   * @brief Member variable for cycleTime.
   */

  SFTime _cycleTime{};

  /**
   * @brief Member variable for fraction_changed.
   */

  SFFloat _fraction_changed{};

  /**
   * @brief Member variable for loop.
   */

  SFBool _loop{false};

  /**
   * @brief Member variable for time.
   */

  SFTime _time{};
};

#endif // TIMESENSOR_HPP