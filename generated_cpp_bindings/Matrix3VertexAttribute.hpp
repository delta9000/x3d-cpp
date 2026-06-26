// Matrix3VertexAttribute.hpp
#ifndef MATRIX3VERTEXATTRIBUTE_HPP
#define MATRIX3VERTEXATTRIBUTE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometricPropertyNode.hpp"

#include "X3DVertexAttributeNode.hpp"

/**
 * @class Matrix3VertexAttribute
 * @brief Matrix3VertexAttribute defines a set of per-vertex 3x3 matrix
 * attributes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#Matrix3VertexAttribute
 */
class Matrix3VertexAttribute : public virtual X3DVertexAttributeNode {
public:
  /**
   * @brief Default constructor for Matrix3VertexAttribute
   */
  Matrix3VertexAttribute() = default;

  /**
   * @brief Destructor for Matrix3VertexAttribute
   */
  ~Matrix3VertexAttribute() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "attrib"; }

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
   * @brief Gets the value of value. AccessType: inputOutput
   * @details value specifies an arbitrary collection of matrix values that will
   * be passed to the shader as per-vertex information.
   * @return MFMatrix3f The current value of value.
   */
  MFMatrix3f getValue() const { return _value; }

  /**
   * @brief Sets the value of value. AccessType: inputOutput
   * @details value specifies an arbitrary collection of matrix values that will
   * be passed to the shader as per-vertex information.
   * @param value The new value for value.
   */
  void setValue(const MFMatrix3f &value) { _value = value; }

  void setValue(MFMatrix3f &&value) { _value = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "Matrix3VertexAttribute").
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
   * @brief Member variable for value.
   */

  MFMatrix3f _value{};
};

#endif // MATRIX3VERTEXATTRIBUTE_HPP