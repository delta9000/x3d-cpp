// Owner-thread, sampled input only. No device I/O and no camera ownership.
#ifndef X3D_RUNTIME_EXT_CAVE_INPUT_SYSTEM_HPP
#define X3D_RUNTIME_EXT_CAVE_INPUT_SYSTEM_HPP

#include "CaveSensors.hpp"
#include "Mat4.hpp"
#include "TransformSystem.hpp"
#include "X3DSceneBridge.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::runtime::ext {
struct DevicePose {
  SFVec3f position{0, 0, 0};
  SFRotation orientation{0, 1, 0, 0};
};

struct DeviceSample {
  std::string name;
  bool valid = false;
  DevicePose roomFromDevice;
  MFBool buttons;
  SFVec2f joystick{0, 0};
};

struct RoomFrame {
  bool valid = false;
  Mat4 roomFromWorld = Mat4::identity();
  std::vector<DeviceSample> devices;
};

struct RoomGroupWarning {
  const X3DNode *node = nullptr; // Borrowed until the next scene topology edit.
  std::string message;
};

namespace cave_detail {
struct Similarity {
  SFVec3f position;
  SFRotation orientation;
  float scale;
};

// Relative tolerance for orthogonality and equal column lengths. Float hosts
// that accumulate a navigation matrix frame by frame drift around 1e-6..1e-4;
// genuine shear or non-uniform scale is orders of magnitude larger.
inline constexpr double kSimilarityTolerance = 1e-3;

// Validate instead of silently accepting shear/reflection/non-uniform scale.
// Within kSimilarityTolerance the 3x3 part is re-orthonormalized (Gram-Schmidt
// in double, scale = mean column length), so `matrix` is the returned
// similarity plus round-off; cleanedMatrix() rebuilds that exact similarity.
inline std::optional<Similarity> similarity(const Mat4 &matrix,
                                           std::string *reason = nullptr) {
  auto reject = [&](const char *message) -> std::optional<Similarity> {
    if (reason) *reason = message;
    return std::nullopt;
  };
  for (float v : matrix.m) if (!std::isfinite(v)) return reject("non-finite matrix");
  if (std::fabs(matrix.m[3]) > 1e-6f || std::fabs(matrix.m[7]) > 1e-6f ||
      std::fabs(matrix.m[11]) > 1e-6f || std::fabs(matrix.m[15] - 1) > 1e-6f)
    return reject("non-affine matrix");
  double col[3][3], lengths[3];
  for (int c = 0; c < 3; ++c) {
    double sum = 0;
    for (int r = 0; r < 3; ++r) {
      col[c][r] = matrix.m[c*4+r];
      sum += col[c][r] * col[c][r];
    }
    lengths[c] = std::sqrt(sum);
    if (!(lengths[c] > 0) || !std::isfinite(lengths[c])) return reject("singular matrix");
  }
  auto dot = [](const double *a, const double *b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; };
  for (int a = 0; a < 3; ++a)
    for (int b = a + 1; b < 3; ++b)
      if (std::fabs(dot(col[a], col[b])) > kSimilarityTolerance * lengths[a] * lengths[b])
        return reject("shear");
  for (int c = 1; c < 3; ++c)
    if (std::fabs(lengths[c] - lengths[0]) > kSimilarityTolerance * lengths[0])
      return reject("non-uniform scale");
  const double det = col[0][0] * (col[1][1]*col[2][2] - col[2][1]*col[1][2]) -
                     col[1][0] * (col[0][1]*col[2][2] - col[2][1]*col[0][2]) +
                     col[2][0] * (col[0][1]*col[1][2] - col[1][1]*col[0][2]);
  if (!(det > 0)) return reject("reflection");
  // Gram-Schmidt: x = norm(c0), y = norm(c1 - (x.c1)x), z = x cross y.
  double x[3], y[3], z[3];
  for (int r = 0; r < 3; ++r) x[r] = col[0][r] / lengths[0];
  const double xy = dot(x, col[1]);
  for (int r = 0; r < 3; ++r) y[r] = col[1][r] - xy * x[r];
  const double yLength = std::sqrt(dot(y, y));
  for (int r = 0; r < 3; ++r) y[r] /= yLength;
  z[0] = x[1]*y[2] - x[2]*y[1];
  z[1] = x[2]*y[0] - x[0]*y[2];
  z[2] = x[0]*y[1] - x[1]*y[0];
  Mat4 rotation = Mat4::identity();
  for (int r = 0; r < 3; ++r) {
    rotation.m[0*4+r] = static_cast<float>(x[r]);
    rotation.m[1*4+r] = static_cast<float>(y[r]);
    rotation.m[2*4+r] = static_cast<float>(z[r]);
  }
  const float scale = static_cast<float>((lengths[0] + lengths[1] + lengths[2]) / 3);
  if (!std::isfinite(scale) || !(scale > 0)) return reject("unrepresentable scale");
  return Similarity{{matrix.m[12], matrix.m[13], matrix.m[14]},
                    rotationFromMatrix(rotation), scale};
}

// The exact similarity T * R * S that similarity() accepted.
inline Mat4 cleanedMatrix(const Similarity &s) {
  return Mat4::translation(s.position) * Mat4::rotation(s.orientation) *
         Mat4::scale({s.scale, s.scale, s.scale});
}

inline bool validPose(const DevicePose &pose, std::string *reason = nullptr) {
  const auto &p = pose.position;
  const auto &o = pose.orientation;
  if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
      !std::isfinite(o.x) || !std::isfinite(o.y) || !std::isfinite(o.z) ||
      !std::isfinite(o.angle)) {
    if (reason) *reason = "non-finite pose";
    return false;
  }
  if (o.angle != 0 && double(o.x)*o.x + double(o.y)*o.y + double(o.z)*o.z == 0) {
    if (reason) *reason = "zero rotation axis";
    return false;
  }
  return true;
}

