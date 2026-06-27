// X3DTimeDependentSystem.hpp
// Reusable time-dependent lifecycle (X3DTimeDependentNode) as a System base.
// Implements the X3D clock state machine shared by every time-dependent node
// (TimeSensor here; AudioClip/MovieTexture/Sound layer on later): enabled
// gating, startTime/stopTime activation, loop + cycle accounting, and
// pause/resume with elapsed-time freezing. Concrete systems (see
// TimeSensorSystem) supply the per-tick fraction/cycle outputs; this base owns
// the activation/pause/deactivation edges and the elapsed-time clock.
//
// State lives in the System keyed by node pointer — never on the node (the
// node + reflection FieldTable stays the source of truth; outputs flow back
// onto the node's outputOnly fields through the cascade). All writes go through
// ctx.postEvent(node, "<x3dName>", value) so downstream ROUTEs fire in the same
// timestamp.
#ifndef X3D_RUNTIME_TIME_DEPENDENT_SYSTEM_HPP
#define X3D_RUNTIME_TIME_DEPENDENT_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/X3DTimeDependentNode.hpp"

#include <any>
#include <cmath>
#include <unordered_map>

namespace x3d::runtime {

/**
 * @brief Base System implementing the X3DTimeDependentNode clock lifecycle.
 * @details Time-driven: `attach` enrolls a node; `update(now, ctx)` runs the
 *          per-node state machine each tick. The base owns the edges common to
 *          every time-dependent node — activation gating
 *          (enabled/startTime/stopTime), pause/resume, deactivation, and the
 *          elapsed-time clock (which excludes paused spans). A derived system
 *          overrides `emitCycleOutputs` to produce its node-specific continuous
 *          outputs (TimeSensor: fraction_changed/time/cycleTime) and may
 *          override `enabled`/`loop`/timing reads if its node spells them
 *          differently. Outputs are only emitted on change for edge-triggered
 *          fields (isActive/isPaused/cycleTime); continuous fields are emitted
 *          every active, unpaused tick.
 */
class X3DTimeDependentSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    if (auto *tdn = dynamic_cast<X3DTimeDependentNode *>(node)) {
      state_.emplace(tdn, State{});
    }
  }

  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &[node, st] : state_) {
      advance(node, st, now, ctx);
    }
  }

