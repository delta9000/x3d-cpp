// MetadataSet.hpp
#ifndef METADATASET_HPP
#define METADATASET_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DMetadataObject.hpp"

/**
 * @class MetadataSet
 * @brief The metadata provided by this node is contained in the metadata nodes
 * of the value field.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#MetadataSet
 */
class MetadataSet : public virtual X3DNode, public virtual X3DMetadataObject {
public:
  /**
   * @brief Default constructor for MetadataSet
   */
  MetadataSet() = default;

  /**
   * @brief Destructor for MetadataSet
   */
  ~MetadataSet() = default;

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
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesMetadata";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "value"; }

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
   * @details
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
   * @details
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
   * @brief Gets the value of value. AccessType: inputOutput
   * @details The value field provides a list of X3DMetadataObject nodes whose
   * meaning is determined by the name field.
   * @return MFNode The current value of value.
   */
  MFNode getValue() const { return _value; }

  /**
   * @brief Acceptable node types for the value field.
   * @details Permitted X3D node types: X3DMetadataObject
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableValueNodeTypes() {
    static const std::vector<std::string> types = {"X3DMetadataObject"};
    return types;
  }

  /**
   * @brief Sets the value of value. AccessType: inputOutput
   * @details The value field provides a list of X3DMetadataObject nodes whose
   * meaning is determined by the name field.
   * @param value The new value for value.
   */
  void setValue(const MFNode &value) { _value = value; }

  void setValue(MFNode &&value) { _value = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MetadataSet").
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
   * @brief Member variable for IS.
   */

  SFNode _IS{nullptr};

  /**
   * @brief Member variable for metadata.
   */

  SFNode _metadata{nullptr};

  /**
   * @brief Member variable for value.
   */

  MFNode _value{};
};

#endif // METADATASET_HPP