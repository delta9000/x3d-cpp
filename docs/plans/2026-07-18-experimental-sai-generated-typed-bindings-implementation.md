# Experimental SAI Generated Typed Bindings Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make ordinary generated SAI authoring use typed node handles and generated field keys without field-name strings, dynamic lookup, or `.as<T>()` layers.

**Architecture:** Add a small typed-node/key vocabulary over the existing semantic handles, then generate owner-specific field keys from the same resolved UOM descriptors as the metadata catalog. Dynamic inspection remains unchanged and fallible; generated field acquisition is infallible, while create/read/write/commit preserve their existing expected-style runtime failures.

**Tech Stack:** C++20, `tl::expected`, Python 3.11 generator, Jinja-free deterministic emitters, generated UOM metadata, CMake/CTest, doctest, pytest, clang-format, YAML conformance registers.

---

### Task 1: Prove the typed-node and field-key semantic shape

**Files:**
- Modify: `experimental/sai/include/x3d/sai/experimental/kernel.hpp`
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing compile-and-behavior test**

Add local schema tags to `semantic_kernel_test.cpp`:

```cpp
struct BoundTransform {
  static constexpr std::string_view x3d_name = "Transform";
  inline static constexpr sai::field_key<BoundTransform, sai::vec3f>
      translation{"translation", sai::access_type::input_output};
};

struct BoundGroup {
  static constexpr std::string_view x3d_name = "Group";
  inline static constexpr sai::field_key<BoundGroup, sai::node_list>
      children{"children", sai::access_type::input_output};
};

template <class Node, class Key>
concept accepts_generated_key = requires(const Node &node, const Key &key) {
  node.field(key);
};
```

Add a test named `generated field keys remove dynamic authoring lookup` that:

1. statically rejects `BoundTransform::translation` on
   `typed_node<BoundGroup>`;
2. creates `BoundTransform` through `scene_edit::create<BoundTransform>()`;
3. obtains `field<vec3f>` directly from the typed node;
4. sets, commits, and reads it;
5. discovers the same field dynamically and proves value/identity parity;
6. proves a stale generated field fails at `set`, not at acquisition.

**Step 2: Run the test to verify RED**

Run:

```sh
cmake --build build-sai-ci --target x3d_sai_experimental_tests -j2
```

Expected: compilation fails because `field_key`, `typed_node`, and
`scene_edit::create<Tag>()` do not exist.

**Step 3: Implement the minimal generic vocabulary**

In `kernel.hpp`, add:

```cpp
template <class Owner, class T> class field_key {
public:
  std::string_view name() const noexcept { return name_; }
  static constexpr value_kind kind = value_traits<T>::kind;
  access_type access() const noexcept { return access_; }

private:
  constexpr field_key(std::string_view name, access_type access)
      : name_(name), access_(access) {}
  std::string_view name_;
  access_type access_;
  friend Owner;
  template <class> friend class typed_node;
};
```

Add `typed_node<Tag>` as a thin wrapper around `node`. Its constructor is
private and friended only to checked SAI factories. Its only Phase 1 additions
are `id()`, `dynamic()`, and:

```cpp
template <class T>
experimental::field<T> field(const field_key<Tag, T> &key) const noexcept;
```

The method constructs the existing `dynamic_field` from the wrapped semantic
handle plus the generated name/kind/access and returns `field<T>` directly. Add
the required template friendship to `node`, `dynamic_field`, and `field<T>`;
do not add an unchecked public conversion from `node`.

Add this overload to `scene_edit`:

```cpp
template <class Tag> result<typed_node<Tag>> create() {
  return create_node(std::string{Tag::x3d_name})
      .transform([](node created) {
        return typed_node<Tag>{std::move(created)};
      });
}
```

**Step 4: Run focused tests to verify GREEN**

Run:

```sh
cmake --build build-sai-ci --target x3d_sai_experimental_tests -j2
./build-sai-ci/x3d_sai_experimental_tests \
  --test-case='generated field keys remove dynamic authoring lookup,dynamic discovery and typed fields are substitutable'
```

