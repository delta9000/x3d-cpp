// VolumeEmitter.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DParticleEmitterNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class VolumeEmitter
 * @brief VolumeEmitter emits particles from a random position confined within
 * the given closed geometry volume.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#VolumeEmitter
 */
class VolumeEmitter : public virtual X3DParticleEmitterNode {
public:
  /**
   * @brief Default constructor for VolumeEmitter
   */
  VolumeEmitter() = default;

  /**
   * @brief Destructor for VolumeEmitter
   */
  ~VolumeEmitter() = default;

  /**
   * @brief Get the default value for coord
   * @return SFNode The default value
   */
  static SFNode getDefaultCoord() { return nullptr; }

  /**
   * @brief Get the default value for coordIndex
   * @return MFInt32 The default value
   */
  static MFInt32 getDefaultCoordIndex() { return std::vector<int>{-1}; }

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for internal
   * @return SFBool The default value
   */
  static SFBool getDefaultInternal() { return true; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "emitter"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "ParticleSystems"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of coord. AccessType: inputOutput
   * @details Coordinates for the geometry used as the emitting volume.
   * @return SFNode The current value of coord.
   */
  SFNode getCoord() const { return _coord; }

  /**
   * @brief Acceptable node types for the coord field.
   * @details Permitted X3D node types: X3DCoordinateNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"X3DCoordinateNode"};
    return types;
  }

  /**
   * @brief Sets the value of coord. AccessType: inputOutput
   * @details Coordinates for the geometry used as the emitting volume.
   * @param value The new value for coord.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * @brief Gets the value of coordIndex. AccessType: initializeOnly
   * @details coordIndex indices are applied to contained Coordinate values in
   * order to define randomly generated initial geometry of the particles.
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
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details Initial direction from which particles emanate.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details Initial direction from which particles emanate.
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) {

    validateDirection(value);

    _direction = value;
  }

  void setDirection(SFVec3f &&value) {

    validateDirection(value);

    _direction = std::move(value);
  }

  /**
   * @brief Non-validating write of direction (runtime/reader ingest path).
   * @details Assigns without the range check setDirection() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDirection() stays the
   *          enforcement point for programmatic callers.
   */
  void setDirectionUnchecked(const SFVec3f &value) { _direction = value; }

  /**
   * @brief Gets the value of internal. AccessType: initializeOnly
   * @details TODO, X3D specification is undefined.
   * @return SFBool The current value of internal.
   */
  SFBool getInternal() const { return _internal; }
  /**
   * @brief Data-layer write of internal (reader/init ingest path).
   * @details internal is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setInternal().
   */
  void setInternalUnchecked(const SFBool &value) { _internal = value; }

  /**
   * @brief Event handler invoked when an event is received on set_coordIndex.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnSet_coordIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
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
   * @brief The X3D type name of this node (e.g. "VolumeEmitter").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCoordIndex(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDirection(const SFVec3f &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateDirection(const SFVec3f &value) {

    if (value.x < -1)
      throw std::out_of_range("direction.x below minimum of -1");
    if (value.x > 1)
      throw std::out_of_range("direction.x above maximum of 1");

    if (value.y < -1)
      throw std::out_of_range("direction.y below minimum of -1");
    if (value.y > 1)
      throw std::out_of_range("direction.y above maximum of 1");

    if (value.z < -1)
      throw std::out_of_range("direction.z below minimum of -1");
    if (value.z > 1)
      throw std::out_of_range("direction.z above maximum of 1");
  }

  /**
   * @brief Member variable for coord.
   */

  SFNode _coord{nullptr};

  /**
   * @brief Member variable for coordIndex.
   */

  MFInt32 _coordIndex{std::vector<int>{-1}};

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for internal.
   */

  SFBool _internal{true};

  /**
   * @brief Registered event handler for set_coordIndex (inputOnly); empty until
   * set.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};
};

} // namespace x3d::nodes
