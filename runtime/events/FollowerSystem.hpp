#ifndef X3D_RUNTIME_FOLLOWER_SYSTEM_HPP
#define X3D_RUNTIME_FOLLOWER_SYSTEM_HPP
#include "FollowerArith.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include <any>
#include <cmath>
#include <memory>
#include <vector>
namespace x3d::runtime {

/// DamperSystem<NodeT, ValueT> — §39.3.2 IIR damper cascade.
///
/// Models a cascade of `order` first-order IIR lerp-filters each driven with
/// α = exp(-dt/τ).  When order==0 or τ<=0 the output is a passthrough
/// (immediate snap to destination, then isActive=false).  Convergence is
/// tested BEFORE the update step (the end-of-transition snap fires before the
/// next filter pass) using FollowerArith<ValueT>::dist against the per-filter
/// tolerance.  set_value resets all filter states and stops the transition.
template <typename NodeT, typename ValueT>
class DamperSystem : public System {
  struct Entry {
    NodeT *node;
    std::vector<ValueT> filters; // size order+1; [0] = input (destination)
    ValueT destination;
    bool active = false;
    double lastTick = -1.0;
  };
  std::vector<std::unique_ptr<Entry>> entries_;

public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *d = dynamic_cast<NodeT *>(node);
    if (!d) return;
    auto e = std::make_unique<Entry>();
    e->node = d;
    int order = std::max(0, d->getOrder());
    e->filters.assign(order + 1, d->getInitialValue());
    e->destination = d->getInitialDestination();
    bool initiallyActive = FollowerArith<ValueT>::dist(d->getInitialValue(), d->getInitialDestination()) > 0.0f;
    e->active = initiallyActive;
    Entry *ep = e.get();
    entries_.push_back(std::move(e));
    // Emit isActive TRUE now if initial transition is pending (§39 requires
    // isActive to go TRUE when a transition begins).
    if (initiallyActive) {
      d->emitIsActive(SFBool{true});
      ctx.postEvent(d, "isActive", std::any(SFBool{true}));
    }
    // set_destination: update target, activate if not already active.
    d->setOnSet_destinationHandler([ep, d, &ctx](const ValueT &v) {
      ep->destination = v;
      if (!ep->active) {
        ep->active = true;
        d->emitIsActive(SFBool{true});
        ctx.postEvent(d, "isActive", std::any(SFBool{true}));
      }
    });
    // set_value: snap all filters + destination to v, deactivate.
    d->setOnSet_valueHandler([ep, d, &ctx](const ValueT &v) {
      for (auto &f : ep->filters) f = v;
      ep->destination = v;
      d->emitValue_changed(v);
      ctx.postEvent(d, "value_changed", std::any(v));
      if (ep->active) {
        ep->active = false;
        d->emitIsActive(SFBool{false});
        ctx.postEvent(d, "isActive", std::any(SFBool{false}));
      }
      ep->active = false;
    });
  }

  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &e : entries_) {
      if (!e->active) {
        e->lastTick = now;
        continue;
      }
      // On the first tick, treat as if the follower started at t=0 (lastTick=0).
      double dt = (e->lastTick < 0) ? now : (now - e->lastTick);
      e->lastTick = now;
      NodeT *d = e->node;
      int order = std::max(0, d->getOrder());
      double tau = d->getTau();
      float tol = d->getTolerance();
      if (tol < 0) tol = 0.001f; // §39.3.2: tolerance<0 means use a default

      // Passthrough: order==0 or tau<=0 → snap immediately.
      if (order == 0 || tau <= 0.0) {
        d->emitValue_changed(e->destination);
        ctx.postEvent(d, "value_changed", std::any(e->destination));
        e->active = false;
        d->emitIsActive(false);
        ctx.postEvent(d, "isActive", std::any(SFBool{false}));
        continue;
      }

      // Resize filter bank if order changed at runtime.
      if ((int)e->filters.size() != order + 1)
        e->filters.assign(order + 1, e->destination);

      // Drive filter[0] with current destination.
      e->filters[0] = e->destination;

      // Reconcile inner filter shapes to the destination's length (MF broadcast fix).
      // For SF/scalar types reshapeLike is a no-op; for MF it broadcasts a length-1
      // seed to length-N so the cascade produces output that can converge element-wise.
      for (int i = 1; i <= order; ++i)
        e->filters[i] = FollowerArith<ValueT>::reshapeLike(e->filters[i], e->destination);

      // §39.3.2: test convergence BEFORE the update pass.
      bool done = true;
      for (int i = 1; i <= order && done; ++i)
        if (FollowerArith<ValueT>::dist(e->filters[i], e->filters[i - 1]) > tol)
          done = false;

      if (done) {
        // Snap all filters to destination and stop.
        for (int i = 1; i <= order; ++i) e->filters[i] = e->destination;
        d->emitValue_changed(e->destination);
        ctx.postEvent(d, "value_changed", std::any(e->destination));
        e->active = false;
        d->emitIsActive(false);
        ctx.postEvent(d, "isActive", std::any(SFBool{false}));
        continue;
      }

      // IIR cascade: each filter lerps toward the previous with α=e^(-dt/τ).
      float a = static_cast<float>(std::exp(-dt / tau));
      for (int i = 1; i <= order; ++i)
        e->filters[i] = FollowerArith<ValueT>::lerp(e->filters[i - 1], e->filters[i], a);

      d->emitValue_changed(e->filters[order]);
      ctx.postEvent(d, "value_changed", std::any(e->filters[order]));
    }
  }
};

