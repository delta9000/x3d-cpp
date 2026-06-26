// X3DSingleTextureCoordinateNode.hpp
#ifndef X3DSINGLETEXTURECOORDINATENODE_HPP
#define X3DSINGLETEXTURECOORDINATENODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometricPropertyNode.hpp"

#include "X3DTextureCoordinateNode.hpp"

/**
 * @class X3DSingleTextureCoordinateNode
 * @brief Base type for all texture coordinate nodes which specify texture
 * coordinates for a single texture.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#X3DSingleTextureCoordinateNode
 */
class X3DSingleTextureCoordinateNode : public virtual X3DTextureCoordinateNode {
public:
  /**
   * @brief Default constructor for X3DSingleTextureCoordinateNode
   */
  X3DSingleTextureCoordinateNode() = default;

  /**
   * @brief Virtual destructor for X3DSingleTextureCoordinateNode
   */
  virtual ~X3DSingleTextureCoordinateNode() = default;

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
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of mapping. AccessType: inputOutput
   * @details
   * @return SFString The current value of mapping.
   */
  SFString getMapping() const { return _mapping; }

  /**
   * @brief Sets the value of mapping. AccessType: inputOutput
   * @details
   * @param value The new value for mapping.
   */
  void setMapping(const SFString &value) { _mapping = value; }

  void setMapping(SFString &&value) { _mapping = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g.
   * "X3DSingleTextureCoordinateNode").
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
   * @brief Member variable for mapping.
   */

  SFString _mapping{};
};

#endif // X3DSINGLETEXTURECOORDINATENODE_HPP