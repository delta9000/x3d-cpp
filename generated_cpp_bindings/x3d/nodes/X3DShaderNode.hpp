#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DShaderNode
 * @brief Base type for all nodes that specify a programmable shader.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#X3DShaderNode
 */
class X3DShaderNode : public virtual X3DAppearanceChildNode {
public:
  X3DShaderNode() = default;

  virtual ~X3DShaderNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via setOnActivateHandler();
   *          a no-op if none is set.
   */
  void onActivate(const SFBool &value) {
    if (_onActivateHandler)
      _onActivateHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnActivateHandler(std::function<void(const SFBool &)> handler) {
    _onActivateHandler = std::move(handler);
  }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsSelected() const { return _isSelected; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsSelected(const SFBool &value) { _isSelected = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsValid() const { return _isValid; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsValid(const SFBool &value) { _isValid = value; }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFString &getLanguage() const { return _language; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setLanguageUnchecked(const SFString &value) { _language = value; }

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
  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onActivateHandler{};

  SFBool _isSelected{};

  SFBool _isValid{};

  SFString _language{};
};

} // namespace x3d::nodes
