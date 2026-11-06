// Check interpolated values, sensor crossings, and trace determinism.
// Pass the fixtures directory as argv[1].
#include "x3d/sdk.hpp"

#include "sim_runtime.hpp"
#include "sim_tracer.hpp"

#include "x3d/nodes/Transform.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace x3d::core;

namespace {

// Build + wire a scene's full runtime; returns the live context (the doc must
// outlive it, so the caller owns both).
void wire(x3d::sdk::X3DDocument &doc, x3d::runtime::X3DExecutionContext &ctx) {
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  x3d::sim::attachFullRuntime(doc.scene, ctx);
}

void testInterpolatorMidCycle(const std::string &dir) {
  auto doc = x3d::sdk::parseFile(dir + "/sim-anim.x3d");
  x3d::runtime::X3DExecutionContext ctx;
  wire(doc, ctx);

  // Drive to t=0.5 at fps=4 (ticks at 0, .25, .5).
  const double dt = 0.25;
  for (int k = 0; k <= 2; ++k) ctx.tick(k * dt);  // last tick t=0.5

  auto mover = doc.scene.resolve("Mover");
  assert(mover && "Mover DEF must resolve");
  auto *xf = dynamic_cast<x3d::nodes::Transform *>(mover.get());
  assert(xf && "Mover must be a Transform");
  const SFVec3f tr = xf->getTranslation();
  std::cout << "  [interp] Mover.translation at t=0.5 = (" << tr.x << ", "
            << tr.y << ", " << tr.z << ")\n";
  // keyValue 0..10 over fraction 0..1; at t=0.5 in a 1s cycle, fraction=0.5.
  assert(std::fabs(tr.x - 5.0f) < 1e-3f &&
         "translation.x must be ~5 at t=0.5 (lerp 0..10 @ 0.5)");
  assert(std::fabs(tr.y) < 1e-4f && std::fabs(tr.z) < 1e-4f);
}

void testProximityCrossing(const std::string &dir) {
  auto doc = x3d::sdk::parseFile(dir + "/sim-proximity.x3d");
  x3d::runtime::X3DExecutionContext ctx;
  wire(doc, ctx);

  auto region = doc.scene.resolve("Region");
  assert(region && "Region DEF must resolve");

  // Record (tick -> isActive) via the tracer watching Region.isActive.
  x3d::sim::FieldTracer tracer(doc.scene, {"Region.enterTime", "Region.exitTime"});
  const double fps = 10.0, dur = 1.0;
  const SFVec3f from{-5, 0, 0}, to{5, 0, 0};

  int enterTick = -1, exitTick = -1;
  auto applyMove = [&](double t) {
    double f = std::min(1.0, std::max(0.0, t / dur));
    SFVec3f p{from.x + (to.x - from.x) * (float)f, 0, 0};
    ctx.setHeadPose(p, SFRotation{0, 0, 1, 0});
  };
  applyMove(0.0);
  tracer.baseline();
  // Use the same t-formula as cmdSim: k * (1.0/fps) — not k/fps — so this
  // test exercises the exact double values the CLI produces (they differ by 1
  // ULP for some k).
  const double dt_prox = 1.0 / fps;
  for (int k = 0; k < 10; ++k) {
    double t = static_cast<double>(k) * dt_prox;
    applyMove(t);
    ctx.tick(t);
    auto tr = tracer.diff(k, t);
    for (auto &c : tr.changes) {
      if (c.field == "enterTime") enterTick = k;
      if (c.field == "exitTime") exitTick = k;
    }
  }
  std::cout << "  [proximity] enterTick=" << enterTick
            << " exitTick=" << exitTick << "\n";
  // Box size 2 (|x|<=1). Viewer x = -5 + 10*t. Enters at x=-1 -> t=0.4 (tick 4),
  // exits at x=+1 -> t=0.7 (tick 7).
  assert(enterTick == 4 && "ProximitySensor enterTime must fire at tick 4 (x=-1)");
  assert(exitTick == 7 && "ProximitySensor exitTime must fire at tick 7 (x=+1)");
}

std::string runTrace(const std::string &path) {
  auto doc = x3d::sdk::parseFile(path);
  x3d::runtime::X3DExecutionContext ctx;
  wire(doc, ctx);
  x3d::sim::FieldTracer tracer(doc.scene, {});
  tracer.baseline();
  std::string out;
  for (int k = 0; k < 30; ++k) {
    double t = k / 30.0;
    ctx.tick(t);
    auto tr = tracer.diff(k, t);
    for (auto &c : tr.changes)
      out += std::to_string(tr.tick) + ":" + c.node + "." + c.field + "=" +
             c.value + "\n";
  }
  return out;
}

void testDeterminism(const std::string &dir) {
  std::string a = runTrace(dir + "/sim-anim.x3d");
  std::string b = runTrace(dir + "/sim-anim.x3d");
  std::cout << "  [determinism] trace bytes=" << a.size() << "\n";
  assert(!a.empty() && "trace must be non-empty");
  assert(a == b && "two runs must yield byte-identical traces");
}

}  // namespace

int main(int argc, char **argv) {
  assert(argc >= 2 && "pass the fixtures dir as argv[1]");
  const std::string dir = argv[1];

  testInterpolatorMidCycle(dir);
  testProximityCrossing(dir);
  testDeterminism(dir);

  std::cout << "sim_behavior_test OK\n";
  return 0;
}
