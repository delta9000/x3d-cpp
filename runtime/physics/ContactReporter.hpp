// ContactReporter.hpp
// Translates already-resolved §37 collision contacts into X3D events on the
// watching CollisionSensor nodes. ENGINE-AGNOSTIC and HANDLE-FREE: it names no
// physics-engine type and no backend handle — the PhysicsSystem resolves each
// drained ContactPoint to scene nodes (RigidBody + CollidableShape) and a stable
// pair ordinal before handing a vector of ResolvedContact here. This file builds
// without X3D_CPP_BUILD_PHYSICS. A seam-purity check greps it for engine leakage.
//
// §37.4.5: the collision system runs regardless of any CollisionSensor; the
// sensor only REPORTS. We never feed contacts back into the solver (the spec
// strongly advises against routing contacts -> set_contacts). This is pure
// read-only output: build Contact nodes, emit contacts/intersections/isActive.
#ifndef X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP
#define X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/CollisionCollection.hpp"
#include "x3d/nodes/CollisionSensor.hpp"
#include "x3d/nodes/CollisionSpace.hpp"
#include "x3d/nodes/Contact.hpp"
#include "x3d/nodes/RigidBody.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

/**
 * @brief One contact resolved to scene nodes, ready to report.
 * @details The PhysicsSystem fills this from a backend ContactPoint: handles ->
 *          the RigidBody + CollidableShape SFNodes, plus a STABLE pairKey built
 *          from enrollment ordinals (min<<32 | max) so dedupe + ordering are
 *          deterministic regardless of pointer addresses or engine callback order.
 */
struct ResolvedContact {
  SFNode body1;
  SFNode body2;
  SFNode geometry1;
  SFNode geometry2;
  SFVec3f position{0, 0, 0};
  SFVec3f normal{0, 1, 0};
  float depth = 0.0f;
  std::uint64_t pairKey = 0;
};

/**
 * @brief Emits §37 CollisionSensor outputs from resolved contacts.
 * @details Per frame, report() is called once with all resolved contacts. For
 *          each enrolled sensor it: gates on sensor/collection enabled + a
 *          non-null collider; flattens collider->collidables (descending nested
 *          CollisionSpaces) into a CollidableShape set; keeps contacts whose both
 *          geometries are in that set; dedupes per body-pair (max depth) and
 *          sorts by pairKey; builds Contact nodes (geometry from the engine,
 *          response fields from the collection defaults); and posts contacts /
 *          intersections / isActive (isActive only on change).
 */
class ContactReporter {
public:
  /** @brief Enroll a CollisionSensor to watch (idempotent per pointer). */
  void addSensor(x3d::nodes::CollisionSensor *sensor) {
    if (sensor &&
        std::find(sensors_.begin(), sensors_.end(), sensor) == sensors_.end())
      sensors_.push_back(sensor);
  }

  /** @brief Build + emit this frame's contact outputs for every sensor. */
  void report(const std::vector<ResolvedContact> &contacts,
              X3DExecutionContext &ctx) {
    for (x3d::nodes::CollisionSensor *sensor : sensors_) {
      reportSensor(sensor, contacts, ctx);
    }
  }

private:
  void reportSensor(x3d::nodes::CollisionSensor *sensor,
                    const std::vector<ResolvedContact> &contacts,
                    X3DExecutionContext &ctx) {
    // Gate: a disabled sensor / null collider / disabled collection reports
    // nothing this frame (§37.4.4/.4.5). Drop isActive on the active->inactive
    // transition so a downstream route sees the sensor go quiet.
    auto *collection =
        sensor->getEnabled()
            ? dynamic_cast<x3d::nodes::CollisionCollection *>(sensor->getCollider().get())
            : nullptr;
    if (!collection || !collection->getEnabled()) {
      setActive(sensor, false, ctx);
      return;
    }

    std::unordered_set<const X3DNode *> watched;
    collectCollidables(collection->getCollidables(), watched);
    visitedSpaces_.clear();  // reset the per-sensor recursion guard

    // Filter to contacts whose BOTH geometries are in the watched set, deduped
    // per pairKey keeping the deepest, then ordered by pairKey (deterministic).
    std::unordered_map<std::uint64_t, const ResolvedContact *> byPair;
    for (const ResolvedContact &c : contacts) {
      if (!watched.count(c.geometry1.get()) ||
          !watched.count(c.geometry2.get()))
        continue;
      auto it = byPair.find(c.pairKey);
      if (it == byPair.end() || c.depth > it->second->depth)
        byPair[c.pairKey] = &c;
    }
    if (byPair.empty()) {
      setActive(sensor, false, ctx);
      return;
    }

    std::vector<const ResolvedContact *> ordered;
    ordered.reserve(byPair.size());
    for (auto &kv : byPair) ordered.push_back(kv.second);
    std::sort(ordered.begin(), ordered.end(),
              [](const ResolvedContact *a, const ResolvedContact *b) {
                return a->pairKey < b->pairKey;
              });

    MFNode out;
    MFNode intersections;
    std::unordered_set<const X3DNode *> seenGeom;
    out.reserve(ordered.size());
    for (const ResolvedContact *c : ordered) {
      out.push_back(makeContact(*c, *collection));
      addUnique(intersections, seenGeom, c->geometry1);
      addUnique(intersections, seenGeom, c->geometry2);
    }

    ctx.postEvent(sensor, "contacts", std::any(out));
    ctx.postEvent(sensor, "intersections", std::any(intersections));
    setActive(sensor, true, ctx);
  }

