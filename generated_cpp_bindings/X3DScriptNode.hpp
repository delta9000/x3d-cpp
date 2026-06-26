// X3DScriptNode.hpp
#ifndef X3DSCRIPTNODE_HPP
#define X3DSCRIPTNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DUrlObject.hpp"

/**
 * @class X3DScriptNode
 * @brief Base type for scripting nodes (but not shader nodes).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/scripting.html#X3DScriptNode
 */
class X3DScriptNode : public virtual X3DChildNode, public virtual X3DUrlObject {
public:
  /**
   * @brief Default constructor for X3DScriptNode
   */
  X3DScriptNode() = default;

  /**
   * @brief Virtual destructor for X3DScriptNode
   */
  virtual ~X3DScriptNode() = default;

  /**
   * @brief Get the default value for autoRefresh
   * @return SFTime The default value
   */
  static SFTime getDefaultAutoRefresh() { return 0; }

  /**
   * @brief Get the default value for autoRefreshTimeLimit
   * @return SFTime The default value
   */
  static SFTime getDefaultAutoRefreshTimeLimit() { return 3600; }

  /**
   * @brief Get the default value for IS
   * @return SFNode The default value
   */
  static SFNode getDefaultIS() { return nullptr; }

  /**
   * @brief Get the default value for load
   * @return SFBool The default value
   */
  static SFBool getDefaultLoad() { return true; }

  /**
   * @brief Get the default value for metadata
   * @return SFNode The default value
   */
  static SFNode getDefaultMetadata() { return nullptr; }

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
  static std::string componentName() { return "Scripting"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of autoRefresh. AccessType: inputOutput
   * @details
   * @return SFTime The current value of autoRefresh.
   */
  SFTime getAutoRefresh() const { return _autoRefresh; }

  /**
   * @brief Sets the value of autoRefresh. AccessType: inputOutput
   * @details
   * @param value The new value for autoRefresh.
   */
  void setAutoRefresh(const SFTime &value) {

    validateAutoRefresh(value);

    _autoRefresh = value;
  }

  void setAutoRefresh(SFTime &&value) {

    validateAutoRefresh(value);

    _autoRefresh = std::move(value);
  }

  /**
   * @brief Non-validating write of autoRefresh (runtime/reader ingest path).
   * @details Assigns without the range check setAutoRefresh() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAutoRefresh() stays the
   *          enforcement point for programmatic callers.
   */
  void setAutoRefreshUnchecked(const SFTime &value) { _autoRefresh = value; }

  /**
   * @brief Gets the value of autoRefreshTimeLimit. AccessType: inputOutput
   * @details
   * @return SFTime The current value of autoRefreshTimeLimit.
   */
  SFTime getAutoRefreshTimeLimit() const { return _autoRefreshTimeLimit; }

  /**
   * @brief Sets the value of autoRefreshTimeLimit. AccessType: inputOutput
   * @details
   * @param value The new value for autoRefreshTimeLimit.
   */
  void setAutoRefreshTimeLimit(const SFTime &value) {

    validateAutoRefreshTimeLimit(value);

    _autoRefreshTimeLimit = value;
  }

  void setAutoRefreshTimeLimit(SFTime &&value) {

    validateAutoRefreshTimeLimit(value);

    _autoRefreshTimeLimit = std::move(value);
  }

  /**
   * @brief Non-validating write of autoRefreshTimeLimit (runtime/reader ingest
   * path).
   * @details Assigns without the range check setAutoRefreshTimeLimit()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setAutoRefreshTimeLimit() stays the
   *          enforcement point for programmatic callers.
   */
  void setAutoRefreshTimeLimitUnchecked(const SFTime &value) {
    _autoRefreshTimeLimit = value;
  }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of IS. AccessType: inputOutput
   * @details
   * @return SFNode The current value of IS.
   */
  SFNode getIS() const { return _IS; }

  /**
   * @brief Acceptable node types for the IS field.
   * @details Permitted X3D node types: IS
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableISNodeTypes() {
    static const std::vector<std::string> types = {"IS"};
    return types;
  }

  /**
   * @brief Sets the value of IS. AccessType: inputOutput
   * @details
   * @param value The new value for IS.
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * @brief Gets the value of load. AccessType: inputOutput
   * @details
   * @return SFBool The current value of load.
   */
  SFBool getLoad() const { return _load; }

  /**
   * @brief Sets the value of load. AccessType: inputOutput
   * @details
   * @param value The new value for load.
   */
  void setLoad(const SFBool &value) { _load = value; }

  /**
   * @brief Gets the value of metadata. AccessType: inputOutput
   * @details
   * @return SFNode The current value of metadata.
   */
  SFNode getMetadata() const { return _metadata; }

  /**
   * @brief Acceptable node types for the metadata field.
   * @details Permitted X3D node types: X3DMetadataObject
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMetadataNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * @brief Sets the value of metadata. AccessType: inputOutput
   * @details
   * @param value The new value for metadata.
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

  /**
   * @brief Gets the value of url. AccessType: inputOutput
   * @details
   * @return MFString The current value of url.
   */
  MFString getUrl() const { return _url; }

  /**
   * @brief Sets the value of url. AccessType: inputOutput
   * @details
   * @param value The new value for url.
   */
  void setUrl(const MFString &value) { _url = value; }

  void setUrl(MFString &&value) { _url = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DScriptNode").
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
  static void checkRangesAutoRefresh(const SFTime &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesAutoRefreshTimeLimit(
      const SFTime &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

private:
  static void validateAutoRefresh(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("autoRefresh below minimum of 0");
  }

  static void validateAutoRefreshTimeLimit(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("autoRefreshTimeLimit below minimum of 0");
  }

  /**
   * @brief Member variable for autoRefresh.
   */

  SFTime _autoRefresh{0};

  /**
   * @brief Member variable for autoRefreshTimeLimit.
   */

  SFTime _autoRefreshTimeLimit{3600};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for IS.
   */

  SFNode _IS{nullptr};

  /**
   * @brief Member variable for load.
   */

  SFBool _load{true};

  /**
   * @brief Member variable for metadata.
   */

  SFNode _metadata{nullptr};

  /**
   * @brief Member variable for url.
   */

  MFString _url{};
};

#endif // X3DSCRIPTNODE_HPP