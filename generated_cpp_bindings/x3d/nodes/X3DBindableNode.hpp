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
 * @class X3DBindableNode
 * @brief Bindable nodes implement the binding stack, so that only one of each
 * node type is active at a given time.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DBindableNode
 */
class X3DBindableNode : public virtual X3DChildNode {
public:
  X3DBindableNode() = default;

  virtual ~X3DBindableNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFTime &getBindTime() const { return _bindTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitBindTime(const SFTime &value) { _bindTime = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsBound() const { return _isBound; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsBound(const SFBool &value) { _isBound = value; }

  /**
   * SFBool; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via setOnSet_bindHandler();
   *          a no-op if none is set.
   */
  void onSet_bind(const SFBool &value) {
    if (_onSet_bindHandler)
      _onSet_bindHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_bindHandler(std::function<void(const SFBool &)> handler) {
    _onSet_bindHandler = std::move(handler);
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
  SFTime _bindTime{};

  SFBool _isBound{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onSet_bindHandler{};
};

} // namespace x3d::nodes
