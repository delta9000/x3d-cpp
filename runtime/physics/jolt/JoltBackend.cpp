// JoltBackend.cpp — the Jolt Physics implementation of the PhysicsBackend seam.
//
// This is the single translation unit where Jolt and the runtime meet. It is
// compiled with -w (Jolt headers are warning-noisy) and only when
// X3D_CPP_BUILD_PHYSICS=ON. The layer/broadphase filter boilerplate is the
// canonical Jolt HelloWorld pattern (two object layers: NON_MOVING / MOVING),
// and these filter objects must OUTLIVE every JPH::PhysicsSystem, so they are
// owned by the backend Impl (not per-world locals).

#include <mutex>
#include "JoltBackend.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>

JPH_SUPPRESS_WARNINGS

using namespace JPH;
using namespace JPH::literals;

namespace x3d::runtime {
namespace {

// ── Collision layers: a body is either NON_MOVING (static) or MOVING (dynamic).
namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

// ── Broadphase layers (one per object layer is fine for this MVP).
namespace BroadPhaseLayers {
static constexpr BroadPhaseLayer NON_MOVING(0);
static constexpr BroadPhaseLayer MOVING(1);
static constexpr uint NUM_LAYERS(2);
} // namespace BroadPhaseLayers

// Maps each object layer to a broadphase layer.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
  BPLayerInterfaceImpl() {
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
  }
  uint GetNumBroadPhaseLayers() const override {
    return BroadPhaseLayers::NUM_LAYERS;
  }
  BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
    return mObjectToBroadPhase[inLayer];
  }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
    return "Layer";
  }
#endif
private:
  BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// Which object layers collide with which broadphase layers.
class ObjectVsBroadPhaseLayerFilterImpl final
    : public ObjectVsBroadPhaseLayerFilter {
public:
  bool ShouldCollide(ObjectLayer inLayer1,
                     BroadPhaseLayer inLayer2) const override {
    switch (inLayer1) {
    case Layers::NON_MOVING:
      return inLayer2 == BroadPhaseLayers::MOVING;
    case Layers::MOVING:
      return true;
    default:
      return false;
    }
  }
};

// Which object layers collide with which object layers.
class ObjectLayerPairFilterImpl final : public ObjectLayerPairFilter {
public:
  bool ShouldCollide(ObjectLayer inObject1,
                     ObjectLayer inObject2) const override {
    switch (inObject1) {
    case Layers::NON_MOVING:
      return inObject2 == Layers::MOVING; // static collides only with dynamic
    case Layers::MOVING:
      return true; // dynamic collides with everything
    default:
      return false;
    }
  }
};

// Records contacts during PhysicsSystem::Update into a buffer the runtime drains
// after each frame. Single-threaded job system (the deterministic default)
// serialises these callbacks; the mutex keeps it correct under a thread pool too.
class ContactCollector : public ContactListener {
public:
  void OnContactAdded(const Body &b1, const Body &b2,
                      const ContactManifold &manifold,
                      ContactSettings &ioSettings) override {
    applyResponse(ioSettings);
    record(b1, b2, manifold);
  }
  void OnContactPersisted(const Body &b1, const Body &b2,
                          const ContactManifold &manifold,
                          ContactSettings &ioSettings) override {
    applyResponse(ioSettings);
    record(b1, b2, manifold);
  }

  // Map a Jolt BodyID to the runtime's BodyHandle (index+1 into world.bodies).
  // Bound once after the world is in impl_->worlds (see createWorld). This vector
  // is mutated only by addBody at ATTACH time; it is fully populated and
  // thereafter immutable before any step()/Update runs, so the contact callbacks
  // (which fire only during Update) always read a stable vector — safe even under
  // a thread-pool job system without locking handleFor's read.
  const std::vector<BodyID> *bodies = nullptr;

  // Per-contact response override (negative = unset, keep Jolt's default combine).
  // Set by setContactResponse from §37 CollisionCollection.frictionCoefficients/bounce.
  float combinedFriction = -1.0f;
  float combinedRestitution = -1.0f;

  void drain(std::vector<ContactPoint> &out) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Move-assign (not append) — replaces out's contents; the caller need not
    // pre-clear. clear() then normalises the moved-from buffer back to empty.
    out = std::move(buffer_);
    buffer_.clear();
  }

