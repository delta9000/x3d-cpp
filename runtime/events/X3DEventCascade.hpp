// X3DEventCascade.hpp
// The event cascade engine: propagates field events along an EventGraph in a
// single timestamp with fan-out and per-route loop-breaking.
#ifndef X3D_RUNTIME_EVENT_CASCADE_HPP
#define X3D_RUNTIME_EVENT_CASCADE_HPP

#include "X3DEventGraph.hpp"
#include "X3DFieldAddress.hpp"
#include "DynamicField.hpp"   // author-field (Script) delivery fallback
#include "x3d/nodes/X3DNode.hpp"

#include <algorithm>
#include <any>
#include <deque>
#include <functional>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives a single logical timestamp over an EventGraph.
 * @details ISO/IEC 19775-1 §4.4.8.3 limits OUTPUT events and ROUTEs, not
 *          inputOnly delivery occurrences. Each ROUTE fires at most once.
 *          postOutputEvent queues a generated outputOnly value; first-admitted
 *          wins before field storage, observers or ROUTEs can see that value.
 *          All inputOnly arrivals (including fan-in and equal repeats) reach
 *          their handlers. First-admitted is our selection policy, not a
 *          uniquely required ISO ordering of simultaneous events.
 *
 *          postEvent remains a legacy direct-seed API: every seed delivers.
 *          Unmigrated Systems using it for output are NOT output-capped. Routed
 *          non-inputOnly destinations retain their legacy per-field cap. This
 *          is intentionally not a claim of complete event-model conformance.
 *
 *          Guards span every process(false) drain in one timestamp. A fresh
 *          process() or beginTimestamp() opens a new logical timestamp.
 */
class EventCascade {
public:
  explicit EventCascade(const EventGraph &graph,
      std::shared_ptr<DynamicFieldStore> fields = std::make_shared<DynamicFieldStore>())
      : graph_(graph), authorFields_(std::move(fields)) {
    if (!authorFields_) throw std::invalid_argument("null author-field owner");
  }

  DynamicFieldStore &authorFields() const { return *authorFields_; }

  /// Queue external input or a legacy direct seed. Every accepted occurrence
  /// delivers; generated outputOnly producers should use postOutputEvent.
  void postEvent(x3d::nodes::X3DNode *node, const std::string &field, std::any value) {
    pending_.push_back(
        Delivery{FieldAddress{node, field}, std::move(value), Origin::Seed});
  }

