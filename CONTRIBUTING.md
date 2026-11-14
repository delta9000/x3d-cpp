# Contributing to x3d-cpp

Branch off `main`, make the change, get `mise run ci` green, update the docs
in the same diff, and open a pull request. To-dos are plain GitHub issues.
Gaps against the X3D specification go in
[`docs/conformance/findings.yaml`](docs/conformance/findings.yaml) instead.

| Task | Guide |
|---|---|
| Build, run the gates and update the docs | [Verify a change](docs/wiki/how-to/verify-a-change.md) |
| Report a bug or a security problem | [Report a bug with a scene](docs/wiki/how-to/report-a-bug.md) |
| Know what a release may break | [API stability](docs/wiki/reference/api-stability.md) |
| Change the generator output | [Regenerate the golden bindings](docs/wiki/how-to/regenerate-goldens.md) |
| Write or move documentation | [Documentation conventions](docs/wiki/reference/doc-conventions.md) |
| Work on the SAI SDK | [Verify a change (SAI)](docs/wiki/sai/how-to/verify-a-change.md) |

The pull request checklist is
[`.github/PULL_REQUEST_TEMPLATE.md`](.github/PULL_REQUEST_TEMPLATE.md): a
conformance claim you can point at, a test that fails without your change (or
a stated N/A), and the docs updated in the same diff.
