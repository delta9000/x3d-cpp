// OpacityMapVolumeStyle.hpp
#ifndef OPACITYMAPVOLUMESTYLE_HPP
#define OPACITYMAPVOLUMESTYLE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DVolumeRenderStyleNode.hpp"

#include "X3DComposableVolumeRenderStyleNode.hpp"

/**
 * @class OpacityMapVolumeStyle
 * @brief OpacityMapVolumeStyle specifies that volumetric data is rendered using
 * opacity mapped to a transfer function texture.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#OpacityMapVolumeStyle
 */
class OpacityMapVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for OpacityMapVolumeStyle
   */
  OpacityMapVolumeStyle() = default;

  /**
   * @brief Destructor for OpacityMapVolumeStyle
   */
  ~OpacityMapVolumeStyle() = default;

  /**
   * @brief Get the default value for transferFunction
   * @return SFNode The default value
   */
  static SFNode getDefaultTransferFunction() { return nullptr; }

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of transferFunction. AccessType: inputOutput
   * @details The transferFunction field holds a single texture representation
   * in either two or three dimensions that maps the voxel data values to a
   * specific colour output.
   * @return SFNode The current value of transferFunction.
   */
  SFNode getTransferFunction() const { return _transferFunction; }

  /**
   * @brief Acceptable node types for the transferFunction field.
   * @details Permitted X3D node types: X3DTexture2DNode, X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTransferFunctionNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of transferFunction. AccessType: inputOutput
   * @details The transferFunction field holds a single texture representation
   * in either two or three dimensions that maps the voxel data values to a
   * specific colour output.
   * @param value The new value for transferFunction.
   */
  void setTransferFunction(const SFNode &value) { _transferFunction = value; }

  void setTransferFunction(SFNode &&value) {

    _transferFunction = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "OpacityMapVolumeStyle").
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
   * @brief Member variable for transferFunction.
   */

  SFNode _transferFunction{nullptr};
};

#endif // OPACITYMAPVOLUMESTYLE_HPP