protected:
  /**
   * @brief Per-node lifecycle state. Held in the system, never on the node.
   */
  struct State {
    bool active = false;
    bool paused = false;
    // Effective start of the current activation (== startTime at activation,
    // shifted forward by any paused span so elapsed/fraction exclude pauses).
    double timeBase = 0.0;
    // startTime snapshotted at activation. While active, the spec (8.2.4.3)
    // requires set_startTime events to be ignored, so the lifecycle reads this
    // frozen value rather than the (possibly ROUTE-mutated) live node field.
    double activeStartTime = 0.0;
    // True once the node completed its run (single-shot end / loop-end). A
    // completed node must NOT auto-restart until a NEW startTime is set
    // (§8.2.4.3); the activation gate skips while startTime == completedStartTime.
    bool completed = false;
    double completedStartTime = 0.0;
    // Index of the cycle whose start was last announced via cycleTime, so each
    // cycle boundary emits exactly one cycleTime pulse.
    long lastCycleAnnounced = -1;
  };

  // --- timing reads (overridable for nodes that spell these differently) ---
  virtual bool readEnabled(X3DTimeDependentNode *node) const {
    (void)node;
    return true; // base X3DTimeDependentNode has no `enabled`; default on.
  }
  virtual bool readLoop(X3DTimeDependentNode *node) const {
    (void)node;
    return false;
  }
  virtual double readCycleInterval(X3DTimeDependentNode *node) const {
    (void)node;
    return 1.0;
  }
  double readStartTime(X3DTimeDependentNode *node) const {
    return node->getStartTime();
  }
  double readStopTime(X3DTimeDependentNode *node) const {
    return node->getStopTime();
  }
  double readPauseTime(X3DTimeDependentNode *node) const {
    return node->getPauseTime();
  }
  double readResumeTime(X3DTimeDependentNode *node) const {
    return node->getResumeTime();
  }

  /**
   * @brief Derived hook: emit the node's continuous outputs for this tick.
   * @param node    The time-dependent node.
   * @param frac    Cycle fraction in [0,1] (already loop-wrapped / clamped).
   * @param now     Current clock time.
   * @param elapsed Active+unpaused seconds since activation.
   * @param ctx     Execution context to post events into.
   * @details Called only while active and not paused. The base has already
   *          emitted isActive/cycleTime/elapsedTime edges; derived systems add
   *          fraction_changed/time (TimeSensor) etc. elapsedTime is emitted by
   *          the base, so a derived system need not re-emit it.
   */
  virtual void emitCycleOutputs(X3DTimeDependentNode *node, double frac,
                                double now, double elapsed,
                                X3DExecutionContext &ctx) {
    (void)node;
    (void)frac;
    (void)now;
    (void)elapsed;
    (void)ctx;
  }

  /**
   * @brief Derived hook: emit a cycleTime pulse at a cycle boundary.
   * @param cycleStart The time at the beginning of the cycle just entered
   *                   (== startTime + cycleIndex*cycleInterval; == startTime at
   *                   activation). This is the value the spec requires for the
   *                   cycleTime event — NOT the tick's current `now`.
   * @details Called by the base each time a new cycle starts (including the
   *          very first, at activation). Default no-op; TimeSensor emits its
   *          cycleTime outputOnly here.
   */
  virtual void emitCycleTime(X3DTimeDependentNode *node, double cycleStart,
                             X3DExecutionContext &ctx) {
    (void)node;
    (void)cycleStart;
    (void)ctx;
  }

  // Post an outputOnly via the cascade (writes the node field + fans out).
  template <typename T>
  static void emit(X3DExecutionContext &ctx, X3DNode *node,
                   const std::string &field, T value) {
    ctx.postEvent(node, field, std::any(value));
  }

