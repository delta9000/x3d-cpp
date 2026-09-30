#ifndef X3D_RUNTIME_UNIT_CONVERSION_HPP
#define X3D_RUNTIME_UNIT_CONVERSION_HPP

#include "FieldRead.hpp"
#include "RecursionLimits.hpp"
#include "X3DScene.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <any>
#include <cmath>
#include <functional>
#include <type_traits>
#include <string_view>
#include <unordered_set>

namespace x3d::runtime {
namespace unit_detail {
using namespace x3d::core;

struct Dimension { int length = 0, angle = 0, mass = 0, force = 0; };

inline bool includes(std::string_view words, std::string_view word) {
  while (!words.empty()) {
    const auto end = words.find(' ');
    if (words.substr(0, end) == word) return true;
    if (end == std::string_view::npos) break;
    words.remove_prefix(end + 1);
  }
  return false;
}

// Explicit dimensions of fields consumed by the existing scene, extraction,
// animation, navigation and physics systems. Numeric types alone imply no unit
// (e.g. normals, colors, scale, fractions, texture coordinates, generic scalars).
inline Dimension dimension(std::string_view node, std::string_view field) {
  struct Entry { std::string_view nodes, fields; Dimension units; };
  static constexpr Entry entries[] = {
    {"Transform CADPart HAnimHumanoid HAnimJoint HAnimSite", "translation center", {1}},
    {"Transform CADPart HAnimHumanoid HAnimJoint HAnimSite", "rotation scaleOrientation", {0,1}},
    {"HAnimHumanoid", "jointBindingPositions", {1}},
    {"HAnimHumanoid", "jointBindingRotations", {0,1}},
    {"HAnimDisplacer", "displacements", {1}},
    {"Box", "size", {1}},
    {"Sphere", "radius", {1}},
    {"Cylinder", "height radius", {1}},
    {"Cone", "height bottomRadius", {1}},
    {"Coordinate CoordinateDouble", "point", {1}},
    {"ElevationGrid", "height xSpacing zSpacing", {1}},
    {"Extrusion", "crossSection spine", {1}},
    {"Extrusion", "orientation", {0,1}},
    {"IndexedFaceSet ElevationGrid Extrusion", "creaseAngle", {0,1}},
    {"Circle2D Disk2D Arc2D ArcClose2D", "radius innerRadius outerRadius", {1}},
    {"Arc2D ArcClose2D", "startAngle endAngle", {0,1}},
    {"Rectangle2D", "size", {1}},
    {"Polyline2D", "lineSegments", {1}},
    {"Polypoint2D", "point", {1}},
    {"TriangleSet2D", "vertices", {1}},
    {"PointLight SpotLight", "location radius", {1}},
    {"SpotLight", "beamWidth cutOffAngle", {0,1}},
    {"Viewpoint OrthoViewpoint", "position centerOfRotation nearDistance farDistance", {1}},
    {"Viewpoint OrthoViewpoint", "orientation", {0,1}},
    {"Viewpoint", "fieldOfView", {0,1}},
    {"OrthoViewpoint", "fieldOfView", {1}},
    {"NavigationInfo", "avatarSize speed visibilityLimit", {1}},
    {"Fog LocalFog", "visibilityRange", {1}},
    {"PositionInterpolator PositionInterpolator2D CoordinateInterpolator CoordinateInterpolator2D SplinePositionInterpolator SplinePositionInterpolator2D", "keyValue", {1}},
    {"OrientationInterpolator SquadOrientationInterpolator", "keyValue", {0,1}},
    {"SplinePositionInterpolator SplinePositionInterpolator2D", "keyVelocity", {1}},
    {"PositionChaser PositionDamper PositionChaser2D PositionDamper2D CoordinateChaser CoordinateDamper", "initialDestination initialValue", {1}},
    {"OrientationChaser OrientationDamper", "initialDestination initialValue", {0,1}},
    {"ProximitySensor VisibilitySensor TransformSensor", "center size", {1}},
    {"LOD", "center range", {1}},
    {"PlaneSensor", "minPosition maxPosition offset", {1}},
    {"CylinderSensor SphereSensor", "offset axisRotation", {0,1}},
    {"CylinderSensor", "diskAngle minAngle maxAngle", {0,1}},
    {"CollisionSpace", "bboxCenter bboxSize", {1}},
    {"CollidableShape CollidableOffset", "translation", {1}},
    {"CollidableShape CollidableOffset", "rotation", {0,1}},
    {"RigidBody", "position centerOfMass linearVelocity", {1}},
    {"RigidBody", "orientation angularVelocity", {0,1}},
    {"RigidBody", "mass", {0,0,1}},
    {"RigidBody", "inertia", {2,0,1}},
    {"RigidBody", "forces", {0,0,0,1}},
    {"RigidBody", "torques", {1,0,0,1}},
    {"RigidBodyCollection", "gravity contactSurfaceThickness", {1}},
    {"BallJoint SingleAxisHingeJoint DoubleAxisHingeJoint UniversalJoint", "anchorPoint", {1}},
    {"SingleAxisHingeJoint", "minAngle maxAngle", {0,1}},
    {"SliderJoint", "minSeparation maxSeparation", {1}},
  };
  for (const auto &entry : entries)
    if (includes(entry.nodes, node) && includes(entry.fields, field))
      return entry.units;
  return {};
}

inline double factor(const std::vector<Unit> &units, Dimension d) {
  double result = 1;
  for (const auto &unit : units) {
    int power = unit.category == "length" ? d.length :
                unit.category == "angle" ? d.angle :
                unit.category == "mass" ? d.mass :
                unit.category == "force" ? d.force : 0;
    result *= std::pow(unit.conversionFactor, power);
  }
  return result;
}

template<class T> inline void scale(T &v, double f) {
  if constexpr (std::is_arithmetic_v<T>) v = static_cast<T>(v * f);
  else if constexpr (std::is_same_v<T, SFRotation>)
    v.angle = static_cast<float>(v.angle * f); // rotation axis is dimensionless
  else if constexpr (std::is_same_v<T, SFMatrix3f>) {
    for (auto &row : v.matrix) for (auto &element : row)
      element = static_cast<float>(element * f);
  } else {
    v.x = static_cast<decltype(v.x)>(v.x * f);
    v.y = static_cast<decltype(v.y)>(v.y * f);
    if constexpr (requires { v.z; }) v.z = static_cast<decltype(v.z)>(v.z * f);
    if constexpr (requires { v.w; }) v.w = static_cast<decltype(v.w)>(v.w * f);
  }
}

template<class T> inline bool convert(std::any &value, double f) {
  if (auto *v = std::any_cast<T>(&value)) { scale(*v, f); return true; }
  if (auto *v = std::any_cast<std::vector<T>>(&value)) {
    for (auto &element : *v) scale(element, f);
    return true;
  }
  return false;
}

inline void normalize(Scene &scene, std::unordered_set<const X3DNode *> &seen) {
  // Child nodes are also spliced into the parent. Visit their own source first
  // so the shared-node guard prevents applying the parent's factor afterwards.
  for (auto &[node, child] : scene.expandedInlineScenes)
    if (child) normalize(*child, seen);
  std::function<void(const std::shared_ptr<X3DNode>&, std::size_t)> walk;
  walk = [&](const std::shared_ptr<X3DNode> &node, std::size_t depth) {
    if (!node || depth >= kMaxNestingDepth || !seen.insert(node.get()).second) return;
    for (const auto &field : node->fields()) {
      if (!field.isNode() && field.get && field.set &&
          scene.authoredScalarFields.contains(node, field.x3dName) &&
          !scene.normalizedUnitFields.contains(node, field.x3dName)) {
        const auto d = dimension(node->nodeTypeName(), field.x3dName);
        if (d.length == 0 && d.angle == 0 && d.mass == 0 && d.force == 0)
          continue;
        auto source = scene.unitFieldSources.find(node);
        const auto *units = &scene.sourceUnits;
        if (source != scene.unitFieldSources.end()) {
          auto entry = source->second.find(field.x3dName);
          if (entry != source->second.end()) units = &entry->second;
        }
        const double f = factor(*units, d);
        if (f != 1) {
          auto value = field.get(*node);
          if (convert<float>(value, f) || convert<double>(value, f) ||
              convert<SFVec2f>(value, f) || convert<SFVec2d>(value, f) ||
              convert<SFVec3f>(value, f) || convert<SFVec3d>(value, f) ||
              convert<SFRotation>(value, f) || convert<SFMatrix3f>(value, f))
            field.set(*node, value);
        }
        scene.normalizedUnitFields.record(node, field.x3dName);
      }
    }
    forEachChildNode(*node, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &child) { walk(child, depth + 1); });
  };
  for (const auto &root : scene.rootNodes) walk(root, 0);
  for (const auto &peer : scene.protoPeerNodes) walk(peer, 0);
}
} // namespace unit_detail

// Entering runtime mutates explicitly authored dimensional fields to initial
// units. Built-in defaults remain unchanged. Custom PROTO interfaces are
// converted at known dimensional IS targets, independently for each target.
// Subsequent field writes/event payloads are already runtime units.
// Serialize a parsed document before entering runtime to retain authored units.
inline void normalizeRuntimeUnits(Scene &scene) {
  std::unordered_set<const X3DNode *> seen;
  unit_detail::normalize(scene, seen);
}
} // namespace x3d::runtime
#endif
