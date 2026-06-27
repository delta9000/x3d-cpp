// X3DEventCascade.hpp
// The event cascade engine: propagates field events along an EventGraph in a
// single timestamp with fan-out and per-route loop-breaking.
#ifndef X3D_RUNTIME_EVENT_CASCADE_HPP
#define X3D_RUNTIME_EVENT_CASCADE_HPP

#include "X3DEventGraph.hpp"
#include "X3DFieldAddress.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <deque>
#include <functional>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives a single-timestamp event cascade over an EventGraph.
 * @details `postEvent` seeds one or more initial field events; `process`
 *          delivers each event to its target field and fans it out along the
 *          ROUTEs, enqueuing the downstream events. X3D single-timestamp
 *          semantics are enforced by TWO independent guards (ISO 19775-1
 *          §4.4.8.3):
 *            - the per-ROUTE-edge guard: each ROUTE edge fires at most once per
 *              timestamp, which bounds fan-out and breaks cyclic routes;
 *            - the per-FIELD cap (RTC-5): a given `(node, field)` is produced at
 *              most once per timestamp, so fan-in (two ROUTEs into one field)
 *              delivers ONCE and a node re-emitting on input cannot re-drive a
 *              loop. The per-edge guard alone does not catch fan-in (the edges
 *              have distinct sources), hence the distinct node/output-layer cap.
 *          The per-field cap applies to ROUTED (fan-out) deliveries only: a
 *          routed event whose target field was already produced this timestamp
 *          is dropped. SEED events — those a System/handler posts directly via
 *          `postEvent` — always deliver (last-writer-wins), so a node that emits
 *          a deliberate sequence on its own output within one update (e.g. a
 *          TimeSensor that activates then immediately completes, isActive
 *          TRUE→FALSE) lands its final value. Seeds still MARK the field
 *          produced, so a route that would re-drive that field is still broken.
 *          The per-field cap also bounds the §4.4.8.3 step-4 re-evaluation loop
 *          that X3DExecutionContext::tick runs (guaranteeing termination), so
 *          the cap must span an entire timestamp — which may comprise several
 *          `process()` calls interleaved with System re-evaluation. Callers pass
 *          `freshTimestamp=false` to continue the current timestamp; the default
 *          `true` starts a new one (the standalone one-shot drain).
 */
class EventCascade {
public:
  explicit EventCascade(const EventGraph &graph) : graph_(graph) {}

  /**
   * @brief Seed an initial event: deliver `value` to (node, field) and cascade.
   * @details The event is queued; call `process()` to run the cascade.
   */
  void postEvent(x3d::nodes::X3DNode *node, const std::string &field, std::any value) {
    // A direct post is a SEED (routed=false): it always delivers (last-wins) and
    // is exempt from the per-field cap. Fan-out within process() enqueues ROUTED
    // deliveries, which the cap can drop.
    pending_.push_back(
        Delivery{FieldAddress{node, field}, std::move(value), /*routed=*/false});
  }

  /**
   * @brief Begin a new timestamp: clear the per-field/per-edge produced guards.
   * @details A "timestamp" is one logical instant of event processing, which may
   *          span several `process()` calls (the §4.4.8.3 step-4 re-evaluation
   *          loop alternates System updates with cascade drains). The per-field
   *          cap (RTC-5) and per-edge guard must persist across those drains, so
   *          they are reset here — once per timestamp — not inside `process()`.
   *          `process(freshTimestamp=true)` (the default) calls this for the
   *          common standalone one-shot drain.
   */
  void beginTimestamp() {
    fired_.clear();
    produced_.clear();
  }

