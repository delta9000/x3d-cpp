// ComposedVolumeStyle.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DVolumeRenderStyleNode.hpp"

#include "x3d/nodes/X3DComposableVolumeRenderStyleNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ComposedVolumeStyle
 * @brief ComposedVolumeStyle allows compositing multiple rendering styles into
 * single rendering pass.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#ComposedVolumeStyle
 */
class ComposedVolumeStyle : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for ComposedVolumeStyle
   */
  ComposedVolumeStyle() = default;

  /**
   * @brief Destructor for ComposedVolumeStyle
   */
  ~ComposedVolumeStyle() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "renderStyle"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "VolumeRendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of renderStyle. AccessType: inputOutput
   * @details List of contributing rendering style nodes or node references that
   * can be applied to the object.
   * @return MFNode The current value of renderStyle.
   */
  MFNode getRenderStyle() const { return _renderStyle; }

  /**
   * @brief Acceptable node types for the renderStyle field.
   * @details Permitted X3D node types: X3DComposableVolumeRenderStyleNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DComposableVolumeRenderStyleNode"};
    return types;
  }

  /**
   * @brief Sets the value of renderStyle. AccessType: inputOutput
   * @details List of contributing rendering style nodes or node references that
   * can be applied to the object.
   * @param value The new value for renderStyle.
   */
  void setRenderStyle(const MFNode &value) { _renderStyle = value; }

  void setRenderStyle(MFNode &&value) { _renderStyle = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ComposedVolumeStyle").
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
   * @brief Member variable for renderStyle.
   */

  MFNode _renderStyle{};
};

} // namespace x3d::nodes
