// IndexedTriangleFanSet.hpp
#ifndef INDEXEDTRIANGLEFANSET_HPP
#define INDEXEDTRIANGLEFANSET_HPP

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
 * @class IndexedTriangleFanSet
 * @brief IndexedTriangleFanSet is a geometry node containing a
 * Coordinate|CoordinateDouble node, and can also contain Color|ColorRGBA,
 * Normal and TextureCoordinate nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#IndexedTriangleFanSet
 */
class IndexedTriangleFanSet : public virtual X3DComposedGeometryNode {
public:
  /**
   * @brief Default constructor for IndexedTriangleFanSet
   */
  IndexedTriangleFanSet() = default;

  /**
   * @brief Destructor for IndexedTriangleFanSet
   */
  ~IndexedTriangleFanSet() = default;

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
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of index. AccessType: initializeOnly
   * @details index list specifies triangles by connecting Coordinate vertices,
   * each individual fan separated by -1 sentinel value.
   * @return MFInt32 The current value of index.
   */
  MFInt32 getIndex() const { return _index; }
  /**
   * @brief Data-layer write of index (reader/init ingest path).
   * @details index is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setIndex().
   */
  void setIndexUnchecked(const MFInt32 &value) { _index = value; }

  /**
   * @brief Event handler invoked when an event is received on set_index.
   * AccessType: inputOnly
   * @details index list specifies triangles by connecting Coordinate vertices,
   * each individual fan separated by -1 sentinel value. Dispatches to the
   * handler registered via setOnSet_indexHandler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_index(const MFInt32 &value) {
    if (_onSet_indexHandler)
      _onSet_indexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_index.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_indexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_indexHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "IndexedTriangleFanSet").
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
   * @brief Member variable for index.
   */

  MFInt32 _index{};

  /**
   * @brief Registered event handler for set_index (inputOnly); empty until set.
   */
  std::function<void(const MFInt32 &)> _onSet_indexHandler{};
};

#endif // INDEXEDTRIANGLEFANSET_HPP