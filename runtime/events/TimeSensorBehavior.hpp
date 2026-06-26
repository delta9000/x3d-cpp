// TimeSensorBehavior.hpp
// Reference active-node behavior for TimeSensor — seed of browser sub-project #3
// (time-dependent nodes). Minimal on purpose: it maps the clock to the
// fraction_changed output over a single cycle (optionally looping). Full
// TimeSensor semantics (enabled, pause/resume, isActive/cycleTime/elapsedTime,
// startTime/stopTime gating) belong to that later sub-project.
#ifndef X3D_RUNTIME_TIME_SENSOR_BEHAVIOR_HPP
#define X3D_RUNTIME_TIME_SENSOR_BEHAVIOR_HPP

#include "X3DActiveNode.hpp"
#include "X3DExecutionContext.hpp"

#include "TimeSensor.hpp"

#include <any>
#include <cmath>

namespace x3d::runtime {

/**
 * @brief Drives a TimeSensor's fraction_changed output from the clock.
 * @details Each tick computes the cycle fraction `(now - startTime)/cycleInterval`.
 *          When `loop` is true the fraction wraps into [0,1); otherwise it is
 *          clamped to [0,1]. The fraction is emitted as an output event, which
 *          the cascade routes onward (typically to an interpolator).
 */
class TimeSensorBehavior : public ActiveNode {
public:
  explicit TimeSensorBehavior(TimeSensor *sensor) : sensor_(sensor) {}

  void update(double now, X3DExecutionContext &ctx) override {
    if (!sensor_) {
      return;
    }
    double cycle = sensor_->getCycleInterval();
    if (cycle <= 0.0) {
      cycle = 1.0; // spec default; guard against divide-by-zero
    }
    double frac = (now - sensor_->getStartTime()) / cycle;
    if (sensor_->getLoop()) {
      frac = frac - std::floor(frac); // wrap into [0,1)
    } else if (frac < 0.0) {
      frac = 0.0;
    } else if (frac > 1.0) {
      frac = 1.0;
    }
    ctx.postEvent(sensor_, "fraction_changed",
                  std::any(static_cast<SFFloat>(frac)));
  }

private:
  TimeSensor *sensor_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TIME_SENSOR_BEHAVIOR_HPP
