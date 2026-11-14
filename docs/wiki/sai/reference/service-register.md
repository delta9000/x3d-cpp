---
title: SAI service register
summary: The SAI SDK's service and invariant registers, their statuses, current counts and the tasks that check them.
tags: [sai, reference, conformance]
updated: 2026-11-14
related:
  - ../how-to/update-the-service-register.md
  - ../explanation/iso-binding.md
---

# SAI service register

The register tracks each ISO SAI service the SDK maps, with the ISO clause
behind it and the tests that back it. To change it see
[Update the service register](../how-to/update-the-service-register.md).

## Files

All under `sai/docs/conformance/`:

| File | Role | Edited by |
|---|---|---|
| `sai-services.yaml` | One row per service: clause, profiles, support, C++ surface, tests, preserved invariants | Hand |
| `sai-invariants.yaml` | Semantic invariants grouped under axioms, each naming its falsification tests | Hand |
| `sai-service-catalog.yaml` | The normative clause catalog the services map onto | Hand |
| `SAI-BASELINE.md` | Generated view of the above | `mise run sai-baseline`; never by hand |

Read the generated view on GitHub:
[SAI-BASELINE.md](https://github.com/delta9000/x3d-cpp/blob/main/sai/docs/conformance/SAI-BASELINE.md).

## A different register from the runtime's

`docs/conformance/sai-services.yaml` (checked by `mise run sai-services`) is
the *runtime's* Script-SAI register for
[`SaiContext`](../../reference/subsystems/system-script-sai.md). The two registers stay
separate.

## Support values

| Value | Meaning |
|---|---|
| `implemented` | Complete portable SAI behaviour with executable coverage |
| `partial` | Useful SDK behaviour exists, but its surface or semantics do not yet satisfy the complete abstract service |
| `planned` | No suitable public semantic service yet; may name a `planned:` C++ surface |
| `unsupported` | Intentionally unavailable; must carry a reason in `errors` |

A test name may be written `planned:<name>` before the CTest exists. Removing
the prefix is a conformance claim: the normal gate then requires a literal
CTest of that name.

## Counts on 2026-11-14

| Measure | Value |
|---|---|
| Services | 88: 79 partial, 9 planned |
| Services by profile | authoring 42, browser-adapter 25, inspection 49, live 51 |
| Semantic invariants | 35 under 6 axioms |
| Normative catalog clauses mapped | 49 of 91 |
| C++ binding obligations mapped | 14 of 16 (audit incomplete) |

Row counts and catalog mappings measure inventory coverage, not an ISO
conformance percentage. Rows may include explicitly labelled SDK policy
extensions.

## Checks

| Task | Checks | In `sai-ci`? |
|---|---|---|
| `mise run sai-conformance-gate` | Service and invariant registers, cross-links, CTest names, generated baseline drift | Yes |
| `mise run sai-invariants` | Invariant registry, service cross-links, generated coverage view | Yes |
| `mise run sai-baseline` | Regenerates `SAI-BASELINE.md` | No (run after editing a YAML) |
| `mise run sai-conformance` | Opt-in convergence report: lists every service and falsification test still to finish; exits non-zero until none remain | No |

The normal checks validate consistency and drift. They do not require every
ISO service or C++ binding obligation to be mapped; the baseline lists those
gaps, and only the strict report requires complete coverage. A green strict
report alone would still not prove backend substitutability or ISO
conformance.

## Invariant test naming

| Registry family | CTest prefix | Example |
|---|---|---|
| `INV-ID-*` | `sai_id_` | `sai_id_shared_node` |
| `INV-OWN-*` | `sai_own_` | `sai_own_context_boundary` |
| `INV-FIELD-*` | `sai_field_` | `sai_field_units_once` |
| `INV-EDIT-*` | `sai_edit_` | `sai_edit_failure_nonmutation` |
| `INV-EVT-*` | `sai_evt_` | `sai_evt_observer_mutation` |
| All other families | `sai_<family>_` | `sai_rt_semantic` |

Use the registry ID as the durable vocabulary and a lowercase CTest name.

## Editions

Which ISO editions the catalog targets, and why the C++ binding audit is marked
incomplete, is explained in
[Relationship to the ISO C++ binding](../explanation/iso-binding.md).