template <class T>
void output(X3DExecutionContext &ctx, X3DNode *node, const char *field,
            const T &current, const T &next, bool force = false) {
  if (force || current != next) ctx.postOutputEvent(node, field, std::any(next));
}
} // namespace cave_detail

class CaveInputSystem : public System {
public:
  explicit CaveInputSystem(const Scene &scene) : scene_(scene) {}

  // Replace the complete snapshot between ticks. Validity is host-owned: the
  // latest submission persists until replaced (including across paused ticks).
  void submit(RoomFrame frame) { frame_ = std::move(frame); }

  void attach(X3DNode *node, X3DExecutionContext &) override {
    invalidatePlacement();
    if (dynamic_cast<TrackedDeviceSensor *>(node) || dynamic_cast<RoomFrameSensor *>(node))
      nodes_.insert(node);
  }
  void detach(X3DNode *node, X3DExecutionContext &) override {
    // Inline/GeoLOD retirement calls detach before releasing node ownership.
    // Clear the composed view too: it contains borrowed placement/device nodes.
    invalidatePlacement();
    nodes_.erase(node);
  }

  // Current placement and input diagnostics, refreshed during update().
  // A null node identifies the submitted room frame. Identity Transform
  // ancestors count too: their later animation would otherwise break anchoring.
  const std::vector<RoomGroupWarning> &warnings() const { return warnings_; }

