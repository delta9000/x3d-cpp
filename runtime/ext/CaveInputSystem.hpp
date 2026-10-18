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

// Validate instead of silently removing shear/reflection/non-uniform scale.
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
  double lengths[3];
  for (int c = 0; c < 3; ++c) {
    double sum = 0;
    for (int r = 0; r < 3; ++r) sum += double(matrix.m[c*4+r]) * matrix.m[c*4+r];
    lengths[c] = std::sqrt(sum);
    if (!(lengths[c] > 0)) return reject("singular matrix");
  }
  for (int a = 0; a < 3; ++a)
    for (int b = a + 1; b < 3; ++b) {
      double dot = 0;
      for (int r = 0; r < 3; ++r) dot += double(matrix.m[a*4+r]) * matrix.m[b*4+r];
      if (std::fabs(dot) > 1e-5 * lengths[a] * lengths[b]) return reject("shear");
    }
  for (int c = 1; c < 3; ++c)
    if (std::fabs(lengths[c] - lengths[0]) > 1e-5 * lengths[0])
      return reject("non-uniform scale");
  Mat4 rotation = matrix;
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) rotation.m[c*4+r] /= lengths[c];
  const auto &m = rotation.m;
  const double det = double(m[0]) * (double(m[5])*m[10] - double(m[9])*m[6]) -
                     double(m[4]) * (double(m[1])*m[10] - double(m[9])*m[2]) +
                     double(m[8]) * (double(m[1])*m[6] - double(m[5])*m[2]);
  if (!(det > 0)) return reject("reflection");
  const float scale = static_cast<float>(lengths[0]);
  if (!std::isfinite(scale) || !(scale > 0)) return reject("unrepresentable scale");
  return Similarity{{matrix.m[12], matrix.m[13], matrix.m[14]},
                    rotationFromMatrix(rotation), scale};
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
    if (frame_.valid && cave_detail::similarity(frame_.roomFromWorld, &roomReason)) {
      worldFromRoom = frame_.roomFromWorld.tryInverse();
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

  void diagnoseRoomGroups() {
    placementWarnings_.clear();
    // Follow the composition's private sensor/ROUTE rather than the primary's
    // expandedSources entry: an enclosing PROTO can replace that source record.
    std::unordered_set<const X3DNode *> anchors;
    auto collect = [&](const auto &routes) {
      for (const auto &route : routes)
        if (dynamic_cast<RoomFrameSensor *>(route.from.get()) &&
            route.from->getDEF() == "RoomGroupFrame" && nodes_.count(route.from.get()) &&
            route.toField == "translation")
          anchors.insert(route.to.get());
    };
    collect(scene_.resolvedProtoRoutes);
    collect(scene_.resolvedInlineRoutes); // Inline hoists child-private ROUTEs here.
    if (anchors.empty()) return;
    std::unordered_set<const X3DNode *> seen[2], warned;
    std::function<void(const X3DNode *, bool)> walk = [&](const X3DNode *node, bool transformed) {
      if (!node || !seen[transformed].insert(node).second) return;
      if (transformed && anchors.count(node) && warned.insert(node).second)
        placementWarnings_.push_back({node, "RoomGroup '" + node->getDEF() +
            "' requires a transform-free path to the world root; put room-local transforms inside it"});
      const auto type = node->nodeTypeName();
      const bool below = transformed || TransformSystem::isTransform(node) ||
          type == "Billboard" || type == "ScreenGroup" || type == "LayoutGroup" || type == "LayoutLayer";
      forEachChildNode(*node, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &child) {
        walk(child.get(), below);
      });
    };
    for (const auto &root : scene_.rootNodes) walk(root.get(), false);
  }

  const Scene &scene_; // Must outlive the system, like other scene-backed Systems.
  RoomFrame frame_;
  std::unordered_set<X3DNode *> nodes_;
  bool placementDirty_ = true;
  std::uint64_t topologyRevision_ = 0;
  std::vector<RoomGroupWarning> placementWarnings_;
  std::vector<RoomGroupWarning> warnings_;
};

inline std::shared_ptr<CaveInputSystem> installCaveInput(Scene &scene, X3DExecutionContext &ctx) {
  auto system = std::make_shared<CaveInputSystem>(scene);
  detail::forEachNode(scene, [&](X3DNode *node) { system->attach(node, ctx); });
  ctx.addSystem(system);
  return system;
}
} // namespace x3d::runtime::ext
#endif
