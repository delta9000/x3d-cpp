#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DFontStyleNode
 * @brief Base type for all font style nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/text.html#X3DFontStyleNode
 */
class X3DFontStyleNode : public virtual X3DNode {
public:
  X3DFontStyleNode() = default;

  virtual ~X3DFontStyleNode() = default;

  static SFNode getDefaultIS() { return nullptr; }

  static SFNode getDefaultMetadata() { return nullptr; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Text"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The class attribute on each X3D node and statement is a
   * space-separated list of classes, reserved for use by Cascading Style Sheets
   * (CSS) and XML stylesheets.
   */
  const SFString &getClass_() const { return _class_; }

  /**
   * Access: inputOutput.
   * @details The class attribute on each X3D node and statement is a
   * space-separated list of classes, reserved for use by Cascading Style Sheets
   * (CSS) and XML stylesheets.
   */
  void setClass_(const SFString &value) { _class_ = value; }

  void setClass_(SFString &&value) { _class_ = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The id attribute on each X3D node and statement is considered a
   * unique identifier when used as part of an encompassing HTML/DOM context.
   */
  const SFString &getId() const { return _id; }

  /**
   * Access: inputOutput.
   * @details The id attribute on each X3D node and statement is considered a
   * unique identifier when used as part of an encompassing HTML/DOM context.
   */
  void setId(const SFString &value) { _id = value; }

  void setId(SFString &&value) { _id = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setIS(const SFNode &value) { _IS = value; }

  void setIS(SFNode &&value) { _IS = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setMetadata(const SFNode &value) { _metadata = value; }

  void setMetadata(SFNode &&value) { _metadata = std::move(value); }

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
  SFString _class_{};

  SFString _id{};

  SFNode _IS{nullptr};

  SFNode _metadata{nullptr};
};

} // namespace x3d::nodes
