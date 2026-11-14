---
title: Verify a change
summary: Run the gates that match what you changed before you open a pull request.
tags: [how-to, contributing, gates]
updated: 2026-11-14
related:
  - ../reference/gates.md
  - ../reference/build-and-tasks.md
  - ../sai/how-to/verify-a-change.md
---

# Verify a change

Work is: branch off `main`, make the change, get the fast gate green, update
the docs in the same diff, open a pull request. This guide picks the commands.
What each gate checks is in [Gate system](../reference/gates.md).

## 1. Build and run the fast gate

```sh
mise run build   # configure, build and ctest with the dev preset
mise run ci      # Python tests, conformance and doc checks, C++ contracts, corpus regressions, SAI gates
```

`mise run ci` mirrors the required CI gates. Get it green before opening the
pull request. Without mise, the plain CMake build works:

```sh
cmake -S . -B build -G Ninja && cmake --build build
```

## 2. Add the checks `ci` does not cover

| You changed | Also run |
|---|---|
| Risky C++ | `mise run ci-all` (adds sanitizers and the example renderers) |
| The public surface used by the example renderers | `mise run validate-examples` |
| Generator emitters or templates | [Regenerate the golden bindings](regenerate-goldens.md) |
| Generated header ownership, exported include paths or the SAI boundary | `bash scripts/verify_sai_coexistence.sh`, then again with `X3D_CPP_SHARED_NODES=OFF` in the environment |
| Anything under `sai/` or `adapters/sai_provider/` | [Verify a change (SAI)](../sai/how-to/verify-a-change.md) |
| Any wiki page | `mise run docs-build` |

## 3. Update the docs in the same diff

```sh
mise run docs-drift working
```

This lists pages your change may have made stale. It is advisory, and it needs
`X3D_QDRANT_URL`; see [Search with the RAG tools](search-with-the-rags.md).
Check the pages it flags, then update:

- the matching page under `reference/subsystems/` (a new subsystem needs a new
  page and a `mkdocs.yml` nav entry)
- `docs/conformance/findings.yaml`, then `mise run conformance`; never edit
  the generated view by hand
- [SDK capabilities](../reference/capabilities.md)
- the root `NOTICE`, if you added a third-party dependency or backend

## 4. Open the pull request

Fill in `.github/PULL_REQUEST_TEMPLATE.md`: a conformance claim you can point
at, a test that fails without your change (or a stated N/A), and the docs
update.

## Related

- [Build and mise tasks](../reference/build-and-tasks.md)
