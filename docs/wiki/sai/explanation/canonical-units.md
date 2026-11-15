---
title: Canonical units in the SAI SDK
summary: Why provider values are always canonical SI and what UNIT declarations do and do not change.
tags: [sai, explanation, units]
updated: 2026-11-14
related:
  - ../reference/scene-information.md
---

# Canonical units in the SAI SDK

## The rule

Every provider field value is in canonical SI units: authoring, readback,
setup and live inputs, event values, interpolation outputs and extracted
geometry. Time is always seconds.

A `declare_unit` call records what units the *document* uses. It does not
rescale any value that crosses the provider API.

## Why

One conversion boundary is easier to reason about than many. If declarations
rescaled API values, the same `author` call would mean different things
depending on setup order, and ROUTEs between fields would need to know which
space each side was in. Keeping storage, ROUTEs and change records in canonical
space removes that whole class of error.

## What the native backend does

Parsed documents go through unit normalization when a session activates. The
native backend marks provider-authored scalar fields as already normalized,
including on detached nodes, so activation does not convert them a second
time. Generated defaults and live writes stay canonical.

## The serializer caveat

There is no authored-space input, parser or serializer in this slice. A future
serializer has to do one of two things:

- inverse-convert the relevant canonical values, or
- adjust the UNIT declarations.

Writing canonical bytes beside non-identity UNIT statements would change the
physical scene when it is parsed again. The native diagnostic scene access is
not a promise of lossless source-unit document serialization.

## The reference kernel is different

The reference kernel's own snapshot and editor API has an explicit
authored-space option: `read(field, value_space::authored)` converts on
request, and values cross one explicit conversion boundary while storage stays
canonical. That API is separate from the provider's rule above. See [Reference kernel](../reference/kernel.md).

The exact operations are in
[Scene units and metadata](../reference/scene-information.md).
