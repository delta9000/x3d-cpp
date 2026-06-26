---
title: "ADR-0036: Codec Round-Trip Fidelity Properties and Fault-Injected Oracles"
summary: "Names the structural properties a codec round-trip must preserve (DEF survival, USE-as-shared-reference, ROUTE survival, authored containerField placement, idempotence) and requires that any oracle asserting them be proven to fail under injected faults before it is trusted."
tags: [adr, codecs, testing, roundtrip, fidelity]
updated: 2026-06-26
related:
  - ../architecture.md
---

# ADR-0036: Codec Round-Trip Fidelity Properties and Fault-Injected Oracles

## Status

Accepted — 2026-06-26.

## Context

The codecs (XML, X3D-JSON, ClassicVRML) form a round-trip lattice: any encoding parses to the
same in-memory `X3DDocument`, and any writer re-emits it. "It round-trips" is routinely asserted by
**idempotence** — `convert(convert(x)) == convert(x)` modulo whitespace. Idempotence is cheap and
necessary, but it is a *weak* oracle: a transform that is **stably wrong** (drops the same construct
on every pass, or migrates a node to the same wrong place every time) reaches a fixed point and
passes. Two real defects were found that idempotence alone could not see:

- a node DEF'd in one container of a parent and USE'd in another lost its **authored containerField
  placement** on round-trip (it fell back to static field-declaration order — graph-equivalent but
  an authored-intent infidelity; see the HAnim `skeleton`/`joints` case);
- an unrelated typing bug dropped open-vocabulary enumeration values.

A property fuzzer that round-tripped randomized DEF/USE/ROUTE/PROTO/HAnim scenes initially looked
reassuringly clean — but the cleanliness was partly an artifact: some generated constructs were
silently dropped at the first parse (so the compared artifact no longer contained them), and one
fidelity oracle was effectively tautological — it accepted "the DEF of the same name survived" as
proof that a **USE** survived, even though a USE and its DEF share a name by definition, so the check
passed even when the USE node itself had been deleted. An oracle that cannot fail proves nothing.

## Decision

**1. Name the fidelity properties a codec round-trip must preserve.** Beyond idempotence, a round-trip
(within one encoding, and across the full `XML→JSON→VRML→XML` cycle) must preserve:

- **P1 DEF survival** — every DEF reachable in scene scope survives (PROTO-body DEFs are scope-local,
  ISO/IEC 19775-1 §4.4.4.4, and are excluded).
- **P2 USE as a shared reference** — a USE stays a USE (§4.4.3: a shared instance, *not* a copy); it
  must not be silently expanded into a duplicate node nor dropped. The strong form asserts a single
  shared `shared_ptr` after the cycle, not merely that a `USE` token is present.
- **P3 ROUTE survival** — every ROUTE survives with all four endpoints intact.
- **P4 authored containerField placement** — a node whose authored placement is recorded survives into
  the same container, independent of field-declaration order.
- **P5 idempotence** — the encoding reaches a stable fixed point. Necessary, but never the *only*
  oracle for a given property.

**2. Pin them with deterministic tests.** P1–P5 are exercised by `roundtrip_test.cpp` (the full
cross-encoding cycle, parsing back at every hop so each reader — not just each writer — is on the
hook) and the HAnim container-order regression test. New codec work that could affect structure adds
a deterministic case here rather than relying on the fuzzer to rediscover it.

**3. Fault-injected oracle discipline.** Any oracle asserting a fidelity property — in a unit test or a
fuzzer — must be **proven to fail when the property is violated** before its green is trusted:

- ground truth comes from the **input** (the constructs authored into the scene), never re-derived
  from the implementation's own output (which makes the check `true == true`);
- the oracle is validated by **fault injection**: corrupt the converter's output (drop a DEF / a USE /
  a ROUTE / a containerField) and confirm the matching oracle goes red. An oracle that stays green
  under its corresponding fault is tautological and must be strengthened (this is how the USE-survival
  oracle's `or the DEF survived` escape hatch was found and removed);
- a fuzzer reports **coverage** (parse-success rate, constructs that actually survived to be checked,
  and scenes skipped past invariants) so a hollow run cannot masquerade as a clean one.

## Consequences

**Positive:**
- A vocabulary for codec fidelity beyond "it round-trips," with each property pinned by a fast,
  deterministic, CI-friendly test.
- Reader regressions (not just writer regressions) in any encoding are caught: the cross-encoding case
  parses JSON and ClassicVRML back, closing the writer-only blind spot in the prior test.
- Oracles carry their own anti-regression discipline — fault injection keeps them from rotting into
  tautologies.

**Trade-offs:**
- The randomized property fuzzer remains an opt-in discovery tool, not a CI gate (seed variance, needs
  a built binary). Its findings are banked as deterministic tests; it is not itself load-bearing.
- Fault-injection validation is extra up-front work per oracle, justified only for structural
  properties where a silently-passing check would hide real data loss.
