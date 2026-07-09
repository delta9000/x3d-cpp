# cgltf Import Backend + Priority-Registry Selection — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a default-ON, MIT-header cgltf glTF import backend to the asset-import consumer, behind a new priority-based `BackendRegistry` that resolves overlap when multiple backends (cgltf + assimp) claim the same file, proven generic by a tolerant cross-backend swap-test.

**Architecture:** A pure `BackendRegistry` (priority + name lookup) replaces the hardcoded extension if/else in `main.cpp`; backends are registered explicitly behind `HAVE_*` macros. `CgltfSource` walks a parsed glTF into the existing `ImportScene` IR — no IR or `emit` changes. A committed, script-generated `.glb` fixture drives a `HAVE_CGLTF && HAVE_ASSIMP` invariant differential swap-test.

**Tech Stack:** C++20, CMake (Ninja), doctest, mise tasks. Vendored `cgltf.h` (MIT). Fixture generator in Python (stdlib only, run via `uv run`).

## Global Constraints

- **Backend priorities (verbatim):** fixture=100 on `fixture:` prefix; cgltf=100 on `.gltf`/`.glb`; assimp=10 on `.gltf`/`.glb`, 50 on its other formats; usd=100 on `.usd`/`.usda`/`.usdc`/`.usdz`. Non-positive = declines.
- **cgltf default ON, assimp default OFF, usd default OFF.** Swap-test compiles only when `HAVE_CGLTF AND HAVE_ASSIMP`.
- **No IR / emit changes.** cgltf produces the existing `x3d::asset_import::ImportScene` (`import_source.hpp`).
- **No new image-decode dep.** data-URI = inline base64 decode to bytes; texel decode stays in the stb texture pipeline.
- **glTF convention:** right-handed, Y-up, column-major matrices = our `Mat4` — NO transpose/axis flip.
- **Footprint invariant:** backends never link `x3d_cpp::authoring`; the `emit` TU still includes only `x3d/authoring.hpp`.
- **Namespaces:** everything under `namespace x3d::asset_import`.
- **Test naming:** every ctest is `add_test(NAME x3d_assetimport_...)`.
- **NOTICE is DoD:** vendoring cgltf requires a root `NOTICE` entry (MIT).
- **Non-disparagement:** describe assimp/cgltf/glTF factually in every committed artifact.
- **Commit conventions:** no `Claude-Session:` trailers / session URLs; tool-agnostic messages.
- **Build dir:** reuse `build-asset-import/` (as `mise run asset-import` does).

---

## File Structure

**New**
- `examples/asset_import/backend_registry.hpp` / `.cpp` — pure priority/name registry.
- `examples/asset_import/cgltf_source.hpp` / `.cpp` — cgltf backend (gated).
- `examples/asset_import/third_party/cgltf.h` — vendored MIT header.
- `examples/asset_import/tests/backend_registry_test.cpp` — pure registry unit test.
- `examples/asset_import/tests/cgltf_source_test.cpp` — cgltf load coverage (gated).
- `examples/asset_import/tests/backend_swap_test.cpp` — cgltf-vs-assimp differential (double-gated).
- `examples/asset_import/assets/fixtures/gen_gltf_fixture.py` — reproducible `.glb` generator.
- `examples/asset_import/assets/fixtures/twobox.glb` — committed generated fixture.
- `docs/wiki/decisions/0044-asset-import-backend-selection.md` — ADR.

**Modified**
- `examples/asset_import/main.cpp` — dispatch via registry + `--backend` flag + `--help`.
- `examples/asset_import/CMakeLists.txt` — cgltf option/sources/test targets.
- `CMakeLists.txt` (top) — `option(X3D_CPP_BUILD_CGLTF … ON)`.
- `scripts/validate-examples.sh` — both-backends-ON asset-import config (runs swap-test).
- `NOTICE`, `docs/wiki/subsystems/asset-import.md`, `examples/asset_import/README.md`,
  `docs/sdk/v1-capabilities.md`, `docs/wiki/coverage.md`, `mkdocs.yml`.

---

## Task 1: `BackendRegistry` (pure, unit-tested)

**Files:**
- Create: `examples/asset_import/backend_registry.hpp`, `.cpp`
- Test: `examples/asset_import/tests/backend_registry_test.cpp`
- Modify: `examples/asset_import/CMakeLists.txt` (add test target near the fixture_source test, ~line 27)

**Interfaces:**
- Produces: `struct ImportBackend { std::string name; std::function<int(const std::string&)> priority; std::function<std::unique_ptr<ImportSource>()> make; };`
  `class BackendRegistry { void add(ImportBackend); const ImportBackend* select(const std::string& input) const; const ImportBackend* byName(const std::string&) const; std::vector<std::string> names() const; };`
- Consumes: `ImportSource` from `import_source.hpp`.

- [ ] **Step 1: Write the failing test** — `tests/backend_registry_test.cpp`:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "backend_registry.hpp"
using namespace x3d::asset_import;

// A backend that ignores its call; only priority/name matter for selection.
static ImportBackend fake(std::string n, std::function<int(const std::string&)> p) {
  return ImportBackend{std::move(n), std::move(p), [] { return std::unique_ptr<ImportSource>{}; }};
}
static int ext_is(const std::string& in, const char* e, int prio) {
  auto d = in.find_last_of('.');
  return d != std::string::npos && in.substr(d) == e ? prio : -1;
}