  void update(double, X3DExecutionContext &ctx) override {
    if (nodes_.empty()) {
      warnings_.clear();
      return;
    }
    if (placementDirty_ || topologyRevision_ != ctx.sceneTopologyRevision()) {
      diagnoseRoomGroups();
      topologyRevision_ = ctx.sceneTopologyRevision();
      placementDirty_ = false;
    }
    warnings_ = placementWarnings_;
    std::optional<Mat4> worldFromRoom;
    std::optional<cave_detail::Similarity> room;
    std::string roomReason;
    if (!frame_.valid) roomReason = "host marked frame invalid (or no submission)";
    if (!frame_.valid) {
    } else if (auto clean = cave_detail::similarity(frame_.roomFromWorld, &roomReason)) {
      // Invert the re-orthonormalized frame, not the drifted submission, so
      // device poses and RoomGroup placement use one exact similarity.
      worldFromRoom = cave_detail::cleanedMatrix(*clean).tryInverse();
      if (worldFromRoom) room = cave_detail::similarity(*worldFromRoom, &roomReason);
      else roomReason = "matrix inverse is not representable";
    }
    if (!room) warnings_.push_back({nullptr, "room frame rejected: " + roomReason});
    using cave_detail::output;
    for (X3DNode *node : nodes_) {
      if (auto *sensor = dynamic_cast<RoomFrameSensor *>(node)) {
        const bool active = sensor->getEnabled() && room.has_value();
        const bool initial = active && !sensor->getIsActive();
        output(ctx, node, "isActive", sensor->getIsActive(), active);
        if (!active) continue; // Last valid transform remains readable.
        output(ctx, node, "position_changed", sensor->getPosition_changed(), room->position, initial);
        output(ctx, node, "orientation_changed", sensor->getOrientation_changed(), room->orientation, initial);
        output(ctx, node, "scale_changed", sensor->getScale_changed(),
               SFVec3f{room->scale, room->scale, room->scale}, initial);
        continue;
      }
      auto *sensor = dynamic_cast<TrackedDeviceSensor *>(node);
      const DeviceSample *sample = nullptr;
      for (const auto &candidate : frame_.devices)
        if (candidate.name == sensor->getDevice()) { sample = &candidate; break; }
      std::string deviceReason;
      if (!sample) deviceReason = "unknown device";
      else if (!sample->valid) deviceReason = "host marked sample invalid";
      else if (cave_detail::validPose(sample->roomFromDevice, &deviceReason) &&
               (!std::isfinite(sample->joystick.x) || !std::isfinite(sample->joystick.y)))
        deviceReason = "non-finite joystick";
      if (sensor->getEnabled() && !deviceReason.empty())
        warnings_.push_back({node, "device '" + sensor->getDevice() + "' rejected: " + deviceReason});
      const bool active = sensor->getEnabled() && room && deviceReason.empty();
      const bool initial = active && !sensor->getIsActive();
      output(ctx, node, "isActive", sensor->getIsActive(), active);
      if (!active) {
        MFBool released(sensor->getButtons_changed().size(), false);
        output(ctx, node, "buttons_changed", sensor->getButtons_changed(), released);
        output(ctx, node, "joystick_changed", sensor->getJoystick_changed(), SFVec2f{0, 0});
        continue;
      }
      // position/orientation are device-to-WORLD (scene root) wherever the
      // sensor sits, unlike ProximitySensor. Content inside a RoomGroup routes
      // the room* outputs, which are RoomGroup-local by construction.
      const auto &pose = sample->roomFromDevice;
      const SFVec3f position = worldFromRoom->transformPoint(pose.position);
      const Mat4 rotation = Mat4::rotation(room->orientation) * Mat4::rotation(pose.orientation);
      output(ctx, node, "position_changed", sensor->getPosition_changed(), position, initial);
      output(ctx, node, "orientation_changed", sensor->getOrientation_changed(), rotationFromMatrix(rotation), initial);
      output(ctx, node, "roomPosition_changed", sensor->getRoomPosition_changed(), pose.position, initial);
      output(ctx, node, "roomOrientation_changed", sensor->getRoomOrientation_changed(), pose.orientation, initial);
      const bool head = sensor->getDevice() == "HEAD";
      output(ctx, node, "buttons_changed", sensor->getButtons_changed(), head ? MFBool{} : sample->buttons, initial);
      const SFVec2f joystick = head ? SFVec2f{0, 0} : SFVec2f{
          std::clamp(sample->joystick.x, -1.0f, 1.0f), std::clamp(sample->joystick.y, -1.0f, 1.0f)};
      output(ctx, node, "joystick_changed", sensor->getJoystick_changed(), joystick, initial);
    }
  }

private:
  void invalidatePlacement() {
    placementDirty_ = true;
    placementWarnings_.clear();
    warnings_.clear();
  }

