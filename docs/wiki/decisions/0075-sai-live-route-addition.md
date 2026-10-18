---
title: "ADR-0066: Queued live SAI ROUTE addition"
summary: Admit live routes through the existing portable FIFO and install them before evaluation.
tags: [adr, sai, routes, runtime]
updated: 2026-10-18
related:
  - ../subsystems/sai-provider.md
  - 0065-sai-adopt-session.md
---

# ADR-0075: Queued live SAI ROUTE addition

Proposed. An adopted session is already active, so setup-only `add_route` cannot
connect new application-selected behavior. Add optional portable
`service_id::live_routes` and `enqueue_route(client, from, to)` in x3d-sai.
Retain setup authoring and existing backend subclasses: the live backend virtual
defaults to unsupported without effects. Frontends and backends need rebuilding;
this experimental extension has no binary ABI guarantee.

Use the existing owning tickets, per-client update gate, per-scene FIFO,
admission budgets, cancellation, and pump. Validate canonical endpoint authority,
lexical context, access direction, and exact portable kind before admission.
Install each request's routes before evaluating its turn. Earlier submissions
cannot use them, and earlier outputs are not replayed. In a released gate, routes
install in their relative order before all writes and input occurrences, even
when a write was admitted first. This explicit phase ordering avoids splitting
one gate into several ticks or inventing interleaving with coalesced writes.

Native installation additionally checks exact reflected kinds and refuses
opaque endpoints, cross-context routes, PROTO interface redirection, and
duplicates. Adopted node-valued destinations remain refused, preserving the
adoption slice's topology-write restriction. Prepare a direct weak-identity entry in the native document before
adding its executable edge. Undo that new entry if executable insertion throws
or declines a detached endpoint. A successful insertion leaves both views
consistent. It never extracts a frame or reconstructs the session. The reference
provider reuses its existing route transaction; no second route graph is added.

No transaction spans an entire route/write gate. An installation refusal before
any effects can fail its ticket while leaving the activation active. If an
earlier route was installed, a later failure reports partial effects and faults
the activation, cancelling later requests. Installed native document entries
remain truthful after such a fault. Deferred observation and host extraction
retain their existing contracts.

Existing test targets gain assertions: `sai_provider_contract` covers FIFO,
gates, refusals, defaults, cancellation, budgets, and partial effects;
`sai_provider_reference` covers actual reference delivery;
`sai_provider_adopt_session` covers adopted native delivery and retained document
agreement in source and relocated consumers. No builds or tests were run for
this slice. Route removal, nested context exposure, and PROTO interface
redirection are deferred.
