---
title: Add a ROUTE to a live SAI scene
summary: Use enqueue_route to connect two fields after a scene has been activated.
tags: [sai, how-to, provider, routes]
updated: 2026-11-14
related:
  - ../reference/live-routes.md
---

# Add a ROUTE to a live SAI scene

`add_route` only works during setup. After activation, queue the ROUTE as a
request. Exact semantics are in [Live ROUTEs](../reference/live-routes.md).

## Steps

1. Check the capability:

    ```cpp
    if (!service.supported().supports(provider::service_id::live_routes)) {
      // This backend cannot add ROUTEs to an active scene.
    }
    ```

2. Get the two field handles. The source must be outputOnly or inputOutput and
   the destination inputOnly or inputOutput, with the same kind.

    ```cpp
    auto from = service.get_field(path, bindings::PositionInterpolator::value_changed);
    auto to   = service.get_field(mover, bindings::Transform::translation);
    ```

3. Queue the request:

    ```cpp
    auto ticket = service.enqueue_route(client, *from, *to);
    if (!ticket) { /* refused at admission: inspect ticket.error() */ }
    ```

4. Pump. The route installs at the start of that turn, before evaluation:

    ```cpp
    auto report = service.pump(active, {now});
    ```

5. Check `report->error`, `report->state` and `ticket->status()`. Installation
   can still be refused at pump if the graph changed after admission.

## Install a route together with its first input

Put both behind one gate. Routes in a gate install before all of that gate's
writes, whatever order you called them in:

```cpp
service.begin_update(client);
service.enqueue(client, path, bindings::PositionInterpolator::set_fraction, 0.5f);
service.enqueue_route(client, *from, *to);
service.end_update(client);
service.pump(active, {now});   // the fraction travels the new route
```

## Things to expect

- Work queued *before* the route request runs without the route.
- Installing a route does not replay output the source already produced.
- Adding a route that already exists succeeds and does nothing.
- You cannot remove a route.

## Native backend refusals

The native backend refuses some routes at `enqueue_route` that the portable
checks allow, such as SFEnum to SFString or any node-valued destination. The
list is in
[Native provider](../reference/native-provider.md#live-route-restrictions).