private:
  void applyResponse(ContactSettings &ioSettings) const {
    if (combinedFriction >= 0.0f) ioSettings.mCombinedFriction = combinedFriction;
    if (combinedRestitution >= 0.0f)
      ioSettings.mCombinedRestitution = combinedRestitution;
  }
  BodyHandle handleFor(const BodyID &id) const {
    if (!bodies) return kInvalidBodyHandle;
    for (std::size_t i = 0; i < bodies->size(); ++i)
      if ((*bodies)[i] == id)
        return static_cast<BodyHandle>(i + 1);
    return kInvalidBodyHandle;
  }
  void record(const Body &b1, const Body &b2,
              const ContactManifold &manifold) {
    ContactPoint cp;
    cp.bodyA = handleFor(b1.GetID());
    cp.bodyB = handleFor(b2.GetID());
    if (cp.bodyA == kInvalidBodyHandle || cp.bodyB == kInvalidBodyHandle)
      return;
    // GetWorldSpaceContactPointOn1 is confirmed present in Jolt v5.5:
    // inline RVec3 GetWorldSpaceContactPointOn1(uint inIndex) const
    //   { return mBaseOffset + mRelativeContactPointsOn1[inIndex]; }
    if (manifold.mRelativeContactPointsOn1.empty()) return;
    RVec3 p = manifold.GetWorldSpaceContactPointOn1(0);
    cp.position = SFVec3f{static_cast<float>(p.GetX()),
                          static_cast<float>(p.GetY()),
                          static_cast<float>(p.GetZ())};
    Vec3 n = manifold.mWorldSpaceNormal;
    cp.normal = SFVec3f{n.GetX(), n.GetY(), n.GetZ()};
    cp.depth = manifold.mPenetrationDepth;
    std::lock_guard<std::mutex> lk(mutex_);
    buffer_.push_back(cp);
  }
  std::vector<ContactPoint> buffer_;
  std::mutex mutex_;
};

// Convert an X3D axis-angle SFRotation to a Jolt quaternion.
inline Quat toQuat(const SFRotation &r) {
  float ax = r.x, ay = r.y, az = r.z;
  float len = std::sqrt(ax * ax + ay * ay + az * az);
  if (len < 1e-8f) return Quat::sIdentity();
  ax /= len;
  ay /= len;
  az /= len;
  return Quat::sRotation(Vec3(ax, ay, az), r.angle);
}

// Convert a Jolt quaternion to an X3D axis-angle SFRotation.
inline SFRotation toRotation(const Quat &q) {
  Quat n = q.Normalized();
  Vec3 axis;
  float angle = 0.0f;
  n.GetAxisAngle(axis, angle);
  if (axis.LengthSq() < 1e-12f) return SFRotation{0, 0, 1, 0};
  return SFRotation{axis.GetX(), axis.GetY(), axis.GetZ(), angle};
}

} // namespace

// ── World: one JPH::PhysicsSystem + the bodies created in it.
struct JoltWorld {
  std::unique_ptr<PhysicsSystem> physics;
  std::vector<BodyID> bodies;       // index+1 is the BodyHandle we hand out
  std::vector<Body *> bodyPtrs;     // parallel: the Body* for constraint Create()
  std::vector<Ref<Constraint>> constraints;  // keep joints alive for the world
  std::unique_ptr<ContactCollector> contacts;  // per-world contact buffer
  bool optimized = false;           // broadphase optimized after bulk insertion
};

// ── pImpl: all Jolt-owning state. The filter interfaces are members (they must
//    outlive every JPH::PhysicsSystem). Globals (Factory, types) are init'd once
//    per process here; refcounted so multiple backends don't double-init.
struct JoltBackend::Impl {
  TempAllocatorImpl tempAllocator{10 * 1024 * 1024};
  std::unique_ptr<JobSystem> jobSystem;
  BPLayerInterfaceImpl broadPhaseLayerInterface;
  ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
  ObjectLayerPairFilterImpl objectVsObjectLayerFilter;
  std::unordered_map<WorldHandle, JoltWorld> worlds;
  WorldHandle nextWorld = 1;

  explicit Impl(unsigned workerThreads) {
    if (workerThreads == 0) {
      // Host-independent deterministic default: a single-threaded job system
      // gives identical results regardless of the machine's core count.
      jobSystem = std::make_unique<JobSystemSingleThreaded>(cMaxPhysicsJobs);
    } else {
      // Fixed worker count: faster for large scenes; deterministic only for a
      // fixed thread count (not across differing pools).
      jobSystem = std::make_unique<JobSystemThreadPool>(
          cMaxPhysicsJobs, cMaxPhysicsBarriers,
          static_cast<int>(workerThreads));
    }
  }
};

