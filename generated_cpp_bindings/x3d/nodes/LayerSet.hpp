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
  LayerSet() = default;

  ~LayerSet() = default;

  static SFInt32 getDefaultActiveLayer() { return 0; }

  static MFInt32 getDefaultOrder() { return std::vector<int>{0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Layering"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details activeLayer field specifies the layer in which navigation takes
   * place.
   */
  const SFInt32 &getActiveLayer() const { return _activeLayer; }

  /**
   * Access: inputOutput.
   * @details activeLayer field specifies the layer in which navigation takes
   * place.
   */
  void setActiveLayer(const SFInt32 &value) {

    validateActiveLayer(value);

    _activeLayer = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setActiveLayer() for range validation.
   */
  void setActiveLayerUnchecked(const SFInt32 &value) { _activeLayer = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The layers list defines a list of Layer nodes that contain the
   * constituent parts of the scene.
   */
  const MFNode &getLayers() const { return _layers; }

  /**
   * @details Permitted X3D node types: X3DLayerNode
   */
  static const std::vector<std::string> &acceptableLayersNodeTypes() {
    static const std::vector<std::string> types = {"X3DLayerNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The layers list defines a list of Layer nodes that contain the
   * constituent parts of the scene.
   */
  void setLayers(const MFNode &value) { _layers = value; }

  void setLayers(MFNode &&value) { _layers = std::move(value); }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details The order list defines the order in which layers are rendered.
   */
  const MFInt32 &getOrder() const { return _order; }

  /**
   * Access: inputOutput.
   * @details The order list defines the order in which layers are rendered.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setOrder() for range validation.
   */
  void setOrderUnchecked(const MFInt32 &value) { _order = value; }

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

  SFInt32 _activeLayer{0};

  MFNode _layers{};

  MFInt32 _order{std::vector<int>{0}};
};

} // namespace x3d::nodes
