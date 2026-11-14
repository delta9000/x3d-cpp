---
title: SAI SDK
summary: Entry point for the C++20 Scene Access Interface SDK, its two backends and its documentation.
tags: [sai, index]
updated: 2026-11-14
related:
  - ../reference/subsystems/system-script-sai.md
  - ../explanation/decisions/0076-fold-sai-into-x3d-cpp.md
---

# SAI SDK

The SAI SDK is a C++20 Scene Access Interface for X3D. An application programs
against one contract, `provider::service` in
`<x3d/sai/experimental/provider.hpp>`, and picks a backend when it constructs
the service.

| Backend | Lives in | Link target | Scene storage |
|---|---|---|---|
| Reference | `sai/` | `x3d::sai_reference_provider` | The SDK's own reference kernel |
| Native | `adapters/sai_provider/` | `x3d_cpp::sai_provider` | A real x3d-cpp `RuntimeSession` |

Both backends share handles, values, errors, update gates, receipts and
notification rules, and the same test oracle runs against both.

## Find what you need

| You want to | Go to |
|---|---|
| Learn the API by building something | [Tutorials](#tutorials) |
| Get a specific job done | [How-to guides](#how-to-guides) |
| Look up exact behaviour | [Reference](#reference) |
| Understand why it works this way | [Explanation](#explanation) |

## Tutorials

- [Animate your first scene](tutorials/first-scene.md): build a routed scene on the reference backend and pump one input through it.
- [Run a scene on the native runtime](tutorials/native-host.md): drive the same contract over a `RuntimeSession` and present frames from a headless host.

## How-to guides

Portable contract (either backend):

- [Consume the SDK from CMake](how-to/consume-the-sdk.md)
- [Run the host loop](how-to/run-the-host-loop.md)
- [Add a ROUTE to a live scene](how-to/add-a-live-route.md)
- [Find named nodes](how-to/find-named-nodes.md)
- [Declare scene units and metadata](how-to/declare-units-and-metadata.md)
- [Attach user data to nodes and fields](how-to/attach-user-data.md)

Native backend:

- [Build and link the native backend](how-to/use-the-native-backend.md)
- [Adopt a loaded session](how-to/adopt-a-session.md)
- [Present frames from the native feed](how-to/present-frames.md)

Reference kernel extensions:

- [Author and inspect with the reference kernel](how-to/author-with-the-kernel.md)
- [Evaluate events with a kernel runtime host](how-to/write-a-kernel-runtime-host.md)
- [Publish browser information from a host](how-to/publish-browser-information.md)

Working on the SDK:

- [Verify a change](how-to/verify-a-change.md)
- [Update the service register](how-to/update-the-service-register.md)

## Reference

Portable contract:

- [Packages, targets and headers](reference/packages-and-targets.md)
- [Provider contract](reference/provider-contract.md): handles, operations, states, threading
- [Submissions, turns and notifications](reference/submissions.md)
- [Resource limits](reference/resource-limits.md)
- [Live ROUTEs](reference/live-routes.md)
- [Named nodes](reference/named-nodes.md)
- [Scene units and metadata](reference/scene-information.md)
- [User data](reference/user-data.md)
- [Field values and validity](reference/field-values.md)

Native backend:

- [Native provider](reference/native-provider.md): supported slice, adopted scenes, native extensions
- [Presentation feed](reference/presentation-feed.md)
- [Headless host example](reference/host-example.md)

Reference kernel extensions:

- [Reference kernel](reference/kernel.md): snapshots, edits, realization, publication
- [Kernel runtime evaluation](reference/kernel-runtime.md)
- [Multi-field services](reference/multi-field-services.md)
- [Browser services](reference/browser-services.md)
- [Matrix algebra](reference/matrix.md)
- [Generated field projection](reference/generated-fields.md)

Evidence:

- [Service register](reference/service-register.md)
- [Test suites and gates](reference/test-suites.md)

## Explanation

- [Architecture: one contract, two backends](explanation/architecture.md)
- [Relationship to the ISO C++ binding](explanation/iso-binding.md)
- [Reference kernel design](explanation/kernel-design.md)
- [Canonical units](explanation/canonical-units.md)
- [Scope and limits](explanation/scope-and-limits.md)

## Status

- The namespace is `x3d::sai::experimental`. The API can change between 0.x
  releases and makes no binary ABI promise.
- Each backend advertises the node types and services it supports. Check
  `service.supported()` instead of assuming the whole ISO SAI.
- The [service register](reference/service-register.md) lists every ISO/IEC
  19775-2 service as partial or planned, with the tests behind it.

## Not the Script API

The runtime's Script nodes use a different, in-process API, `SaiContext`,
described in [Script and SAI runtime](../reference/subsystems/system-script-sai.md). It
does not expose provider handles, and linking both packages does not connect
their scenes.

The SDK was developed in a separate `x3d-sai` repository and folded back into
x3d-cpp on 2026-11-11
([ADR-0076](../explanation/decisions/0076-fold-sai-into-x3d-cpp.md)).
