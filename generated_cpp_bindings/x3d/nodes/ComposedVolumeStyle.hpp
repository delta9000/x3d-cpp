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
  ComposedVolumeStyle() = default;

  ~ComposedVolumeStyle() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 3; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details List of contributing rendering style nodes or node references that
   * can be applied to the object.
   */
  const MFNode &getRenderStyle() const { return _renderStyle; }

  /**
   * @details Permitted X3D node types: X3DComposableVolumeRenderStyleNode
   */
  static const std::vector<std::string> &acceptableRenderStyleNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DComposableVolumeRenderStyleNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details List of contributing rendering style nodes or node references that
   * can be applied to the object.
   */
  void setRenderStyle(const MFNode &value) { _renderStyle = value; }

  void setRenderStyle(MFNode &&value) { _renderStyle = std::move(value); }

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
  MFNode _renderStyle{};
};

} // namespace x3d::nodes
