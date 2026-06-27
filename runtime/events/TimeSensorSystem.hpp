// TimeSensorSystem.hpp
// The canonical animation driver: a time-driven System implementing the full
// TimeSensor lifecycle on top of the shared X3DTimeDependentNode clock
// (X3DTimeDependentSystem). It reads enabled/loop/cycleInterval off the node
// and emits the TimeSensor-specific continuous outputs — fraction_changed,
// time, and the cycleTime pulse — while the base owns isActive/isPaused and the
// elapsed-time clock.
//
// This supersedes the minimal TimeSensorBehavior (which only mapped the clock
// to fraction_changed over one cycle). A browser registers one TimeSensorSystem
// and `attach`es every TimeSensor; each `tick(now)` advances them all and the
// emitted outputs cascade onward (typically to an interpolator).
#ifndef X3D_RUNTIME_TIME_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_TIME_SENSOR_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DTimeDependentSystem.hpp"

#include "x3d/nodes/TimeSensor.hpp"

namespace x3d::runtime {

/**
 * @brief Full TimeSensor lifecycle as a time-driven System.
 * @details Derives the shared clock machine from X3DTimeDependentSystem and
 *          plugs in TimeSensor's enabled/loop/cycleInterval reads plus its
 *          continuous outputs. Each active, unpaused tick emits
 *          `time = now`, `fraction_changed = frac`; each cycle boundary
 *          (including activation) emits `cycleTime = startTime +
 *          cycleIndex*cycleInterval` (the cycle-start time). The base emits
 *          `isActive`, `isPaused`, and `elapsedTime`.
 */
class TimeSensorSystem : public X3DTimeDependentSystem {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (dynamic_cast<TimeSensor *>(node)) {
      X3DTimeDependentSystem::attach(node, ctx);
    }
  }

protected:
  bool readEnabled(X3DTimeDependentNode *node) const override {
    return sensor(node)->getEnabled();
  }
  bool readLoop(X3DTimeDependentNode *node) const override {
    return sensor(node)->getLoop();
  }
  double readCycleInterval(X3DTimeDependentNode *node) const override {
    return sensor(node)->getCycleInterval();
  }

  void emitCycleOutputs(X3DTimeDependentNode *node, double frac, double now,
                        double elapsed, X3DExecutionContext &ctx) override {
    (void)elapsed; // elapsedTime is emitted by the base.
    emit<SFTime>(ctx, node, "time", static_cast<SFTime>(now));
    emit<SFFloat>(ctx, node, "fraction_changed", static_cast<SFFloat>(frac));
  }

  void emitCycleTime(X3DTimeDependentNode *node, double cycleStart,
                     X3DExecutionContext &ctx) override {
    // Spec 8.4.1: cycleTime == time at the beginning of the current cycle
    // (startTime + cycleIndex*cycleInterval), supplied by the base (RTC-3).
    emit<SFTime>(ctx, node, "cycleTime", static_cast<SFTime>(cycleStart));
  }

private:
  static TimeSensor *sensor(X3DTimeDependentNode *node) {
    return dynamic_cast<TimeSensor *>(node);
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TIME_SENSOR_SYSTEM_HPP
