// X3DDamperNode.hpp
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
  /**
   * @brief Default constructor for X3DDamperNode
   */
  X3DDamperNode() = default;

  /**
   * @brief Virtual destructor for X3DDamperNode
   */
  virtual ~X3DDamperNode() = default;

  /**
   * @brief Get the default value for order
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultOrder() { return 3; }

  /**
   * @brief Get the default value for tau
   * @return SFTime The default value
   */
  static SFTime getDefaultTau() { return 0.3; }

  /**
   * @brief Get the default value for tolerance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTolerance() { return -1; }

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
  static std::string componentName() { return "Followers"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of order. AccessType: initializeOnly
   * @details
   * @return SFInt32 The current value of order.
   */
  SFInt32 getOrder() const { return _order; }
  /**
   * @brief Data-layer write of order (reader/init ingest path).
   * @details order is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setOrder().
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * @brief Gets the value of tau. AccessType: inputOutput
   * @details
   * @return SFTime The current value of tau.
   */
  SFTime getTau() const { return _tau; }

  /**
   * @brief Sets the value of tau. AccessType: inputOutput
   * @details
   * @param value The new value for tau.
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
   * @brief Non-validating write of tau (runtime/reader ingest path).
   * @details Assigns without the range check setTau() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTau() stays the
   *          enforcement point for programmatic callers.
   */
  void setTauUnchecked(const SFTime &value) { _tau = value; }

  /**
   * @brief Gets the value of tolerance. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of tolerance.
   */
  SFFloat getTolerance() const { return _tolerance; }

  /**
   * @brief Sets the value of tolerance. AccessType: inputOutput
   * @details
   * @param value The new value for tolerance.
   */
  void setTolerance(const SFFloat &value) { _tolerance = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DDamperNode").
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

  /**
   * @brief Member variable for order.
   */

  SFInt32 _order{3};

  /**
   * @brief Member variable for tau.
   */

  SFTime _tau{0.3};

  /**
   * @brief Member variable for tolerance.
   */

  SFFloat _tolerance{-1};
};

} // namespace x3d::nodes
