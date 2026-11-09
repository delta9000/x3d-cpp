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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DTimeDependentNode
 * @brief Base type from which all time-dependent nodes are derived.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/time.html#X3DTimeDependentNode
 */
class X3DTimeDependentNode : public virtual X3DChildNode {
public:
  X3DTimeDependentNode() = default;

  virtual ~X3DTimeDependentNode() = default;

  static SFTime getDefaultPauseTime() { return 0; }

  static SFTime getDefaultResumeTime() { return 0; }

  static SFTime getDefaultStartTime() { return 0; }

  static SFTime getDefaultStopTime() { return 0; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Time"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFTime &getElapsedTime() const { return _elapsedTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitElapsedTime(const SFTime &value) { _elapsedTime = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsActive() const { return _isActive; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsPaused() const { return _isPaused; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsPaused(const SFBool &value) { _isPaused = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getPauseTime() const { return _pauseTime; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setPauseTime(const SFTime &value) { _pauseTime = value; }

  void setPauseTime(SFTime &&value) { _pauseTime = std::move(value); }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getResumeTime() const { return _resumeTime; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setResumeTime(const SFTime &value) { _resumeTime = value; }

  void setResumeTime(SFTime &&value) { _resumeTime = std::move(value); }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getStartTime() const { return _startTime; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setStartTime(const SFTime &value) { _startTime = value; }

  void setStartTime(SFTime &&value) { _startTime = std::move(value); }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getStopTime() const { return _stopTime; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setStopTime(const SFTime &value) { _stopTime = value; }

  void setStopTime(SFTime &&value) { _stopTime = std::move(value); }

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
  SFString _description{};

  SFTime _elapsedTime{};

  SFBool _isActive{};

  SFBool _isPaused{};

  SFTime _pauseTime{0};

  SFTime _resumeTime{0};

  SFTime _startTime{0};

  SFTime _stopTime{0};
};

} // namespace x3d::nodes
