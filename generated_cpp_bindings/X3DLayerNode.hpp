// X3DLayerNode.hpp
#ifndef X3DLAYERNODE_HPP
#define X3DLAYERNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DPickableObject.hpp"

/**
 * @class X3DLayerNode
 * @brief The X3DLayerNode abstract node type is the base node type for layer
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#X3DLayerNode
 */
class X3DLayerNode : public virtual X3DNode, public virtual X3DPickableObject {
public:
  /**
   * @brief Default constructor for X3DLayerNode
   */
  X3DLayerNode() = default;

  /**
   * @brief Virtual destructor for X3DLayerNode
   */
  virtual ~X3DLayerNode() = default;

  /**
   * @brief Get the default value for objectType
   * @return std::vector<PickableObjectTypeValues> The default value
   */
  static std::vector<PickableObjectTypeValues> getDefaultObjectType() {
    return std::vector<PickableObjectTypeValues>{PickableObjectTypeValues::ALL};
  }

  /**
   * @brief Get the default value for pickable
   * @return SFBool The default value
   */
  static SFBool getDefaultPickable() { return true; }

  /**
   * @brief Get the default value for viewport
   * @return SFNode The default value
   */
  static SFNode getDefaultViewport() { return nullptr; }

  /**
   * @brief Get the default value for visible
   * @return SFBool The default value
   */
  static SFBool getDefaultVisible() { return true; }

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
  static std::string componentName() { return "Layering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of objectType. AccessType: inputOutput
   * @details
   * @return std::vector<PickableObjectTypeValues> The current value of
   * objectType.
   */
  std::vector<PickableObjectTypeValues> getObjectType() const {
    return _objectType;
  }

  /**
   * @brief Sets the value of objectType. AccessType: inputOutput
   * @details
   * @param value The new value for objectType.
   */
  void setObjectType(const std::vector<PickableObjectTypeValues> &value) {

    _objectType = value;
  }

  void setObjectType(std::vector<PickableObjectTypeValues> &&value) {

    _objectType = std::move(value);
  }

  /**
   * @brief Gets the value of pickable. AccessType: inputOutput
   * @details
   * @return SFBool The current value of pickable.
   */
  SFBool getPickable() const { return _pickable; }

  /**
   * @brief Sets the value of pickable. AccessType: inputOutput
   * @details
   * @param value The new value for pickable.
   */
  void setPickable(const SFBool &value) { _pickable = value; }

  /**
   * @brief Gets the value of viewport. AccessType: inputOutput
   * @details
   * @return SFNode The current value of viewport.
   */
  SFNode getViewport() const { return _viewport; }

  /**
   * @brief Acceptable node types for the viewport field.
   * @details Permitted X3D node types: X3DViewportNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableViewportNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewportNode"};
    return types;
  }

  /**
   * @brief Sets the value of viewport. AccessType: inputOutput
   * @details
   * @param value The new value for viewport.
   */
  void setViewport(const SFNode &value) { _viewport = value; }

  void setViewport(SFNode &&value) { _viewport = std::move(value); }

  /**
   * @brief Gets the value of visible. AccessType: inputOutput
   * @details
   * @return SFBool The current value of visible.
   */
  SFBool getVisible() const { return _visible; }

  /**
   * @brief Sets the value of visible. AccessType: inputOutput
   * @details
   * @param value The new value for visible.
   */
  void setVisible(const SFBool &value) { _visible = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DLayerNode").
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
   * @brief Member variable for objectType.
   */

  std::vector<PickableObjectTypeValues> _objectType{
      std::vector<PickableObjectTypeValues>{PickableObjectTypeValues::ALL}};

  /**
   * @brief Member variable for pickable.
   */

  SFBool _pickable{true};

  /**
   * @brief Member variable for viewport.
   */

  SFNode _viewport{nullptr};

  /**
   * @brief Member variable for visible.
   */

  SFBool _visible{true};
};

#endif // X3DLAYERNODE_HPP