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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Cone
 * @brief Cone is a geometry node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#Cone
 */
class Cone : public virtual X3DGeometryNode {
public:
  Cone() = default;

  ~Cone() = default;

  static SFBool getDefaultBottom() { return true; }

  static SFFloat getDefaultBottomRadius() { return 1; }

  static SFFloat getDefaultHeight() { return 2; }

  static SFBool getDefaultSide() { return true; }

  static SFBool getDefaultSolid() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry3D"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether to draw bottom (other inside faces are not drawn).
   */
  const SFBool &getBottom() const { return _bottom; }

  /**
   * Access: inputOutput.
   * @details Whether to draw bottom (other inside faces are not drawn).
   */
  void setBottom(const SFBool &value) { _bottom = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Size in meters.
   */
  const SFFloat &getBottomRadius() const { return _bottomRadius; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBottomRadiusUnchecked(const SFFloat &value) { _bottomRadius = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Size in meters.
   */
  const SFFloat &getHeight() const { return _height; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setHeightUnchecked(const SFFloat &value) { _height = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether to draw sides (other inside faces are not drawn).
   */
  const SFBool &getSide() const { return _side; }

  /**
   * Access: inputOutput.
   * @details Whether to draw sides (other inside faces are not drawn).
   */
  void setSide(const SFBool &value) { _side = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

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
  SFBool _bottom{true};

  SFFloat _bottomRadius{1};

  SFFloat _height{2};

  SFBool _side{true};

  SFBool _solid{true};
};

} // namespace x3d::nodes