Expected: both tests pass; the new test contains no `edit.field(..., string)`
or `.as<T>()` in its generated lane.

**Step 5: Register and commit the behavior**

Register `sai_generated_typed_access` in `CMakeLists.txt` with the `behavior`
label, then run:

```sh
ctest --test-dir build-sai-ci -R '^sai_generated_typed_access$' --output-on-failure
git add CMakeLists.txt experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/tests/semantic_kernel_test.cpp
git commit -m "feat(sai): add typed node field keys"
```

### Task 2: Generate exact SAI node bindings from the UOM

**Files:**
- Create: `src/x3d_cpp_gen/emit/sai_bindings.py`
- Create: `tests/test_sai_bindings.py`
- Modify: `src/x3d_cpp_gen/emit/semantic_metadata.py`

**Step 1: Write failing emitter tests**

In `tests/test_sai_bindings.py`, use the existing parsed `model` fixture and
assert that emitting `Transform` produces:

```cpp
struct Transform {
  static constexpr std::string_view x3d_name = "Transform";
  inline static constexpr field_key<Transform, vec3f> translation{
      "translation", access_type::input_output};
};
```

Also test:

- `Group.children` maps `MFNode` to `node_list`;
- `PixelTexture.image` maps `SFImage` to `image`;
- bounded SF/MF string enums map to `enum_value`/`enum_list`;
- `class` uses the shared `class_` C++ sanitizer while retaining wire name
  `"class"`;
- all 44 SF/MF kinds have exactly one owning-type mapping;
- an unknown kind and duplicate sanitized field name fail generation.

**Step 2: Run the tests to verify RED**

Run:

```sh
env UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache \
  uv run pytest tests/test_sai_bindings.py -q
```

Expected: import failure for `x3d_cpp_gen.emit.sai_bindings`.

**Step 3: Implement a deterministic, fail-closed emitter**

Create `sai_bindings.py` with:

- an exhaustive `X3DType -> experimental owning C++ type` table;
- enum overrides to `enum_value` and `enum_list`;
- canonical `value_kind` and `access_type` spellings;
- `resolved_binding_fields(node, nodes, graph, enum_defs)` built through the
  same `build_reflection_descriptors` path used by semantic metadata;
- `gen_sai_node_binding(...)` for one owner-specific complete key set;
- `gen_sai_bindings_catalog(...)` carrying specification version, model
  fingerprint, and generator version.

Extract only the shared descriptor-resolution helper needed by both emitters
from `semantic_metadata.py`; do not duplicate inheritance/phantom-field rules.
Render with line-oriented Python like the other registry emitters, not a new
template dependency.

**Step 4: Run emitter and existing metadata tests**

Run:

```sh
env UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache uv run pytest \
  tests/test_sai_bindings.py tests/test_interface_registry.py \
  tests/test_descriptors.py -q
```

Expected: all tests pass and generated output is byte-deterministic across two
calls.

**Step 5: Commit the emitter**

```sh
git add src/x3d_cpp_gen/emit/sai_bindings.py \
  src/x3d_cpp_gen/emit/semantic_metadata.py tests/test_sai_bindings.py
git commit -m "feat(generator): emit experimental SAI bindings"
```

### Task 3: Integrate and regenerate the committed binding catalog

**Files:**
- Modify: `src/x3d_cpp_gen/generator.py`
- Modify: `src/x3d_cpp_gen/cli.py`
- Modify: `tests/test_emission.py`
- Create: `generated_cpp_bindings/x3d/sai/experimental/bindings/*.hpp`
- Create: `generated_cpp_bindings/x3d/sai/experimental/X3DSAIBindings.hpp`

**Step 1: Write the failing integration test**

Extend `tests/test_emission.py` to generate into `tmp_path` and assert:

