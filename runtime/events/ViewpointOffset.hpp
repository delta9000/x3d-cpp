// ViewpointOffset.hpp — CONF-VIEWNAV: the per-viewpoint user offset.
// The "relative viewing transformation" of §23.3.1: a rigid transform in the
// viewpoint's local frame, applied AFTER the authored pose. Navigation
// accumulates it; the authored position/orientation fields are never mutated
// (§23.2.3). Process-local runtime state — NOT part of the synced scene graph
// (so a CAVE master broadcasts the authored pose while each wall owns its offset).
#ifndef X3D_RUNTIME_VIEWPOINT_OFFSET_HPP
#define X3D_RUNTIME_VIEWPOINT_OFFSET_HPP

#include "Mat4.hpp"

namespace x3d::runtime {

/// A viewpoint's user offset: effectiveLocalEye = T(pos)·R(ori) · local.
struct ViewpointOffset {
  Mat4 local = Mat4::identity();
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_VIEWPOINT_OFFSET_HPP
