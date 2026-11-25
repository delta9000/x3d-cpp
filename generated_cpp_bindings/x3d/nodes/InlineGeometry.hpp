#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class InlineGeometry
 * @brief InlineGeometry loads geometry from an external file.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/networking.html#InlineGeometry
 */
class InlineGeometry : public virtual X3DGeometryNode,
                       public virtual X3DUrlObject {
public:
  InlineGeometry() = default;

  ~InlineGeometry() = default;

  static SFBool getDefaultSmooth() { return true; }

  static SFBool getDefaultSolid() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 4; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details The smooth field provides a hint to the browser whether smooth
   * rendering is preferred for a retrieved polygonal mesh.
   */
  const SFBool &getSmooth() const { return _smooth; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSmoothUnchecked(const SFBool &value) { _smooth = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  const SFBool &getSolid() const { return _solid; }

  /**
   * Access: inputOutput.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  void setSolid(const SFBool &value) { _solid = value; }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFBool _smooth{true};

  SFBool _solid{false};
};

} // namespace x3d::nodes
