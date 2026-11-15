---
title: Add a behavior System
summary: Subclass sdk::System to run your own per-tick logic and write fields the extractor can see.
tags: [how-to, execution, embedding]
updated: 2026-11-14
related:
  - ../reference/subsystems/execution-context.md
  - ../reference/native-host-contract.md
---

# Add a behavior System

A `System` is how a host adds logic that runs inside the tick. The complete
program for this guide is `examples/03_attach_behavior_tick.cpp`.

## 1. Subclass `sdk::System`

```cpp
#include "x3d/sdk.hpp"
#include <any>
namespace sdk = x3d::sdk;
using namespace x3d::core;

class Spinner : public sdk::System {
public:
  explicit Spinner(double radiansPerSecond) : rate_(radiansPerSecond) {}

  void attach(X3DNode *node, sdk::X3DExecutionContext &) override {
    target_ = node;
  }

  void update(double now, sdk::X3DExecutionContext &ctx) override {
    if (!target_) return;
    const float angle = static_cast<float>(now * rate_);
    const sdk::FieldWriteResult r = ctx.writeField(
        target_, "rotation", std::any(SFRotation{0.0f, 1.0f, 0.0f, angle}));
    if (r != sdk::FieldWriteResult::Ok) target_ = nullptr; // stop retrying
  }

private:
  X3DNode *target_ = nullptr;
  double rate_;
};
```

## 2. Write through the context

Use `ctx.writeField(node, "field", value)`, not the generated setters. The
context write marks the node dirty, so the next `delta()` reports the change.
A raw setter changes the value and the extractor never hears about it.

`writeField` returns a `FieldWriteResult`. A misspelled field, an outputOnly
field and a value of the wrong C++ type each come back as a distinct result;
`sdk::fieldWriteResultName(r)` names it. Check the result every time.

## 3. Register it and attach the node

```cpp
std::shared_ptr<X3DNode> node = doc.scene.resolve("Spinner"); // by DEF
auto spinner = std::make_shared<Spinner>(1.5708);             // 90 degrees/s
ctx.addSystem(spinner);
spinner->attach(node.get(), ctx);
```

With a `RuntimeSession`, use `session->context()` and `session->scene()` in
place of `ctx` and `doc.scene`.

## 4. Tick

```cpp
ctx.tick(t);
sdk::RenderDelta d = ex.delta(); // d.updatedTransform lists the spun placement
```

## Things to watch

- Settlement can call `update` more than once in a tick. If your System
  consumes a queue of host commands, drain it once.
- A write made just before `tick` loses its dirty mark when the tick clears the
  previous set. Write from inside `update`, or post an X3D input event with
  `postEvent` for delivery in the next cascade.

The full mutation rules are in the
[native host contract](../reference/native-host-contract.md#mutation); the
context API is in
[Execution context](../reference/subsystems/execution-context.md).
