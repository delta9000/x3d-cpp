// PickSensorSystem.hpp — the line pick-sensor driver for the X3D §38 Picking
// component. Unlike the pointing-device sensors (TouchSensor / drag sensors,
// §20) a pick sensor is NOT driven by the pointer: it tests its own authored
// `pickingGeometry` against the authored `pickTarget` shapes every tick. This
// file wires LinePickSensor: it extracts the segments of an IndexedLineSet /
// LineSet pickingGeometry, transforms them into world space, and tests each
// segment against every geometry-bearing node in each pickTarget subtree using
// the PickSystem narrow-phase helper (the segment is a bounded ray — hits
// beyond the segment length are rejected). The matchCriterion selects which
// target hits deactivate/activate the sensor; isActive, pickedGeometry and
// pickedPoint are posted through the cascade.
//
// Scope (CONF-PICKSENSOR): LinePickSensor only, intersectionType GEOMETRY (the
// BOUNDS/other modes are not implemented — targets are always tested by exact
// geometry). PointPickSensor / PrimitivePickSensor / VolumePickSensor are NOT
// wired. Codegen-free: every emit is a postEvent on an existing outputOnly
// field. namespace x3d::runtime.
#ifndef X3D_RUNTIME_PICK_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_PICK_SENSOR_SYSTEM_HPP

#include "FieldRead.hpp"
#include "GeometryBounds.hpp" // geombounds::getField/getNode/hasField/getPointsLenient
#include "Mat4.hpp"
#include "PickSystem.hpp"
#include "Ray.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/X3DNode.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives X3D pick sensors (§38). Enrolls LinePickSensor nodes and runs
 *        the pick engine against their pickingGeometry/pickTarget each tick.
 */
class PickSensorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext & /*ctx*/) override {
    if (node && node->nodeTypeName() == "LinePickSensor" && !contains(sensors_, node))
      sensors_.push_back(node);
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    sensors_.erase(std::remove(sensors_.begin(), sensors_.end(), node), sensors_.end());
    active_.erase(node);
  }

  void update(double /*now*/, X3DExecutionContext &ctx) override {
    for (X3DNode *s : sensors_)
      evaluateLineSensor(s, ctx);
  }

