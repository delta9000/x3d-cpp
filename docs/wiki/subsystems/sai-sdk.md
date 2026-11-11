---
title: SAI SDK (sai/)
summary: The portable C++20 SAI provider contract, its reference backend, kernel extensions, generator and service register.
tags: [subsystem, sai, provider, generator, conformance]
updated: 2026-11-11
related:
  - sai-provider.md
  - system-script-sai.md
  - ../decisions/0076-fold-sai-into-x3d-cpp.md
  - ../decisions/0047-sai-sister-repo-split.md
---

# SAI SDK (sai/)

`sai/` holds x3d-cpp's modern C++20 Scene Access Interface. Applications program
against one portable contract, `provider::service` in
`<x3d/sai/experimental/provider.hpp>`, and pick a backend at construction:

- the **reference backend** in `sai/` (`x3d::sai_reference_provider`), and
- the **native backend** in `adapters/sai_provider` (`x3d_cpp::sai_provider`),
  which runs a real `RuntimeSession`. See [Native SAI provider](sai-provider.md).

Both share handles, owning values, errors, update gates, receipts and
notification rules, and the same oracle (`testing/provider_fixture.hpp`) runs
against both. The SDK was developed in a separate `x3d-sai` repository from
2026-07-19 and folded back on 2026-11-11 ([ADR-0076](../decisions/0076-fold-sai-into-x3d-cpp.md)).

## Layout

| Path | Role |
|---|---|
| `sai/include/x3d/sai/experimental/` | Public headers: `provider.hpp` (contract), `reference_provider.hpp`, `types.hpp`, `schema.hpp`, and reference-only `kernel.hpp`, `session.hpp`, `browser_services.hpp`, `matrix.hpp`, `image.hpp`, `user_data.hpp` |
| `sai/src/` | Frontend (`provider.cpp`), reference backend, kernel, session and browser services |
| `sai/src/x3d_sai_gen/` | Python generator (uv package `x3d-sai-gen`) reading its own SHA-pinned UOM 4.0 |
| `sai/generated_cpp_bindings/` | Generated field keys, typed bindings and the semantic metadata catalog |
| `sai/tests/` | doctest suites registered as CTests, generator pytest |
| `sai/docs/` | Contract, runtime, kernel guide and topic pages; `docs/conformance/` holds the register |
| `sai/docs/plans/` | Frozen archive of the 2026-07-18/19 plans |

## CMake targets

| Target | Role |
|---|---|
| `x3d::sai` (alias `x3d::sai_provider`) | Provider-neutral frontend and generated field keys |
| `x3d::sai_reference_provider` | Reference backend behind that frontend |
| `x3d::sai_provider_fixture` | Shared behavioural oracle for backends |
| `x3d::sai_reference_kernel`, `x3d::sai_reference_metadata` | Reference-only snapshots, atomic editors, kernel sessions and the generated catalog |

`sai/` is its own CMake project (`find_package(x3d_sai)` after install, or
`add_subdirectory(sai)`). The runtime's own build never adds it, and the SDK has
no build, link or Python dependency on the runtime. The composition boundary from
ADR-0047's 2026-10-07 update still applies: SAI headers live under
`x3d/sai/experimental`, never under the runtime's `x3d/core` or `x3d/nodes`.

## What it covers

Each backend advertises what it supports through `service.supported()`. Both
current backends construct Transform, Shape, Box and PositionInterpolator, and
the contract defines scene authoring, host turns, notifications, node values,
indexed writes, user data, scene units, scene metadata and live ROUTE services,
plus reference-only snapshot, transaction and checkpoint services. The native
backend can also adopt a loaded session and expose its nodes for discovery and
supported field kinds; PROTO instances, Inline, IMPORT and Script stay opaque.

The [service register](https://github.com/delta9000/x3d-cpp/blob/main/sai/docs/conformance/SAI-BASELINE.md)
maps the ISO/IEC 19775-2 services: on 2026-11-11 it lists 88 services, 79
partial and 9 planned, and 49 of 91 catalog clauses mapped. "Partial" means
working, tested behaviour whose surface or semantics are not yet the whole ISO
service. Row counts measure inventory, not a conformance percentage.

Start with the
[portable contract](https://github.com/delta9000/x3d-cpp/blob/main/sai/docs/portable-contract.md),
then the [SDK README](https://github.com/delta9000/x3d-cpp/blob/main/sai/README.md)
and the [reference-kernel guide](https://github.com/delta9000/x3d-cpp/blob/main/sai/docs/reference-kernel-guide.md).

## Gates

```sh
mise run sai-ci        # UOM pin, generator determinism, registers, pytest, build, ctest
mise run sai-provider  # native backend vs sai/: parity, relocated install, host consumer
mise run sai-conformance  # opt-in: lists services/tests still to finish; non-zero until none
```

`mise run ci` includes `sai-ci` and `mise run ci-all` includes `sai-provider`.
In GitHub CI the `sai` job runs the first and the `sai-adapter` job runs the
second on any C++ or SAI change. After editing a register YAML, run
`mise run sai-baseline` and commit the regenerated baseline.

The runtime's Script nodes use a different, in-process API,
[`SaiContext`](system-script-sai.md). Its register is
`docs/conformance/sai-services.yaml` (`mise run sai-services`).