TEST_CASE("registry selects highest positive priority") {
  BackendRegistry r;
  r.add(fake("cgltf",  [](const std::string& i){ return ext_is(i, ".glb", 100); }));
  r.add(fake("assimp", [](const std::string& i){ return ext_is(i, ".glb", 10); }));
  REQUIRE(r.select("model.glb") != nullptr);
  CHECK(r.select("model.glb")->name == "cgltf");        // 100 beats 10
  CHECK(r.select("model.fbx") == nullptr);              // nobody claims it
}
TEST_CASE("byName overrides priority and ignores decline") {
  BackendRegistry r;
  r.add(fake("cgltf",  [](const std::string& i){ return ext_is(i, ".glb", 100); }));
  r.add(fake("assimp", [](const std::string& i){ return ext_is(i, ".glb", 10); }));
  CHECK(r.byName("assimp") != nullptr);                 // forced even though cgltf outranks
  CHECK(r.byName("nope") == nullptr);
}
TEST_CASE("fixture prefix routes by whole input, not extension") {
  BackendRegistry r;
  r.add(fake("fixture", [](const std::string& i){ return i.rfind("fixture:", 0) == 0 ? 100 : -1; }));
  CHECK(r.select("fixture:cube") != nullptr);
  CHECK(r.select("fixture:cube")->name == "fixture");
  CHECK(r.select("plain.obj") == nullptr);
}
TEST_CASE("names lists registered backends") {
  BackendRegistry r;
  r.add(fake("cgltf", [](const std::string&){ return -1; }));
  r.add(fake("assimp", [](const std::string&){ return -1; }));
  CHECK(r.names() == std::vector<std::string>{"cgltf", "assimp"});
}
```

- [ ] **Step 2: Write `backend_registry.hpp`:**

```cpp
#ifndef X3D_ASSET_IMPORT_BACKEND_REGISTRY_HPP
#define X3D_ASSET_IMPORT_BACKEND_REGISTRY_HPP
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "import_source.hpp"

namespace x3d::asset_import {

// One selectable import backend. `priority` inspects the WHOLE input string
// (so a "fixture:" prefix and a ".glb" extension resolve through one path);
// it returns a positive score when it can handle the input, <=0 to decline.
struct ImportBackend {
  std::string name;
  std::function<int(const std::string& input)> priority;
  std::function<std::unique_ptr<ImportSource>()> make;
};

class BackendRegistry {
public:
  void add(ImportBackend b) { backends_.push_back(std::move(b)); }

  // Highest positive priority for `input`; first-registered wins ties.
  // nullptr when no backend claims the input.
  const ImportBackend* select(const std::string& input) const {
    const ImportBackend* best = nullptr;
    int bestP = 0;
    for (const auto& b : backends_) {
      int p = b.priority(input);
      if (p > bestP) { bestP = p; best = &b; }
    }
    return best;
  }

  // Exact name match, ignoring priority (the --backend override path).
  const ImportBackend* byName(const std::string& name) const {
    for (const auto& b : backends_) if (b.name == name) return &b;
    return nullptr;
  }