  /**
   * @brief Run the cascade to quiescence within the current timestamp.
   * @details Breadth-first: deliver each queued event to its target field, then
   *          fan it out along every ROUTE leaving that target, enqueuing the
   *          downstream events. Two guards bound the work and enforce X3D
   *          single-timestamp semantics (§4.4.8.3):
   *            - a ROUTE edge that has already fired this timestamp is skipped
   *              (loop-breaking / fan-out bound);
   *            - a ROUTED delivery whose `(node, field)` was already produced
   *              this timestamp is dropped (RTC-5 per-field cap) — fan-in
   *              delivers once and a re-emitting node cannot re-drive a loop.
   *              SEED deliveries (direct `postEvent`) are exempt (last-wins) but
   *              still mark the field produced, so they break routed re-drives.
   * @param freshTimestamp When true (default) start a new timestamp first
   *        (`beginTimestamp`); pass false to continue the current timestamp,
   *        which the tick re-evaluation loop does so the per-field cap spans the
   *        whole tick.
   * @return The number of NEW fields produced this call (first-time productions,
   *         seed or routed). The tick re-eval loop uses this to detect
   *         quiescence; the per-field cap guarantees it reaches zero (a field
   *         re-posted every pass is no longer "new", so the count drops to 0).
   */
  std::size_t process(bool freshTimestamp = true) {
    if (freshTimestamp) {
      beginTimestamp();
    }
    std::size_t newProductions = 0;
    while (!pending_.empty()) {
      Delivery d = std::move(pending_.front());
      pending_.pop_front();

      // Normalize field aliases before cap-check + delivery so that a SEED
      // posted as `set_translation` and a ROUTE to `translation` share the
      // same per-field entry. §4.4.2.2 alias resolution.
      FieldAddress norm{d.target.node,
                        resolveFieldAlias(d.target.node, d.target.field)};

      const bool firstProduction = produced_.insert(norm).second;

      // RTC-5 per-field cap: drop a ROUTED delivery whose field was already
      // produced this timestamp. This is the loop-break for fan-in (two ROUTEs
      // into one field) and cyclic re-drive (a node re-emitting on input) that
      // the per-edge `fired_` guard cannot see — distinct edges target the same
      // field. A SEED (direct post) is exempt: a System may emit a deliberate
      // last-wins sequence on its own output within one update. §4.4.8.3.
      if (d.routed && !firstProduction) {
        continue;
      }

      // Snapshot the sink list (copy) BEFORE delivering. Delivery can invoke a
      // handler that mutates graph_ (e.g. ctx.addRoute/removeRoute, X3D
      // §4.3.7); holding a reference into graph_ internals across that call
      // would be iterator/reference-invalidation UB. Copying also gives the
      // correct single-timestamp semantics: a route added mid-cascade is not
      // delivered until the NEXT cascade.
      const std::vector<FieldAddress> sinks = graph_.sinks(norm);

      deliver(norm, d.value);
      if (firstProduction) {
        ++newProductions; // only first-time productions count toward quiescence
      }

      for (const auto &sink : sinks) {
        RouteEdge edge{norm, sink};
        if (fired_.insert(edge).second) {
          // Fan-out events are ROUTED: subject to the per-field cap downstream.
          pending_.push_back(Delivery{sink, d.value, /*routed=*/true});
        }
      }
    }
    return newProductions;
  }

  /// Register a callback invoked after each field is successfully delivered.
  /// Used by the runtime to feed the dirty-tracking layer; null by default.
  void setFieldObserver(std::function<void(const FieldAddress &)> obs) {
    observer_ = std::move(obs);
  }

private:
  struct Delivery {
    FieldAddress target;
    std::any value;
    bool routed;  // false = seed (direct post); true = fan-out along a ROUTE
  };

  struct RouteEdge {
    FieldAddress from;
    FieldAddress to;
    bool operator==(const RouteEdge &o) const {
      return from == o.from && to == o.to;
    }
  };

  struct RouteEdgeHash {
    std::size_t operator()(const RouteEdge &e) const noexcept {
      std::size_t h1 = std::hash<FieldAddress>{}(e.from);
      std::size_t h2 = std::hash<FieldAddress>{}(e.to);
      return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
  };

  // Deliver a value to one field endpoint via the node's reflection table, then
  // notify the field observer (the dirty-tracking feed). Fields with no writable
  // thunk are ignored; a ROUTE to an undeliverable field is a no-op.
  // See also X3DExecutionContext::writeField for the System-side equivalent.
  void deliver(const FieldAddress &addr, const std::any &value) {
    if (!addr.node) {
      return;
    }
    for (const auto &info : addr.node->fields()) {
      if (info.x3dName == addr.field) {
        if (info.set) {
          info.set(*addr.node, value);
          if (observer_) observer_(addr);
        }
        return;
      }
    }
  }

  const EventGraph &graph_;
  std::deque<Delivery> pending_;
  std::function<void(const FieldAddress &)> observer_;
  // Per-timestamp guards (reset by beginTimestamp). They persist across the
  // several process() calls one tick may make (the §4.4.8.3 step-4 re-eval
  // loop), so the per-field cap bounds the whole tick — not just one drain.
  std::unordered_set<RouteEdge, RouteEdgeHash> fired_;  // per-ROUTE-edge guard
  std::unordered_set<FieldAddress> produced_;           // RTC-5 per-field cap
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_EVENT_CASCADE_HPP
