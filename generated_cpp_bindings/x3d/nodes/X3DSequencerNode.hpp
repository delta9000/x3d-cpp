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
 * @class X3DSequencerNode
 * @brief Base type from which all Sequencers are derived.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#X3DSequencerNode
 */
class X3DSequencerNode : public virtual X3DChildNode {
public:
  X3DSequencerNode() = default;

  virtual ~X3DSequencerNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

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
   * SFBool; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via setOnNextHandler();
   *          a no-op if none is set.
   */
  void onNext(const SFBool &value) {
    if (_onNextHandler)
      _onNextHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnNextHandler(std::function<void(const SFBool &)> handler) {
    _onNextHandler = std::move(handler);
  }

  /**
   * SFBool; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via setOnPreviousHandler();
   *          a no-op if none is set.
   */
  void onPrevious(const SFBool &value) {
    if (_onPreviousHandler)
      _onPreviousHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnPreviousHandler(std::function<void(const SFBool &)> handler) {
    _onPreviousHandler = std::move(handler);
  }

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
  std::function<void(const SFBool &)> _onNextHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onPreviousHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};
};

} // namespace x3d::nodes
