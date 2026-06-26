// IndexedFaceSet.hpp
#ifndef INDEXEDFACESET_HPP
#define INDEXEDFACESET_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

#include "X3DComposedGeometryNode.hpp"

/**
 * @class IndexedFaceSet
 * @brief IndexedFaceSet defines polygons using index lists corresponding to
 * vertex coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#IndexedFaceSet
 */
class IndexedFaceSet : public virtual X3DComposedGeometryNode {
public:
  /**
   * @brief Default constructor for IndexedFaceSet
   */
  IndexedFaceSet() = default;

  /**
   * @brief Destructor for IndexedFaceSet
   */
  ~IndexedFaceSet() = default;

  /**
   * @brief Get the default value for convex
   * @return SFBool The default value
   */
  static SFBool getDefaultConvex() { return true; }

  /**
   * @brief Get the default value for creaseAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultCreaseAngle() { return 0; }

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
  static std::string componentName() { return "Geometry3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of colorIndex. AccessType: initializeOnly
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices).
   * @return MFInt32 The current value of colorIndex.
   */
  MFInt32 getColorIndex() const { return _colorIndex; }
  /**
   * @brief Data-layer write of colorIndex (reader/init ingest path).
   * @details colorIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setColorIndex().
   */
  void setColorIndexUnchecked(const MFInt32 &value) { _colorIndex = value; }

  /**
   * @brief Gets the value of convex. AccessType: initializeOnly
   * @details The convex field is a hint to renderers whether all polygons in a
   * shape are convex (true), or possibly concave (false).
   * @return SFBool The current value of convex.
   */
  SFBool getConvex() const { return _convex; }
  /**
   * @brief Data-layer write of convex (reader/init ingest path).
   * @details convex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setConvex().
   */
  void setConvexUnchecked(const SFBool &value) { _convex = value; }

  /**
   * @brief Gets the value of coordIndex. AccessType: initializeOnly
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face.
   * @return MFInt32 The current value of coordIndex.
   */
  MFInt32 getCoordIndex() const { return _coordIndex; }
  /**
   * @brief Data-layer write of coordIndex (reader/init ingest path).
   * @details coordIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCoordIndex().
   */
  void setCoordIndexUnchecked(const MFInt32 &value) { _coordIndex = value; }

  /**
   * @brief Gets the value of creaseAngle. AccessType: initializeOnly
   * @details creaseAngle defines angle (in radians) for determining whether
   * adjacent polygons are drawn with sharp edges or smooth shading.
   * @return SFFloat The current value of creaseAngle.
   */
  SFFloat getCreaseAngle() const { return _creaseAngle; }
  /**
   * @brief Data-layer write of creaseAngle (reader/init ingest path).
   * @details creaseAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCreaseAngle().
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * @brief Gets the value of normalIndex. AccessType: initializeOnly
   * @details normalIndex values define the order in which normal vectors are
   * applied to polygons (or vertices).
   * @return MFInt32 The current value of normalIndex.
   */
  MFInt32 getNormalIndex() const { return _normalIndex; }
  /**
   * @brief Data-layer write of normalIndex (reader/init ingest path).
   * @details normalIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setNormalIndex().
   */
  void setNormalIndexUnchecked(const MFInt32 &value) { _normalIndex = value; }

  /**
   * @brief Event handler invoked when an event is received on set_colorIndex.
   * AccessType: inputOnly
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices). Dispatches to the handler registered
   * via setOnSet_colorIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_colorIndex(const MFInt32 &value) {
    if (_onSet_colorIndexHandler)
      _onSet_colorIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_colorIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_colorIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_colorIndexHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_coordIndex.
   * AccessType: inputOnly
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face. Dispatches to the handler
   * registered via setOnSet_coordIndexHandler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_coordIndex(const MFInt32 &value) {
    if (_onSet_coordIndexHandler)
      _onSet_coordIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_coordIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_coordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_coordIndexHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_normalIndex.
   * AccessType: inputOnly
   * @details normalIndex values define the order in which normal vectors are
   * applied to polygons (or vertices). Dispatches to the handler registered via
   * setOnSet_normalIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_normalIndex(const MFInt32 &value) {
    if (_onSet_normalIndexHandler)
      _onSet_normalIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_normalIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_normalIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_normalIndexHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_texCoordIndex. AccessType: inputOnly
   * @details List of texture-coordinate indices mapping attached texture to
   * corresponding coordinates. Dispatches to the handler registered via
   * setOnSet_texCoordIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_texCoordIndex(const MFInt32 &value) {
    if (_onSet_texCoordIndexHandler)
      _onSet_texCoordIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_texCoordIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_texCoordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_texCoordIndexHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of texCoordIndex. AccessType: initializeOnly
   * @details List of texture-coordinate indices mapping attached texture to
   * corresponding coordinates.
   * @return MFInt32 The current value of texCoordIndex.
   */
  MFInt32 getTexCoordIndex() const { return _texCoordIndex; }
  /**
   * @brief Data-layer write of texCoordIndex (reader/init ingest path).
   * @details texCoordIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setTexCoordIndex().
   */
  void setTexCoordIndexUnchecked(const MFInt32 &value) {
    _texCoordIndex = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "IndexedFaceSet").
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
   * @brief Member variable for colorIndex.
   */

  MFInt32 _colorIndex{};

  /**
   * @brief Member variable for convex.
   */

  SFBool _convex{true};

  /**
   * @brief Member variable for coordIndex.
   */

  MFInt32 _coordIndex{};

  /**
   * @brief Member variable for creaseAngle.
   */

  SFFloat _creaseAngle{0};

  /**
   * @brief Member variable for normalIndex.
   */

  MFInt32 _normalIndex{};

  /**
   * @brief Registered event handler for set_colorIndex (inputOnly); empty until
   * set.
   */
  std::function<void(const MFInt32 &)> _onSet_colorIndexHandler{};

  /**
   * @brief Registered event handler for set_coordIndex (inputOnly); empty until
   * set.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};

  /**
   * @brief Registered event handler for set_normalIndex (inputOnly); empty
   * until set.
   */
  std::function<void(const MFInt32 &)> _onSet_normalIndexHandler{};

  /**
   * @brief Registered event handler for set_texCoordIndex (inputOnly); empty
   * until set.
   */
  std::function<void(const MFInt32 &)> _onSet_texCoordIndexHandler{};

  /**
   * @brief Member variable for texCoordIndex.
   */

  MFInt32 _texCoordIndex{};
};

#endif // INDEXEDFACESET_HPP