  std::vector<std::string> names() const {
    std::vector<std::string> out;
    for (const auto& b : backends_) out.push_back(b.name);
    return out;
  }

private:
  std::vector<ImportBackend> backends_;
};

} // namespace x3d::asset_import
#endif
```

`backend_registry.cpp` is empty-but-present (keeps the CMake source list uniform) — or fold header-only; **implement header-only, no `.cpp`** (all methods inline). Update File Structure mentally: drop `.cpp`.

- [ ] **Step 3: Add the CMake test target** — in `examples/asset_import/CMakeLists.txt` after the `x3d_assetimport_fixture_source` test (~line 27):

```cmake
add_executable(x3d_assetimport_backend_registry
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/backend_registry_test.cpp")
target_include_directories(x3d_assetimport_backend_registry PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${X3D_CPP_DOCTEST_INCLUDE_DIR}")
target_compile_features(x3d_assetimport_backend_registry PRIVATE cxx_std_20)
add_test(NAME x3d_assetimport_backend_registry COMMAND x3d_assetimport_backend_registry)
```

(Match the exact doctest include var the sibling test targets use — grep `DOCTEST` in this file and copy it.)

- [ ] **Step 4: Build + run:**

```
mise run asset-import
ctest --test-dir build-asset-import -R x3d_assetimport_backend_registry -V
```
Expected: 4 test cases pass.

- [ ] **Step 5: Commit:** `git add -A && git commit -m "feat(asset-import): pure priority BackendRegistry + unit test"`

---

## Task 2: Route `main.cpp` through the registry + `--backend` flag

**Files:**
- Modify: `examples/asset_import/main.cpp` (dispatch block ~180-220; arg parse; `--help`)

**Interfaces:**
- Consumes: `BackendRegistry`, `ImportBackend`.
- Produces: `registerBuiltinBackends(BackendRegistry&)` (file-local static in `main.cpp`); new `--backend <name>` CLI option.

- [ ] **Step 1: Add the registration function** near the top of `main.cpp` (after includes; include `backend_registry.hpp`, `fixture_source.hpp`, and the gated backend headers already included). The fixture backend strips its own prefix inside `load`, so keep the existing `fixture:` substring handling by having the fixture `make` return a source and letting `main` pass the post-prefix name (see Step 3):

```cpp
static void registerBuiltinBackends(x3d::asset_import::BackendRegistry& reg) {
  using namespace x3d::asset_import;
  auto extIs = [](const std::string& in, std::initializer_list<const char*> exts) {
    auto d = in.find_last_of('.');
    if (d == std::string::npos) return false;
    std::string e = in.substr(d);
    for (auto& c : e) c = (char)std::tolower((unsigned char)c);
    for (const char* x : exts) if (e == x) return true;
    return false;
  };
  reg.add({"fixture",
    [](const std::string& in){ return in.rfind("fixture:", 0) == 0 ? 100 : -1; },
    []{ return std::unique_ptr<ImportSource>(std::make_unique<FixtureSource>()); }});
#ifdef X3D_ASSET_IMPORT_HAVE_ASSIMP
  reg.add({"assimp",
    [extIs](const std::string& in){
      if (extIs(in, {".gltf", ".glb"})) return 10;
      if (extIs(in, {".obj", ".fbx", ".dae", ".ply", ".stl", ".3ds"})) return 50;
      return -1; },
    []{ return std::unique_ptr<ImportSource>(std::make_unique<AssimpSource>()); }});
#endif
#ifdef X3D_ASSET_IMPORT_HAVE_USD
  reg.add({"usd",
    [extIs](const std::string& in){ return extIs(in, {".usd", ".usda", ".usdc", ".usdz"}) ? 100 : -1; },
    []{ return std::unique_ptr<ImportSource>(std::make_unique<UsdSource>()); }});
#endif
  // cgltf registered in Task 4.
}
```

- [ ] **Step 2: Parse `--backend`** in the existing arg loop (alongside `--format`, `--emit-glsl`): add `std::string backendFlag;` and a branch `else if (arg == "--backend" && i + 1 < argc) backendFlag = argv[++i];`.

- [ ] **Step 3: Replace the dispatch block** (the `if (input.rfind("fixture:",0)==0) … else { isUsd … }` block, ~184-216) with:

```cpp
  BackendRegistry reg;
  registerBuiltinBackends(reg);
  const ImportBackend* backend =
      backendFlag.empty() ? reg.select(input) : reg.byName(backendFlag);
  if (!backend) {
    std::cerr << "error: no import backend for '" << input << "'.\n";
    if (!backendFlag.empty()) std::cerr << "  unknown --backend '" << backendFlag << "'.\n";
    std::cerr << "  available backends:";
    for (const auto& n : reg.names()) std::cerr << ' ' << n;
    std::cerr << "\n  (glTF needs cgltf [default] or assimp; USD needs -DX3D_CPP_BUILD_USD=ON;"
                 " other formats need -DX3D_CPP_BUILD_ASSIMP=ON)\n";
    return 1;
  }
  try {
    source = backend->make();
    // FixtureSource::load expects the name AFTER the "fixture:" prefix.
    std::string loadArg = input.rfind("fixture:", 0) == 0 ? input.substr(8) : input;
    scene = source->load(loadArg);
  } catch (const std::exception& e) {
    std::cerr << "error: failed to load input '" << input << "': " << e.what() << "\n";
    return 2;
  }
```

- [ ] **Step 4: Update `--help`** — in the usage string, under the backend-autoselect block, add a line:

```
      "  --backend <name>      force a backend (cgltf|assimp|usd|fixture); default: auto by input\n"
```
and note glTF prefers cgltf.

- [ ] **Step 5: Build + regression-check the fixture path** (assimp/usd OFF):

```
mise run asset-import
./build-asset-import/examples/asset_import/x3d_asset_import fixture:cube -o /tmp/c.x3d && echo OK
```
Expected: writes `/tmp/c.x3d`, prints `OK`. (Fixture routing unchanged.)

- [ ] **Step 6: Commit:** `git commit -am "feat(asset-import): dispatch via BackendRegistry + --backend override"`

---

## Task 3: Reproducible glTF `.glb` fixture

**Files:**
- Create: `examples/asset_import/assets/fixtures/gen_gltf_fixture.py`
- Create (generated, committed): `examples/asset_import/assets/fixtures/twobox.glb`

**Interfaces:**
- Produces: a self-contained `.glb` with 2 nodes (parent box + child box), 1 PBR
  metallic-roughness material with an embedded 2×2 PNG baseColor texture, 1 perspective
  camera node, 1 `KHR_lights_punctual` point light. Both cgltf and assimp read it.

- [ ] **Step 1: Write the generator** — `gen_gltf_fixture.py` (stdlib only). It builds
  glTF JSON + a single binary buffer (interleaved is unnecessary; separate bufferViews),
  packs the GLB container (`glTF` magic, JSON chunk, BIN chunk), and writes `twobox.glb`
  next to itself. Full script:

```python
#!/usr/bin/env python3
"""Generate examples/asset_import/assets/fixtures/twobox.glb (reproducible).
Two boxes (parent/child), 1 PBR material + embedded 2x2 PNG, 1 camera, 1 point light."""
import json, struct, zlib, pathlib

def png_2x2():
    # 2x2 RGBA, four solid colors, hand-built (no PIL dependency).
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff))
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", 2, 2, 8, 6, 0, 0, 0)  # 8-bit RGBA
    raw = b""
    px = [(255,0,0,255),(0,255,0,255),(0,0,255,255),(255,255,0,255)]
    for row in range(2):
        raw += b"\x00" + b"".join(struct.pack("BBBB", *px[row*2+col]) for col in range(2))
    idat = zlib.compress(raw)
    return sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")

