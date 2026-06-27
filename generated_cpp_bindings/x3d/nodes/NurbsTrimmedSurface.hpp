// NurbsTrimmedSurface.hpp
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

#include "x3d/nodes/X3DParametricGeometryNode.hpp"

#include "x3d/nodes/X3DNurbsSurfaceGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsTrimmedSurface
 * @brief NurbsTrimmedSurface generates texture coordinates from a Non-Uniform
 * Rational B-Spline (NURBS) surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsTrimmedSurface
 */
class NurbsTrimmedSurface : public virtual X3DNurbsSurfaceGeometryNode {
public:
  /**
   * @brief Default constructor for NurbsTrimmedSurface
   */
  NurbsTrimmedSurface() = default;

  /**
   * @brief Destructor for NurbsTrimmedSurface
   */
  ~NurbsTrimmedSurface() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Acceptable node types for the addTrimmingContour field.
   * @details Permitted X3D node types: Contour2D
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableAddTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on
   * addTrimmingContour. AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnAddTrimmingContourHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onAddTrimmingContour(const MFNode &value) {
    if (_onAddTrimmingContourHandler)
      _onAddTrimmingContourHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * addTrimmingContour.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnAddTrimmingContourHandler(std::function<void(const MFNode &)> handler) {
    _onAddTrimmingContourHandler = std::move(handler);
  }

  /**
   * @brief Acceptable node types for the removeTrimmingContour field.
   * @details Permitted X3D node types: Contour2D
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableRemoveTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on
   * removeTrimmingContour. AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveTrimmingContourHandler(); a no-op if none is set. The event
   * cascade reaches this through the node's reflected field table, so routing
   * is node-agnostic.
   * @param value The value of the received event.
   */
  void onRemoveTrimmingContour(const MFNode &value) {
    if (_onRemoveTrimmingContourHandler)
      _onRemoveTrimmingContourHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * removeTrimmingContour.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnRemoveTrimmingContourHandler(
      std::function<void(const MFNode &)> handler) {
    _onRemoveTrimmingContourHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of trimmingContour. AccessType: inputOutput
   * @details A set of Contour2D nodes are used as trimming loops.
   * @return MFNode The current value of trimmingContour.
   */
  MFNode getTrimmingContour() const { return _trimmingContour; }

  /**
   * @brief Acceptable node types for the trimmingContour field.
   * @details Permitted X3D node types: Contour2D
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTrimmingContourNodeTypes() {
    static const std::vector<std::string> types = {"Contour2D"};
    return types;
  }

  /**
   * @brief Sets the value of trimmingContour. AccessType: inputOutput
   * @details A set of Contour2D nodes are used as trimming loops.
   * @param value The new value for trimmingContour.
   */
  void setTrimmingContour(const MFNode &value) { _trimmingContour = value; }

  void setTrimmingContour(MFNode &&value) {

    _trimmingContour = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsTrimmedSurface").
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
   * @brief Registered event handler for addTrimmingContour (inputOnly); empty
   * until set.
   */
  std::function<void(const MFNode &)> _onAddTrimmingContourHandler{};

  /**
   * @brief Registered event handler for removeTrimmingContour (inputOnly);
   * empty until set.
   */
  std::function<void(const MFNode &)> _onRemoveTrimmingContourHandler{};

  /**
   * @brief Member variable for trimmingContour.
   */

  MFNode _trimmingContour{};
};

} // namespace x3d::nodes
