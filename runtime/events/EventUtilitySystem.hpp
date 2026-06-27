// EventUtilitySystem.hpp
// Event-driven Systems for the ISO/IEC 19775-1 §30 Event Utilities cluster
// (campaign wave-3 fix). Each node was behaviorally inert (no System wired);
// these wire the node's inputOnly handlers in attach() and emit the spec-mandated
// outputs through the cascade (ctx.postEvent writes the source field via the
// reflection set-thunk AND fans out along ROUTEs).
//
//   - BooleanTriggerSystem  §30.4.4: set_triggerTime -> triggerTrue=TRUE
//   - IntegerTriggerSystem  §30.4.6: set_boolean=TRUE -> triggerValue=integerKey (FALSE ignored)
//   - TimeTriggerSystem     §30.4.7: set_boolean (any value) -> triggerTime=now
//   - BooleanFilterSystem   §30.4.1: set_boolean -> inputTrue|inputFalse (by value) + always inputNegate
//   - BooleanToggleSystem   §30.4.3: set_boolean=TRUE flips toggle; FALSE is a no-op
//   - SequencerSystem<N,V>  §30.2.4/§30.3.1: stepwise (NON-interpolated) keyValue
//     selection on set_fraction + next/previous index stepping with wrap-around
#ifndef X3D_RUNTIME_EVENT_UTILITY_SYSTEM_HPP
#define X3D_RUNTIME_EVENT_UTILITY_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/BooleanFilter.hpp"
#include "x3d/nodes/BooleanToggle.hpp"
#include "x3d/nodes/BooleanTrigger.hpp"
#include "x3d/nodes/IntegerTrigger.hpp"
#include "x3d/nodes/TimeTrigger.hpp"

#include <any>
#include <cstddef>
#include <unordered_map>

namespace x3d::runtime {
using namespace x3d::core;

/// §30.4.4 BooleanTrigger: any set_triggerTime -> triggerTrue=TRUE.
class BooleanTriggerSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::BooleanTrigger *>(node);
    if (!n) return;
    n->setOnSet_triggerTimeHandler([&ctx, n](const SFTime &) {
      ctx.postEvent(n, "triggerTrue", std::any(SFBool{true}));
    });
  }
};

/// §30.4.6 IntegerTrigger: set_boolean=TRUE -> triggerValue=integerKey; FALSE ignored.
class IntegerTriggerSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::IntegerTrigger *>(node);
    if (!n) return;
    n->setOnSet_booleanHandler([&ctx, n](const SFBool &v) {
      if (!v) return; // honored only on TRUE
      ctx.postEvent(n, "triggerValue", std::any(SFInt32{n->getIntegerKey()}));
    });
  }
};

/// §30.4.7 TimeTrigger: set_boolean (any value, value ignored) -> triggerTime=now.
class TimeTriggerSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::TimeTrigger *>(node);
    if (!n) return;
    n->setOnSet_booleanHandler([&ctx, n](const SFBool &) {
      ctx.postEvent(n, "triggerTime", std::any(SFTime{ctx.now()}));
    });
  }
};

/// §30.4.1 BooleanFilter: emit exactly one of inputTrue/inputFalse (by value) and
/// always inputNegate=!value.
class BooleanFilterSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::BooleanFilter *>(node);
    if (!n) return;
    n->setOnSet_booleanHandler([&ctx, n](const SFBool &v) {
      if (v) ctx.postEvent(n, "inputTrue", std::any(SFBool{true}));
      else   ctx.postEvent(n, "inputFalse", std::any(SFBool{false}));
      ctx.postEvent(n, "inputNegate", std::any(SFBool{!v}));
    });
  }
};

/// §30.4.3 BooleanToggle: set_boolean=TRUE flips toggle (emits toggle_changed via
/// the inputOutput alias); FALSE is a no-op.
class BooleanToggleSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<x3d::nodes::BooleanToggle *>(node);
    if (!n) return;
    n->setOnSet_booleanHandler([&ctx, n](const SFBool &v) {
      if (!v) return; // FALSE has no effect (§30.4.3)
      ctx.postEvent(n, "toggle", std::any(SFBool{!n->getToggle()}));
    });
  }
};

/// §30.2.4 stepwise selection index: largest i with key[i] <= t (boundary-clamped),
/// then walk back over equal keys so the lowest index of a duplicated key wins
/// (SEQ-7, "first definition wins"). NOT interpolated.
inline std::size_t sequencerStepIndex(const MFFloat &key, float t) {
  const std::size_t n = key.size();
  if (n == 0) return 0;
  if (t <= key.front()) return 0;
  if (t >= key.back()) return n - 1;
  std::size_t i = 0;
  for (std::size_t k = 0; k < n; ++k)
    if (key[k] <= t) i = k;
  // SEQ-7: only when t sits exactly ON a duplicated key value does the lowest
  // such index win; a fraction strictly above the duplicate keeps the floor index.
  while (i > 0 && key[i] == t && key[i - 1] == key[i]) --i;
  return i;
}

/// §30.2.4/§30.3.1 X3DSequencerNode: BooleanSequencer (SFBool/MFBool) and
/// IntegerSequencer (SFInt32/MFInt32). Stepwise value_changed on set_fraction;
/// next/previous step an internal index (TRUE only) with wrap-around.
template <typename NodeT, typename ValueT>
class SequencerSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *n = dynamic_cast<NodeT *>(node);
    if (!n) return;
    n->setOnSet_fractionHandler([&ctx, n, this](const SFFloat &f) {
      const auto kv = n->getKeyValue();
      const auto key = n->getKey();
      if (key.empty() || kv.empty()) return; // §19.3.1-style: no keys -> no events
      std::size_t i = sequencerStepIndex(key, f);
      if (i >= kv.size()) i = kv.size() - 1;
      index_[n] = i;
      ctx.postEvent(n, "value_changed", std::any(ValueT{kv[i]}));
    });
    n->setOnNextHandler([&ctx, n, this](const SFBool &v) {
      if (v) step(ctx, n, +1);
    });
    n->setOnPreviousHandler([&ctx, n, this](const SFBool &v) {
      if (v) step(ctx, n, -1);
    });
  }

private:
  void step(X3DExecutionContext &ctx, NodeT *n, int dir) {
    const auto kv = n->getKeyValue();
    if (kv.empty()) return;
    const std::size_t sz = kv.size();
    const std::size_t cur = index_.count(n) ? index_[n] : 0;
    const std::size_t next =
        dir > 0 ? (cur + 1) % sz : (cur + sz - 1) % sz;
    index_[n] = next;
    ctx.postEvent(n, "value_changed", std::any(ValueT{kv[next]}));
  }

  std::unordered_map<X3DNode *, std::size_t> index_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_EVENT_UTILITY_SYSTEM_HPP
