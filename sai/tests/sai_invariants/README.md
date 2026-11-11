# C++ SAI semantic invariant suites

This directory is the landing zone for executable falsification tests named by
`docs/conformance/sai-invariants.yaml`. A semantic invariant is not considered
proved by a happy-path unit test: its suite must try to produce a counterexample
through generated graphs and adversarial operation sequences.

## Stable naming

Use the registry ID as the durable vocabulary and a lowercase CTest name:

| Registry family | CTest prefix | Example |
|---|---|---|
| `INV-ID-*` | `sai_id_` | `sai_id_shared_node` |
| `INV-OWN-*` | `sai_own_` | `sai_own_context_boundary` |
| `INV-FIELD-*` | `sai_field_` | `sai_field_units_once` |
| `INV-EDIT-*` | `sai_edit_` | `sai_edit_failure_nonmutation` |
| `INV-EVT-*` | `sai_evt_` | `sai_evt_observer_mutation` |
| all other families | `sai_<family>_` | `sai_rt_semantic` |

The YAML registry may use `planned:<name>` before a CTest exists. Removing the
prefix is a conformance claim: the normal gate then requires a literal CTest of
that name, and the strict gate requires executable coverage for every invariant.

## Shape of a falsification suite

Each suite should cover four layers:

1. A minimal example that communicates the invariant to an API user.
2. A deterministic regression for every known historical failure.
3. Property-generated scenes, values, aliases, routes, and context boundaries.
4. Adversarial sequences including failure, rollback, disposal, reentrancy,
   cancellation, serialization, and replay where relevant.

Tests should compare observable semantic traces and diagnostics, not private
container layout, pointer values, scheduler order, or serialized bytes unless
the invariant explicitly makes those observable. A failing generated case must
print its seed and a minimized operation trace so it can become a regression.

## Review rule

Changing invariant prose requires an explicit review-marker digest update in the
registry. Changing a generator or adversarial sequence requires retaining the
old counterexamples unless the invariant itself changed. This keeps the suites
monotonic: implementation refactors may broaden evidence but cannot quietly
weaken the semantic contract.
