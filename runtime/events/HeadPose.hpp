// HeadPose.hpp — CONF-VIEWNAV: the consumer→runtime head-tracking seam.
// A process-local viewer head pose in the bound viewpoint's local frame,
// composed onto the effective view AFTER the navigation offset (CAVE: each wall
// process supplies its own head pose). Mirrors PointerState/KeyState: pure data
// + a setter that bumps a revision. Identity until the consumer sets it.
#ifndef X3D_RUNTIME_HEAD_POSE_HPP
#define X3D_RUNTIME_HEAD_POSE_HPP

#include "x3d/core/X3Dtypes.hpp"

namespace x3d::runtime {

using namespace x3d::core;

struct HeadPose {
  SFVec3f position{0, 0, 0};
  SFRotation orientation{0, 0, 1, 0};
  unsigned long revision = 0;

  void set(const SFVec3f &pos, const SFRotation &ori) {
    position = pos;
    orientation = ori;
    ++revision;
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_HEAD_POSE_HPP
