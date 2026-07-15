// examples/03_attach_behavior_tick.cpp
// ─────────────────────────────────────────────────────────────────────────────
// ATTACH A BEHAVIOR + DRIVE TICKS, using only the x3d::sdk façade.
//
// The SDK lets an embedder add its OWN behavior by subclassing x3d::sdk::System
// (attach + update) and registering it with ctx.addSystem(). Each tick the
// runtime calls update(now, ctx); the system writes fields via ctx.writeField,
// which is dirty-aware so the extractor sees the change. Here a "Spinner"
// rotates a DEF'd Transform a little every tick. We drive a handful of ticks
// and watch the node's resolved world transform change.
//
// (The same registration path carries the X3D Script node: implement
//  x3d::sdk::ScriptEngine, wrap it in x3d::sdk::ScriptSystem, and call
//  ctx.addScriptSystem(ss). That seam is EXPERIMENTAL; this example uses the
//  stable System base so it has no language-backend dependency.)
//
// Run: 03_attach_behavior_tick
// ─────────────────────────────────────────────────────────────────────────────
#include "x3d/sdk.hpp"

#include "x3d/core/X3Dtypes.hpp" // SFRotation (the value type we write)

#include <any>
#include <cmath>
#include <cstdio>
#include <iostream>

using namespace x3d::core;

namespace sdk = x3d::sdk;

static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Transform DEF="Spinner" translation="0 0 0">
      <Shape>
        <Appearance><Material diffuseColor="0.8 0.7 0.1"/></Appearance>
        <Box size="1 1 1"/>
      </Shape>
    </Transform>
  </Scene>
</X3D>)X3D";

// An embedder-defined behavior: spin a target node about +Y at a fixed rate.
class Spinner : public sdk::System {
public:
  Spinner(double radiansPerSecond) : rate_(radiansPerSecond) {}

  void attach(X3DNode *node, sdk::X3DExecutionContext &) override {
    target_ = node;
  }

  void update(double now, sdk::X3DExecutionContext &ctx) override {
    if (!target_)
      return;
    float angle = static_cast<float>(now * rate_);
    // Dirty-aware write: the extractor's delta() will report updatedTransform.
    //
    // writeField is stringly-typed and std::any-valued, so it REPORTS rather than
    // guessing: a misspelled field name, an outputOnly field, or a value of the
    // wrong type each come back as a distinct FieldWriteResult. Check it — a
    // discarded result is the silent no-op the API exists to prevent. Here a
    // failure means this System was attached to a node that is not a Transform,
    // which is a wiring bug worth surfacing rather than spinning nothing.
    const sdk::FieldWriteResult r =
        ctx.writeField(target_, "rotation",
                       std::any(SFRotation{0.0f, 1.0f, 0.0f, angle}));
    if (r != sdk::FieldWriteResult::Ok) {
      std::fprintf(stderr, "Spinner: cannot write 'rotation' on %s: %s\n",
                   target_->nodeTypeName().c_str(), sdk::fieldWriteResultName(r));
      target_ = nullptr; // stop retrying every tick.
    }
  }

private:
  X3DNode *target_ = nullptr;
  double rate_;
};

int main() {
  sdk::X3DDocument doc = sdk::parseDocument(kScene, sdk::Encoding::XML);

  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);

  // Find the target node by DEF and attach our behavior.
  std::shared_ptr<X3DNode> spinNode = doc.scene.resolve("Spinner");
  if (!spinNode) {
    std::cerr << "DEF 'Spinner' not found\n";
    return 1;
  }
  auto spinner = std::make_shared<Spinner>(/*rad/s*/ 1.5708); // 90°/s
  ctx.addSystem(spinner);
  spinner->attach(spinNode.get(), ctx);

  sdk::SceneExtractor ex(ctx, doc.scene);
  ex.fullSnapshot();

  // Drive ticks; report how the spinner's world transform evolves.
  std::cout << "Driving ticks (90 deg/s spinner):\n";
  for (int i = 1; i <= 4; ++i) {
    double t = 0.25 * i; // quarter-second steps
    ctx.tick(t);
    sdk::RenderDelta d = ex.delta();
    sdk::Mat4 world = ctx.worldTransform(spinNode.get());
    // Where does local +X (1,0,0) end up after the rotation? (proxy for angle)
    auto x = world.transformDirection(SFVec3f{1, 0, 0});
    std::cout << "  t=" << t << "s  updatedTransform=" << d.updatedTransform.size()
              << "  localX -> (" << x.x << ", " << x.y << ", " << x.z << ")\n";
  }

  std::cout << "OK\n";
  return 0;
}
