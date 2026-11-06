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
 * @class X3DInterpolatorNode
 * @brief Interpolator nodes are designed for linear keyframed animation.
 * Interpolators are driven by an input key ranging [0..1] and produce
 * corresponding piecewise-linear output functions.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#X3DInterpolatorNode
 */
class X3DInterpolatorNode : public virtual X3DChildNode {
public:
  X3DInterpolatorNode() = default;

  virtual ~X3DInterpolatorNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 1; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFFloat &getKey() const { return _key; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setKey(const MFFloat &value) { _key = value; }

  void setKey(MFFloat &&value) { _key = std::move(value); }

  /**
   * SFFloat; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnSet_fractionHandler(); a no-op if none is set.
   */
  void onSet_fraction(const SFFloat &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_fractionHandler(std::function<void(const SFFloat &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

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
  MFFloat _key{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};
};

} // namespace x3d::nodes
