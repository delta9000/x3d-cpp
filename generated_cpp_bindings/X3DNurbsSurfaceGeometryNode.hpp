// X3DNurbsSurfaceGeometryNode.hpp
#ifndef X3DNURBSSURFACEGEOMETRYNODE_HPP
#define X3DNURBSSURFACEGEOMETRYNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

#include "X3DParametricGeometryNode.hpp"

/**
 * @class X3DNurbsSurfaceGeometryNode
 * @brief Abstract geometry type for all types of NURBS surfaces.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#X3DNurbsSurfaceGeometryNode
 */
class X3DNurbsSurfaceGeometryNode : public virtual X3DParametricGeometryNode {
public:
  /**
   * @brief Default constructor for X3DNurbsSurfaceGeometryNode
   */
  X3DNurbsSurfaceGeometryNode() = default;

  /**
   * @brief Virtual destructor for X3DNurbsSurfaceGeometryNode
   */
  virtual ~X3DNurbsSurfaceGeometryNode() = default;

  /**
   * @brief Get the default value for controlPoint
   * @return SFNode The default value
   */
  static SFNode getDefaultControlPoint() { return nullptr; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

  /**
   * @brief Get the default value for texCoord
   * @return SFNode The default value
   */
  static SFNode getDefaultTexCoord() { return nullptr; }

  /**
   * @brief Get the default value for uClosed
   * @return SFBool The default value
   */
  static SFBool getDefaultUClosed() { return false; }

  /**
   * @brief Get the default value for uDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUDimension() { return 0; }

  /**
   * @brief Get the default value for uOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUOrder() { return 3; }

  /**
   * @brief Get the default value for uTessellation
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUTessellation() { return 0; }

  /**
   * @brief Get the default value for vClosed
   * @return SFBool The default value
   */
  static SFBool getDefaultVClosed() { return false; }

  /**
   * @brief Get the default value for vDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVDimension() { return 0; }

  /**
   * @brief Get the default value for vOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVOrder() { return 3; }

  /**
   * @brief Get the default value for vTessellation
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVTessellation() { return 0; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of controlPoint. AccessType: inputOutput
   * @details
   * @return SFNode The current value of controlPoint.
   */
  SFNode getControlPoint() const { return _controlPoint; }

  /**
   * @brief Acceptable node types for the controlPoint field.
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableControlPointNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * @brief Sets the value of controlPoint. AccessType: inputOutput
   * @details
   * @param value The new value for controlPoint.
   */
  void setControlPoint(const SFNode &value) { _controlPoint = value; }

  void setControlPoint(SFNode &&value) { _controlPoint = std::move(value); }

  /**
   * @brief Gets the value of solid. AccessType: initializeOnly
   * @details
   * @return SFBool The current value of solid.
   */
  SFBool getSolid() const { return _solid; }
  /**
   * @brief Data-layer write of solid (reader/init ingest path).
   * @details solid is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSolid().
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * @brief Gets the value of texCoord. AccessType: inputOutput
   * @details
   * @return SFNode The current value of texCoord.
   */
  SFNode getTexCoord() const { return _texCoord; }

  /**
   * @brief Acceptable node types for the texCoord field.
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode,
   * NurbsTextureCoordinate
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode", "NurbsTextureCoordinate"};
    return types;
  }

  /**
   * @brief Sets the value of texCoord. AccessType: inputOutput
   * @details
   * @param value The new value for texCoord.
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

  /**
   * @brief Gets the value of uClosed. AccessType: initializeOnly
   * @details
   * @return SFBool The current value of uClosed.
   */
  SFBool getUClosed() const { return _uClosed; }
  /**
   * @brief Data-layer write of uClosed (reader/init ingest path).
   * @details uClosed is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUClosed().
   */
  void setUClosedUnchecked(const SFBool &value) { _uClosed = value; }

  /**
   * @brief Gets the value of uDimension. AccessType: initializeOnly
   * @details
   * @return SFInt32 The current value of uDimension.
   */
  SFInt32 getUDimension() const { return _uDimension; }
  /**
   * @brief Data-layer write of uDimension (reader/init ingest path).
   * @details uDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUDimension().
   */
  void setUDimensionUnchecked(const SFInt32 &value) { _uDimension = value; }

  /**
   * @brief Gets the value of uKnot. AccessType: initializeOnly
   * @details
   * @return MFDouble The current value of uKnot.
   */
  MFDouble getUKnot() const { return _uKnot; }
  /**
   * @brief Data-layer write of uKnot (reader/init ingest path).
   * @details uKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUKnot().
   */
  void setUKnotUnchecked(const MFDouble &value) { _uKnot = value; }

  /**
   * @brief Gets the value of uOrder. AccessType: initializeOnly
   * @details
   * @return SFInt32 The current value of uOrder.
   */
  SFInt32 getUOrder() const { return _uOrder; }
  /**
   * @brief Data-layer write of uOrder (reader/init ingest path).
   * @details uOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUOrder().
   */
  void setUOrderUnchecked(const SFInt32 &value) { _uOrder = value; }

  /**
   * @brief Gets the value of uTessellation. AccessType: inputOutput
   * @details
   * @return SFInt32 The current value of uTessellation.
   */
  SFInt32 getUTessellation() const { return _uTessellation; }

  /**
   * @brief Sets the value of uTessellation. AccessType: inputOutput
   * @details
   * @param value The new value for uTessellation.
   */
  void setUTessellation(const SFInt32 &value) { _uTessellation = value; }

  /**
   * @brief Gets the value of vClosed. AccessType: initializeOnly
   * @details
   * @return SFBool The current value of vClosed.
   */
  SFBool getVClosed() const { return _vClosed; }
  /**
   * @brief Data-layer write of vClosed (reader/init ingest path).
   * @details vClosed is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVClosed().
   */
  void setVClosedUnchecked(const SFBool &value) { _vClosed = value; }

  /**
   * @brief Gets the value of vDimension. AccessType: initializeOnly
   * @details
   * @return SFInt32 The current value of vDimension.
   */
  SFInt32 getVDimension() const { return _vDimension; }
  /**
   * @brief Data-layer write of vDimension (reader/init ingest path).
   * @details vDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVDimension().
   */
  void setVDimensionUnchecked(const SFInt32 &value) { _vDimension = value; }

  /**
   * @brief Gets the value of vKnot. AccessType: initializeOnly
   * @details
   * @return MFDouble The current value of vKnot.
   */
  MFDouble getVKnot() const { return _vKnot; }
  /**
   * @brief Data-layer write of vKnot (reader/init ingest path).
   * @details vKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVKnot().
   */
  void setVKnotUnchecked(const MFDouble &value) { _vKnot = value; }

  /**
   * @brief Gets the value of vOrder. AccessType: initializeOnly
   * @details
   * @return SFInt32 The current value of vOrder.
   */
  SFInt32 getVOrder() const { return _vOrder; }
  /**
   * @brief Data-layer write of vOrder (reader/init ingest path).
   * @details vOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVOrder().
   */
  void setVOrderUnchecked(const SFInt32 &value) { _vOrder = value; }

  /**
   * @brief Gets the value of vTessellation. AccessType: inputOutput
   * @details
   * @return SFInt32 The current value of vTessellation.
   */
  SFInt32 getVTessellation() const { return _vTessellation; }

  /**
   * @brief Sets the value of vTessellation. AccessType: inputOutput
   * @details
   * @param value The new value for vTessellation.
   */
  void setVTessellation(const SFInt32 &value) { _vTessellation = value; }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details
   * @return MFDouble The current value of weight.
   */
  MFDouble getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details
   * @param value The new value for weight.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DNurbsSurfaceGeometryNode").
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
   * @brief Member variable for controlPoint.
   */

  SFNode _controlPoint{nullptr};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for texCoord.
   */

  SFNode _texCoord{nullptr};

  /**
   * @brief Member variable for uClosed.
   */

  SFBool _uClosed{false};

  /**
   * @brief Member variable for uDimension.
   */

  SFInt32 _uDimension{0};

  /**
   * @brief Member variable for uKnot.
   */

  MFDouble _uKnot{};

  /**
   * @brief Member variable for uOrder.
   */

  SFInt32 _uOrder{3};

  /**
   * @brief Member variable for uTessellation.
   */

  SFInt32 _uTessellation{0};

  /**
   * @brief Member variable for vClosed.
   */

  SFBool _vClosed{false};

  /**
   * @brief Member variable for vDimension.
   */

  SFInt32 _vDimension{0};

  /**
   * @brief Member variable for vKnot.
   */

  MFDouble _vKnot{};

  /**
   * @brief Member variable for vOrder.
   */

  SFInt32 _vOrder{3};

  /**
   * @brief Member variable for vTessellation.
   */

  SFInt32 _vTessellation{0};

  /**
   * @brief Member variable for weight.
   */

  MFDouble _weight{};
};

#endif // X3DNURBSSURFACEGEOMETRYNODE_HPP