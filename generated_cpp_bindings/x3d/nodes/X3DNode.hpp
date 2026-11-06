#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DNode
 * @brief All instantiable nodes implement X3DNode, which corresponds to SFNode
 * type in the X3D specification.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DNode
 */
class X3DNode {
public:
  X3DNode() = default;

  virtual ~X3DNode() = default;

  static SFNode getDefaultIS() { return nullptr; }

  static SFNode getDefaultMetadata() { return nullptr; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details IS/connect statements define prototype connections between
   * ProtoInterface fields and node fields within a ProtoBody.
   */
  const SFNode &getIS() const { return _IS; }

  /**
   * @details Permitted X3D node types: IS
   */
  static const std::vector<std::string> &acceptableISNodeTypes() {
    static const std::vector<std::string> types = {"IS"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details IS/connect statements define prototype connections between
   * ProtoInterface fields and node fields within a ProtoBody.
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   */
  const SFNode &getMetadata() const { return _metadata; }

  /**
   * @details Permitted X3D node types: X3DMetadataObject
   */
  static const std::vector<std::string> &acceptableMetadataNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Information about this node can be contained in a MetadataBoolean,
   * MetadataDouble, MetadataFloat, MetadataInteger, MetadataString or
   * MetadataSet node.
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details DEF defines a unique ID name for each node, referenceable by other
   * nodes.
   */
  const SFString &getDEF() const { return _DEF; }

  /**
   * Access: inputOutput.
   * @details DEF defines a unique ID name for each node, referenceable by other
   * nodes.
   */
  void setDEF(const SFString &value) { _DEF = value; }

  void setDEF(SFString &&value) { _DEF = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details USE means reuse an already DEF-ed node ID, excluding all child
   * nodes and all other attributes (except for containerField, which can have a
   * different value).
   */
  const SFString &getUSE() const { return _USE; }

  /**
   * Access: inputOutput.
   * @details USE means reuse an already DEF-ed node ID, excluding all child
   * nodes and all other attributes (except for containerField, which can have a
   * different value).
   */
  void setUSE(const SFString &value) { _USE = value; }

  void setUSE(SFString &&value) { _USE = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The class attribute is a space-separated list of classes, reserved
   * for use by Cascading Style Sheets (CSS) and XML stylesheets. This attribute
   * is only functional if the X3D model is loaded within an HTML page.
   */
  const SFString &getClass_() const { return _class_; }

  /**
   * Access: inputOutput.
   * @details The class attribute is a space-separated list of classes, reserved
   * for use by Cascading Style Sheets (CSS) and XML stylesheets. This attribute
   * is only functional if the X3D model is loaded within an HTML page.
   */
  void setClass_(const SFString &value) { _class_ = value; }

  void setClass_(SFString &&value) { _class_ = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The id attribute is a unique identifier, reserved for use by
   * HTML5/DOM pages, independent of DEF labeling and internal X3D node
   * referencing. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   */
  const SFString &getId() const { return _id; }

  /**
   * Access: inputOutput.
   * @details The id attribute is a unique identifier, reserved for use by
   * HTML5/DOM pages, independent of DEF labeling and internal X3D node
   * referencing. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   */
  void setId(const SFString &value) { _id = value; }

  void setId(SFString &&value) { _id = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The style attribute provides an inline block of CSS for element
   * styling, reserved for use by Cascading Style Sheets (CSS) and XML
   * stylesheets. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   */
  const SFString &getStyle() const { return _style; }

  /**
   * Access: inputOutput.
   * @details The style attribute provides an inline block of CSS for element
   * styling, reserved for use by Cascading Style Sheets (CSS) and XML
   * stylesheets. This attribute is only functional if the X3D model is loaded
   * within an HTML page.
   */
  void setStyle(const SFString &value) { _style = value; }

  void setStyle(SFString &&value) { _style = std::move(value); }

  virtual std::string nodeTypeName() const;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  virtual std::string defaultContainerField() const;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  virtual const FieldTable &fields() const;

  virtual void accept(NodeVisitor &visitor) const;

  /**
   * @brief Append RangeDiagnostics for this node's own out-of-range constrained
   *        fields. Base no-op; constrained nodes override. Traversal of
   * children is collectRangeWarnings()'s job, not this method's.
   */
  virtual void validateRanges(std::vector<RangeDiagnostic> &out) const;

private:
  SFNode _IS{nullptr};

  SFNode _metadata{nullptr};

  SFString _DEF{};

  SFString _USE{};

  SFString _class_{};

  SFString _id{};

  SFString _style{};
};

} // namespace x3d::nodes