# Unit box (24 verts, 36 indices) — positions/normals/uv, then indices.
V = []  # (px,py,pz, nx,ny,nz, u,v)
faces = [((0,0,1),[(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]),
         ((0,0,-1),[(1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)]),
         ((1,0,0),[(1,-1,1),(1,-1,-1),(1,1,-1),(1,1,1)]),
         ((-1,0,0),[(-1,-1,-1),(-1,-1,1),(-1,1,1),(-1,1,-1)]),
         ((0,1,0),[(-1,1,1),(1,1,1),(1,1,-1),(-1,1,-1)]),
         ((0,-1,0),[(-1,-1,-1),(1,-1,-1),(1,-1,1),(-1,-1,1)])]
uv = [(0,0),(1,0),(1,1),(0,1)]
idx = []
for n,(a,b,c,d) in faces:
    base = len(V)
    for k,p in enumerate([a,b,c,d]):
        V.append((p[0]*0.5,p[1]*0.5,p[2]*0.5, *n, *uv[k]))
    idx += [base,base+1,base+2, base,base+2,base+3]

pos = b"".join(struct.pack("<3f", v[0],v[1],v[2]) for v in V)
nrm = b"".join(struct.pack("<3f", v[3],v[4],v[5]) for v in V)
tex = b"".join(struct.pack("<2f", v[6],v[7]) for v in V)
ind = b"".join(struct.pack("<H", i) for i in idx)
png = png_2x2()

def pad4(b): return b + b"\x00" * ((4 - len(b) % 4) % 4)
blob = b""
def add(data):
    global blob
    off = len(blob); blob = pad4(blob) ; start = len(blob); blob += data
    return start, len(data)
p_off,p_len = add(pos); n_off,n_len = add(nrm); t_off,t_len = add(tex)
i_off,i_len = add(ind); img_off,img_len = add(png)
blob = pad4(blob)

mn = [min(v[i] for v in V) for i in range(3)]
mx = [max(v[i] for v in V) for i in range(3)]

gltf = {
  "asset": {"version": "2.0", "generator": "x3d-cpp gen_gltf_fixture"},
  "extensionsUsed": ["KHR_lights_punctual"],
  "extensions": {"KHR_lights_punctual": {"lights": [
      {"type": "point", "color": [1,1,1], "intensity": 5.0, "range": 20.0, "name": "lamp"}]}},
  "scene": 0,
  "scenes": [{"nodes": [0]}],
  "nodes": [
    {"name": "parent", "mesh": 0, "translation": [0,0,0], "children": [1,2,3]},
    {"name": "child",  "mesh": 0, "translation": [2,0,0]},
    {"name": "cam", "camera": 0, "translation": [0,0,6]},
    {"name": "lightnode", "extensions": {"KHR_lights_punctual": {"light": 0}}, "translation": [3,3,3]}],
  "cameras": [{"type": "perspective",
      "perspective": {"yfov": 0.7853982, "znear": 0.1, "zfar": 1000.0, "aspectRatio": 1.0}}],
  "meshes": [{"name": "box", "primitives": [
      {"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3, "material": 0}]}],
  "materials": [{"name": "mat",
      "pbrMetallicRoughness": {"baseColorFactor": [1,1,1,1], "metallicFactor": 0.0,
          "roughnessFactor": 0.6, "baseColorTexture": {"index": 0}},
      "emissiveFactor": [0,0,0]}],
  "textures": [{"source": 0}],
  "images": [{"bufferView": 4, "mimeType": "image/png"}],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": len(V), "type": "VEC3", "min": mn, "max": mx},
    {"bufferView": 1, "componentType": 5126, "count": len(V), "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": len(V), "type": "VEC2"},
    {"bufferView": 3, "componentType": 5123, "count": len(idx), "type": "SCALAR"}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": p_off, "byteLength": p_len, "target": 34962},
    {"buffer": 0, "byteOffset": n_off, "byteLength": n_len, "target": 34962},
    {"buffer": 0, "byteOffset": t_off, "byteLength": t_len, "target": 34962},
    {"buffer": 0, "byteOffset": i_off, "byteLength": i_len, "target": 34963},
    {"buffer": 0, "byteOffset": img_off, "byteLength": img_len}],
  "buffers": [{"byteLength": len(blob)}],
}

jsonb = pad4(json.dumps(gltf, separators=(",", ":")).encode("utf-8"))
out = struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(jsonb) + 8 + len(blob))
out += struct.pack("<II", len(jsonb), 0x4E4F534A) + jsonb
out += struct.pack("<II", len(blob), 0x004E4942) + blob
p = pathlib.Path(__file__).with_name("twobox.glb")
p.write_bytes(out)
print(f"wrote {p} ({len(out)} bytes)")
```

- [ ] **Step 2: Generate + sanity-check:**

```
uv run examples/asset_import/assets/fixtures/gen_gltf_fixture.py
python3 -c "import struct;b=open('examples/asset_import/assets/fixtures/twobox.glb','rb').read();print('magic ok', b[:4]==b'glTF', 'len', len(b))"
```
Expected: prints `wrote …/twobox.glb (…)` then `magic ok True len …`.

- [ ] **Step 3: Commit:** `git add examples/asset_import/assets/fixtures/ && git commit -m "test(asset-import): reproducible twobox.glb fixture + generator"`

---

## Task 4: `CgltfSource` backend (gated) + register it

**Files:**
- Create: `examples/asset_import/third_party/cgltf.h` (vendored MIT, upstream jkuhlmann/cgltf)
- Create: `examples/asset_import/cgltf_source.hpp`, `.cpp`
- Test: `examples/asset_import/tests/cgltf_source_test.cpp`
- Modify: top `CMakeLists.txt` (option), `examples/asset_import/CMakeLists.txt` (sources/macros/tests),
  `examples/asset_import/main.cpp` (register cgltf), `NOTICE`.

**Interfaces:**
- Produces: `class CgltfSource : public ImportSource { ImportScene load(const std::string& path) override; };`
- Consumes: `ImportScene`, `BackendRegistry`.

- [ ] **Step 1: Vendor cgltf.h** — download the single MIT header (pin a released tag) to
  `examples/asset_import/third_party/cgltf.h`:

```
curl -fsSL https://raw.githubusercontent.com/jkuhlmann/cgltf/v1.14/cgltf.h \
  -o examples/asset_import/third_party/cgltf.h
head -40 examples/asset_import/third_party/cgltf.h | grep -i "MIT\|version"
```
Expected: MIT license banner + version present. (If v1.14 moves, pick the current release tag; record it in NOTICE.)

- [ ] **Step 2: Add the CMake option** — top `CMakeLists.txt`, next to the assimp option:

```cmake
option(X3D_CPP_BUILD_CGLTF "Build the cgltf glTF asset-import backend (header-only, MIT)" ON)
```

- [ ] **Step 3: Write the failing test** — `tests/cgltf_source_test.cpp`:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "cgltf_source.hpp"
#include <string>
using namespace x3d::asset_import;

static std::string fixture() {
  return std::string(X3D_ASSET_IMPORT_FIXTURE_DIR) + "/twobox.glb";
}

TEST_CASE("cgltf loads full glTF scene") {
  CgltfSource src;
  ImportScene s = src.load(fixture());
  CHECK(s.meshes.size() >= 1);
  size_t tris = 0; for (auto& m : s.meshes) tris += m.indices.size() / 3;
  CHECK(tris == 24);                         // two boxes share one mesh (12 tris) x2 nodes? see note
  CHECK(s.materials.size() == 1);
  REQUIRE(s.materials[0].pbr.has_value());
  CHECK(s.materials[0].pbr->metallic == doctest::Approx(0.0f));
  CHECK(s.materials[0].pbr->roughness == doctest::Approx(0.6f));
  CHECK(s.materials[0].textures.baseColor.has_value());
  CHECK(s.embedded.size() == 1);             // the 2x2 PNG from the GLB bin chunk
  CHECK(s.nodes.size() >= 4);                // parent, child, cam, light
  CHECK(s.cameras.size() == 1);
  CHECK(s.lights.size() == 1);
  CHECK(s.lights[0].kind == ImportLight::Kind::Point);
}
```

> Note on `tris`: the fixture references one mesh (12 tris) from two nodes. If the backend
> counts triangles per *mesh* (not per node instance), assert `tris == 12` and check
> `s.nodes` covers both box nodes. Decide during implementation and make the assertion
> match the backend's actual per-mesh vs per-instance emission; keep it internally
> consistent with the swap-test in Task 5.

- [ ] **Step 4: Write `cgltf_source.hpp`:**

```cpp
#ifndef X3D_ASSET_IMPORT_CGLTF_SOURCE_HPP
#define X3D_ASSET_IMPORT_CGLTF_SOURCE_HPP
#include "import_source.hpp"
namespace x3d::asset_import {
class CgltfSource : public ImportSource {
public:
  ImportScene load(const std::string& path) override;
};
} // namespace x3d::asset_import
#endif
```

- [ ] **Step 5: Write `cgltf_source.cpp`** — one TU owns the implementation:

```cpp
#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf.h"
#include "cgltf_source.hpp"
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace x3d::asset_import {
namespace {

// Minimal base64 decode for "data:...;base64," image URIs.
std::vector<std::uint8_t> b64decode(const char* s) {
  static const std::string T =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::vector<std::uint8_t> out; int val = 0, bits = -8;
  for (; *s && *s != '"'; ++s) {
    if (*s == '=') break;
    auto p = T.find(*s); if (p == std::string::npos) continue;
    val = (val << 6) + (int)p; bits += 6;
    if (bits >= 0) { out.push_back((std::uint8_t)((val >> bits) & 0xFF)); bits -= 8; }
  }
  return out;
}

Mat4 nodeLocal(const cgltf_node* n) {
  Mat4 m; cgltf_node_transform_local(n, m.m.data()); return m; // column-major, no transpose
}

Vec3 readV3(const cgltf_float* f) { return {f[0], f[1], f[2]}; }

int matIndex(const cgltf_data* d, const cgltf_material* m) {
  if (!m) return -1;
  return (int)(m - d->materials);
}

// Resolve an image to an external path OR an embedded-bytes index.
std::optional<TextureRef> imageRef(const cgltf_data* d, const cgltf_texture* t,
                                   ImportScene& scene,
                                   std::unordered_map<const void*, int>& cache) {
  if (!t || !t->image) return std::nullopt;
  const cgltf_image* img = t->image;
  auto it = cache.find(img);
  if (it != cache.end()) { TextureRef r; r.embeddedIndex = it->second; return r; }
  // External URI (not a data: URI).
  if (img->uri && std::strncmp(img->uri, "data:", 5) != 0) {
    TextureRef r; r.externalPath = img->uri; return r;
  }
  // Embedded: data: URI or GLB bufferView.
  std::vector<std::uint8_t> bytes; std::string ext = "png";
  if (img->uri) { // data: URI
    const char* comma = std::strchr(img->uri, ','); if (comma) bytes = b64decode(comma + 1);
    if (std::strstr(img->uri, "jpeg") || std::strstr(img->uri, "jpg")) ext = "jpg";
  } else if (img->buffer_view) {
    const cgltf_buffer_view* bv = img->buffer_view;
    const std::uint8_t* base = (const std::uint8_t*)bv->buffer->data + bv->offset;
    bytes.assign(base, base + bv->size);
    if (img->mime_type && std::strstr(img->mime_type, "jpeg")) ext = "jpg";
  }
  if (bytes.empty()) return std::nullopt;
  int idx = (int)scene.embedded.size();
  scene.embedded.push_back({std::string("cgltf_img_") + std::to_string(idx), ext, std::move(bytes)});
  cache[img] = idx;
  TextureRef r; r.embeddedIndex = idx; return r;
}

} // namespace

ImportScene CgltfSource::load(const std::string& path) {
  cgltf_options opt{};
  cgltf_data* d = nullptr;
  if (cgltf_parse_file(&opt, path.c_str(), &d) != cgltf_result_success)
    throw std::runtime_error("cgltf: parse failed");
  struct Guard { cgltf_data* d; ~Guard(){ if (d) cgltf_free(d); } } guard{d};
  if (cgltf_load_buffers(&opt, d, path.c_str()) != cgltf_result_success)
    throw std::runtime_error("cgltf: buffer load failed");

  ImportScene scene;
  std::unordered_map<const void*, int> imgCache;

  // Materials.
  for (size_t i = 0; i < d->materials_count; ++i) {
    const cgltf_material& gm = d->materials[i];
    ImportMaterial m; m.name = gm.name ? gm.name : ("material_" + std::to_string(i));
    if (gm.has_pbr_metallic_roughness) {
      const auto& pm = gm.pbr_metallic_roughness;
      PbrParams p;
      p.baseColor = {pm.base_color_factor[0], pm.base_color_factor[1],
                     pm.base_color_factor[2], pm.base_color_factor[3]};
      p.metallic = pm.metallic_factor; p.roughness = pm.roughness_factor;
      m.pbr = p;
      m.diffuse = {p.baseColor.x, p.baseColor.y, p.baseColor.z};
      m.opacity = p.baseColor.w;
      if (auto r = imageRef(d, pm.base_color_texture.texture, scene, imgCache)) m.textures.baseColor = r;
      if (auto r = imageRef(d, pm.metallic_roughness_texture.texture, scene, imgCache)) m.textures.metallicRoughness = r;
    }
    m.emissive = readV3(gm.emissive_factor);
    if (auto r = imageRef(d, gm.normal_texture.texture, scene, imgCache)) m.textures.normal = r;
    if (auto r = imageRef(d, gm.emissive_texture.texture, scene, imgCache)) m.textures.emissive = r;
    if (auto r = imageRef(d, gm.occlusion_texture.texture, scene, imgCache)) m.textures.occlusion = r;
    if (gm.alpha_mode == cgltf_alpha_mode_mask)  { m.alpha = AlphaMode::Mask;  m.opacityThreshold = gm.alpha_cutoff; }
    if (gm.alpha_mode == cgltf_alpha_mode_blend) { m.alpha = AlphaMode::Blend; }
    scene.materials.push_back(std::move(m));
  }

  // Meshes (one ImportMesh per triangle primitive).
  std::unordered_map<const cgltf_mesh*, std::vector<int>> meshPrims;
  for (size_t i = 0; i < d->meshes_count; ++i) {
    const cgltf_mesh& gmesh = d->meshes[i];
    for (size_t pi = 0; pi < gmesh.primitives_count; ++pi) {
      const cgltf_primitive& prim = gmesh.primitives[pi];
      if (prim.type != cgltf_primitive_type_triangles) continue;
      ImportMesh mesh; mesh.materialIndex = matIndex(d, prim.material);
      for (size_t a = 0; a < prim.attributes_count; ++a) {
        const cgltf_attribute& at = prim.attributes[a];
        size_t n = cgltf_accessor_unpack_floats(at.data, nullptr, 0);
        std::vector<float> buf(n);
        cgltf_accessor_unpack_floats(at.data, buf.data(), n);
        size_t comp = cgltf_num_components(at.data->type), count = n / comp;
        if (at.type == cgltf_attribute_type_position)
          for (size_t k=0;k<count;++k) mesh.positions.push_back({buf[k*comp],buf[k*comp+1],buf[k*comp+2]});
        else if (at.type == cgltf_attribute_type_normal)
          for (size_t k=0;k<count;++k) mesh.normals.push_back({buf[k*comp],buf[k*comp+1],buf[k*comp+2]});
        else if (at.type == cgltf_attribute_type_texcoord && at.index == 0)
          for (size_t k=0;k<count;++k) mesh.uv.push_back({buf[k*comp],buf[k*comp+1]});
        else if (at.type == cgltf_attribute_type_color)
          for (size_t k=0;k<count;++k) mesh.colors.push_back({buf[k*comp],buf[k*comp+1],buf[k*comp+2],
                                                              comp>3?buf[k*comp+3]:1.0f});
      }
      if (prim.indices) {
        size_t ic = prim.indices->count;
        for (size_t k=0;k<ic;++k) mesh.indices.push_back((std::uint32_t)cgltf_accessor_read_index(prim.indices, k));
      } else {
        for (std::uint32_t k=0;k<(std::uint32_t)mesh.positions.size();++k) mesh.indices.push_back(k);
      }
      meshPrims[&gmesh].push_back((int)scene.meshes.size());
      scene.meshes.push_back(std::move(mesh));
    }
  }

  // Nodes (hierarchy + TRS), cameras, lights.
  std::unordered_map<const cgltf_node*, int> nodeIdx;
  for (size_t i = 0; i < d->nodes_count; ++i) nodeIdx[&d->nodes[i]] = (int)i;
  for (size_t i = 0; i < d->nodes_count; ++i) {
    const cgltf_node& gn = d->nodes[i];
    ImportNode node; node.name = gn.name ? gn.name : ("node_" + std::to_string(i));
    node.localTransform = nodeLocal(&gn);
    if (gn.mesh) for (int mp : meshPrims[gn.mesh]) node.meshIndices.push_back(mp);
    for (size_t c = 0; c < gn.children_count; ++c) node.childIndices.push_back(nodeIdx[gn.children[c]]);
    scene.nodes.push_back(std::move(node));

    if (gn.camera && gn.camera->type == cgltf_camera_type_perspective) {
      const auto& pc = gn.camera->data.perspective;
      ImportCamera cam; cam.world = nodeLocal(&gn);
      cam.yfov = pc.yfov; cam.znear = pc.znear; cam.zfar = pc.has_zfar ? pc.zfar : 1000.0f;
      scene.cameras.push_back(cam);
    }
    if (gn.light) {
      const cgltf_light* gl = gn.light;
      ImportLight L; L.color = readV3(gl->color); L.intensity = gl->intensity;
      L.radius = gl->range > 0 ? gl->range : 100.0f;
      if (gl->type == cgltf_light_type_directional) L.kind = ImportLight::Kind::Dir;
      else if (gl->type == cgltf_light_type_spot) {
        L.kind = ImportLight::Kind::Spot;
        L.beamWidth = gl->spot_inner_cone_angle; L.cutOffAngle = gl->spot_outer_cone_angle;
      } else L.kind = ImportLight::Kind::Point;
      Mat4 w = nodeLocal(&gn);
      L.position = {w.m[12], w.m[13], w.m[14]};
      scene.lights.push_back(L);
    }
  }
  // Root node = first scene's first root, else 0.
  if (d->scene && d->scene->nodes_count > 0) scene.rootNode = nodeIdx[d->scene->nodes[0]];
  else if (!scene.nodes.empty()) scene.rootNode = 0;
  return scene;
}
} // namespace x3d::asset_import
```

> Verify field names against the vendored cgltf.h version (`spot_inner_cone_angle`,
> `has_zfar`, `cgltf_accessor_unpack_floats`, `cgltf_accessor_read_index` are all in v1.14).
> Fix any that drifted, keeping behavior identical.

- [ ] **Step 6: Wire CMake** — `examples/asset_import/CMakeLists.txt`:
  1. Add a gated unit-test block (mirror the assimp one) that builds `x3d_assetimport_cgltf`
     from `cgltf_source.cpp` + `tests/cgltf_source_test.cpp`, with
     `target_compile_definitions(... X3D_ASSET_IMPORT_HAVE_CGLTF=1
     X3D_ASSET_IMPORT_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures\")`, include
     dir `${CMAKE_CURRENT_SOURCE_DIR}` and doctest, and `add_test(NAME x3d_assetimport_cgltf …)`.
  2. In the main-exe assembly (near the `if(X3D_CPP_BUILD_ASSIMP)` block ~195), add:
     ```cmake
     if(X3D_CPP_BUILD_CGLTF)
         list(APPEND ASSET_IMPORT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cgltf_source.cpp")
         set(HAVE_CGLTF 1)
     else()
         set(HAVE_CGLTF 0)
     endif()
     ```
     and after the `HAVE_USD` define:
     ```cmake
     if(HAVE_CGLTF)
         target_compile_definitions(x3d_asset_import PRIVATE X3D_ASSET_IMPORT_HAVE_CGLTF=1)
     endif()
     ```

- [ ] **Step 7: Register cgltf in `main.cpp`** — inside `registerBuiltinBackends`, add
  (include `cgltf_source.hpp` under the same gate):

```cpp
#ifdef X3D_ASSET_IMPORT_HAVE_CGLTF
  reg.add({"cgltf",
    [extIs](const std::string& in){ return extIs(in, {".gltf", ".glb"}) ? 100 : -1; },
    []{ return std::unique_ptr<ImportSource>(std::make_unique<CgltfSource>()); }});
#endif
```

- [ ] **Step 8: NOTICE** — add a cgltf (MIT, © Johannes Kuhlmann) stanza to root `NOTICE`,
  matching the format of the existing `stb_image_write` entry, recording the pinned version.

- [ ] **Step 9: Build + test:**

```
mise run asset-import                       # default: cgltf ON
ctest --test-dir build-asset-import -R x3d_assetimport_cgltf -V
./build-asset-import/examples/asset_import/x3d_asset_import \
  examples/asset_import/assets/fixtures/twobox.glb -o /tmp/tb.x3d --assets-dir /tmp && echo OK
```
Expected: cgltf test passes; CLI converts the `.glb` and writes `/tmp/tb.x3d` + assets.

- [ ] **Step 10: Commit:** `git commit -am "feat(asset-import): cgltf glTF backend (default ON) + registry entry + NOTICE"`

---

## Task 5: Cross-backend swap-test (the genericity proof)

**Files:**
- Test: `examples/asset_import/tests/backend_swap_test.cpp` (double-gated)
- Modify: `examples/asset_import/CMakeLists.txt` (swap target when both ON), `scripts/validate-examples.sh`

**Interfaces:**
- Consumes: `CgltfSource`, `AssimpSource`, `ImportScene`.

- [ ] **Step 1: Write the swap-test** — `tests/backend_swap_test.cpp`:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "cgltf_source.hpp"
#include "assimp_source.hpp"
#include <string>
using namespace x3d::asset_import;

static std::string fixture() {
  return std::string(X3D_ASSET_IMPORT_FIXTURE_DIR) + "/twobox.glb";
}
static size_t totalTris(const ImportScene& s) {
  size_t t = 0; for (auto& m : s.meshes) t += m.indices.size() / 3; return t;
}

// The ImportSource seam is generic iff two independent backends agree on the
// scene's structural invariants (tolerant to vertex order + material naming).
TEST_CASE("cgltf and assimp agree on twobox.glb invariants") {
  ImportScene a = CgltfSource{}.load(fixture());
  ImportScene b = AssimpSource{}.load(fixture());
  CHECK(totalTris(a) == totalTris(b));
  CHECK(a.materials.size() == b.materials.size());
  CHECK(a.cameras.size() == b.cameras.size());
  CHECK(a.lights.size() == b.lights.size());
  REQUIRE(!a.materials.empty()); REQUIRE(!b.materials.empty());
  REQUIRE(a.materials[0].pbr.has_value()); REQUIRE(b.materials[0].pbr.has_value());
  CHECK(a.materials[0].pbr->metallic  == doctest::Approx(b.materials[0].pbr->metallic).epsilon(1e-4));
  CHECK(a.materials[0].pbr->roughness == doctest::Approx(b.materials[0].pbr->roughness).epsilon(1e-4));
}
```

> If assimp merges/splits meshes differently, compare `totalTris` (already order-independent)
> and relax mesh-count to `>= 1` on both. Keep node-count out of the CHECK if assimp injects a
> synthetic root — assert `a.nodes.size() >= 2 && b.nodes.size() >= 2` instead. Tune only as
> the real assimp output requires; document any relaxation in a comment.

- [ ] **Step 2: CMake swap target** — after the cgltf test block:

```cmake
if(X3D_CPP_BUILD_CGLTF AND X3D_CPP_BUILD_ASSIMP AND assimp_FOUND)
    add_executable(x3d_assetimport_backend_swap
        "${CMAKE_CURRENT_SOURCE_DIR}/cgltf_source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/assimp_source.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/backend_swap_test.cpp")
    target_include_directories(x3d_assetimport_backend_swap PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}" "${X3D_CPP_DOCTEST_INCLUDE_DIR}")
    target_link_libraries(x3d_assetimport_backend_swap PRIVATE assimp::assimp)
    target_compile_definitions(x3d_assetimport_backend_swap PRIVATE
        X3D_ASSET_IMPORT_HAVE_CGLTF=1 X3D_ASSET_IMPORT_HAVE_ASSIMP=1
        X3D_ASSET_IMPORT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures")
    target_compile_features(x3d_assetimport_backend_swap PRIVATE cxx_std_20)
    add_test(NAME x3d_assetimport_backend_swap COMMAND x3d_assetimport_backend_swap)
endif()
```

- [ ] **Step 3: Run the swap-test with both backends ON:**

```
cmake -S . -B build-swap -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON \
      -DX3D_CPP_BUILD_CGLTF=ON -DX3D_CPP_BUILD_ASSIMP=ON
cmake --build build-swap --target x3d_assetimport_backend_swap
ctest --test-dir build-swap -R x3d_assetimport_backend_swap -V
```
Expected: PASS. (If assimp isn't installed locally, note it and rely on CI's both-ON config.)

- [ ] **Step 4: Wire the examples-gate** — in `scripts/validate-examples.sh`, add an
  asset-import configuration that sets both `-DX3D_CPP_BUILD_CGLTF=ON -DX3D_CPP_BUILD_ASSIMP=ON`
  and runs `ctest -R x3d_assetimport_backend_swap`. Follow the file's existing per-consumer
  block structure (grep `asset` / `cpuraster` for the pattern).

- [ ] **Step 5: Commit:** `git commit -am "test(asset-import): cgltf-vs-assimp invariant swap-test + examples-gate wiring"`

---

## Task 6: Docs, ADR, footprint check, full CI

**Files:**
- Create: `docs/wiki/decisions/0044-asset-import-backend-selection.md`
- Modify: `docs/wiki/subsystems/asset-import.md`, `examples/asset_import/README.md`,
  `docs/sdk/v1-capabilities.md`, `docs/wiki/coverage.md`, `mkdocs.yml`

- [ ] **Step 1: Write ADR-0044** — decision: priority `BackendRegistry` + `--backend` override
  replacing the extension if/else; cgltf default-ON as the lightweight glTF path with assimp as
  the opt-in fallback (priority 100 vs 10); genericity proven by the invariant swap-test. Match
  the heading/format of a recent ADR (e.g. `0043-*.md`). Include Context / Decision / Consequences.

- [ ] **Step 2: Add the coverage row + nav** — a Decisions-table row for 0044 in
  `docs/wiki/coverage.md` (keep numbering contiguous — Step passes `coverage-gate`), and an ADR
  nav entry in `mkdocs.yml`.

- [ ] **Step 3: Update the subsystem page** — `docs/wiki/subsystems/asset-import.md`: a
  "Backend selection" section (registry, priority table, `--backend`, the cgltf backend,
  the swap-test) and the glTF-via-cgltf default-ON note.

- [ ] **Step 4: Update README + capabilities** — `examples/asset_import/README.md` backend table
  (add cgltf row, priority column, `--backend`); `docs/sdk/v1-capabilities.md` — glTF is
  first-class (cgltf, default-ON; assimp for the broader format set).

- [ ] **Step 5: Run the drift suggester** (advisory): `mise run docs-drift working` — address any
  CITES hits on the touched runtime/tools/docs.

- [ ] **Step 6: Footprint invariant** — confirm cgltf did not leak into the slim target:
```
mise run authoring-footprint
```
Expected: PASS (no excluded-subsystem symbols; size baseline holds).

- [ ] **Step 7: Full gates:**
```
mise run ci
mise run docs-build
```
Expected: both green. (`ci` builds the ci preset + all gates; `docs-build` is the strict wiki gate.)

- [ ] **Step 8: Refresh RAG stores** (symbols moved): `mise run code-ingest && mise run docs-ingest`.

- [ ] **Step 9: Commit:** `git commit -am "docs(asset-import): backend registry + cgltf — ADR-0044, subsystem, README, capabilities"`

---

## Self-Review

**Spec coverage:** registry (T1), main dispatch + `--backend` (T2), fixture (T3), cgltf full
parity (T4), swap-test + examples-gate (T5), docs/ADR/NOTICE/footprint/CI (T4 NOTICE, T6 rest).
Default-ON gating (T4 Step 2). No IR/emit change (T4 produces existing IR). ✓

**Placeholder scan:** the two `>` notes (tris per-mesh-vs-instance in T4; assimp mesh-merge
tolerance in T5) are deliberate implementation-time decisions with explicit fallback assertions,
not TODOs. ADR body content is described, not code. ✓

**Type consistency:** `ImportBackend`/`BackendRegistry` signatures identical across T1/T2/T4;
`CgltfSource::load` matches the `ImportSource` seam; `X3D_ASSET_IMPORT_FIXTURE_DIR` /
`X3D_ASSET_IMPORT_HAVE_CGLTF` macros consistent across T4/T5 CMake + tests. ✓
