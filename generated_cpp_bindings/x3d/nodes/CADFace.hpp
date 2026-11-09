#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DProductStructureChildNode.hpp"

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CADFace
 * @brief CADFace holds geometry representing one face in a Computer-Aided
 * Design (CAD) CADPart.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/CADGeometry.html#CADFace
 */
class CADFace : public virtual X3DProductStructureChildNode,
                public virtual X3DBoundedObject {
public:
  CADFace() = default;

  ~CADFace() = default;

  static SFNode getDefaultShape() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "CADGeometry"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Contained Shape for this CADPart.
   */
  const SFNode &getShape() const { return _shape; }

  /**
   * @details Permitted X3D node types: Shape, LOD, Transform
   */
  static const std::vector<std::string> &acceptableShapeNodeTypes() {
    static const std::vector<std::string> types = {"Shape", "LOD", "Transform"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Contained Shape for this CADPart.
   */
  void setShape(const SFNode &value) { _shape = value; }

  void setShape(SFNode &&value) { _shape = std::move(value); }

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
  SFNode _shape{nullptr};
};

} // namespace x3d::nodes