- `x3d/sai/experimental/bindings/Transform.hpp` exists;
- `X3DSAIBindings.hpp` carries the same SHA-256 model fingerprint as
  `X3DSemanticMetadataRegistry`;
- the generated smoke-test include graph can find the experimental kernel only
  when SAI bindings are explicitly compiled, keeping runtime node generation
  independent.

**Step 2: Run the integration test to verify RED**

```sh
env UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache \
  uv run pytest tests/test_emission.py -q
```

Expected: generated SAI binding files are absent.

**Step 3: Wire generation into the CLI**

Add `write_sai_bindings(...)` to `generator.py`. It creates
`x3d/sai/experimental/bindings`, writes one lightweight header per node, and
writes the catalog header. Call it from `cli.py` after semantic metadata
generation. Feed all emitted headers through the pinned clang-format pass.

The generated headers include
`x3d/sai/experimental/kernel.hpp`; they never include `x3d/nodes/*.hpp`.

**Step 4: Regenerate and run drift tests**

Run:

```sh
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run gen
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run golden
```

Expected: only the new SAI generated tree and intentional generator changes
appear; golden drift passes.

**Step 5: Commit generated output**

```sh
git add src/x3d_cpp_gen/generator.py src/x3d_cpp_gen/cli.py \
  tests/test_emission.py generated_cpp_bindings/x3d/sai
git commit -m "chore(generator): generate experimental SAI bindings"
```

### Task 4: Prove generated-key and metadata parity in C++

**Files:**
- Modify: `experimental/sai/tests/semantic_kernel_test.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/conformance/sai-invariants.yaml`
- Regenerate: `docs/conformance/SAI-BASELINE.md`

**Step 1: Write the failing parity test**

Include generated bindings for `Transform`, `Group`, `PixelTexture`,
`Coordinate`, and `WorldInfo`. Add a test named
`generated typed keys and metadata are one schema view` that compares every
selected generated key's wire name, `value_kind`, and `access_type` with the
same ordered field in `generated_metadata_catalog()`.

Use concepts/static assertions to prove:

```cpp
static_assert(accepts_generated_key<typed_node<bindings::Transform>,
                                    decltype(bindings::Transform::translation)>);
static_assert(!accepts_generated_key<typed_node<bindings::Group>,
                                     decltype(bindings::Transform::translation)>);
```

Then author and dynamically rediscover at least one field of every selected
owning value family.

**Step 2: Run the test to verify RED**

```sh
cmake --build build-sai-ci --target x3d_sai_experimental_tests -j2
```

Expected: compile or parity failure until the generated include paths and exact
emitter output are wired correctly.

**Step 3: Add only the required build and evidence wiring**

Expose `${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings` to the experimental
test target through its existing metadata dependency. Register
`sai_generated_binding_parity` as a stable behavior CTest. Add it as executable
evidence for `INV-GEN-1`, but keep that invariant open if the test samples
rather than exhausts the generated catalog.

Regenerate the baseline:

```sh
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run sai-baseline
```

**Step 4: Run parity and conformance gates**

```sh
ctest --test-dir build-sai-ci \
  -R '^sai_generated_(typed_access|binding_parity)$' --output-on-failure
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run sai-conformance-gate
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run sai-invariants
```

Expected: all pass; the baseline reports only evidence actually exercised.

**Step 5: Commit parity evidence**

```sh
git add CMakeLists.txt experimental/sai/tests/semantic_kernel_test.cpp \
  docs/conformance/sai-invariants.yaml docs/conformance/SAI-BASELINE.md
git commit -m "test(sai): prove generated binding parity"
```

### Task 5: Replace the ceremonial example with generated authoring

**Files:**
- Create: `experimental/sai/examples/generated_author_inspect.cpp`
- Modify: `CMakeLists.txt`
- Modify: `experimental/sai/README.md`
- Modify: `docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md`

**Step 1: Write the generated-only example as a failing user story**

Use generated bindings and `generated_type_registry()` for:

