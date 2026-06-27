// LayerSet.hpp
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
 * @class LayerSet
 * @brief LayerSet defines a list of layers and a rendering order.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#LayerSet
 */
class LayerSet : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for LayerSet
   */
  LayerSet() = default;

  /**
   * @brief Destructor for LayerSet
   */
  ~LayerSet() = default;

  /**
   * @brief Get the default value for activeLayer
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultActiveLayer() { return 0; }

  /**
   * @brief Get the default value for order
   * @return MFInt32 The default value
   */
  static MFInt32 getDefaultOrder() { return std::vector<int>{0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
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
   * @brief Gets the value of activeLayer. AccessType: inputOutput
   * @details activeLayer field specifies the layer in which navigation takes
   * place.
   * @return SFInt32 The current value of activeLayer.
   */
  SFInt32 getActiveLayer() const { return _activeLayer; }

  /**
   * @brief Sets the value of activeLayer. AccessType: inputOutput
   * @details activeLayer field specifies the layer in which navigation takes
   * place.
   * @param value The new value for activeLayer.
   */
  void setActiveLayer(const SFInt32 &value) {

    validateActiveLayer(value);

    _activeLayer = value;
  }

  /**
   * @brief Non-validating write of activeLayer (runtime/reader ingest path).
   * @details Assigns without the range check setActiveLayer() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setActiveLayer() stays the
   *          enforcement point for programmatic callers.
   */
  void setActiveLayerUnchecked(const SFInt32 &value) { _activeLayer = value; }

  /**
   * @brief Gets the value of layers. AccessType: inputOutput
   * @details The layers list defines a list of Layer nodes that contain the
   * constituent parts of the scene.
   * @return MFNode The current value of layers.
   */
  MFNode getLayers() const { return _layers; }

  /**
   * @brief Acceptable node types for the layers field.
   * @details Permitted X3D node types: X3DLayerNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableLayersNodeTypes() {
    static const std::vector<std::string> types = {"X3DLayerNode"};
    return types;
  }

  /**
   * @brief Sets the value of layers. AccessType: inputOutput
   * @details The layers list defines a list of Layer nodes that contain the
   * constituent parts of the scene.
   * @param value The new value for layers.
   */
  void setLayers(const MFNode &value) { _layers = value; }

  void setLayers(MFNode &&value) { _layers = std::move(value); }

  /**
   * @brief Gets the value of order. AccessType: inputOutput
   * @details The order list defines the order in which layers are rendered.
   * @return MFInt32 The current value of order.
   */
  MFInt32 getOrder() const { return _order; }

  /**
   * @brief Sets the value of order. AccessType: inputOutput
   * @details The order list defines the order in which layers are rendered.
   * @param value The new value for order.
   */
  void setOrder(const MFInt32 &value) {

    validateOrder(value);

    _order = value;
  }

  void setOrder(MFInt32 &&value) {

    validateOrder(value);

    _order = std::move(value);
  }

  /**
   * @brief Non-validating write of order (runtime/reader ingest path).
   * @details Assigns without the range check setOrder() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setOrder() stays the
   *          enforcement point for programmatic callers.
   */
  void setOrderUnchecked(const MFInt32 &value) { _order = value; }

  /**
   * @brief The X3D type name of this node (e.g. "LayerSet").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesActiveLayer(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesOrder(const MFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateActiveLayer(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("activeLayer below minimum of 0");
  }

  static void validateOrder(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 0)
        throw std::out_of_range("order below minimum of 0");
    }
  }

  /**
   * @brief Member variable for activeLayer.
   */

  SFInt32 _activeLayer{0};

  /**
   * @brief Member variable for layers.
   */

  MFNode _layers{};

  /**
   * @brief Member variable for order.
   */

  MFInt32 _order{std::vector<int>{0}};
};

} // namespace x3d::nodes