private:
  static bool contains(const std::vector<X3DNode *> &v, X3DNode *n) {
    return std::find(v.begin(), v.end(), n) != v.end();
  }

  // A pickingGeometry segment, in the pickingGeometry's LOCAL frame.
  struct Segment { SFVec3f a, b; };

  static SFVec3f sub(const SFVec3f &a, const SFVec3f &b) {
    return SFVec3f{a.x - b.x, a.y - b.y, a.z - b.z};
  }

  // Pull the coord node's points, tolerating MFVec3f/MFVec3d (Coordinate /
  // CoordinateDouble/GeoCoordinate) via the shared lenient reader.
  static std::vector<SFVec3f> coordPoints(const X3DNode *geom) {
    auto coord = geombounds::getNode(*geom, "coord");
    if (!coord) return {};
    return geombounds::getPointsLenient(*coord, "point");
  }

  // Extract the segments of a LineSet / IndexedLineSet pickingGeometry (§11).
  // IndexedLineSet: coordIndex with -1 runs, consecutive vertices per polyline.
  // LineSet: vertexCount consuming `coord` sequentially. Other geometry types
  // contribute no segments (LinePickSensor tests line geometry only).
  static std::vector<Segment> segmentsOf(const X3DNode *geom) {
    std::vector<Segment> out;
    if (!geom) return out;
    const std::string &t = geom->nodeTypeName();
    const std::vector<SFVec3f> pts = coordPoints(geom);
    if (pts.empty()) return out;
    auto pushRun = [&](const std::vector<int> &idx) {
      for (std::size_t i = 0; i + 1 < idx.size(); ++i) {
        const int a = idx[i], b = idx[i + 1];
        if (a < 0 || b < 0 || a >= static_cast<int>(pts.size()) ||
            b >= static_cast<int>(pts.size()))
          continue;
        out.push_back(Segment{pts[a], pts[b]});
      }
    };
    if (t == "IndexedLineSet") {
      const MFInt32 ci = geombounds::getField<MFInt32>(*geom, "coordIndex", {});
      std::vector<int> run;
      for (int v : ci) {
        if (v < 0) { pushRun(run); run.clear(); }
        else run.push_back(v);
      }
      pushRun(run);
    } else if (t == "LineSet") {
      const MFInt32 vc = geombounds::getField<MFInt32>(*geom, "vertexCount", {});
      std::size_t base = 0;
      for (int count : vc) {
        std::vector<int> run;
        for (int k = 0; k < count; ++k) run.push_back(static_cast<int>(base + k));
        pushRun(run);
        base += static_cast<std::size_t>(std::max(0, count));
      }
    }
    return out;
  }

  // Every geometry-bearing (Shape) node in `n`'s subtree, including `n`.
  static void collectShapes(X3DNode *n, std::vector<X3DNode *> &out) {
    if (!n) return;
    if (geombounds::hasField(*n, "geometry") && geombounds::getNode(*n, "geometry"))
      out.push_back(n);
    forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      collectShapes(c.get(), out);
    });
  }

  // objectType (§38): 'ALL' matches everything; 'GEOMETRY' matches a
  // geometry-bearing target (always true here); any other token excludes.
  static bool typeAllowed(const MFString &objectType) {
    for (const std::string &t : objectType) {
      if (t == "ALL" || t == "GEOMETRY") return true;
    }
    return objectType.empty(); // default entry {"ALL"} applies when absent
  }

  void evaluateLineSensor(X3DNode *s, X3DExecutionContext &ctx) {
    const bool enabled = geombounds::getField<bool>(*s, "enabled", true);
    if (!enabled) {
      // §38: a disabled sensor is inactive and produces no picks. Post the
      // falling edge once so author ROUTEs from isActive still fire.
      if (active_[s]) { ctx.postEvent(s, "isActive", std::any(SFBool{false})); active_[s] = false; }
      return;
    }

    auto geom = geombounds::getNode(*s, "pickingGeometry");
    const std::vector<Segment> segs = geom ? segmentsOf(geom.get()) : std::vector<Segment>{};

    const MFNode targets = geombounds::getField<MFNode>(*s, "pickTarget", {});
    const MFString objectType = geombounds::getField<MFString>(*s, "objectType", MFString{"ALL"});
    const std::string criterion = enumToken(*s, "matchCriterion", "MATCH_ANY");
    const bool allowed = typeAllowed(objectType);

    // Test each target subtree; a target matches if ANY segment hits ANY of its
    // shapes. Record the hit points (pickingGeometry-local) of matched targets.
    MFNode picked;
    MFVec3f pickedPoints;
    int matchCount = 0;
    if (allowed && !segs.empty() && geom) {
      const Mat4 geomWorld = ctx.worldTransformAny(geom.get());
      const Mat4 geomInv = geomWorld.inverse();
      for (const auto &target : targets) {
        if (!target) continue;
        std::vector<X3DNode *> shapes;
        collectShapes(target.get(), shapes);
        bool matched = false;
        for (X3DNode *shape : shapes) {
          const Mat4 wm = ctx.worldTransformAny(shape);
          const Mat4 inv = wm.inverse();
          auto shapeGeom = geombounds::getNode(*shape, "geometry");
          if (!shapeGeom) continue;
          for (const Segment &seg : segs) {
            const SFVec3f dir = sub(seg.b, seg.a);
            Ray localRay{inv.transformPoint(seg.a), inv.transformDirection(dir)};
            auto t = PickSystem::intersectGeometry(shapeGeom.get(), localRay);
            if (!t || *t < -1e-5f || *t > 1.0f + 1e-5f) continue; // beyond segment
            const SFVec3f worldHit = wm.transformPoint(localRay.pointAt(*t));
            pickedPoints.push_back(geomInv.transformPoint(worldHit));
            matched = true;
            break;
          }
          if (matched) break;
        }
        if (matched) {
          picked.push_back(target);
          ++matchCount;
        }
      }
    }

    const std::size_t total = targets.size();
    bool nowActive = false;
    if (criterion == "MATCH_EVERY")
      nowActive = total > 0 && matchCount == static_cast<int>(total);
    else if (criterion == "MATCH_ONLY_ONE")
      nowActive = matchCount == 1;
    else // MATCH_ANY (default)
      nowActive = matchCount >= 1;

    if (nowActive) {
      // Outputs fire while active; pickedGeometry lists the matched targets and
      // pickedPoint the geometry-local intersection points (§38).
      ctx.postEvent(s, "pickedGeometry", std::any(MFNode{picked}));
      ctx.postEvent(s, "pickedPoint", std::any(MFVec3f{pickedPoints}));
    }
    if (nowActive != active_[s]) {
      ctx.postEvent(s, "isActive", std::any(SFBool{nowActive}));
      active_[s] = nowActive;
    }
  }

  std::vector<X3DNode *> sensors_;
  std::unordered_map<X3DNode *, bool> active_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PICK_SENSOR_SYSTEM_HPP
