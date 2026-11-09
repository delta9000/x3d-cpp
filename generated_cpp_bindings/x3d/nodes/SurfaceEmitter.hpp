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
  SurfaceEmitter() = default;

  ~SurfaceEmitter() = default;

  static SFNode getDefaultSurface() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "emitter"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The geometry node provides geometry used as the emitting surface.
   */
  const SFNode &getSurface() const { return _surface; }

  /**
   * @details Permitted X3D node types: X3DGeometryNode
   */
  static const std::vector<std::string> &acceptableSurfaceNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSurfaceUnchecked(const SFNode &value) { _surface = value; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFNode _surface{nullptr};
};

} // namespace x3d::nodes