// ── Process-wide Jolt globals (Factory + type registration), refcounted so
//    constructing/destroying multiple backends is safe.
namespace {
std::mutex g_joltMutex;       // guards the refcount AND the register/unregister blocks:
int g_joltRefCount = 0;       // an atomic alone is insufficient — the init/teardown must be
                              // mutually exclusive so two threads can't both run RegisterTypes.
void acquireJoltGlobals() {
  std::lock_guard<std::mutex> lock(g_joltMutex);
  if (g_joltRefCount++ == 0) {
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();
  }
}
void releaseJoltGlobals() {
  std::lock_guard<std::mutex> lock(g_joltMutex);
  if (--g_joltRefCount == 0) {
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
  }
}
} // namespace

JoltBackend::JoltBackend(unsigned workerThreads) {
  acquireJoltGlobals();
  impl_ = std::make_unique<Impl>(workerThreads);
}

JoltBackend::~JoltBackend() {
  // Remove every constraint, then remove + destroy every body, before the
  // worlds are torn down (constraints reference the bodies they join).
  for (auto &[h, world] : impl_->worlds) {
    if (!world.physics) continue;
    for (Ref<Constraint> &c : world.constraints)
      if (c != nullptr) world.physics->RemoveConstraint(c.GetPtr());
    world.constraints.clear();
    BodyInterface &bi = world.physics->GetBodyInterface();
    for (BodyID id : world.bodies) {
      if (!id.IsInvalid()) {
        bi.RemoveBody(id);
        bi.DestroyBody(id);
      }
    }
  }
  impl_.reset();
  releaseJoltGlobals();
}

// Per-world body / pair / contact capacity. Sized to match the 10 MB temp
// allocator; raising it materially requires a larger allocator too (a tracked
// follow-up). Overflow is reported (addBody warns), never a silent drop.
static constexpr uint kMaxBodiesPerWorld = 1024;

WorldHandle JoltBackend::createWorld(const SFVec3f &gravity) {
  const uint cMaxBodies = kMaxBodiesPerWorld;
  const uint cNumBodyMutexes = 0;
  const uint cMaxBodyPairs = kMaxBodiesPerWorld;
  const uint cMaxContactConstraints = kMaxBodiesPerWorld;

  JoltWorld world;
  world.physics = std::make_unique<PhysicsSystem>();
  world.physics->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                      cMaxContactConstraints, impl_->broadPhaseLayerInterface,
                      impl_->objectVsBroadPhaseLayerFilter,
                      impl_->objectVsObjectLayerFilter);
  world.physics->SetGravity(Vec3(gravity.x, gravity.y, gravity.z));

  world.contacts = std::make_unique<ContactCollector>();
  world.physics->SetContactListener(world.contacts.get());

  WorldHandle h = impl_->nextWorld++;
  auto [it, ok] = impl_->worlds.emplace(h, std::move(world));
  // The collector resolves BodyID -> handle via the world's bodies vector;
  // bind it now that the world (and its bodies vector) lives in the map.
  it->second.contacts->bodies = &it->second.bodies;
  return h;
}

