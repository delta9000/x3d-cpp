// ViewpointBindSystem.hpp — CONF-VIEWNAV Phases 2-3: bind-time offset rules
// (ISO/IEC 19775-1 §23.3.1). A post-cascade hook that, when the bound viewpoint
// changes, applies jump / retainUserOffsets to the per-viewpoint user offset —
// never the authored fields (BIND-04/07/08), dispatches the viewpoint's
// navigationInfo (BIND-02), and animates the effective camera between viewpoints
// over the bound NavigationInfo's transitionTime, emitting transitionComplete
// (BIND-05). The per-node offset store persists each viewpoint's "stored relative
// transformation" (BIND-08).
#ifndef X3D_RUNTIME_VIEWPOINT_BIND_SYSTEM_HPP
#define X3D_RUNTIME_VIEWPOINT_BIND_SYSTEM_HPP

#include "GeometryBounds.hpp" // geombounds::getField
#include "HeadPose.hpp"
#include "Interpolation.hpp"  // slerpRotation
#include "Mat4.hpp"           // rotationFromMatrix
#include "ViewpointOffset.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/NavigationInfo.hpp" // NavigationTransitionTypeValues

#include <memory>

namespace x3d::runtime {

using namespace x3d::core;

class ViewpointBindSystem {
public:
  void onPostCascade(X3DExecutionContext &ctx) {
    X3DNode *cur = ctx.boundViewpoint();
    if (cur != lastVp_) {
      anim_.active = false; // a new bind supersedes any in-flight transition
      const BindTransition kind = ctx.lastViewpointBindTransition();
      onBind(ctx, cur, kind);
      lastVp_ = cur;
      ctx.setLastViewpointBindTransition(BindTransition::None); // consume
    } else if (anim_.active && cur == anim_.vp) {
      advance(ctx);
    }
    if (cur) {
      lastCam_ = ctx.viewMatrix().inverse(); // effective camera of the bound vp
      haveCam_ = true;
    }
  }

private:
  struct Anim {
    bool active = false;
    X3DNode *vp = nullptr;
    X3DNode *nav = nullptr;
    Mat4 from = Mat4::identity();
    Mat4 to = Mat4::identity();
    double t0 = 0.0, dur = 0.0;
  } anim_;

  // Choose the offset so the effective camera equals `targetCam` (§23.3.1):
  //   Onew = Ocur · head · currentCam⁻¹ · targetCam · head⁻¹   (all public API).
  static void driveCameraTo(X3DExecutionContext &ctx, X3DNode *vp, const Mat4 &targetCam) {
    const Mat4 Ocur = ctx.viewpointOffset(vp).local;
    const Mat4 currentCam = ctx.viewMatrix().inverse();
    const HeadPose &h = ctx.headPose();
    const Mat4 head = Mat4::translation(h.position) * Mat4::rotation(h.orientation);
    ViewpointOffset off;
    off.local = Ocur * head * currentCam.inverse() * targetCam * head.inverse();
    ctx.setViewpointOffset(vp, off);
  }

  // Rigid interpolation of two camera matrices: lerp position, slerp rotation.
  static Mat4 rigidLerp(const Mat4 &a, const Mat4 &b, float t) {
    const SFVec3f pa = a.transformPoint(SFVec3f{0,0,0}), pb = b.transformPoint(SFVec3f{0,0,0});
    const SFVec3f p{pa.x+(pb.x-pa.x)*t, pa.y+(pb.y-pa.y)*t, pa.z+(pb.z-pa.z)*t};
    const SFRotation r = slerpRotation(rotationFromMatrix(a), rotationFromMatrix(b), t);
    return Mat4::translation(p) * Mat4::rotation(r);
  }

  static bool isTeleport(X3DNode *nav) {
    if (!nav) return false;
    // transitionType is an open MFString vocabulary — match the token.
    for (const auto &tt : geombounds::getField<std::vector<std::string>>(
             *nav, "transitionType", {}))
      return tt == "TELEPORT"; // first wins
    return false;
  }

  void onBind(X3DExecutionContext &ctx, X3DNode *vp, BindTransition kind) {
    if (!vp) return;
    const bool jump = geombounds::getField<bool>(*vp, "jump", true);
    const bool retain = geombounds::getField<bool>(*vp, "retainUserOffsets", false);

    // 1. Decide the final offset (BIND-04/07/08, BIND-09).
    //    BIND-09 §23.3.1 r6.3: a Pop restores the popped-to vp's stored offset
    //    — never reset it, regardless of jump/retain (those govern push binds).
    if (kind == BindTransition::Pop) {
      // stored offset in offsets_[vp] is already correct; leave it be
    } else if (!jump) {
      if (haveCam_) driveCameraTo(ctx, vp, lastCam_); // continuous: view doesn't move
    } else if (!retain) {
      ctx.setViewpointOffset(vp, ViewpointOffset{}); // snap to authored pose
    } // retain=TRUE: keep the viewpoint's stored offset

    // 2. BIND-05: animate the effective camera from the old view to the target,
    //    unless TELEPORT / zero transitionTime / no prior view. The transition is
    //    governed by the BINDING viewpoint's own navigationInfo if it names one
    //    (that NavigationInfo is becoming bound), else the currently-bound one.
    const Mat4 targetCam = ctx.viewMatrix().inverse();
    SFNode niField = geombounds::getField<SFNode>(*vp, "navigationInfo", nullptr);
    X3DNode *nav = niField ? niField.get() : ctx.boundNavigationInfo();
    const double dur = nav ? geombounds::getField<double>(*nav, "transitionTime", 1.0) : 0.0;
    if (haveCam_ && dur > 0.0 && !isTeleport(nav) && camsDiffer(lastCam_, targetCam)) {
      anim_ = Anim{true, vp, nav, lastCam_, targetCam, ctx.now(), dur};
      driveCameraTo(ctx, vp, lastCam_); // start the animation at the old camera
    }

    // 3. BIND-02: bind this viewpoint's navigationInfo, if it names one.
    if (niField) ctx.postEvent(niField.get(), "set_bind", std::any(SFBool{true}));
  }

  void advance(X3DExecutionContext &ctx) {
    double t = anim_.dur > 0.0 ? (ctx.now() - anim_.t0) / anim_.dur : 1.0;
    if (t < 0.0) t = 0.0;
    if (t >= 1.0) {
      driveCameraTo(ctx, anim_.vp, anim_.to);
      anim_.active = false;
      if (anim_.nav) ctx.postEvent(anim_.nav, "transitionComplete", std::any(SFBool{true}));
    } else {
      driveCameraTo(ctx, anim_.vp, rigidLerp(anim_.from, anim_.to, static_cast<float>(t)));
    }
  }

  // Two effective cameras differ if their eye position OR view direction differs
  // (so a rotation-only bind still animates, not just a translation).
  static bool camsDiffer(const Mat4 &a, const Mat4 &b) {
    const SFVec3f pa = a.transformPoint(SFVec3f{0,0,0}), pb = b.transformPoint(SFVec3f{0,0,0});
    const float dp = (pa.x-pb.x)*(pa.x-pb.x)+(pa.y-pb.y)*(pa.y-pb.y)+(pa.z-pb.z)*(pa.z-pb.z);
    const SFVec3f fa = a.transformDirection(SFVec3f{0,0,-1}), fb = b.transformDirection(SFVec3f{0,0,-1});
    const float df = (fa.x-fb.x)*(fa.x-fb.x)+(fa.y-fb.y)*(fa.y-fb.y)+(fa.z-fb.z)*(fa.z-fb.z);
    return dp > 1e-12f || df > 1e-12f;
  }

  X3DNode *lastVp_ = nullptr;
  Mat4 lastCam_ = Mat4::identity();
  bool haveCam_ = false;
};

/// Production wiring: drive a ViewpointBindSystem from the per-tick post-cascade hook.
inline void attachViewpointBind(X3DExecutionContext &ctx) {
  auto sys = std::make_shared<ViewpointBindSystem>();
  ctx.addPostCascadeHook([sys](X3DExecutionContext &c) { sys->onPostCascade(c); });
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_VIEWPOINT_BIND_SYSTEM_HPP
