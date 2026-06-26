// NavigationSystem.hpp — the interactive navigation driver for the M2D
// interaction layer. A System (update(now,ctx)) that reads the input seam
// (PointerState drag deltas + KeyState) and the bound NavigationInfo, and
// mutates the bound Viewpoint per the active navigation mode. Collision-free:
// implements EXAMINE / FLY / LOOKAT / NONE (and ANY -> EXAMINE). WALK,
// terrain-following, gravity, and collision are deferred (NAV-COLLISION).
//
// Spec grounding (ISO/IEC 19775-1:2023):
//   §23.2.3  Navigation is performed relative to the viewpoint's location.
//   §23.3.1  centerOfRotation in the same local frame as position/orientation;
//            the up vector is +Y of the viewpoint-local coord system (NOT the
//            orientation field); default pose: on +Z looking down -Z.
//   §23.4.4  NavigationInfo type list (first recognized type wins), speed,
//            transitionType/transitionTime; EXAMINE orbit; FLY free-flight;
//            LOOKAT frame-and-animate to a picked object's bbox center.
//   §23.4.6  Viewpoint position/orientation/centerOfRotation/fieldOfView.
//
// The pointer-drag delta is taken from the change in the consumer-supplied
// pointer bearing across ticks (the seam carries a world Ray; for navigation we
// use the ray ORIGIN's screen-plane proxy — the consumer reports pointer motion
// by moving the ray, and no projection params cross the seam). Keys are opaque
// ints; this System maps a small fixed set (kKey*) to FLY movement, matching
// Annex G.3's informative arrow-key mapping. The consumer chooses which native
// key codes it forwards as these values.
//
// Codegen-free: reads/writes Viewpoint + NavigationInfo through the existing
// generated accessors + ctx.writeField/postEvent. namespace x3d::runtime.
#ifndef X3D_RUNTIME_NAVIGATION_SYSTEM_HPP
#define X3D_RUNTIME_NAVIGATION_SYSTEM_HPP

#include "GeometryBounds.hpp" // geombounds::getField (generic viewpoint field read)
#include "Interpolation.hpp" // Quat, quatFromRotation, rotationFromQuat, slerp
#include "Mat4.hpp"          // rotationFromMatrix
#include "PointerState.hpp"
#include "ViewpointOffset.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "NavigationInfo.hpp" // getType/getSpeed/getTransition*; emitTransitionComplete

#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Interactive navigation: mutate the bound Viewpoint from the input seam.
 * @details Registered as a System; its update(now,ctx) runs each tick before
 *          the cascade drains. The active mode is the first recognized value of
 *          the bound NavigationInfo.type (§23.4.4). Cross-tick state holds the
 *          last pointer position (for drag deltas), the last tick time (for dt),
 *          and the in-flight LOOKAT transition.
 */
class NavigationSystem : public System {
public:
  // Navigation modes (public so consumers can call setForcedMode).
  enum class Mode { None, Examine, Fly, Lookat };

  // Opaque key codes this System understands. The consumer maps its native key
  // constants onto these via ctx.setKey (Annex G.3 informative arrow mapping).
  static constexpr int kKeyForward = 1; // UP    arrow / W
  static constexpr int kKeyBack    = 2; // DOWN  arrow / S
  static constexpr int kKeyLeft    = 3; // LEFT  arrow / A
  static constexpr int kKeyRight   = 4; // RIGHT arrow / D

  void attach(X3DNode * /*node*/, X3DExecutionContext & /*ctx*/) override {}

  // Override the scene's NavigationInfo.type (dev affordance: the consumer's
  // mode-cycle key). std::nullopt = scene-driven (default).
  void setForcedMode(std::optional<Mode> m) { forcedMode_ = m; }

