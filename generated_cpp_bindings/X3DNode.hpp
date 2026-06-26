// X3DNode.hpp
#ifndef X3DNODE_HPP
#define X3DNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

/**
 * @class X3DNode
 * @brief All instantiable nodes implement X3DNode, which corresponds to SFNode
 * type in the X3D specification.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DNode
 */
class X3DNode {
public:
  /**
   * @brief Default constructor for X3DNode
   */
  X3DNode() = default;

  /**
   * @brief Virtual destructor for X3DNode
   */
  virtual ~X3DNode() = default;

  /**
   * @brief Get the default value for IS
   * @return SFNode The default value
   */
  static SFNode getDefaultIS() { return nullptr; }

  /**
   * @brief Get the default value for metadata
   * @return SFNode The default value
   */
  static SFNode getDefaultMetadata() { return nullptr; }

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
  static std::string componentName() { return "Core"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of IS. AccessType: inputOutput
   * @details IS/connect statements define prototype connections between
   * ProtoInterface fields and node fields within a ProtoBody.
   * @return SFNode The current value of IS.
   */
  SFNode getIS() const { return _IS; }

  /**
   * @brief Acceptable node types for the IS field.
   * @details Permitted X3D node types: IS
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableISNodeTypes() {
    static const std::vector<std::string> types = {"IS"};
    return types;
  }

  /**
   * @brief Sets the value of IS. AccessType: inputOutput
   * @details IS/connect statements define prototype connections between
   * ProtoInterface fields and node fields within a ProtoBody.
   * @param value The new value for IS.
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * @brief Gets the value of metadata. AccessType: inputOutput
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   * @return SFNode The current value of metadata.
   */
  SFNode getMetadata() const { return _metadata; }

  /**
   * @brief Acceptable node types for the metadata field.
   * @details Permitted X3D node types: X3DMetadataObject
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMetadataNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * @brief Sets the value of metadata. AccessType: inputOutput
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   * @param value The new value for metadata.
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

  /**
   * @brief Gets the value of DEF. AccessType: inputOutput
   * @details DEF defines a unique ID name for each node, referenceable by other
   * nodes.
   * @return SFString The current value of DEF.
   */
  SFString getDEF() const { return _DEF; }

  /**
   * @brief Sets the value of DEF. AccessType: inputOutput
   * @details DEF defines a unique ID name for each node, referenceable by other
   * nodes.
   * @param value The new value for DEF.
   */
  void setDEF(const SFString &value) { _DEF = value; }

  void setDEF(SFString &&value) { _DEF = std::move(value); }

  /**
   * @brief Gets the value of USE. AccessType: inputOutput
   * @details USE means reuse an already DEF-ed node ID, excluding all child
   * nodes and all other attributes (except for containerField, which can have a
   * different value).
   * @return SFString The current value of USE.
   */
  SFString getUSE() const { return _USE; }

  /**
   * @brief Sets the value of USE. AccessType: inputOutput
   * @details USE means reuse an already DEF-ed node ID, excluding all child
   * nodes and all other attributes (except for containerField, which can have a
   * different value).
   * @param value The new value for USE.
   */
  void setUSE(const SFString &value) { _USE = value; }

  void setUSE(SFString &&value) { _USE = std::move(value); }

  /**
   * @brief Gets the value of class. AccessType: inputOutput
   * @details The class attribute is a space-separated list of classes, reserved
   * for use by Cascading Style Sheets (CSS) and XML stylesheets. This attribute
   * is only functional if the X3D model is loaded within an HTML page.
   * @return SFString The current value of class.
   */
  SFString getClass_() const { return _class_; }

  /**
   * @brief Sets the value of class. AccessType: inputOutput
   * @details The class attribute is a space-separated list of classes, reserved
   * for use by Cascading Style Sheets (CSS) and XML stylesheets. This attribute
   * is only functional if the X3D model is loaded within an HTML page.
   * @param value The new value for class.
   */
  void setClass_(const SFString &value) { _class_ = value; }

  void setClass_(SFString &&value) { _class_ = std::move(value); }

  /**
   * @brief Gets the value of id. AccessType: inputOutput
   * @details The id attribute is a unique identifier, reserved for use by
   * HTML5/DOM pages, independent of DEF labeling and internal X3D node
   * referencing. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   * @return SFString The current value of id.
   */
  SFString getId() const { return _id; }

  /**
   * @brief Sets the value of id. AccessType: inputOutput
   * @details The id attribute is a unique identifier, reserved for use by
   * HTML5/DOM pages, independent of DEF labeling and internal X3D node
   * referencing. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   * @param value The new value for id.
   */
  void setId(const SFString &value) { _id = value; }

  void setId(SFString &&value) { _id = std::move(value); }

  /**
   * @brief Gets the value of style. AccessType: inputOutput
   * @details The style attribute provides an inline block of CSS for element
   * styling, reserved for use by Cascading Style Sheets (CSS) and XML
   * stylesheets. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   * @return SFString The current value of style.
   */
  SFString getStyle() const { return _style; }

  /**
   * @brief Sets the value of style. AccessType: inputOutput
   * @details The style attribute provides an inline block of CSS for element
   * styling, reserved for use by Cascading Style Sheets (CSS) and XML
   * stylesheets. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   * @param value The new value for style.
   */
  void setStyle(const SFString &value) { _style = value; }

  void setStyle(SFString &&value) { _style = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DNode").
   */
  virtual std::string nodeTypeName() const;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  virtual std::string defaultContainerField() const;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  virtual const FieldTable &fields() const;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  virtual void accept(NodeVisitor &visitor) const;

  /**
   * @brief Append RangeDiagnostics for this node's own out-of-range constrained
   *        fields. Base no-op; constrained nodes override. Traversal of
   * children is collectRangeWarnings()'s job, not this method's.
   */
  virtual void validateRanges(std::vector<RangeDiagnostic> &out) const;

private:
  /**
   * @brief Member variable for IS.
   */

  SFNode _IS{nullptr};

  /**
   * @brief Member variable for metadata.
   */

  SFNode _metadata{nullptr};

  /**
   * @brief Member variable for DEF.
   */

  SFString _DEF{};

  /**
   * @brief Member variable for USE.
   */

  SFString _USE{};

  /**
   * @brief Member variable for class.
   */

  SFString _class_{};

  /**
   * @brief Member variable for id.
   */

  SFString _id{};

  /**
   * @brief Member variable for style.
   */

  SFString _style{};
};

#endif // X3DNODE_HPP