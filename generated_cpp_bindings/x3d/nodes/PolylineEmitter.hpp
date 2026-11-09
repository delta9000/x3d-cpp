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
 * @class PolylineEmitter
 * @brief PolylineEmitter emits particles along a single polyline.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#PolylineEmitter
 */
class PolylineEmitter : public virtual X3DParticleEmitterNode {
public:
  PolylineEmitter() = default;

  ~PolylineEmitter() = default;

  static SFNode getDefaultCoord() { return nullptr; }

  static MFInt32 getDefaultCoordIndex() { return std::vector<int>{-1}; }

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 1, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "emitter"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Coordinates for the line along which particles are randomly
   * generated.
   */
  const SFNode &getCoord() const { return _coord; }

  /**
   * @details Permitted X3D node types: X3DCoordinateNode
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"X3DCoordinateNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Coordinates for the line along which particles are randomly
   * generated.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details coordIndex indices are applied to contained Coordinate values in
   * order to define randomly generated initial geometry of the particles.
   */
  const MFInt32 &getCoordIndex() const { return _coordIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCoordIndexUnchecked(const MFInt32 &value) { _coordIndex = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Initial direction from which particles emanate.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details Initial direction from which particles emanate.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDirection() for range validation.
   */
  void setDirectionUnchecked(const SFVec3f &value) { _direction = value; }

  /**
   * MFInt32; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnSet_coordIndexHandler(); a no-op if none is set.
   */
  void onSet_coordIndex(const MFInt32 &value) {
    if (_onSet_coordIndexHandler)
      _onSet_coordIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_coordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_coordIndexHandler = std::move(handler);
  }

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

    if (value.x < -1.0f)
      throw std::out_of_range("direction.x below minimum of -1");
    if (value.x > 1.0f)
      throw std::out_of_range("direction.x above maximum of 1");

    if (value.y < -1.0f)
      throw std::out_of_range("direction.y below minimum of -1");
    if (value.y > 1.0f)
      throw std::out_of_range("direction.y above maximum of 1");

    if (value.z < -1.0f)
      throw std::out_of_range("direction.z below minimum of -1");
    if (value.z > 1.0f)
      throw std::out_of_range("direction.z above maximum of 1");
  }

  SFNode _coord{nullptr};

  MFInt32 _coordIndex{std::vector<int>{-1}};

  SFVec3f _direction{SFVec3f{0, 1, 0}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};
};

} // namespace x3d::nodes
