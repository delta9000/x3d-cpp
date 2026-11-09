#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DUrlObject
 * @brief X3DUrlObject indicates that a node has content loaded from a Uniform
 * Resource Locator (URL) and can be tracked via a LoadSensor. Such child nodes
 * have containerField='children' to indicate their relationship to the parent
 * LoadSensor node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#X3DUrlObject
 */
class X3DUrlObject {
public:
  X3DUrlObject() = default;

  virtual ~X3DUrlObject() = default;

  static SFTime getDefaultAutoRefresh() { return 0; }

  static SFTime getDefaultAutoRefreshTimeLimit() { return 3600; }

  static SFBool getDefaultLoad() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getAutoRefresh() const { return _autoRefresh; }

  /**
   * Access: inputOutput.
   * @details
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAutoRefresh() for range validation.
   */
  void setAutoRefreshUnchecked(const SFTime &value) { _autoRefresh = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getAutoRefreshTimeLimit() const {
    return _autoRefreshTimeLimit;
  }

  /**
   * Access: inputOutput.
   * @details
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAutoRefreshTimeLimit() for range validation.
   */
  void setAutoRefreshTimeLimitUnchecked(const SFTime &value) {
    _autoRefreshTimeLimit = value;
  }

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
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getLoad() const { return _load; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setLoad(const SFBool &value) { _load = value; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFString &getUrl() const { return _url; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setUrl(const MFString &value) { _url = value; }

  void setUrl(MFString &&value) { _url = std::move(value); }

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

  SFTime _autoRefresh{0};

  SFTime _autoRefreshTimeLimit{3600};

  SFString _description{};

  SFBool _load{true};

  MFString _url{};
};

} // namespace x3d::nodes