  // Fields that place a child in the rendered scene graph. Everything else that
  // holds nodes (TransformSensor.targetObject, Collision.proxy, metadata,
  // HAnimHumanoid.joints/sites, geometry/appearance, sensor targets, ...) is a
  // reference or non-rendered slot and cannot put a RoomGroup under a transform.
  static bool placesChild(const std::string &type, const std::string &field) {
    if (field == "children") return true;
    if (field == "level") return type == "LOD";             // X3D 3.x name.
    if (field == "choice") return type == "Switch";         // X3D 3.x name.
    if (field == "rootNode") return type == "GeoLOD";
    if (field == "layers") return type == "LayerSet";
    if (field == "skeleton" || field == "skin") return type == "HAnimHumanoid";
    if (field == "shape") return type == "CADFace";
    return false;
  }

  void diagnoseRoomGroups() {
    placementWarnings_.clear();
    // Follow the composition's private sensor/ROUTE rather than the primary's
    // expandedSources entry: an enclosing PROTO can replace that source record.
    // The private sensor TYPE marks a RoomGroup; author DEF names cannot.
    std::unordered_set<const X3DNode *> anchors;
    auto collect = [&](const auto &routes) {
      for (const auto &route : routes)
        if (dynamic_cast<cave_detail::RoomGroupFrameSensor *>(route.from.get()) &&
            nodes_.count(route.from.get()) && route.toField == "translation")
          anchors.insert(route.to.get());
    };
    collect(scene_.resolvedProtoRoutes);
    collect(scene_.resolvedInlineRoutes); // Inline hoists child-private ROUTEs here.
    if (anchors.empty()) return;
    // Path state bits: transformed ancestor, inside a LayerSet layer.
    enum : unsigned { Transformed = 1, Layered = 2 };
    std::unordered_set<const X3DNode *> seen[4], warnedTransform, warnedLayer;
    std::function<void(const X3DNode *, unsigned)> walk = [&](const X3DNode *node, unsigned state) {
      if (!node || !seen[state].insert(node).second) return;
      if (anchors.count(node)) {
        if ((state & Transformed) && warnedTransform.insert(node).second)
          placementWarnings_.push_back({node, "RoomGroup '" + node->getDEF() +
              "' requires a transform-free path to the world root; put room-local transforms inside it"});
        if ((state & Layered) && warnedLayer.insert(node).second)
          placementWarnings_.push_back({node, "RoomGroup '" + node->getDEF() +
              "' is inside a Layer, which has its own Viewpoint/NavigationInfo bindings; "
              "the room frame follows root-scene navigation, so place RoomGroup outside LayerSet"});
      }
      const auto type = node->nodeTypeName();
      unsigned below = state;
      if (TransformSystem::isTransform(node) || type == "Billboard" || type == "ScreenGroup" ||
          type == "LayoutGroup" || type == "LayoutLayer")
        below |= Transformed;
      if (type == "LayerSet") below |= Layered;
      forEachChildNode(*node, [&](const FieldInfo &field, const std::shared_ptr<X3DNode> &child) {
        if (placesChild(type, field.x3dName)) walk(child.get(), below);
      });
    };
    for (const auto &root : scene_.rootNodes) walk(root.get(), 0);
  }

  const Scene &scene_; // Must outlive the system, like other scene-backed Systems.
  RoomFrame frame_;
  std::unordered_set<X3DNode *> nodes_;
  bool placementDirty_ = true;
  std::uint64_t topologyRevision_ = 0;
  std::vector<RoomGroupWarning> placementWarnings_;
  std::vector<RoomGroupWarning> warnings_;
};

// Install AFTER the standard runtime systems (attachStandardRuntime /
// RuntimeSession add InlineRuntimeSystem). Systems update in installation
// order; installed earlier, sensors in a freshly loaded Inline or GeoLOD tile
// would first report on the following tick.
inline std::shared_ptr<CaveInputSystem> installCaveInput(Scene &scene, X3DExecutionContext &ctx) {
  auto system = std::make_shared<CaveInputSystem>(scene);
  detail::forEachNode(scene, [&](X3DNode *node) { system->attach(node, ctx); });
  ctx.addSystem(system);
  return system;
}
} // namespace x3d::runtime::ext
#endif
