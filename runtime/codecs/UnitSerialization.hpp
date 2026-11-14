#ifndef X3D_UNIT_SERIALIZATION_HPP
#define X3D_UNIT_SERIALIZATION_HPP

#include "X3DScene.hpp"
#include <stdexcept>

namespace x3d::codec {
// Under nonidentity units, an explicitly authored factory-default number may
// have a different physical meaning from an omitted (canonical) default. Keep
// authored scalar defaults conservatively, including fields whose dimensional
// metadata is not yet modeled. Never synthesize unauthored defaults.
inline const runtime::AuthoredScalarFields *authoredUnitDefaults(
    const runtime::AuthoredScalarFields &fields,
    const std::vector<runtime::Unit> &units) {
  for (const auto &unit : units)
    if (unit.conversionFactor != 1.0)
      return &fields;
  return nullptr;
}

// Runtime conversion mutates shared node values, but retains authoring units
// and PROTO declarations. Removing UNIT metadata alone cannot restore fidelity.
inline void requireUnnormalizedUnits(const runtime::Scene &scene) {
  if (scene.hasNormalizedUnitValues && *scene.hasNormalizedUnitValues)
    throw std::runtime_error(
        "Cannot serialize a scene after nonidentity UNIT normalization; "
        "serialize the parsed document before entering runtime, or reparse "
        "the source into an independent document for authoring-unit output.");
}
} // namespace x3d::codec
#endif