/// ChaserSystem<NodeT, ValueT> — §39.3.1 re-basing linear ramp (FIR chaser).
///
/// On each set_destination event the chaser re-bases: the start is the current
/// output at event time and the destination is the new value.  The output is
/// O(t) = lerp(start, dest, clamp((now - startTime) / duration, 0, 1)).
/// Exactly at t = startTime + duration the output equals destination and the
/// transition ends (isActive → false).  duration≤0 → immediate snap.
/// set_value jumps the output and stops any active transition.
template <typename NodeT, typename ValueT>
class ChaserSystem : public System {
  struct Entry {
    NodeT *node;
    ValueT start, destination;
    double startTime = 0.0;
    bool active = false, started = false;
    double lastTick = -1.0;
  };
  std::vector<std::unique_ptr<Entry>> entries_;

public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *c = dynamic_cast<NodeT *>(node);
    if (!c) return;
    auto e = std::make_unique<Entry>();
    e->node = c;
    e->start = c->getInitialValue();
    e->destination = c->getInitialDestination();
    bool initiallyActive =
        FollowerArith<ValueT>::dist(e->start, e->destination) > 0.0f;
    e->active = initiallyActive;
    // On the initial-active path mark started so a mid-transition set_destination
    // re-bases from the current output rather than from initialValue.
    if (initiallyActive) e->started = true;
    Entry *ep = e.get();
    entries_.push_back(std::move(e)); // push BEFORE handlers capture ep

    if (initiallyActive) {
      c->emitIsActive(SFBool{true});
      ctx.postEvent(c, "isActive", std::any(SFBool{true}));
    }

    // set_destination: re-base start from current output, activate.
    c->setOnSet_destinationHandler([ep, c, &ctx](const ValueT &v) {
      // Re-base from current output if we've already started producing values.
      ValueT newStart = ep->started ? c->getValue_changed() : ep->start;
      // Reconcile start shape to the new destination (MF broadcast fix).
      ep->start = FollowerArith<ValueT>::reshapeLike(newStart, v);
      ep->destination = v;
      // Seed startTime from the last tick so f=0 on the NEXT update call.
      ep->startTime = (ep->lastTick < 0) ? 0.0 : ep->lastTick;
      ep->started = true;
      if (!ep->active) {
        ep->active = true;
        c->emitIsActive(SFBool{true});
        ctx.postEvent(c, "isActive", std::any(SFBool{true}));
      }
    });

    // set_value: jump output, clear transition.
    c->setOnSet_valueHandler([ep, c, &ctx](const ValueT &v) {
      ep->start = v;
      ep->destination = v;
      c->emitValue_changed(v);
      ctx.postEvent(c, "value_changed", std::any(v));
      if (ep->active) {
        ep->active = false;
        c->emitIsActive(SFBool{false});
        ctx.postEvent(c, "isActive", std::any(SFBool{false}));
      }
      ep->active = false;
    });
  }

  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &e : entries_) {
      e->lastTick = now;
      if (!e->active) continue;
      NodeT *c = e->node;
      double D = c->getDuration();
      double f = (D <= 0.0) ? 1.0 : (now - e->startTime) / D;
      if (f < 0.0) f = 0.0;
      if (f > 1.0) f = 1.0;
      ValueT out = FollowerArith<ValueT>::lerp(e->start, e->destination,
                                               static_cast<float>(f));
      c->emitValue_changed(out);
      ctx.postEvent(c, "value_changed", std::any(out));
      if (f >= 1.0) {
        e->active = false;
        c->emitIsActive(SFBool{false});
        ctx.postEvent(c, "isActive", std::any(SFBool{false}));
      }
    }
  }
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_FOLLOWER_SYSTEM_HPP
