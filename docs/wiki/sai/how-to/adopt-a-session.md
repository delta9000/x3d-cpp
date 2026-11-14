---
title: Adopt a loaded session into the SAI
summary: Hand a prepared RuntimeSession to the native provider and drive it through the portable service.
tags: [sai, how-to, native]
updated: 2026-11-14
related:
  - ../reference/native-provider.md
  - ../../explanation/decisions/0074-sai-adopt-session.md
---

# Adopt a loaded session into the SAI

Use this when you already load scenes with the native SDK and want SAI access
to the loaded graph: named lookup, field reads and writes, observation and live
ROUTEs. The exact kind set and restrictions are in
[Native provider](../reference/native-provider.md#adopted-scenes); the design
is [ADR-0074](../../explanation/decisions/0074-sai-adopt-session.md).

## Before you start

Your session must be:

- exclusively owned by you, in a `std::unique_ptr<RuntimeSession>`
- quiescent, and never ticked
- free of other mutable aliases to its graph or author-field store

Call `adopt_session` on the thread that will own the service.

## Steps

Here `take` unwraps a result or reports its error.

1. Include both headers:

    ```cpp
    #include <x3d/sdk.hpp>           // native runtime operations
    #include <x3d/sai_provider.hpp>  // adoption
    ```

2. Transfer the session:

    ```cpp
    auto adopted = take(native::adopt_session(std::move(session)));
    auto &service = *adopted.service;
    ```

    You get back `service`, `scene`, an already-`active` activation and checked
    `host` access.

3. Find a node and a field with ordinary discovery:

    ```cpp
    auto context = take(service.root_context(adopted.scene));
    auto paint = take(service.named(context, "Paint"));
    auto color = take(service.get_field(paint, "diffuseColor"));
    ```

4. Queue a write and pump. Each pump performs one native tick:

    ```cpp
    auto client = take(service.connect(adopted.active));
    take(service.enqueue(client, {{color, sai::color3f{0.8f, 0.1f, 0.3f}}}));
    auto report = take(service.pump(adopted.active, {now}));
    // Check report.error, report.state and request receipts before rendering.
    ```

5. Extract and render. You own the only extraction cursor:

    ```cpp
    auto *runtime = take(adopted.host.session(service));
    auto changes = runtime->delta();
    // Consume changes through runtime->extractor().
    ```

6. Drain SAI observations:

    ```cpp
    take(service.dispatch_notifications());
    ```

## Rules for the borrowed session

Between pumps you may supply native tracking inputs and consume extraction.
Do not:

- call `tick` yourself
- mutate fields or change topology through native APIs
- keep the `runtime` pointer, or any native borrow, across a pump or retirement

Retire or close the service on its owner thread.

## What you can and cannot touch

- Scalar fields of the kinds listed in the reference are readable and
  writable. Node-valued fields are readable only.
- PROTO instances, Inline contents, IMPORT endpoints and Script fields keep
  running, but `fields` and `get_field` refuse them with
  `unsupported_operation`. `type_name` still reports what they are.
- Scene metadata calls on the adopted scene are refused; the META stays in the
  native document.
- To add a ROUTE, see [Add a ROUTE to a live scene](add-a-live-route.md).
