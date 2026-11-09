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
 * @class X3DDamperNode
 * @brief The X3DDamperNode abstract node type creates an IIR response that
 * approaches the destination value according to the shape of the e-function
 * only asymptotically but very quickly.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#X3DDamperNode
 */
class X3DDamperNode : public virtual X3DFollowerNode {
public:
  X3DDamperNode() = default;

  virtual ~X3DDamperNode() = default;

  static SFInt32 getDefaultOrder() { return 3; }

  static SFTime getDefaultTau() { return 0.3; }

  static SFFloat getDefaultTolerance() { return -1; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFInt32 &getOrder() const { return _order; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFTime &getTau() const { return _tau; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTau(const SFTime &value) {

    validateTau(value);

    _tau = value;
  }

  void setTau(SFTime &&value) {

    validateTau(value);

    _tau = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTau() for range validation.
   */
  void setTauUnchecked(const SFTime &value) { _tau = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getTolerance() const { return _tolerance; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTolerance(const SFFloat &value) { _tolerance = value; }

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
  static void checkRangesOrder(const SFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTau(const SFTime &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out);

private:
  static void validateTau(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("tau below minimum of 0");
  }

  SFInt32 _order{3};

  SFTime _tau{0.3};

  SFFloat _tolerance{-1};
};

} // namespace x3d::nodes
