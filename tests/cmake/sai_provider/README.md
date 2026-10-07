# Independent offline SAI provider pilot

This **experimental C++20 source-level review proposal** maps x3d-sai's bounded
provider interface onto actual x3d-cpp Scene/X3DNode state. It is not the legacy
Script `SaiContext`, a replacement for x3d-sai's current concrete handles, an ABI
promise, or complete ISO SAI conformance.

## Run the paired evidence

Requirements: provider-enabled x3d-sai checkout, CMake 3.24+, Ninja, and a C++20
compiler. The check fetches no code and changes neither production package's
normal dependencies.

```sh
bash scripts/verify_sai_provider.sh /path/to/x3d-sai
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_provider.sh /path/to/x3d-sai
```

An optional second argument selects an empty evidence/build directory. Compiler
concurrency is `CMAKE_BUILD_PARALLEL_LEVEL` (default 2). The script retains build
outputs and the native-only link command. Record both repository revisions and
compiler/linkage details in review evidence.

Each run configures independent source consumers and copied consumers of a
relocated installation, with package registries disabled. Both run:

1. The exact shared `testing/provider_fixture.hpp` against the reference kernel
2. That same oracle against the native provider, plus native-only assertions
3. Direct equality of both providers' complete fixture reports

The native-only binary includes neither kernel nor metadata headers and its
link command is checked to contain no SAI reference kernel/metadata library.
The parity binary deliberately links both implementations; it is not the
independence assertion. All checks remain active under Release/NDEBUG.

## Supported mapping and authority

The common contract is documented in x3d-sai's `docs/provider-pilot.md`.
`x3d::runtime::SaiOfflineProvider` creates native Transform objects through
`createX3DNode`. Its identity registry retains created/detached nodes, but stores
no mirrored fields, roots or name bindings. `Scene::rootNodes` and `Scene::defs`
are authoritative; native `FieldInfo` reflection supplies the ordered semantic
field definitions. DEF/USE/IS and host class/id/style syntax are filtered out.

SFVec3f reads inspect current native fields. Supported inputOutput writes call
the generated checked Transform setters and record native authored-field
presence, rather than using the lenient unchecked reflection-write path. DEF
creation updates both the native Scene table and node's DEF. Root insertion uses
`Scene::addRootNode`; duplicate occurrences share the same shared_ptr.

`Transform.children` supports owner-bearing `read_nodes` / `set_nodes`, both by
field name and by the existing generated `Transform::children` key. Reads inspect
the real `getChildren()` pointer sequence and writes publish one complete native
`setChildren()` vector. Order and explicit NULL slots are preserved. A non-NULL
node can occur only once in a single children list (`invalid_value` on a repeat),
but the same identity can be shared by different parents. Repeated scene-root
occurrences remain legal. Children never enter `Scene::authoredScalarFields`.

Each node-valued read reconstructs and validates the full registered native
children graph, including unnamed detached nodes, before returning owned
handles. Foreign native pointers fail with `invalid_context`, same-list duplicate
non-NULL entries with `invalid_value`, and self/indirect cycles with
`containment_cycle`. Iterative traversal avoids recursion-depth limits. Native
out-of-band structural edits are visible immediately and fail closed if invalid;
an unrelated invalid component cannot be hidden by querying a valid node.

Each write resolves the owner-checked payload to native pointers and validates a
candidate graph with the target list replaced, before making any native change.
This allows the offending target list to be repaired when the entire candidate
graph is valid. An invalid list elsewhere still rejects the operation. Ordinary
returned errors leave native fields, roots, names and authored-scalar marks
unchanged; allocation/system exceptions are outside this guarantee. Validation
uses temporary local state, not a persistent mirrored graph or reference-kernel
adapter.

The native-only proof confirms:

- Native DEF and repeated root occurrences resolve to the same actual Transform
- Adapter writes are immediately visible through that Transform's native getter
- Serial native setter writes are immediately visible through adapter reads,
  without a synchronization/copy step
- Thread guards apply to native extension access; close expires SAI wrappers
  even while callers retain the native Scene and its nodes
- Owned MFNode writes install the exact real native pointer sequence, and native
  list edits are visible in dynamic/generated reads without synchronization
- Foreign pointers, duplicates and detached/self/indirect cycles fail closed,
  candidate repair succeeds only for a valid full graph, and errors preserve state
- A 4,096-node unnamed detached chain and cycle exercise iterative validation;
  tests explicitly remove any installed shared_ptr cycles, even on failure

`native_scene()` is a provider-specific inspection/authoring extension, not
part of the portable oracle. Native accesses must be serialized on the creator
thread. It exposes this provider's own offline scene, not an existing live
execution context or extractor. Direct native mutation of existing Transform
fields is supported. Adding/replacing nodes outside the provider's identity
registry is outside the pilot; relevant queries fail with `invalid_context` for
such foreign entries. Children operations validate all registered containment
lists, but do not validate unrelated native structures such as ROUTEs or imports.
Callers using other native structural operations must preserve native Scene
invariants themselves. This is not a live graph-editing integration,
and no derived runtime/extractor state is claimed to be synchronized.

The narrow common interface leaves initialize-only authoring, node-valued fields
beyond `Transform.children` (including SFNode and inputOnly add/removeChildren),
full field kinds, all profiles/components, generated typed handles,
full lifecycle, snapshots/transactions, events/ROUTEs, update buffering, loading,
world replacement, import authority, concurrent access and rendering unproven.
Reference-kernel-only tests do not fill these independent-provider gaps.

## Optional companion package

`adapters/sai` is configured explicitly, either with source targets already in
scope or with installed dependencies. Ordinary x3d-cpp builds do not enter that
directory and acquire no new dependency.

```sh
cmake -S adapters/sai -B build/sai-adapter \
  -DCMAKE_PREFIX_PATH="/prefix/with/x3d_cpp;/prefix/with/x3d_sai"
cmake --build build/sai-adapter
cmake --install build/sai-adapter --prefix /adapter/prefix
```

Installed consumers use:

```cmake
find_package(x3d_cpp_sai CONFIG REQUIRED)
target_link_libraries(application PRIVATE x3d_cpp::sai_provider)
```

The companion exports only the adapter target and public
`<x3d/sai_provider.hpp>` header, under x3d-cpp's isolated include prefix. It links
`x3d_cpp::authoring` and header-only `x3d::sai_provider`, without the reference
kernel or metadata. No generated support headers are copied between projects.
The dependency packages supply their own licenses; no new third-party source is
vendored by this adapter.
