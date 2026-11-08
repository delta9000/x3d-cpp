---
title: "ADR-0064: Pointing sensors without navigation"
summary: Install pointing sensors independently and query the activation resolver before a host claims a trigger.
tags: [adr, pointing, session, consumer]
updated: 2026-10-18
related:
  - ../subsystems/system-pointing.md
  - ../seam-status.md
---

# ADR-0064: Pointing sensors without navigation

## Status

Proposed. Implementation and focused regression cases await Ben's build and test.

## Problem

CAVEOS owns navigation. Its trial integration manually attaches a
`PointingSensorSystem` to every node (`x3d_scene.cpp:205`) because
`SessionOptions::interactive` also installs SDK navigation. Before claiming a
trigger, it duplicates the private lowest-enabled-sibling resolver over a pick
path (`x3d_app.cpp:113`).

## Decision

Append `SessionOptions::pointingSensors`, default false, to install only the
pointing-device driver. `interactive` keeps installing sensors, anchors, and
navigation; setting both flags installs sensors once. Both installation paths
use `attachPointingSensors`, including the existing inventory pass.

Expose `RuntimeSession::pointingSensors(const PickResult &) const`, returning
`std::vector<X3DNode *>`, through the existing public session type. It calls
`PointingSensorSystem::resolve`, now public, so pre-press arbitration and actual
activation use one algorithm. Return all tied sensors because the runtime
activates all of them. Accepting a pick lets a host reuse its existing ray cast.

The query is independent of installation and reads current enablement without
emitting events or changing pointer/grab state. It describes a fresh press,
ignores existing grabs, and doesn't reserve a future press. A miss returns an
empty vector. The caller supplies a current pick from this session; pick paths
and returned nodes borrow scene storage. Re-pick after changes to the hit/path,
and discard borrowed data before node removal or session destruction.

## Consequences

The host replaces per-node attachment with one option and its sibling scan with
`!session->pointingSensors(session->context().pick(ray)).empty()`. Navigation
ownership stays with the host. Anchor handling remains under `interactive`.
Existing source calls keep their defaults; the appended options field changes
struct layout, so consumers must rebuild. No cross-version ABI claim is added.

The cases in `pointing_without_navigation_test.cpp`, registered in
`x3d_extract_tests`, check installation without navigation, query/activation
agreement for tied and disabled sensors, ancestor fallback, misses, sensorless
geometry, query side effects, opt-in defaults, and single installation with both
flags. Builds and tests are intentionally unrun for this proposal.