  /// Queue a generated outputOnly value WITHOUT first mutating node storage.
  /// Validates the writable endpoint now; admission happens when drained. Only
  /// the first admitted value in this timestamp reaches the setter, observer,
  /// or ROUTEs. No synchronous emission or acceptance is implied by queueing.
  /// inputOutput needs a distinct input/output-side contract and is not covered
  /// by this deliberately scoped primitive. Unknown/unwritable/non-outputOnly
  /// endpoints throw invalid_argument; value types follow postEvent's contract.
  void postOutputEvent(X3DNode *node, const std::string &field, std::any value) {
    if (node) {
      for (const auto &info : effectiveFields(*node, *authorFields_)) {
        if (info.x3dName != field) continue;
        if (info.access == AccessType::OutputOnly && info.set) {
          pending_.push_back(
              Delivery{FieldAddress{node, field}, std::move(value), Origin::Output});
          return;
        }
        break;
      }
    }
    throw std::invalid_argument("postOutputEvent requires a writable outputOnly field");
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

  void removeNodes(const std::unordered_set<const X3DNode *> &nodes) {
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(), [&](const Delivery &d) {
      return nodes.count(d.target.node) != 0;
    }), pending_.end());
    for (auto it = produced_.begin(); it != produced_.end();)
      if (nodes.count(it->node)) it = produced_.erase(it); else ++it;
    for (auto it = fired_.begin(); it != fired_.end();)
      if (nodes.count(it->from.node) || nodes.count(it->to.node)) it = fired_.erase(it);
      else ++it;
  }

  /**
   * @brief Drain breadth-first; admit generated output before field mutation.
   * @param freshTimestamp True opens a timestamp for an outermost drain; nested
   *        drains always continue it. False preserves guards across the enclosing
   *        timestamp's System/cascade passes.
   * @return Number of newly reached field endpoints, used for quiescence.
   *         Input occurrences are not deduplicated by this bookkeeping set.
   */
  std::size_t process(bool freshTimestamp = true) {
    // A callback may request a nested drain. It joins the active cascade;
    // only an outermost fresh drain may reset timestamp guards.
    if (freshTimestamp && processDepth_ == 0) beginTimestamp();
    struct DrainGuard {
      std::size_t &depth;
      explicit DrainGuard(std::size_t &value) : depth(value) { ++depth; }
      ~DrainGuard() { --depth; }
    } drain{processDepth_};
    std::size_t newProductions = 0;
    while (!pending_.empty()) {
      Delivery d = std::move(pending_.front());
      pending_.pop_front();
      // An empty value is a re-read request (editChildren): deliver the
      // field's value as it stands now.
      if (!d.value.has_value() && d.target.node) {
        for (const auto &info : d.target.node->fields())
          if (info.x3dName == d.target.field && info.get) d.value = info.get(*d.target.node);
        if (!d.value.has_value()) continue;
      }

      // Normalize field aliases before cap-check + delivery so that a SEED
      // posted as `set_translation` and a ROUTE to `translation` share the
      // same per-field entry. §4.4.2.2 alias resolution.
      FieldAddress norm{d.target.node,
                        resolveFieldAlias(d.target.node, d.target.field)};

      if (d.origin != Origin::Output && !acceptsInput(norm, d.value)) continue;

      const bool firstProduction = produced_.insert(norm).second;

      // Admit the selected output BEFORE reflection mutates its backing field.
      // A suppressed value must never leak into readback or observers. Keep
      // the legacy cap for routed value-bearing destinations, but an inputOnly
      // arrival is an occurrence, not production of an output field (§4.4.8.5).
      if (!firstProduction &&
          (d.origin == Origin::Output ||
           (d.origin == Origin::Route && !isInputOnly(norm)))) {
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
          pending_.push_back(Delivery{sink, d.value, Origin::Route});
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

  void addInputFilter(std::function<bool(const FieldAddress &, const std::any &)> filter) {
    inputFilters_.push_back(std::move(filter));
  }

  bool acceptsInput(const FieldAddress &addr, const std::any &value) const {
    for (const auto &filter : inputFilters_)
      if (!filter(addr, value)) return false;
    return true;
  }

  /// Called when an event is delivered to an author-declared inputOnly or
  /// inputOutput field (e.g. a Script's eventIn, ISO/IEC 19775-1 §29.2): the
  /// target, the field's reflection entry, and the value. The script layer
  /// registers here so a ROUTEd event runs the script's handler (inputOnly) or
  /// updates the script's view of the field (inputOutput) in the same cascade.
  /// The
  /// listener may post events (they join this timestamp) and add or remove
  /// ROUTEs (process() snapshots sinks before delivering).
  using AuthorInputListener = std::function<void(
      const FieldAddress &, const FieldInfo &, const std::any &)>;
  void addAuthorInputListener(AuthorInputListener listener) {
    authorInputListeners_.push_back(std::move(listener));
  }

private:
  enum class Origin { Seed, Output, Route };

  struct Delivery {
    FieldAddress target;
    std::any value;
    Origin origin;
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

  bool isInputOnly(const FieldAddress &addr) const {
    if (!addr.node) return false;
    for (const auto &info : effectiveFields(*addr.node, *authorFields_))
      if (info.x3dName == addr.field) return info.access == AccessType::InputOnly;
    return false;
  }

  // Deliver a value to one field endpoint via the node's reflection table, then
  // notify the field observer (the dirty-tracking feed). Fields with no writable
  // thunk are ignored; a ROUTE to an undeliverable field is a no-op.
  // See also X3DExecutionContext::writeField for the System-side equivalent.
  void deliver(const FieldAddress &addr, const std::any &value) {
    if (!addr.node) {
      return;
    }
    if (addr.field == "addChildren" || addr.field == "removeChildren") {
      if (editChildren(addr, value)) return;
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
    // Author fields (e.g. a Script node's <field> children) are not in the static
    // fields() table — they live in the per-node dynamic-field store. Fall back to
    // it so ROUTEs into a Script's inputOnly/inputOutput fields actually deliver
    // (the embedder's ScriptSystem then picks the value up post-cascade).
    for (FieldInfo &info : authorFields_->authorFields(*addr.node)) {
      if (info.x3dName == addr.field) {
        if (info.set) {
          info.set(*addr.node, value);
          if (observer_) observer_(addr);
          if (info.access == AccessType::InputOnly ||
              info.access == AccessType::InputOutput)
            for (const auto &listener : authorInputListeners_)
              listener(addr, info, value);
        }
        return;
      }
    }
  }

  /// X3D §10.2.1: addChildren appends nodes not already among the children;
  /// removeChildren removes the listed nodes, ignoring absent ones. The edit is
  /// applied immediately and a `children` event carrying the then-current
  /// value follows in this cascade, so the field is marked dirty and ROUTEs
  /// from children_changed fire. Returns false when the node has no writable
  /// MFNode `children` field.
  bool editChildren(const FieldAddress &addr, const std::any &value) {
    using Nodes = std::vector<std::shared_ptr<X3DNode>>;
    const Nodes *delta = std::any_cast<Nodes>(&value);
    if (!delta) return false;
    for (const auto &info : addr.node->fields()) {
      if (info.x3dName != "children" || info.type != X3DFieldType::MFNode ||
          !info.get || !info.set)
        continue;
      Nodes children = std::any_cast<Nodes>(info.get(*addr.node));
      if (addr.field == "addChildren") {
        for (const auto &n : *delta)
          if (n && std::find(children.begin(), children.end(), n) == children.end())
            children.push_back(n);
      } else {
        std::erase_if(children, [&](const auto &c) {
          return std::find(delta->begin(), delta->end(), c) != delta->end();
        });
      }
      // Apply now so a later add/remove in this cascade sees this result, then
      // queue a re-read of the final value for dirty tracking and fan-out.
      info.set(*addr.node, std::any(std::move(children)));
      pending_.push_back(
          Delivery{FieldAddress{addr.node, "children"}, std::any{}, Origin::Seed});
      return true;
    }
    return false;
  }

  friend class X3DExecutionContext;
  void bindAuthorFields(const std::shared_ptr<DynamicFieldStore> &fields) {
    authorFields_ = fields;
  }
  const EventGraph &graph_;
  std::shared_ptr<DynamicFieldStore> authorFields_;
  std::deque<Delivery> pending_;
  std::size_t processDepth_ = 0;  // nested drains share the active timestamp
  std::function<void(const FieldAddress &)> observer_;
  std::vector<std::function<bool(const FieldAddress &, const std::any &)>> inputFilters_;
  std::vector<AuthorInputListener> authorInputListeners_;
  // Per-timestamp guards (reset by beginTimestamp). They persist across the
  // several process() calls one tick may make (the §4.4.8.3 step-4 re-eval
  // loop), so the per-field cap bounds the whole tick — not just one drain.
  std::unordered_set<RouteEdge, RouteEdgeHash> fired_;  // per-ROUTE-edge guard
  std::unordered_set<FieldAddress> produced_;  // reachability + scoped output cap
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_EVENT_CASCADE_HPP
