---
title: Update the SAI service register
summary: Change a service or invariant row, regenerate the baseline and pass the register gates.
tags: [sai, how-to, conformance]
updated: 2026-11-14
related:
  - ../reference/service-register.md
---

# Update the SAI service register

File roles, support values and naming rules are in
[Service register](../reference/service-register.md).

## Steps

1. Edit the YAML under `sai/docs/conformance/`:

    | Change | File |
    |---|---|
    | A service's support, C++ surface, tests or clause | `sai-services.yaml` |
    | An invariant or its falsification tests | `sai-invariants.yaml` |

2. If you cite a test, make sure a CTest with that literal name exists in
   `sai/CMakeLists.txt`. Use `planned:<name>` for a test you have not written.

3. Regenerate the baseline:

    ```sh
    mise run sai-baseline
    ```

4. Check consistency:

    ```sh
    mise run sai-conformance-gate
    mise run sai-invariants
    ```

5. Commit the YAML and `SAI-BASELINE.md` together. Never edit the generated
   file by hand.

## Promote a service

Changing `partial` to `implemented`, or removing a `planned:` prefix, is a
conformance claim. Before you do it:

- Read the code and confirm the whole abstract service is wired, not only the
  part the tests touch.
- Make sure every cited test exists and exercises the claim.
- Describe the published ISO text neutrally. Where the SDK reads it
  differently, record that as the SDK's choice.

## Add an invariant test

1. Name the CTest from the invariant family, for example `INV-EDIT-*` becomes
   `sai_edit_<name>`.
2. Put the suite under `sai/tests/` and register it in `sai/CMakeLists.txt`.
3. Cover the four layers listed under
   [Invariant suites](../reference/test-suites.md#invariant-suites).
4. Changing an invariant's prose needs a review-marker digest update in the
   registry.

## See how far there is to go

```sh
mise run sai-conformance
```

This opt-in report lists every service and falsification test still to finish.
It exits non-zero until none remain, so it is not part of CI.

## Not this register

`docs/conformance/sai-services.yaml` belongs to the runtime's Script
`SaiContext` and is checked by `mise run sai-services`.