private:
  /**
   * @brief Run one tick of the lifecycle for a single node.
   */
  void advance(X3DTimeDependentNode *node, State &st, double now,
               X3DExecutionContext &ctx) {
    const bool enabled = readEnabled(node);
    // 8.2.4.3: an active node ignores set_startTime — use the value snapshotted
    // at activation, not the live (possibly ROUTE-mutated) field, so a
    // set_startTime delivered mid-run cannot corrupt the deactivation guard.
    const double startTime = st.active ? st.activeStartTime : readStartTime(node);
    const double stopTime = readStopTime(node);
    // Raw cycle drives the non-looping completion threshold (cycle <= 0 must
    // deactivate after one tick: sinceStart >= cycle is immediately true).
    // gCycle is the divide-by-zero-guarded value used for ALL index / fraction
    // / cycle-start division math (RTC-1).
    const double cycle = effectiveCycle(node);
    const double gCycle = guardCycle(cycle);

    // --- deactivation: disabled, or a real stopTime has been reached ---
    if (st.active) {
      const bool stoppedByDisable = !enabled;
      const bool stoppedByStopTime = stopTime > startTime && stopTime <= now;
      if (stoppedByDisable || stoppedByStopTime) {
        // Spec 8.4.1: a TimeSensor is guaranteed to generate final time and
        // fraction_changed events. On the disable/stop edge, evaluate and send
        // this tick's continuous outputs (time/fraction_changed/elapsedTime)
        // BEFORE isActive=FALSE (RTC-4). Skip for paused (frozen) sensors.
        if (!st.paused) {
          const double sinceStart = now - st.timeBase;
          const double frac =
              fractionWithBoundary(sinceStart, gCycle, startTime, now,
                                   readLoop(node));
          emit<SFTime>(ctx, node, "elapsedTime",
                       static_cast<SFTime>(sinceStart));
          emitCycleOutputs(node, frac, now, sinceStart, ctx);
        }
        deactivate(node, st, now, cycle, ctx);
        return;
      }
    }

    // --- activation edge ---
    if (!st.active) {
      // A completed run does not auto-restart: only a NEW startTime re-arms it
      // (§8.2.4.3). Without this a finished single-shot would re-activate (and
      // re-complete) every tick, churning spurious events and overwriting the
      // frozen final outputs.
      if (st.completed && startTime == st.completedStartTime) return;
      const bool startReached = startTime <= now;
      const bool stopGate = stopTime <= startTime || now < stopTime;
      if (enabled && startReached && stopGate) {
        activate(node, st, startTime, now, ctx);
        // fall through to emit this tick's outputs
      } else {
        return; // still idle
      }
    }

    // --- pause / resume edges (only meaningful while active) ---
    handlePauseResume(node, st, now, ctx);

    if (st.paused) {
      return; // frozen: no continuous outputs while paused
    }

    // --- non-looping completion: run to the END of the CURRENT cycle ---
    // 8.2.4.3: an active node with loop FALSE becomes inactive at the end of the
    // current cycle, NOT immediately. The current cycle is the one whose start
    // was last announced (st.lastCycleAnnounced): a fresh single-shot is cycle
    // 0; a node that looped then had set_loop FALSE finishes whichever cycle it
    // is in. A degenerate cycleInterval <= 0 still completes after one tick.
    const double sinceStart = now - st.timeBase;
    if (!readLoop(node)) {
      const double endOfCurrentCycle =
          cycle > 0.0 ? (st.lastCycleAnnounced + 1) * gCycle : 0.0;
      if (sinceStart >= endOfCurrentCycle) {
        // Emit the final fraction (1.0) + cycle-boundary elapsed, then
        // deactivate. TDN-7: the final `time` carries the EXACT cycle boundary
        // (timeBase + endOfCurrentCycle), not the (possibly overshooting) tick
        // now. The re-activation guard (st.completed) keeps this final value
        // frozen on subsequent ticks.
        const double elapsed = endOfCurrentCycle;
        const double boundaryNow = st.timeBase + endOfCurrentCycle;
        emit<SFTime>(ctx, node, "elapsedTime", static_cast<SFTime>(elapsed));
        emitCycleOutputs(node, 1.0, boundaryNow, elapsed, ctx);
        st.completed = true;
        st.completedStartTime = st.activeStartTime;
        deactivate(node, st, now, cycle, ctx);
        return;
      }
    }

    // --- cycle boundary detection (loop): announce each new cycle once ---
    // Index via the guarded cycle (RTC-1: never divide by a raw cycle <= 0).
    const long cycleIndex = static_cast<long>(sinceStart / gCycle);
    if (cycleIndex != st.lastCycleAnnounced) {
      st.lastCycleAnnounced = cycleIndex;
      // cycleTime == "time at the beginning of the current cycle" ==
      // startTime + cycleIndex*cycleInterval (RTC-3), NOT the tick's `now`.
      emitCycleTime(node, startTime + cycleIndex * gCycle, ctx);
    }

    // --- continuous outputs ---
    const double frac =
        fractionWithBoundary(sinceStart, gCycle, startTime, now,
                             readLoop(node));
    const double elapsed = sinceStart;
    emit<SFTime>(ctx, node, "elapsedTime", static_cast<SFTime>(elapsed));
    emitCycleOutputs(node, frac, now, elapsed, ctx);
  }

  void activate(X3DTimeDependentNode *node, State &st, double startTime,
                double now, X3DExecutionContext &ctx) {
    st.active = true;
    st.paused = false;
    st.timeBase = startTime;
    st.activeStartTime = startTime;
    st.completed = false;
    st.lastCycleAnnounced = 0;
    (void)now;
    emit<SFBool>(ctx, node, "isActive", true);
    // The first cycle begins at startTime: announce it with the cycle-start
    // time (== startTime), not the activation tick's `now` (RTC-3).
    emitCycleTime(node, startTime, ctx);
  }

  void deactivate(X3DTimeDependentNode *node, State &st, double now,
                  double cycle, X3DExecutionContext &ctx) {
    (void)now;
    (void)cycle;
    if (st.paused) {
      emit<SFBool>(ctx, node, "isPaused", false);
      st.paused = false;
    }
    st.active = false;
    st.lastCycleAnnounced = -1;
    emit<SFBool>(ctx, node, "isActive", false);
  }

  void handlePauseResume(X3DTimeDependentNode *node, State &st, double now,
                         X3DExecutionContext &ctx) {
    const double pauseTime = readPauseTime(node);
    const double resumeTime = readResumeTime(node);

    if (!st.paused) {
      // Pause when pauseTime is in (resumeTime, now]. (pauseTime > resumeTime
      // disambiguates a stale pauseTime that has since been superseded.)
      if (pauseTime > resumeTime && pauseTime > 0.0 && pauseTime <= now) {
        st.paused = true;
        emit<SFBool>(ctx, node, "isPaused", true);
        // §8.2.4.4: also emit pauseTime_changed at the pause edge. Emitting the
        // inputOutput field on its canonical name fans out to its *_changed
        // ROUTEs and re-writes the field to its own value (a no-op). The echo
        // carries the field value (the pause trigger time ~= the pause instant);
        // a strict "simulation now" reading would pass `now` here instead.
        // (TDN-1)
        emit<SFTime>(ctx, node, "pauseTime", static_cast<SFTime>(pauseTime));
      }
    } else {
      // Resume when resumeTime is in (pauseTime, now]. §8.2.4.4 requires
      // resumeTime STRICTLY greater than pauseTime (symmetric with the strict
      // pause guard above), so resumeTime == pauseTime must not resume.
      if (resumeTime > 0.0 && resumeTime <= now && resumeTime > pauseTime) {
        st.paused = false;
        // Shift the time base forward by the paused span so elapsed/fraction
        // continue from where they froze (exclude the paused interval).
        st.timeBase += (resumeTime - pauseTime);
        emit<SFBool>(ctx, node, "isPaused", false);
        // §8.2.4.4: emit resumeTime_changed at the resume edge (TDN-2). See the
        // pause-edge note above on the echo mechanism and value choice.
        emit<SFTime>(ctx, node, "resumeTime", static_cast<SFTime>(resumeTime));
      }
    }
  }

  // cycleInterval with the spec divide-by-zero guard (treat <= 0 as 1 for the
  // fraction math; a non-looping sensor with cycle <= 0 deactivates after one
  // tick because sinceStart >= cycle is immediately true).
  static double guardCycle(double cycle) { return cycle > 0.0 ? cycle : 1.0; }

  double effectiveCycle(X3DTimeDependentNode *node) const {
    return readCycleInterval(node);
  }

  // fraction in [0,1]: looped → wrap (boundary maps to 1.0 momentarily handled
  // by the single-shot/cycle-boundary paths above); non-looped → clamp.
  static double fractionAt(double sinceStart, double cycle, bool loop) {
    const double c = guardCycle(cycle);
    double f = sinceStart / c;
    if (loop) {
      f = f - std::floor(f); // wrap into [0,1)
    } else if (f < 0.0) {
      f = 0.0;
    } else if (f > 1.0) {
      f = 1.0;
    }
    return f;
  }

  // Spec 8.4.1 fraction_changed: f = fractionalPart((now-startTime)/cycle);
  //   if (f == 0.0 && now > startTime) fraction_changed = 1.0 else f.
  // `gCycle` must already be the divide-by-zero-guarded cycle. This applies the
  // boundary rule on the looping continuous path too (RTC-2): a tick that lands
  // exactly on a cycle boundary (sinceStart a positive multiple of the cycle)
  // emits 1.0 rather than the wrapped 0.0.
  static double fractionWithBoundary(double sinceStart, double gCycle,
                                     double startTime, double now, bool loop) {
    const double f = fractionAt(sinceStart, gCycle, loop);
    if (f == 0.0 && now > startTime) {
      return 1.0;
    }
    return f;
  }

  std::unordered_map<X3DTimeDependentNode *, State> state_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TIME_DEPENDENT_SYSTEM_HPP
