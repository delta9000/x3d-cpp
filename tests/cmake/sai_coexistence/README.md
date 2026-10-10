# Independent SAI / runtime consumer contract

This opt-in integration check requires two independent checkouts. It adds no
dependency to either production package and downloads nothing. Use an x3d-sai
revision whose generated metadata is private under `x3d_sai/detail`, with its
own namespace; older revisions exporting `x3d/core` or `x3d/nodes` intentionally
fail the boundary check.

Requirements: CMake 3.24+, a C++20 compiler, and Ninja (or set `CMAKE_GENERATOR`).
From the x3d-cpp root:

```bash
bash scripts/verify_sai_coexistence.sh /path/to/x3d-sai
# Also exercise static runtime symbols, rather than only shared libraries:
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_coexistence.sh /path/to/x3d-sai
```

The optional second argument is an **empty** work directory; otherwise a new
temporary directory is created. Builds/logs are retained at the printed path.
Set `CMAKE_BUILD_PARALLEL_LEVEL` to control compiler concurrency (default: 2).
Record both repository commit IDs with the result, since this is a paired check.

## What is checked

- Four source consumers use only `x3d_cpp::sdk` and `x3d::sai_metadata` usage
  requirements: runtime-first and SAI-first headers, independently crossed with
  runtime-first and SAI-first CMake link/include-search order
- Each consumer includes the SDK and legacy `SaiContext` headers, constructs a
  runtime `Transform`, calls its compiled reflection, and separately creates,
  edits and reads a typed SAI `Transform` through the compiled metadata bridge
- Private SAI metadata cannot be found on a downstream include path
- Both packages install into one prefix without private SAI headers or absolute
  source/build paths in their CMake exports
- The prefix is moved, leaving the original location absent; a copied consumer
  finds, builds and runs all four combinations against the relocated packages,
  with CMake package registries disabled

These are eight build-and-run checks per runtime linkage mode. The scene models
remain separate; the smoke test does not claim to translate runtime nodes into
SAI handles, connect event models, or certify standards conformance.

## Direct CMake use

The source consumer can also be used by an integration CI job that already has
authorized checkouts of both private repositories:

```bash
cmake -S tests/cmake/sai_coexistence -B build/coexistence -G Ninja \
  -DX3D_SAI_SOURCE_DIR=/path/to/x3d-sai
cmake --build build/coexistence --target sai_coexistence_all --parallel 2
ctest --test-dir build/coexistence --output-on-failure -R '^sai_coexistence_'
```

The complete script additionally checks installed relocation. Cross-repository
checkout credentials are an integration-runner concern; normal PR CI and normal
package builds do not gain a mandatory credential or sibling-repository fetch.