  /// Flatten collidables into a CollidableShape set, descending nested
  /// CollisionSpaces (visited-guard against shared/cyclic spaces).
  void collectCollidables(const MFNode &nodes,
                          std::unordered_set<const X3DNode *> &out) {
    for (const SFNode &n : nodes) {
      if (!n) continue;
      if (auto *shape = dynamic_cast<x3d::nodes::CollidableShape *>(n.get())) {
        out.insert(shape);
      } else if (auto *space = dynamic_cast<x3d::nodes::CollisionSpace *>(n.get())) {
        if (visitedSpaces_.insert(space).second)
          collectCollidables(space->getCollidables(), out);
      }
    }
  }

  static void addUnique(MFNode &out, std::unordered_set<const X3DNode *> &seen,
                        const SFNode &n) {
    if (n && seen.insert(n.get()).second) out.push_back(n);
  }

  /// Build one Contact: geometry from the engine, response from the collection.
  static SFNode makeContact(const ResolvedContact &c,
                            const x3d::nodes::CollisionCollection &col) {
    auto contact = std::make_shared<x3d::nodes::Contact>();
    contact->setPosition(c.position);
    contact->setContactNormal(c.normal);
    contact->setDepth(c.depth);
    contact->setBody1(c.body1);
    contact->setBody2(c.body2);
    contact->setGeometry1(c.geometry1);
    contact->setGeometry2(c.geometry2);
    // Response defaults assigned from the generating CollisionCollection
    // (§37.4.4): all generated contacts inherit the collection's parameters.
    // Use the *Unchecked setters for range-checked fields — the values were
    // already validated on the collection (or kept by a permissive reader), and
    // throwing mid-simulation would be wrong.
    contact->setBounceUnchecked(col.getBounce());
    contact->setMinBounceSpeedUnchecked(col.getMinBounceSpeed());
    contact->setFrictionCoefficientsUnchecked(col.getFrictionCoefficients());
    contact->setSurfaceSpeed(col.getSurfaceSpeed());            // unchecked field
    contact->setSlipCoefficients(col.getSlipFactors());         // unchecked field
    contact->setSoftnessConstantForceMixUnchecked(col.getSoftnessConstantForceMix());
    contact->setSoftnessErrorCorrectionUnchecked(col.getSoftnessErrorCorrection());
    contact->setAppliedParameters(col.getAppliedParameters());
    // frictionDirection left at default — the system computes it unless
    // USER_FRICTION is set (spec-correct to omit here).
    return contact;
  }

  /// Emit isActive only when it changes (per-sensor previous state).
  void setActive(x3d::nodes::CollisionSensor *sensor, bool active,
                 X3DExecutionContext &ctx) {
    bool &prev = active_[sensor];  // value-initialises to false on first lookup
    if (prev == active) return;
    prev = active;
    ctx.postEvent(sensor, "isActive", std::any(SFBool(active)));
  }

  std::vector<x3d::nodes::CollisionSensor *> sensors_;
  std::unordered_map<x3d::nodes::CollisionSensor *, bool> active_;
  std::unordered_set<const X3DNode *> visitedSpaces_;  // scratch for flatten
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP
