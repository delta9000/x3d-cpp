---
title: "ADR-0076: Fold the SAI SDK back into x3d-cpp"
summary: The SAI SDK returns from the x3d-sai repository to sai/, keeping its own CMake project, generator and register.
tags: [adr, sai, repository, build]
updated: 2026-11-11
related:
  - 0047-sai-sister-repo-split.md
  - ../subsystems/sai-sdk.md
  - ../subsystems/sai-provider.md
---

# ADR-0076: Fold the SAI SDK back into x3d-cpp

- Status: Accepted
- Date: 2026-11-11
- Supersedes: [ADR-0047](0047-sai-sister-repo-split.md) (repository split and re-entry trigger)

## Context

[ADR-0047](0047-sai-sister-repo-split.md) moved the SAI semantic kernel into a private
`delta9000/x3d-sai` repository and set its re-entry trigger at the strict convergence gate
going green. Since then the work changed shape:

- The kernel became one portable `provider::service` contract with two backends. The
  native backend, `adapters/sai_provider`, has always lived in x3d-cpp and drives a real
  `RuntimeSession` (adoption, live ROUTEs, named-node discovery, a presentation feed).
- Most new SAI work touches both repositories at once. ADR-0074 and ADR-0075 each needed
  a frontend change in x3d-sai and a backend change here, merged in order.
- The split let the pair drift. The 2026-11-11 toolkit review found `adapters/sai_provider`
  on x3d-cpp `main` did not build against x3d-sai `main`, and no CI job built the pair.
  The fix in flight (PR #187) pins an x3d-sai revision and needs a read token for the
  private repository in CI.

The strict gate is a measure of how much of ISO SAI is covered, not of whether the code
belongs here. It still fails (0 of 88 services fully implemented, 79 partial, 9 planned),
and it is not a sensible precondition for sharing one tree with the backend that
implements the contract.

## Decision

The SAI SDK moves back into this repository as `sai/`, with its full history (a
`git subtree` merge of x3d-sai `main` at `4dfb73c`).

- `sai/` stays its own CMake project and uv package (`x3d-sai-gen`). It still builds and
  installs on its own and still has no build-, link- or Python-time dependency on the
  runtime. The runtime build does not add it. `adapters/sai_provider` stays the only
  bridge, and the composition rules from ADR-0047's 2026-10-07 update (header paths,
  link orders, relocated prefixes) still hold.
- The two generators stay separate for now. Reconciling them is optional follow-up work,
  not part of this move.
- The two service registers stay separate: `sai/docs/conformance/` for the SDK, and
  `docs/conformance/sai-services.yaml` for the runtime's Script SAI. Each cites tests its
  own build can resolve.
- CI gates both: the `sai` job runs the SDK's own checks (`mise run sai-ci`), and the
  `sai-adapter` job runs `scripts/verify_sai_provider.sh` against `sai/` on every C++ or
  SAI change. No revision pin or cross-repository token is needed.
- `mise run ci` includes `sai-ci`; `mise run ci-all` includes `sai-provider`.
- The strict convergence gate (`mise run sai-conformance`) remains an opt-in report of
  remaining SAI work. It is no longer a re-entry condition.

## Consequences

- The SAI code is now in this repository's visibility rather than the private one.
- One pull request can change the contract and the native backend together, and CI
  checks them together.
- `x3d-sai` is retired. Its open work moves here; the repository can be archived.
- PR #187's x3d-sai pin (`adapters/sai_provider/x3d-sai.rev`) and token-gated
  `sai-adapter` job are superseded by the in-tree job. Whichever of #187 and this change
  merges second drops them.
- `sai/docs/plans/` joins `docs/plans/` as a frozen archive.

## Alternatives rejected

**Keep the split and pin.** Works, but every contract change still needs two ordered
merges, and CI needs a long-lived token for a private repository.

**Merge `sai/` into the runtime's CMake project and generator now.** That is the
reconciliation ADR-0047 anticipated, and it is a large, separate change. Moving the
tree first keeps this step mechanical and reviewable.