BodyHandle JoltBackend::addBody(WorldHandle world, const ShapeDesc &shape,
                                const MassProperties &mass, bool fixed,
                                const SFVec3f &pos, const SFRotation &ori,
                                const SFVec3f &linVel, const SFVec3f &angVel) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return kInvalidBodyHandle;
  JoltWorld &w = it->second;
  BodyInterface &bi = w.physics->GetBodyInterface();

  // Build the Jolt shape from the ShapeDesc.
  ShapeRefC shapeRef;
  if (shape.kind == ShapeDesc::Kind::Sphere) {
    SphereShapeSettings ss(shape.sphereRadius);
    ss.SetEmbedded();
    ShapeSettings::ShapeResult res = ss.Create();
    if (res.HasError()) return kInvalidBodyHandle;
    shapeRef = res.Get();
  } else if (shape.kind == ShapeDesc::Kind::Cylinder) {
    // Jolt cylinders need a convex radius strictly less than radius/half-height;
    // clamp it so thin/short cylinders still build.
    float cr = std::min(shape.cylinderRadius, shape.cylinderHalfHeight) * 0.5f;
    if (cr > 0.05f) cr = 0.05f;
    CylinderShapeSettings cs(shape.cylinderHalfHeight, shape.cylinderRadius, cr);
    cs.SetEmbedded();
    ShapeSettings::ShapeResult res = cs.Create();
    if (res.HasError()) return kInvalidBodyHandle;
    shapeRef = res.Get();
  } else {
    BoxShapeSettings bs(Vec3(shape.boxHalfExtents.x, shape.boxHalfExtents.y,
                             shape.boxHalfExtents.z));
    bs.SetEmbedded();
    ShapeSettings::ShapeResult res = bs.Create();
    if (res.HasError()) return kInvalidBodyHandle;
    shapeRef = res.Get();
  }

  // §37 centerOfMass: offset the COM from the shape centroid by wrapping the
  // shape. Our primitives are origin-centered (centroid 0), so offset == com.
  if (mass.centerOfMass.x != 0.0f || mass.centerOfMass.y != 0.0f ||
      mass.centerOfMass.z != 0.0f) {
    OffsetCenterOfMassShapeSettings comSettings(
        Vec3(mass.centerOfMass.x, mass.centerOfMass.y, mass.centerOfMass.z),
        shapeRef);
    comSettings.SetEmbedded();
    ShapeSettings::ShapeResult comRes = comSettings.Create();
    if (comRes.HasError()) return kInvalidBodyHandle;
    shapeRef = comRes.Get();
  }

  EMotionType motion = fixed ? EMotionType::Static : EMotionType::Dynamic;
  ObjectLayer layer = fixed ? Layers::NON_MOVING : Layers::MOVING;

  BodyCreationSettings settings(shapeRef, RVec3(pos.x, pos.y, pos.z),
                                toQuat(ori), motion, layer);
  if (!fixed) {
    settings.mLinearVelocity = Vec3(linVel.x, linVel.y, linVel.z);
    settings.mAngularVelocity = Vec3(angVel.x, angVel.y, angVel.z);
    if (mass.mass > 0.0f) {
      settings.mMassPropertiesOverride.mMass = mass.mass;
      if (mass.overrideInertia) {
        // §37 RigidBody.inertia: use the authored 3x3 tensor verbatim. SFMatrix3f
        // is matrix[row][col]; build the Jolt Mat44 column-by-column (symmetric
        // tensor → row/col convention is immaterial).
        const SFMatrix3f &m = mass.inertia;
        Mat44 inertia = Mat44::sZero();
        inertia.SetColumn3(0, Vec3(m.matrix[0][0], m.matrix[1][0], m.matrix[2][0]));
        inertia.SetColumn3(1, Vec3(m.matrix[0][1], m.matrix[1][1], m.matrix[2][1]));
        inertia.SetColumn3(2, Vec3(m.matrix[0][2], m.matrix[1][2], m.matrix[2][2]));
        inertia.SetColumn4(3, Vec4(0, 0, 0, 1));
        settings.mMassPropertiesOverride.mInertia = inertia;
        settings.mOverrideMassProperties =
            EOverrideMassProperties::MassAndInertiaProvided;
      } else {
        settings.mOverrideMassProperties =
            EOverrideMassProperties::CalculateInertia;
      }
    }
  }

  // Create the Body explicitly (returns Body*, which constraints need) and add
  // it. CreateAndAddBody returns only a BodyID; constraints' Create() wants the
  // Body&, so we retain the pointer per handle for addConstraint.
  Body *body = bi.CreateBody(settings);
  if (body == nullptr) {
    // Out of body capacity (cMaxBodies) — report rather than drop silently.
    std::fprintf(stderr,
                 "[JoltBackend] body capacity exhausted (max %u per world); "
                 "body not added\n",
                 kMaxBodiesPerWorld);
    return kInvalidBodyHandle;
  }
  bi.AddBody(body->GetID(),
             fixed ? EActivation::DontActivate : EActivation::Activate);

  w.bodies.push_back(body->GetID());
  w.bodyPtrs.push_back(body);
  return static_cast<BodyHandle>(w.bodies.size()); // 1-based handle
}

// Map a 1-based BodyHandle to its Body* (nullptr if invalid/out of range).
static Body *bodyPtrForHandle(const JoltWorld &w, BodyHandle h) {
  if (h == kInvalidBodyHandle || h > w.bodyPtrs.size()) return nullptr;
  return w.bodyPtrs[static_cast<std::size_t>(h) - 1];
}

// Pick any unit vector perpendicular to `axis` (the angle=0 reference for hinge/
// slider constraints). axis is assumed non-zero; normalized internally.
static Vec3 perpendicularTo(Vec3 axis) {
  if (axis.LengthSq() < 1e-12f) axis = Vec3(0, 1, 0);
  axis = axis.Normalized();
  // Cross with the world axis least parallel to `axis` for numerical stability.
  Vec3 ref = std::abs(axis.GetX()) < 0.9f ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
  Vec3 perp = axis.Cross(ref);
  if (perp.LengthSq() < 1e-12f) perp = axis.Cross(Vec3(0, 0, 1));
  return perp.Normalized();
}

