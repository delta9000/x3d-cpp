// HAnimSkinImpl.hpp — implementation of the HAnimSkin.hpp contract (ADR-0055).
// Included from HAnimSkin.hpp only.
//
// STUB: compileBinding returns an empty binding and deform copies the bind
// pose, so extraction and motion work can build against the contract while the
// deformation core is implemented.
#ifndef X3D_RUNTIME_HANIM_SKIN_IMPL_HPP
#define X3D_RUNTIME_HANIM_SKIN_IMPL_HPP

namespace x3d::runtime::hanim {

inline SkinBinding compileBinding(const X3DNode &humanoid) {
  SkinBinding b;
  b.humanoid = &humanoid;
  b.influenceOffset.push_back(0);
  return b;
}

inline SkinPose evaluatePose(const SkinBinding &binding) {
  SkinPose pose;
  pose.palette.assign(binding.joints.size(), Mat4::identity());
  pose.jointMatrix.assign(binding.joints.size(), Mat4::identity());
  pose.displacerWeight.assign(binding.displacers.size(), 0.0f);
  return pose;
}

inline void deform(const SkinBinding &binding, const SkinPose &, std::vector<SFVec3f> &positions,
                   std::vector<SFVec3f> *normals) {
  positions = binding.bindPositions;
  if (normals) *normals = binding.bindNormals;
}

inline bool displaceSegmentPoints(const X3DNode &, std::vector<SFVec3f> &) { return false; }

} // namespace x3d::runtime::hanim

#endif // X3D_RUNTIME_HANIM_SKIN_IMPL_HPP
