// NurbsSet.hpp
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

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsSet
 * @brief NurbsSet collects a set of NurbsSurface nodes into a common group and
 * treats NurbsSurface set as a unit during tessellation, thereby enforcing
 * tessellation continuity along borders.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSet
 */
class NurbsSet : public virtual X3DChildNode, public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for NurbsSet
   */
  NurbsSet() = default;

  /**
   * @brief Destructor for NurbsSet
   */
  ~NurbsSet() = default;

  /**
   * @brief Get the default value for tessellationScale
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTessellationScale() { return 1.0; }

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
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Acceptable node types for the addGeometry field.
   * @details Permitted X3D node types: X3DParametricGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAddGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on addGeometry.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnAddGeometryHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onAddGeometry(const MFNode &value) {
    if (_onAddGeometryHandler)
      _onAddGeometryHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on addGeometry.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnAddGeometryHandler(std::function<void(const MFNode &)> handler) {
    _onAddGeometryHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of geometry. AccessType: inputOutput
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   * @return const MFNode& The current value of geometry.
   */
  const MFNode &getGeometry() const { return _geometry; }

  /**
   * @brief Acceptable node types for the geometry field.
   * @details Permitted X3D node types: X3DParametricGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry. AccessType: inputOutput
   * @details The children form a closed loop with first point of first child
   * repeated as last point of last child, and the last point of a segment
   * repeated as first point of the consecutive one.
   * @param value The new value for geometry.
   */
  void setGeometry(const MFNode &value) { _geometry = value; }

  void setGeometry(MFNode &&value) { _geometry = std::move(value); }

  /**
   * @brief Acceptable node types for the removeGeometry field.
   * @details Permitted X3D node types: X3DParametricGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRemoveGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DParametricGeometryNode"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on removeGeometry.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveGeometryHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onRemoveGeometry(const MFNode &value) {
    if (_onRemoveGeometryHandler)
      _onRemoveGeometryHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * removeGeometry.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnRemoveGeometryHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveGeometryHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of tessellationScale. AccessType: inputOutput
   * @details scale for surface tessellation in children NurbsSurface nodes.
   * @return SFFloat The current value of tessellationScale.
   */
  SFFloat getTessellationScale() const { return _tessellationScale; }

  /**
   * @brief Sets the value of tessellationScale. AccessType: inputOutput
   * @details scale for surface tessellation in children NurbsSurface nodes.
   * @param value The new value for tessellationScale.
   */
  void setTessellationScale(const SFFloat &value) {

    _tessellationScale = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsSet").
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
   * @brief Registered event handler for addGeometry (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onAddGeometryHandler{};

  /**
   * @brief Member variable for geometry.
   */

  MFNode _geometry{};

  /**
   * @brief Registered event handler for removeGeometry (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onRemoveGeometryHandler{};

  /**
   * @brief Member variable for tessellationScale.
   */

  SFFloat _tessellationScale{1.0};
};

} // namespace x3d::nodes