ConstraintHandle JoltBackend::addConstraint(WorldHandle world,
                                            const ConstraintDesc &desc) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return kInvalidConstraintHandle;
  JoltWorld &w = it->second;

  // Resolve bodyA (required) and bodyB (invalid handle -> bind A to the world).
  Body *bodyA = bodyPtrForHandle(w, desc.bodyA);
  if (bodyA == nullptr) return kInvalidConstraintHandle;
  Body *bodyB = bodyPtrForHandle(w, desc.bodyB);
  Body &b2 = bodyB ? *bodyB : Body::sFixedToWorld;

  const RVec3 anchor(desc.anchor.x, desc.anchor.y, desc.anchor.z);
  const Vec3 axis(desc.axis.x, desc.axis.y, desc.axis.z);

  Ref<Constraint> c;
  switch (desc.kind) {
  case ConstraintDesc::Kind::Ball: {
    PointConstraintSettings ps;
    ps.mSpace = EConstraintSpace::WorldSpace;
    ps.mPoint1 = ps.mPoint2 = anchor;
    c = ps.Create(*bodyA, b2);
    break;
  }
  case ConstraintDesc::Kind::Hinge: {
    HingeConstraintSettings hs;
    hs.mSpace = EConstraintSpace::WorldSpace;
    hs.mPoint1 = hs.mPoint2 = anchor;
    Vec3 a = axis.LengthSq() < 1e-12f ? Vec3(0, 1, 0) : axis.Normalized();
    hs.mHingeAxis1 = hs.mHingeAxis2 = a;
    Vec3 n = perpendicularTo(a);
    hs.mNormalAxis1 = hs.mNormalAxis2 = n;
    // §37 SingleAxisHingeJoint limits are measured from the initial pose: a valid
    // hinge has minAngle <= 0 <= maxAngle and travels at most pi each way. Clamp
    // into that domain (out-of-domain input is malformed — the initial pose must
    // be reachable — and is clamped + warned; see CONF-RBP-HINGE-LIMITS).
    float lo = std::clamp(desc.minLimit, -JPH_PI, 0.0f);
    float hi = std::clamp(desc.maxLimit, 0.0f, JPH_PI);
    constexpr float kMinGap = 1e-4f;
    if (hi - lo < kMinGap) {  // degenerate (locked) range — Jolt needs min != max
      lo = std::min(lo, -kMinGap);
      hi = std::max(hi, kMinGap);
    }
    if (lo != desc.minLimit || hi != desc.maxLimit) {
      std::fprintf(stderr,
                   "[JoltBackend] hinge limits [%g, %g] clamped to the §37 valid "
                   "domain [%g, %g] (minAngle must be <= 0, maxAngle >= 0, |angle| "
                   "<= pi; the initial pose must be in range) — see "
                   "CONF-RBP-HINGE-LIMITS\n",
                   desc.minLimit, desc.maxLimit, lo, hi);
    }
    // Jolt's hinge angle is the NEGATIVE of the body's rotation about the axis
    // (theta = -authoredAngle), so negate: authored [lo, hi] -> Jolt [-hi, -lo].
    // (-hi in [-pi,0], -lo in [0,pi] — Jolt's required ranges still hold.)
    hs.mLimitsMin = -hi;
    hs.mLimitsMax = -lo;
    c = hs.Create(*bodyA, b2);
    break;
  }
  case ConstraintDesc::Kind::Slider: {
    SliderConstraintSettings ss;
    ss.mSpace = EConstraintSpace::WorldSpace;
    ss.mPoint1 = ss.mPoint2 = anchor;
    Vec3 a = axis.LengthSq() < 1e-12f ? Vec3(0, 1, 0) : axis.Normalized();
    ss.mSliderAxis1 = ss.mSliderAxis2 = a;
    ss.mNormalAxis1 = ss.mNormalAxis2 = perpendicularTo(a);
    ss.mLimitsMin = desc.minLimit;
    ss.mLimitsMax = desc.maxLimit;
    c = ss.Create(*bodyA, b2);
    break;
  }
  default:
    return kInvalidConstraintHandle;
  }

  if (c == nullptr) return kInvalidConstraintHandle;
  w.physics->AddConstraint(c.GetPtr());
  w.constraints.push_back(c);  // keep the Ref alive for the world's lifetime
  return static_cast<ConstraintHandle>(w.constraints.size());  // 1-based handle
}