- `Transform.translation` (`SFVec3f`);
- `Transform.children` or `Group.children` (`MFNode`);
- `Coordinate.point` (`MFVec3f`);
- `WorldInfo.title`/`info` (`SFString`/`MFString`);
- `PixelTexture.image` (`SFImage`);
- `Shape.geometry` (`SFNode`).

The authoring lane must contain no handwritten `field_descriptor`, field-name
string passed to `edit.field`, or `.as<T>()`. It must use expected-style
composition for at least one create/edit chain, commit once, and inspect the
same generated fields from a snapshot.

Declare a scene unit to exercise ordered unit metadata, but explicitly do not
claim generated field conversion because the UOM lacks complete per-field unit
categories.

**Step 2: Build to verify RED**

```sh
cmake --build build-sai-ci --target \
  x3d_sai_experimental_generated_example -j2
```

Expected: target or required typed operation is missing.

**Step 3: Wire the example without adding semantics**

Add `x3d_sai_experimental_generated_example`, link it to
`x3d_sai_experimental_metadata`, and register a behavior CTest named
`sai_generated_author_inspect`. Do not make the firewalled core depend on the
generated runtime node library.

Update the README to make the generated example the primary authoring story;
retain `author_inspect.cpp` as the dynamic/handwritten semantic fixture only if
it still proves a distinct invariant.

**Step 4: Build and run the composed story**

```sh
cmake --build build-sai-ci --target \
  x3d_sai_experimental_generated_example -j2
ctest --test-dir build-sai-ci \
  -R '^sai_generated_author_inspect$' --output-on-failure
```

Expected: the example exits successfully and the CTest passes.

**Step 5: Commit the usability gate**

```sh
git add CMakeLists.txt experimental/sai/examples/generated_author_inspect.cpp \
  experimental/sai/README.md \
  docs/plans/2026-07-18-experimental-sai-remaining-roadmap-implementation.md
git commit -m "feat(sai): demonstrate generated typed authoring"
```

### Task 6: Run the Phase 1 stop/go gate

**Files:**
- Modify only if verification exposes an in-scope defect.

**Step 1: Run formatting and diff hygiene**

```sh
clang-format -i \
  experimental/sai/include/x3d/sai/experimental/kernel.hpp \
  experimental/sai/tests/semantic_kernel_test.cpp \
  experimental/sai/examples/generated_author_inspect.cpp
git diff --check
```

Expected: clean.

**Step 2: Run focused and full SAI behavior**

```sh
cmake --build build-sai-ci --target \
  x3d_sai_experimental_tests x3d_sai_experimental_generated_example -j2
ctest --test-dir build-sai-ci -R '^sai_' --output-on-failure
```

Expected: all SAI tests pass under Release/Werror.

**Step 3: Run generator and conformance gates**

```sh
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run golden
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run sai-conformance-gate
env MISE_CACHE_DIR=/tmp/x3d-cpp-mise-cache \
    UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache mise run sai-invariants
env UV_CACHE_DIR=/tmp/x3d-cpp-uv-cache uv run pytest
```

Expected: golden drift clean; 65-service/35-invariant normal gates pass; Python
suite passes.

**Step 4: Run sanitizers**

```sh
cmake --build build-sai-san --target x3d_sai_experimental_tests -j2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sai-san \
  -R '^sai_' --output-on-failure
```

Expected: all available SAI sanitizer tests pass. If the sanitizer build has
not been configured, configure the existing SAI sanitizer build rather than
changing project-wide presets.

**Step 5: Independent review and stop/go decision**

Request review focused on:

- whether generated acquisition is truly infallible and owner-safe;
- whether runtime failures occur only at justified lifecycle boundaries;
- whether dynamic and typed paths share semantics;
- whether generated metadata and keys can drift;
- whether the example is genuinely pleasant rather than ceremony hidden in
  helpers;
- whether unit evidence remains honest.

Do not begin Phase 2 until all Important findings are resolved. If fixes are
required, commit them separately with a narrow message and repeat the affected
gates.
