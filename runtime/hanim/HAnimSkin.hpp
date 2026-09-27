// HAnimSkin.hpp — the H-Anim skinning contract (ISO/IEC 19775-1 §26, ISO/IEC
// 19774-1; ADR-0055).
//
// A humanoid's skin is compiled once into an immutable SkinBinding (bind-pose
// source coordinates, per-vertex joint influences, inverse bind matrices, Joint
// displacers). Each tick a SkinPose (the joint matrix palette plus displacer
// weights) is evaluated from the live skeleton. Renderers that skin on the GPU
// consume the binding + palette; everything else calls deform(), the reference
// CPU skinner and the single implementation of the mathematics:
//
//   D_j        = C_j · B_j⁻¹                 (palette, humanoid-local space)
//   p_i        = Σ_j a_ij · D_j · p_i^bind    (a_ij = w_ij / Σ_j w_ij; unweighted
//                                              vertices keep p_i^bind)
//   n_i        = normalize(Σ_j a_ij · (D_j⁻ᵀ)₃ₓ₃ · n_i^bind)
//   p_i       += Σ_d weight_d · (C_owner(d))₃ₓ₃ · displacement_d,i   (Joint
//                                              displacers, after skinning)
//
// Bind pose: skinBindingCoords / skinBindingNormals and jointBinding* when
// authored (HAnim v2, non-BASIC); otherwise the authored skinCoord/skinNormal
// with every joint at rest, i.e. B_j = identity (HAnim v1 / BASIC: "all the
// joint angles shall be zero"). The SDK never writes deformed values back into
// skinCoord.point (no feedback into the next frame's source).
//
// Space: everything here is humanoid-local (the HAnimHumanoid's own frame, its
// TRS excluded); the HAnimHumanoid's world transform places the result.
//
// This header is the frozen contract between the deformation core, extraction
// and the motion system. The (header-only) implementation is HAnimSkinImpl.hpp.
#ifndef X3D_RUNTIME_HANIM_SKIN_HPP
#define X3D_RUNTIME_HANIM_SKIN_HPP

#include "Mat4.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace x3d::runtime::hanim {

using x3d::core::SFVec3f;
using x3d::nodes::X3DNode;

/// One joint's influence on a source vertex; weights are normalized per vertex.
struct Influence {
  std::uint32_t joint = 0;  ///< index into SkinBinding::joints (palette index)
  float weight = 0.0f;
};

/// A Joint displacer (§26.3.1 HAnimDisplacer on an HAnimJoint): offsets in the
/// owning joint's frame, applied to skin vertices after skinning.
struct JointDisplacer {
  const X3DNode *node = nullptr;          ///< the HAnimDisplacer (its `weight` is live)
  std::uint32_t joint = 0;                ///< owning joint (palette index)
  std::vector<std::uint32_t> index;       ///< skin source coordinate indices
  std::vector<SFVec3f> displacement;      ///< one per index, joint-local
};

/// Immutable per-humanoid skin binding. Source indices refer to the humanoid's
/// skinCoord (and skinNormal) point order.
struct SkinBinding {
  const X3DNode *humanoid = nullptr;
  std::vector<const X3DNode *> joints;    ///< palette order: HAnimHumanoid.joints, deduplicated
  std::vector<Mat4> inverseBind;          ///< B_j⁻¹ per joint, humanoid-local
  std::vector<SFVec3f> bindPositions;     ///< per source coordinate
  std::vector<SFVec3f> bindNormals;       ///< per source normal (empty if none authored)
  std::vector<std::uint32_t> influenceOffset;  ///< CSR offsets, size bindPositions.size()+1
  std::vector<Influence> influences;      ///< CSR payload (variable width; never capped)
  std::vector<JointDisplacer> displacers;
  std::vector<std::string> diagnostics;   ///< malformed weights, bad indices, length mismatches

  bool empty() const { return bindPositions.empty(); }
};

/// The live pose: palette D_j = C_j · B_j⁻¹ and the rotation part of C_j for
/// displacers, both humanoid-local; plus each displacer's current weight.
struct SkinPose {
  std::vector<Mat4> palette;
  std::vector<Mat4> jointMatrix;          ///< C_j (for displacer directions)
  std::vector<float> displacerWeight;     ///< parallel to SkinBinding::displacers
};

/// Compile the skin binding of an HAnimHumanoid. An empty binding (no skinCoord,
/// no weights) means the humanoid has no deformable skin.
SkinBinding compileBinding(const X3DNode &humanoid);

/// Evaluate the current pose from the live skeleton (joint TRS as of this tick).
SkinPose evaluatePose(const SkinBinding &binding);

/// Reference CPU skinner: fill `positions` (and `normals`, if non-null and the
/// binding has normals) indexed by source coordinate/normal index.
void deform(const SkinBinding &binding, const SkinPose &pose, std::vector<SFVec3f> &positions,
            std::vector<SFVec3f> *normals);

/// Segment displacers (§26.3.1 HAnimDisplacer on an HAnimSegment): when
/// `coordNode` is `segment`'s coord, add every displacer of `segment`, weighted
/// by its current weight, to `points` (that Coordinate's points, in Segment
/// coordinates). Returns false when `coordNode` is not the Segment's coord or no
/// displacer applies (points untouched). Extraction calls this when building a
/// mesh under a Segment; the authored Coordinate is never modified.
bool displaceSegmentPoints(const X3DNode &segment, const X3DNode &coordNode,
                           std::vector<SFVec3f> &points);

} // namespace x3d::runtime::hanim

#include "HAnimSkinImpl.hpp"

#endif // X3D_RUNTIME_HANIM_SKIN_HPP
