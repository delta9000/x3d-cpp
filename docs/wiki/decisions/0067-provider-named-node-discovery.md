---
title: "ADR-0067: Provider named-node discovery"
summary: Read native DEF bindings through the portable provider contract.
tags: [adr, sai, provider]
updated: 2026-10-18
related:
  - ../subsystems/sai-provider.md
---

# ADR-0063: Provider named-node discovery

Proposed. CAVEOS retains an application name table to list SAI nodes and format
node references. Name lookup already belongs to the provider backend, so add
`do_named_nodes(scene)` to that contract and implement it from native
`Scene::defs`. The reference backend reads its snapshot bindings. Neither
backend creates another name registry.

The portable service exposes `named_nodes(context)` as owned `name`/`target`
pairs sorted by case-sensitive name, and `names(node)` as an owned string vector
filtered from the same enumeration. Detached named nodes appear; unnamed nodes
have no entries. The current contract still permits one canonical DEF name per
node. Discovery uses lexical context identity and does not flatten nested scopes
or introduce IMPORT aliases.

Both queries use existing owner-thread and handle validation in setup, active,
and faulted scenes. Retirement revokes node authority while returned strings
remain owned. The default backend virtual returns `unsupported_operation`, so
existing backend subclasses still compile from source. Frontends and backends
must be rebuilt together; this virtual addition offers no binary ABI promise.

The shared `check_provider_named_nodes` oracle runs through the existing
`sai_provider_reference` and `sai_provider_native_runtime` tests. Frontend
contract tests exercise unsupported and malformed backend results. These tests
are added as proposal evidence and have not been run here.
