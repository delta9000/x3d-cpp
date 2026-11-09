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

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DNetworkSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class LoadSensor
 * @brief LoadSensor generates events as watchList child nodes are either loaded
 * or fail to load.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#LoadSensor
 */
class LoadSensor : public virtual X3DNetworkSensorNode {
public:
  LoadSensor() = default;

  ~LoadSensor() = default;

  static SFTime getDefaultTimeOut() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 3; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The children field monitors one or more USE nodes that contain a
   * valid url field.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: X3DUrlObject
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DUrlObject"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The children field monitors one or more USE nodes that contain a
   * valid url field.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Notify when all watchList child nodes are loaded, or at least one
   * has failed.
   */
  const SFBool &getIsLoaded() const { return _isLoaded; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Notify when all watchList child nodes are loaded, or at least one
   * has failed.
   */
  void emitIsLoaded(const SFBool &value) { _isLoaded = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time of successful load complete, not sent on failure.
   */
  const SFTime &getLoadTime() const { return _loadTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time of successful load complete, not sent on failure.
   */
  void emitLoadTime(const SFTime &value) { _loadTime = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Sends 0.
   */
  const SFFloat &getProgress() const { return _progress; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends 0.
   */
  void emitProgress(const SFFloat &value) { _progress = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Time in seconds of maximum load duration prior to declaring
   * failure.
   */
  const SFTime &getTimeOut() const { return _timeOut; }

  /**
   * Access: inputOutput.
   * @details Time in seconds of maximum load duration prior to declaring
   * failure.
   */
  void setTimeOut(const SFTime &value) {

    validateTimeOut(value);

    _timeOut = value;
  }

  void setTimeOut(SFTime &&value) {

    validateTimeOut(value);

    _timeOut = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTimeOut() for range validation.
   */
  void setTimeOutUnchecked(const SFTime &value) { _timeOut = value; }

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
  static void checkRangesTimeOut(const SFTime &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

private:
  static void validateTimeOut(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("timeOut below minimum of 0");
  }

  MFNode _children{};

  SFBool _isLoaded{};

  SFTime _loadTime{};

  SFFloat _progress{};

  SFTime _timeOut{0};
};

} // namespace x3d::nodes
