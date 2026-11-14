---
title: Evaluate events with a SAI kernel runtime host
summary: Install a stateless processor or a stateful frame factory so native code computes outputs during an event batch.
tags: [sai, how-to, kernel, events]
updated: 2026-11-14
related:
  - ../reference/kernel-runtime.md
---

# Evaluate events with a SAI kernel runtime host

A runtime host computes node behaviour (an interpolator, a physics step) while
the reference kernel evaluates an event batch. The kernel supplies time, causal
links, output validation, ROUTE propagation and atomic publication. Exact
semantics are in [Kernel runtime evaluation](../reference/kernel-runtime.md).

This is a reference-kernel extension. On the native backend the real runtime
does this work.

## Choose a host kind

| You need | Use |
|---|---|
| Outputs computed from the current input and field values only | A stateless processor |
| State carried from one frame to the next | A stateful frame factory |

## Write a stateless processor

1. Author the scene and keep the field handles the processor needs.

2. Install the callback on the browser that owns the scene:

    ```cpp
    auto installed = host.set_runtime_processor(
        scene, [=](sai::runtime_step &step) -> sai::result<void> {
          const auto *event = step.input();
          if (!event || event->target != interpolator->id() ||
              event->field != input.name())
            return {};                       // not ours, or the preparation call

          auto key = step.read(keys);
          auto point = step.read(points);
          if (!key) return sai::failure(key.error());
          if (!point) return sai::failure(point.error());

          const float fraction = std::get<float>(event->payload);
          return step.emit(output, interpolate(*key, *point, fraction));
        });
    if (!installed) return EXIT_FAILURE;
    ```

3. Send events. Each batch needs a strictly later timestamp than the last:

    ```cpp
    auto events = scene.events({1.0});
    if (!events.send(input, 0.25f)) return EXIT_FAILURE;
    auto completed = events.commit();
    ```

4. Inspect `completed->deliveries`. Each delivery has an `origin` (`external`,
   `route` or `runtime`) and, for runtime and routed deliveries, the `cause`
   that produced it.

The full program is `sai/examples/generated_runtime.cpp`.

## Handle the preparation call

The processor is called once per batch with `step.input() == nullptr` before
any input. Use it to emit time-driven outputs from `step.time()`. Return `{}`
if you have nothing to do.

## Reject or adjust an input

- Call `step.ignore_input()` to drop the incoming value without failing the
  frame.
- Emit a different value on the same inputOutput field to replace the
  automatic echo.

## Fail a frame

Return an error from the callback. The whole frame is aborted: no field
storage, routed changes, observations or time advancement are published.
Start a fresh batch to retry.

## Write a stateful host

1. Write a moveable frame type with two members:

    ```cpp
    struct motion_frame {
      sai::result<void> process(sai::runtime_step &step);
      sai::result<motion_state> finish();
    };
    ```

2. Install a factory. It receives the base snapshot and the frame time and
   returns a `result<runtime_frame>` wrapping your frame:

    ```cpp
    host.set_runtime_factory(scene, make_motion_frame);
    ```

3. Read the checkpoint from any snapshot:

    ```cpp
    auto state = scene.snapshot().runtime_state<motion_state>();
    if (state && *state) { /* **state is the committed motion_state */ }
    ```

Keep working resources private to the frame and return an owned state from
`finish()`. Do not mutate the checkpoint through aliases afterwards. The kernel
cannot undo I/O your host performs.

The full program is `sai/examples/generated_stateful_runtime.cpp`.

## Remove a host

Pass an empty callback or factory. Evaluation stops and the last checkpoint is
kept.
