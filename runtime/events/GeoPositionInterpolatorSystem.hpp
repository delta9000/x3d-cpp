#ifndef X3D_RUNTIME_GEO_POSITION_INTERPOLATOR_SYSTEM_HPP
#define X3D_RUNTIME_GEO_POSITION_INTERPOLATOR_SYSTEM_HPP

#include "GeoNodes.hpp"
#include "Interpolation.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "x3d/nodes/GeoPositionInterpolator.hpp"

namespace x3d::runtime {

class GeoPositionInterpolatorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::GeoPositionInterpolator *>(node);
    if (!n) return;
    if (!n->getKeyValue().empty()) {
      n->emitGeovalue_changed(n->getKeyValue().front());
      SFVec3f world;
      if (geo::toWorld(*n, n->getKeyValue().front(), world, ctx.geoProjection())) n->emitValue_changed(world);
    }
    n->setOnSet_fractionHandler([n, &ctx](const SFFloat &fraction) {
      if (n->getKey().empty() || n->getKeyValue().empty()) return;
      // §25.3.7: interpolate in geoSystem coordinates, then project the result.
      const SFVec3d authored = interpolateValue(n->getKey(), n->getKeyValue(), fraction,
          [](const SFVec3d &a, const SFVec3d &b, float t) {
            return SFVec3d{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                           a.z + (b.z - a.z) * t};
          });
      SFVec3f world;
      if (!geo::toWorld(*n, authored, world, ctx.geoProjection())) return;
      ctx.postEvent(n, "geovalue_changed", std::any(authored));
      ctx.postEvent(n, "value_changed", std::any(world));
    });
  }
  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *n = dynamic_cast<x3d::nodes::GeoPositionInterpolator *>(node))
      n->setOnSet_fractionHandler({});
  }
};

} // namespace x3d::runtime
#endif
