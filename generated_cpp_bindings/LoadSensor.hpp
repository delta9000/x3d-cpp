// LoadSensor.hpp
#ifndef LOADSENSOR_HPP
#define LOADSENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DNetworkSensorNode.hpp"

/**
 * @class LoadSensor
 * @brief LoadSensor generates events as watchList child nodes are either loaded
 * or fail to load.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#LoadSensor
 */
class LoadSensor : public virtual X3DNetworkSensorNode {
public:
  /**
   * @brief Default constructor for LoadSensor
   */
  LoadSensor() = default;

  /**
   * @brief Destructor for LoadSensor
   */
  ~LoadSensor() = default;

  /**
   * @brief Get the default value for timeOut
   * @return SFTime The default value
   */
  static SFTime getDefaultTimeOut() { return 0; }

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
  static std::string componentName() { return "Networking"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details The children field monitors one or more USE nodes that contain a
   * valid url field.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: X3DUrlObject
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DUrlObject"};
    return types;
  }

  /**
   * @brief Sets the value of children. AccessType: inputOutput
   * @details The children field monitors one or more USE nodes that contain a
   * valid url field.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of isLoaded. AccessType: outputOnly
   * @details Notify when all watchList child nodes are loaded, or at least one
   * has failed.
   * @return SFBool The current value of isLoaded.
   */
  SFBool getIsLoaded() const { return _isLoaded; }

  /**
   * @brief Emit an output value on isLoaded. AccessType: outputOnly
   * @details Notify when all watchList child nodes are loaded, or at least one
   * has failed. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsLoaded(const SFBool &value) { _isLoaded = value; }

  /**
   * @brief Gets the value of loadTime. AccessType: outputOnly
   * @details Time of successful load complete, not sent on failure.
   * @return SFTime The current value of loadTime.
   */
  SFTime getLoadTime() const { return _loadTime; }

  /**
   * @brief Emit an output value on loadTime. AccessType: outputOnly
   * @details Time of successful load complete, not sent on failure.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitLoadTime(const SFTime &value) { _loadTime = value; }

  /**
   * @brief Gets the value of progress. AccessType: outputOnly
   * @details Sends 0.
   * @return SFFloat The current value of progress.
   */
  SFFloat getProgress() const { return _progress; }

  /**
   * @brief Emit an output value on progress. AccessType: outputOnly
   * @details Sends 0.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitProgress(const SFFloat &value) { _progress = value; }

  /**
   * @brief Gets the value of timeOut. AccessType: inputOutput
   * @details Time in seconds of maximum load duration prior to declaring
   * failure.
   * @return SFTime The current value of timeOut.
   */
  SFTime getTimeOut() const { return _timeOut; }

  /**
   * @brief Sets the value of timeOut. AccessType: inputOutput
   * @details Time in seconds of maximum load duration prior to declaring
   * failure.
   * @param value The new value for timeOut.
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
   * @brief Non-validating write of timeOut (runtime/reader ingest path).
   * @details Assigns without the range check setTimeOut() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTimeOut() stays the
   *          enforcement point for programmatic callers.
   */
  void setTimeOutUnchecked(const SFTime &value) { _timeOut = value; }

  /**
   * @brief The X3D type name of this node (e.g. "LoadSensor").
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
  static void checkRangesTimeOut(const SFTime &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

private:
  static void validateTimeOut(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("timeOut below minimum of 0");
  }

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Member variable for isLoaded.
   */

  SFBool _isLoaded{};

  /**
   * @brief Member variable for loadTime.
   */

  SFTime _loadTime{};

  /**
   * @brief Member variable for progress.
   */

  SFFloat _progress{};

  /**
   * @brief Member variable for timeOut.
   */

  SFTime _timeOut{0};
};

#endif // LOADSENSOR_HPP