// CPU-only RuntimeSession probe: fixed authored viewer, ticks at 30 Hz from t=0.
#include "x3d/sdk.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdk = x3d::sdk;
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: x3d_runtime_tick_profile FILE N_TICKS\n";
    return 2;
  }
  try {
    std::size_t used = 0;
    const int ticks = std::stoi(argv[2], &used);
    if (ticks < 1 || used != std::string(argv[2]).size())
      throw std::invalid_argument("N_TICKS must be a positive integer");
    std::cout << std::fixed << std::setprecision(3);
    auto start = Clock::now();
    auto document = sdk::parseFile(argv[1]);
    std::cout << "parse_ms=" << ms(start) << '\n';
    start = Clock::now();
    auto session = sdk::RuntimeSession::create(std::move(document));
    std::cout << "session_ms=" << ms(start) << '\n';
    start = Clock::now();
    auto baseline = session->fullSnapshot();
    std::cout << "snapshot_ms=" << ms(start) << " placements=" << baseline.added.size() << '\n';
    if (session->extractor().budgetExceeded())
      throw std::runtime_error("baseline walk budget exceeded; scene is partial");
    double tickTotal = 0, deltaTotal = 0;
    std::cout << "tick,time_s,systems_cascade_ms,post_cascade_ms,transforms_ms,bounds_ms,"
                 "tick_ms,delta_ms,added,removed,geometry,material,transform,lights_changed\n";
    for (int i = 0; i < ticks; ++i) {
      sdk::X3DExecutionContext::TickTimings phases;
      start = Clock::now();
      session->context().tick(i / 30.0, &phases);
      const double tickMs = ms(start);
      start = Clock::now();
      auto delta = session->delta();
      const double deltaMs = ms(start);
      tickTotal += tickMs; deltaTotal += deltaMs;
      std::cout << i << ',' << i / 30.0 << ',' << phases.systemsAndCascadeMs << ','
                << phases.postCascadeMs << ',' << phases.transformsMs << ',' << phases.boundsMs
                << ',' << tickMs << ',' << deltaMs << ',' << delta.added.size() << ','
                << delta.removed.size() << ',' << delta.updatedGeometry.size() << ','
                << delta.updatedMaterial.size() << ',' << delta.updatedTransform.size() << ','
                << delta.lightsChanged << '\n';
      if (session->extractor().budgetExceeded())
        throw std::runtime_error("extraction walk budget exceeded; timings describe a partial scene");
    }
    std::cout << "total_tick_ms=" << tickTotal << " total_delta_ms=" << deltaTotal
              << " mean_tick_ms=" << tickTotal / ticks << '\n';
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