void JoltBackend::applyForce(WorldHandle world, BodyHandle body,
                             const SFVec3f &force, const SFVec3f &torque) {
  // Skip a pure no-op so a resting body isn't needlessly re-activated each frame.
  if (force.x == 0 && force.y == 0 && force.z == 0 && torque.x == 0 &&
      torque.y == 0 && torque.z == 0)
    return;
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return;
  JoltWorld &w = it->second;
  if (body == kInvalidBodyHandle || body > w.bodies.size()) return;
  BodyID id = w.bodies[static_cast<std::size_t>(body) - 1];
  if (id.IsInvalid()) return;
  // AddForce/AddTorque accumulate for the next Update and Jolt clears them after;
  // the PhysicsSystem re-applies every step for §37's continuous-force semantics.
  BodyInterface &bi = w.physics->GetBodyInterface();
  bi.AddForceAndTorque(id, Vec3(force.x, force.y, force.z),
                       Vec3(torque.x, torque.y, torque.z), EActivation::Activate);
}

void JoltBackend::setGravityFactor(WorldHandle world, BodyHandle body,
                                   float factor) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return;
  JoltWorld &w = it->second;
  if (body == kInvalidBodyHandle || body > w.bodies.size()) return;
  BodyID id = w.bodies[static_cast<std::size_t>(body) - 1];
  if (id.IsInvalid()) return;
  w.physics->GetBodyInterface().SetGravityFactor(id, factor);
}

void JoltBackend::setContactResponse(WorldHandle world, float friction,
                                     float restitution) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end() || !it->second.contacts) return;
  it->second.contacts->combinedFriction = friction;
  it->second.contacts->combinedRestitution = restitution;
}

void JoltBackend::getBodyVelocity(WorldHandle world, BodyHandle body,
                                  SFVec3f &lin, SFVec3f &ang) const {
  lin = SFVec3f{0, 0, 0};
  ang = SFVec3f{0, 0, 0};
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return;
  const JoltWorld &w = it->second;
  if (body == kInvalidBodyHandle || body > w.bodies.size()) return;
  BodyID id = w.bodies[static_cast<std::size_t>(body) - 1];
  if (id.IsInvalid()) return;
  const BodyInterface &bi = w.physics->GetBodyInterface();
  Vec3 lv = bi.GetLinearVelocity(id);
  Vec3 av = bi.GetAngularVelocity(id);
  lin = SFVec3f{lv.GetX(), lv.GetY(), lv.GetZ()};
  ang = SFVec3f{av.GetX(), av.GetY(), av.GetZ()};
}

void JoltBackend::step(WorldHandle world, double dt) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return;
  JoltWorld &w = it->second;
  // Optimize the broadphase once, after the bodies for this world are inserted
  // (Jolt requires this exactly once after bulk insertion for correct results).
  if (!w.optimized) {
    w.physics->OptimizeBroadPhase();
    w.optimized = true;
  }
  w.physics->Update(static_cast<float>(dt), 1, &impl_->tempAllocator,
                    impl_->jobSystem.get());
}

void JoltBackend::drainContacts(WorldHandle world,
                                std::vector<ContactPoint> &out) {
  out.clear();
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end() || !it->second.contacts) return;
  it->second.contacts->drain(out);
}

void JoltBackend::getBodyTransform(WorldHandle world, BodyHandle body,
                                   SFVec3f &pos, SFRotation &ori) const {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end()) return;
  const JoltWorld &w = it->second;
  if (body == kInvalidBodyHandle || body > w.bodies.size()) return;
  BodyID id = w.bodies[static_cast<std::size_t>(body) - 1];
  if (id.IsInvalid()) return;
  const BodyInterface &bi = w.physics->GetBodyInterface();
  // §37 RigidBody.position is the body's origin (the authored reference point),
  // NOT its center of mass — these differ when centerOfMass is offset. Report
  // the origin so a routed Transform tracks the authored placement. For centered
  // bodies origin == COM, so existing golden traces are unchanged.
  RVec3 p = bi.GetPosition(id);
  Quat q = bi.GetRotation(id);
  pos = SFVec3f{static_cast<float>(p.GetX()), static_cast<float>(p.GetY()),
                static_cast<float>(p.GetZ())};
  ori = toRotation(q);
}

} // namespace x3d::runtime
