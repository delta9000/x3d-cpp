// X3DShaderNode.hpp
#ifndef X3DSHADERNODE_HPP
#define X3DSHADERNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DAppearanceChildNode.hpp"

/**
 * @class X3DShaderNode
 * @brief Base type for all nodes that specify a programmable shader.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#X3DShaderNode
 */
class X3DShaderNode : public virtual X3DAppearanceChildNode {
public:
  /**
   * @brief Default constructor for X3DShaderNode
   */
  X3DShaderNode() = default;

  /**
   * @brief Virtual destructor for X3DShaderNode
   */
  virtual ~X3DShaderNode() = default;

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
  static std::string componentName() { return "Shaders"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Event handler invoked when an event is received on activate.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via setOnActivateHandler();
   *          a no-op if none is set. The event cascade reaches this through the
   *          node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onActivate(const SFBool &value) {
    if (_onActivateHandler)
      _onActivateHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on activate.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnActivateHandler(std::function<void(const SFBool &)> handler) {
    _onActivateHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of isSelected. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isSelected.
   */
  SFBool getIsSelected() const { return _isSelected; }

  /**
   * @brief Emit an output value on isSelected. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsSelected(const SFBool &value) { _isSelected = value; }

  /**
   * @brief Gets the value of isValid. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isValid.
   */
  SFBool getIsValid() const { return _isValid; }

  /**
   * @brief Emit an output value on isValid. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsValid(const SFBool &value) { _isValid = value; }

  /**
   * @brief Gets the value of language. AccessType: initializeOnly
   * @details
   * @return SFString The current value of language.
   */
  SFString getLanguage() const { return _language; }
  /**
   * @brief Data-layer write of language (reader/init ingest path).
   * @details language is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setLanguage().
   */
  void setLanguageUnchecked(const SFString &value) { _language = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DShaderNode").
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

private:
  /**
   * @brief Registered event handler for activate (inputOnly); empty until set.
   */
  std::function<void(const SFBool &)> _onActivateHandler{};

  /**
   * @brief Member variable for isSelected.
   */

  SFBool _isSelected{};

  /**
   * @brief Member variable for isValid.
   */

  SFBool _isValid{};

  /**
   * @brief Member variable for language.
   */

  SFString _language{};
};

#endif // X3DSHADERNODE_HPP