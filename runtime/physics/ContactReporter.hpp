// §37.4.5: CollisionSensor reports resolved contacts without feeding them back
// into the solver. PhysicsSystem resolves backend handles to scene nodes.
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
 *          the RigidBody + CollidableShape SFNodes, plus a stable pairKey built
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

/** §37 CollisionSensor outputs are deduplicated per body pair by maximum depth
 *  and sorted by pairKey. isActive is emitted only on transitions. */
class ContactReporter {
public:
  /** @brief Enroll a CollisionSensor to watch (idempotent per pointer). */
  void addSensor(x3d::nodes::CollisionSensor *sensor) {
    if (sensor &&
        std::find(sensors_.begin(), sensors_.end(), sensor) == sensors_.end())
      sensors_.push_back(sensor);
  }

  void removeSensor(x3d::nodes::CollisionSensor *sensor) {
    sensors_.erase(std::remove(sensors_.begin(), sensors_.end(), sensor), sensors_.end());
    active_.erase(sensor);
  }

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

    // Keep contacts with both geometries in the watched set, deduped
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