  void update(double now, X3DExecutionContext &ctx) override {
    // BIND-03: any bound X3DViewpointNode (Viewpoint/Ortho/Geo) — boundViewpoint()
    // returns whichever is bound; read its pose via reflection, never a typed cast.
    X3DNode *vp = ctx.boundViewpoint();
    if (!vp) { lastNow_ = now; haveTime_ = true; return; }
    NavigationInfo *nav = dynamic_cast<NavigationInfo *>(ctx.boundNavigationInfo());

    const double dt = haveTime_ ? (now - lastNow_) : 0.0;
    lastNow_ = now;
    haveTime_ = true;

    const Mode mode = forcedMode_ ? *forcedMode_ : resolveMode(nav);
    // NAV-FLY-ROLL: entering FLY (from another mode or a different vp) requires
    // re-decomposing the current orientation into yaw/pitch scalars so FLY drag
    // continues from the effective pose without carrying stale state.
    if (mode != lastMode_ || vp != lastFlyVp_) flyOrientValid_ = false;
    lastMode_ = mode;
    lastFlyVp_ = vp;

    // Pointer drag delta in screen-proxy units (consumer-reported ray origin).
    const PointerState &ps = ctx.pointerState();
    const bool pointerChanged = ps.revision != lastPointerRev_;
    lastPointerRev_ = ps.revision;
    float dx = 0.0f, dy = 0.0f;
    // Arbitration: a pointing-device sensor grab owns the pointer (§20.2.1);
    // do not also drag-navigate this tick.
    const bool dragging = ps.present && ps.buttonDown && !ctx.pointerConsumedBySensor();
    if (dragging) {
      if (dragActive_) { dx = ps.ray.origin.x - lastPx_; dy = ps.ray.origin.y - lastPy_; }
      lastPx_ = ps.ray.origin.x;
      lastPy_ = ps.ray.origin.y;
      dragActive_ = true;
    } else {
      dragActive_ = false;
    }
    (void)pointerChanged;

    // An in-flight LOOKAT transition takes precedence (it owns the pose until
    // it completes), regardless of the current mode — it was committed at click.
    if (lookat_.active) { advanceLookat(now, ctx, vp, nav); return; }

    switch (mode) {
    case Mode::None:
      return; // §23.4.4: NONE produces no output.
    case Mode::Examine:
      if (dragging && (dx != 0.0f || dy != 0.0f)) examineOrbit(ctx, vp, dx, dy);
      return;
    case Mode::Fly:
      flyUpdate(ctx, vp, nav, dx, dy, dragging, dt);
      return;
    case Mode::Lookat:
      // LOOKAT triggers on a button-DOWN edge over a picked object.
      if (ps.present && ps.buttonDown && !buttonWasDown_ && !ctx.pointerConsumedBySensor())
        beginLookat(now, ctx, vp, nav, ps);
      buttonWasDown_ = ps.buttonDown;
      return;
    }
    buttonWasDown_ = ps.buttonDown;
  }

private:
  static constexpr float kPi = 3.14159265358979323846f;
  // Angular sensitivity: a full screen-proxy unit of drag ~ pi/2 radians. Browser
  // -defined; the spec only mandates "tied to pointer, no damping" (§23.4.4).
  static constexpr float kRotScale = kPi; // radians per unit of drag delta
  // NAV-FLY-ROLL: clamp pitch just shy of +/-90 deg so asin() stays defined
  // and the camera never flips through the pole.
  static constexpr float kPitchLimit = kPi * 0.5f - 0.001f;

  // First recognized type wins (§23.4.4). ANY and unrecognized -> EXAMINE.
  // NavigationInfo.type is an open MFString vocabulary, so match tokens (not a
  // closed enum) and skip unrecognized/custom types per the spec.
  static Mode resolveMode(NavigationInfo *nav) {
    if (!nav) return Mode::Examine;
    for (const std::string &t : nav->getType()) {
      if (t == "NONE")    return Mode::None;
      if (t == "EXAMINE") return Mode::Examine;
      if (t == "FLY")     return Mode::Fly;
      if (t == "LOOKAT")  return Mode::Lookat;
      if (t == "ANY")     return Mode::Examine; // sensible default
      // WALK / EXPLORE not implemented (collision/niche) -> try next token.
    }
    return Mode::Examine;
  }

  // ---- vector helpers -------------------------------------------------------
  static SFVec3f sub(const SFVec3f &a, const SFVec3f &b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
  static SFVec3f add(const SFVec3f &a, const SFVec3f &b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
  static SFVec3f mul(const SFVec3f &a, float s) { return {a.x*s, a.y*s, a.z*s}; }
  static float len(const SFVec3f &v) { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
  static SFVec3f norm(const SFVec3f &v) {
    float l = len(v); return l > 1e-12f ? SFVec3f{v.x/l, v.y/l, v.z/l} : v;
  }
  static SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
  }
  static float dot(const SFVec3f &a, const SFVec3f &b) { return a.x*b.x+a.y*b.y+a.z*b.z; }

  // ---- user-offset I/O (CONF-VIEWNAV) --------------------------------------
  // Navigation reads the EFFECTIVE local eye (authored pose ∘ offset) and writes
  // a new offset — it never touches the authored position/orientation (§23.2.3).
  static Mat4 poseOf(X3DNode *vp) {
    // GeoViewpoint.position is SFVec3d; read leniently so a bound GeoViewpoint
    // isn't pinned to the origin (GEO-1 sibling).
    return Mat4::translation(geombounds::getVec3fLenient(*vp, "position", {0,0,0})) *
           Mat4::rotation(geombounds::getField<SFRotation>(*vp, "orientation", {0,0,1,0}));
  }
  static SFVec3f effPos(X3DExecutionContext &ctx, X3DNode *vp) {
    return (poseOf(vp) * ctx.viewpointOffset(vp).local).transformPoint(SFVec3f{0,0,0});
  }
  static SFRotation effOri(X3DExecutionContext &ctx, X3DNode *vp) {
    return rotationFromMatrix(poseOf(vp) * ctx.viewpointOffset(vp).local);
  }
  static SFVec3f cor(X3DNode *vp) {
    // GeoViewpoint.centerOfRotation is SFVec3d; read leniently (GEO-1 sibling).
    return geombounds::getVec3fLenient(*vp, "centerOfRotation", {0,0,0});
  }
  // Set the offset so the effective local eye becomes T(pos)·R(ori).
  static void commitEye(X3DExecutionContext &ctx, X3DNode *vp,
                        const SFVec3f &pos, const SFRotation &ori) {
    ViewpointOffset off;
    off.local = poseOf(vp).inverse() * Mat4::translation(pos) * Mat4::rotation(ori);
    ctx.setViewpointOffset(vp, off);
  }

  static Quat quatMul(const Quat &a, const Quat &b) {
    return Quat{
      a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
      a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
      a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
      a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z};
  }
  static Quat axisAngleQuat(const SFVec3f &axis, float angle) {
    return quatFromRotation(SFRotation{axis.x, axis.y, axis.z, angle});
  }
  static SFVec3f rotateByQuat(const Quat &q, const SFVec3f &v) {
    // v' = q * (0,v) * q^-1, expanded.
    Quat p{v.x, v.y, v.z, 0.0};
    Quat qi{-q.x, -q.y, -q.z, q.w};
    Quat r = quatMul(quatMul(q, p), qi);
    return SFVec3f{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)};
  }

  // NAV-FLY-ROLL: decompose an orientation into yaw (about world up +Y) and
  // pitch (about the yawed camera-local right), such that Rpitch * Ryaw ≈ q.
  // Roll-free by construction: Ryaw is about world-up; Rpitch is about the
  // yawed right (horizontal, no y-component). Used to seed flyYaw_/flyPitch_
  // when entering FLY mode or binding a new viewpoint.
  static void decomposeLookRotation(const Quat &q, float &yaw, float &pitch) {
    SFVec3f f = norm(rotateByQuat(q, SFVec3f{0,0,-1})); // camera forward
    yaw = std::atan2(-f.x, -f.z);                        // angle about world Y
    float fy = f.y;
    if (fy > 1.0f) fy = 1.0f;
    if (fy < -1.0f) fy = -1.0f;
    pitch = std::asin(fy);                               // +pitch looks up
  }

  // Build an orientation (SFRotation, rotation from default -Z look) so that the
  // camera looks along view_dir with +Y up (Gram-Schmidt). §5.3.13.
  static SFRotation lookRotation(const SFVec3f &viewDir, const SFVec3f &upHint) {
    SFVec3f f = norm(viewDir);                 // forward = -Z of camera
    SFVec3f r = cross(f, upHint);              // f x up = +X
    if (len(r) < 1e-5f) r = cross(f, SFVec3f{1,0,0}); // gimbal fallback
    r = norm(r);
    SFVec3f u = norm(cross(r, f));             // reorthogonalize up
    // Camera basis columns: X=r, Y=u, Z=-f (camera looks down -Z).
    Mat4 m = Mat4::identity();
    m.m[0]=r.x;  m.m[1]=r.y;  m.m[2]=r.z;
    m.m[4]=u.x;  m.m[5]=u.y;  m.m[6]=u.z;
    m.m[8]=-f.x; m.m[9]=-f.y; m.m[10]=-f.z;
    return rotationFromQuat(quatFromMat(m));
  }

  // Rotation matrix (upper-left 3x3, column-major) -> unit quaternion (Shepperd).
  static Quat quatFromMat(const Mat4 &m) {
    const float m00=m.m[0], m01=m.m[4], m02=m.m[8];
    const float m10=m.m[1], m11=m.m[5], m12=m.m[9];
    const float m20=m.m[2], m21=m.m[6], m22=m.m[10];
    const float tr = m00 + m11 + m22;
    double x,y,z,w;
    if (tr > 0.0f) {
      float s = std::sqrt(tr + 1.0f) * 2.0f;
      w = 0.25 * s; x = (m21 - m12)/s; y = (m02 - m20)/s; z = (m10 - m01)/s;
    } else if (m00 > m11 && m00 > m22) {
      float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
      w = (m21 - m12)/s; x = 0.25 * s; y = (m01 + m10)/s; z = (m02 + m20)/s;
    } else if (m11 > m22) {
      float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
      w = (m02 - m20)/s; x = (m01 + m10)/s; y = 0.25 * s; z = (m12 + m21)/s;
    } else {
      float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
      w = (m10 - m01)/s; x = (m02 + m20)/s; y = (m12 + m21)/s; z = 0.25 * s;
    }
    double l = std::sqrt(x*x+y*y+z*z+w*w);
    if (l < 1e-12) return Quat{0,0,0,1};
    return Quat{x/l, y/l, z/l, w/l};
  }

  // ---- EXAMINE (orbit/turntable) -------------------------------------------
  // Orbit the eye about centerOfRotation; re-aim the camera at the pivot.
  // up = (0,1,0) in viewpoint-local coords (§23.3.1).
  void examineOrbit(X3DExecutionContext &ctx, X3DNode *vp, float dx, float dy) {
    const SFVec3f C = cor(vp);
    SFVec3f P = effPos(ctx, vp);
    SFVec3f arm = sub(P, C);
    if (len(arm) < 1e-4f) return; // eye at pivot: singular, skip.
    const SFVec3f up{0,1,0};

    Quat Ryaw = axisAngleQuat(up, -dx * kRotScale);
    SFVec3f right = norm(cross(arm, up));
    Quat R = Ryaw;
    // Skip pitch near the pole (arm ~ parallel to up).
    if (std::fabs(dot(norm(arm), up)) < 0.999f && (dy != 0.0f)) {
      Quat Rpitch = axisAngleQuat(right, -dy * kRotScale);
      R = quatMul(Ryaw, Rpitch);
    }
    SFVec3f armNew = rotateByQuat(R, arm);
    SFVec3f Pnew = add(C, armNew);
    SFRotation Onew = lookRotation(sub(C, Pnew), up);

    commitEye(ctx, vp, Pnew, Onew); // accumulate into the offset (not the fields)
  }

  // ---- FLY (free flight) ----------------------------------------------------
  void flyUpdate(X3DExecutionContext &ctx, X3DNode *vp, NavigationInfo *nav,
                 float dx, float dy, bool dragging, double dt) {
    const SFVec3f up{0,1,0};
    Quat q = quatFromRotation(effOri(ctx, vp));
    SFVec3f P = effPos(ctx, vp);
    bool changed = false;

    // NAV-FLY-ROLL: hold orientation as yaw/pitch scalars and reconstruct
    // q = Rpitch * Ryaw each step. Roll-free by construction (Ryaw is about
    // world-up; Rpitch is about the yawed horizontal right). Re-decompose
    // from the effective orientation when entering FLY or binding a new vp.
    if (!flyOrientValid_) {
      decomposeLookRotation(q, flyYaw_, flyPitch_);
      flyOrientValid_ = true;
    }

    // Drag -> accumulate yaw about world-up, pitch about horizontal right.
    if (dragging && (dx != 0.0f || dy != 0.0f)) {
      flyYaw_   += -dx * kRotScale;
      flyPitch_ += -dy * kRotScale;
      if (flyPitch_ >  kPitchLimit) flyPitch_ =  kPitchLimit;
      if (flyPitch_ < -kPitchLimit) flyPitch_ = -kPitchLimit;
      changed = true;
    }

    // Reconstruct the roll-free orientation.
    if (changed) {
      Quat Ryaw = axisAngleQuat(up, flyYaw_);
      SFVec3f right = norm(rotateByQuat(Ryaw, SFVec3f{1,0,0}));
      Quat Rpitch = axisAngleQuat(right, flyPitch_);
      q = quatMul(Rpitch, Ryaw);
    }

    // Keys -> translate along view dir / strafe, scaled by speed*dt (§23.4.4).
    float speed = nav ? nav->getSpeed() : 1.0f;
    const KeyState &ks = ctx.keyState();
    float fwdIn = (ks.isHeld(kKeyForward) ? 1.0f : 0.0f) - (ks.isHeld(kKeyBack) ? 1.0f : 0.0f);
    float strIn = (ks.isHeld(kKeyRight) ? 1.0f : 0.0f) - (ks.isHeld(kKeyLeft) ? 1.0f : 0.0f);
    if ((fwdIn != 0.0f || strIn != 0.0f) && speed > 0.0f && dt > 0.0) {
      SFVec3f fwd = norm(rotateByQuat(q, SFVec3f{0,0,-1}));
      SFVec3f right = norm(rotateByQuat(q, SFVec3f{1,0,0}));
      SFVec3f step = add(mul(fwd, fwdIn), mul(right, strIn));
      P = add(P, mul(step, speed * static_cast<float>(dt)));
      changed = true;
    }
    if (changed) commitEye(ctx, vp, P, rotationFromQuat(q));
  }

  // ---- LOOKAT (frame + animate to a picked object) -------------------------
  void beginLookat(double now, X3DExecutionContext &ctx, X3DNode *vp,
                   NavigationInfo *nav, const PointerState &ps) {
    PickResult pick = ctx.pick(ps.ray);
    if (!pick.hit || !pick.node) { buttonWasDown_ = ps.buttonDown; return; }

    // World bbox of the picked node; center + half-diagonal.
    Aabb wb = ctx.worldBounds(pick.node);
    SFVec3f center{(wb.min.x+wb.max.x)*0.5f, (wb.min.y+wb.max.y)*0.5f, (wb.min.z+wb.max.z)*0.5f};
    SFVec3f half{(wb.max.x-wb.min.x)*0.5f, (wb.max.y-wb.min.y)*0.5f, (wb.max.z-wb.min.z)*0.5f};
    float radius = len(half);
    if (radius < 1e-4f) radius = 1.0f; // degenerate bbox fallback.

    SFVec3f camWorld = ctx.cameraWorldPosition();
    SFVec3f dir = sub(center, camWorld);
    if (len(dir) < 1e-5f) {
      // Camera at center: keep current look direction.
      Quat q = quatFromRotation(effOri(ctx, vp));
      dir = rotateByQuat(q, SFVec3f{0,0,-1});
    }
    dir = norm(dir);

    float fov = geombounds::getField<float>(*vp, "fieldOfView", kPi * 0.25f);
    if (fov <= 0.0f || fov >= kPi) fov = kPi * 0.25f;
    float d = radius / std::tan(fov * 0.5f);
    SFVec3f targetWorld = sub(center, mul(dir, d));

    // World -> viewpoint-local (parent) frame.
    Mat4 parentInv = ctx.worldOf(vp).inverse();
    SFVec3f targetLocal = parentInv.transformPoint(targetWorld);
    SFVec3f centerLocal = parentInv.transformPoint(center);
    // Local look dir from the target toward the center, in local frame.
    SFVec3f dirLocal = norm(sub(centerLocal, targetLocal));
    SFVec3f upLocal = norm(parentInv.transformDirection(SFVec3f{0,1,0}));
    SFRotation targetOri = lookRotation(dirLocal, upLocal);

    // Set centerOfRotation now (§23.4.4: pivot for subsequent EXAMINE).
    ctx.writeField(vp, "centerOfRotation", std::any(SFVec3f{centerLocal}));

    // Begin transition from the CURRENT effective eye (offset-aware).
    lookat_.start = effPos(ctx, vp);
    lookat_.startOri = effOri(ctx, vp);
    lookat_.endPos = targetLocal;
    lookat_.endOri = targetOri;
    lookat_.t0 = now;
    lookat_.duration = resolveTransitionTime(nav);
    lookat_.teleport = isTeleport(nav) || lookat_.duration <= 0.0;
    lookat_.active = true;
    buttonWasDown_ = ps.buttonDown;

    if (lookat_.teleport) advanceLookat(now, ctx, vp, nav); // completes same tick.
  }

  void advanceLookat(double now, X3DExecutionContext &ctx, X3DNode *vp,
                     NavigationInfo *nav) {
    float t = 1.0f;
    if (!lookat_.teleport && lookat_.duration > 0.0)
      t = static_cast<float>((now - lookat_.t0) / lookat_.duration);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    SFVec3f pos{
      lookat_.start.x + (lookat_.endPos.x - lookat_.start.x) * t,
      lookat_.start.y + (lookat_.endPos.y - lookat_.start.y) * t,
      lookat_.start.z + (lookat_.endPos.z - lookat_.start.z) * t};
    SFRotation ori = slerpRotation(lookat_.startOri, lookat_.endOri, t);

    commitEye(ctx, vp, pos, ori); // animate the offset, not the authored fields

    if (t >= 1.0f) {
      lookat_.active = false;
      if (nav) ctx.postEvent(nav, "transitionComplete", std::any(SFBool{true}));
    }
  }

  static double resolveTransitionTime(NavigationInfo *nav) {
    return nav ? nav->getTransitionTime() : 1.0;
  }
  // First recognized transitionType wins; default LINEAR (§23.4.4).
  // transitionType is an open MFString vocabulary — match tokens, skip unknown.
  static bool isTeleport(NavigationInfo *nav) {
    if (!nav) return false;
    for (const std::string &tt : nav->getTransitionType()) {
      if (tt == "TELEPORT") return true;
      if (tt == "LINEAR") return false;
      if (tt == "ANIMATE") return false; // treat as LINEAR (NAV-EXTRA)
    }
    return false;
  }

  // ---- cross-tick state -----------------------------------------------------
  double lastNow_ = 0.0;
  bool haveTime_ = false;
  unsigned long lastPointerRev_ = static_cast<unsigned long>(-1);
  bool dragActive_ = false;
  float lastPx_ = 0.0f, lastPy_ = 0.0f;
  bool buttonWasDown_ = false;

  // Consumer mode override: when set, ignores NavigationInfo.type.
  std::optional<Mode> forcedMode_; // consumer mode override (dev mode-cycle key)

  // NAV-FLY-ROLL: FLY orientation held as yaw/pitch scalars (roll-free).
  // Invalidated on mode switch or viewpoint bind change so the next flyUpdate
  // re-decomposes from the effective orientation.
  Mode lastMode_ = Mode::None;
  X3DNode *lastFlyVp_ = nullptr;
  bool flyOrientValid_ = false;
  float flyYaw_ = 0.0f, flyPitch_ = 0.0f;

  struct LookatState {
    bool active = false;
    bool teleport = false;
    double t0 = 0.0;
    double duration = 1.0;
    SFVec3f start{0,0,0};
    SFVec3f endPos{0,0,0};
    SFRotation startOri{0,0,1,0};
    SFRotation endOri{0,0,1,0};
  } lookat_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_NAVIGATION_SYSTEM_HPP
