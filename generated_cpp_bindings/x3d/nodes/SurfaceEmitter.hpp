// SurfaceEmitter.hpp
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
 * @class SurfaceEmitter
 * @brief SurfaceEmitter generates particles from the surface of an object.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#SurfaceEmitter
 */
class SurfaceEmitter : public virtual X3DParticleEmitterNode {
public:
  /**
   * @brief Default constructor for SurfaceEmitter
   */
  SurfaceEmitter() = default;

  /**
   * @brief Destructor for SurfaceEmitter
   */
  ~SurfaceEmitter() = default;

  /**
   * @brief Get the default value for surface
   * @return SFNode The default value
   */
  static SFNode getDefaultSurface() { return nullptr; }

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
   * @brief Gets the value of surface. AccessType: initializeOnly
   * @details The geometry node provides geometry used as the emitting surface.
   * @return SFNode The current value of surface.
   */
  SFNode getSurface() const { return _surface; }

  /**
   * @brief Acceptable node types for the surface field.
   * @details Permitted X3D node types: X3DGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSurfaceNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }
  /**
   * @brief Data-layer write of surface (reader/init ingest path).
   * @details surface is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSurface().
   */
  void setSurfaceUnchecked(const SFNode &value) { _surface = value; }

  /**
   * @brief The X3D type name of this node (e.g. "SurfaceEmitter").
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

private:
  /**
   * @brief Member variable for surface.
   */

  SFNode _surface{nullptr};
};

} // namespace x3d::nodes
