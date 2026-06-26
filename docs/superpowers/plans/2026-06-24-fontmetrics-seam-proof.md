# FontMetrics Seam Genericity Proof — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the `FontMetrics` seam generic by adding two independent real backends (stb_truetype + FreeType) that return bit-identical `advanceEm` for the same glyph, gated by a CI swap-test, then freeze the interface `[EXPERIMENTAL]` → `[STABLE]`.

**Architecture:** Mirror the proven TextureResolver pattern (ADR-0024): each backend is a flag-gated STATIC lib (`X3D_CPP_BUILD_*`, OFF default) whose heavy dependency is linked PRIVATE in one TU and never leaks to consumers; one grouped doctest binary (`x3d_text_tests`) drives identical font fixtures through both backends and asserts identical observable behavior; a dedicated CI job gates it. No `#ifdef` in core. Both backends take the same `family → font-file` map, so their inputs are identical and only the library differs.

**Tech Stack:** C++17, CMake, doctest, stb_truetype (vendored single header), FreeType (via vcpkg), Liberation fonts (OFL-1.1 fixtures), GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-06-24-fontmetrics-seam-genericity-design.md`

## Global Constraints

- **No core `#ifdef`:** the seam header `runtime/extract/FontMetrics.hpp` and `include/x3d/sdk.hpp` must never include a backend header or reference a backend macro.
- **Heavy dep PRIVATE, one TU:** each backend's library includes its dependency (`stb_truetype.h` / FreeType headers) only in its single `.cpp`, linked/included PRIVATE.
- **Backends OFF by default:** `X3D_CPP_BUILD_STBTT` and `X3D_CPP_BUILD_FREETYPE` default OFF; `mise run ci` does NOT enable them — a dedicated CI job does.
- **Equality model:** assert **bit-identical** `advanceEm` for the matrix cell; only if a real ULP gap appears in implementation, fall back to a tight epsilon and record it in ADR-0025. Do not start with tolerance.
- **Adapter contract rules (both backends):** (1) unscaled metrics only (`FT_LOAD_NO_SCALE` / stb's unscaled `GetGlyphHMetrics`); (2) read `unitsPerEm` as the raw `uint16` from the `head` table and compute `advanceEm = float(advance) / float(unitsPerEm)` — never via stb's `ScaleForMappingEmToPixels` reciprocal; (3) `glyphIndex == 0` ⇒ `makeFailed()`; (4) `hasAtlasUv = false`.
- **Scope:** PLAIN style only; synthetic BOLD/ITALIC and atlas-UV/Pending paths are out of scope (the FontFaceMap simply has no non-PLAIN entries → both backends `Failed`).
- **Backend home:** `runtime/io/stbtt/` and `runtime/io/freetype/` (the seam header stays in `runtime/extract/`, exactly as TextureResolver's backends live in `runtime/io/`).
- **NOTICE update is part of DoD** (the docs-drift tool does not catch NOTICE staleness).

---

### Task 1: Vendor stb_truetype + Liberation fixtures + document the adapter contract

**Files:**
- Create: `runtime/io/stbtt/vendor/stb_truetype.h` (downloaded, unmodified)
- Create: `runtime/io/stbtt/vendor/STB_TRUETYPE_LICENSE.txt`
- Create: `runtime/io/tests/fixtures/font/LiberationSerif-Regular.ttf`
- Create: `runtime/io/tests/fixtures/font/LiberationSans-Regular.ttf`
- Create: `runtime/io/tests/fixtures/font/LiberationMono-Regular.ttf`
- Create: `runtime/io/tests/fixtures/font/OFL.txt` (Liberation license text)
- Modify: `runtime/extract/FontMetrics.hpp` (add the adapter-contract comment block)

**Interfaces:**
- Consumes: nothing.
- Produces: the vendored header at `runtime/io/stbtt/vendor/stb_truetype.h`; three TTF fixtures + their license at `runtime/io/tests/fixtures/font/`; documented adapter rules in `FontMetrics.hpp`.

- [ ] **Step 1: Vendor stb_truetype.h**

Download the single header (public domain / MIT) from the canonical source:

```bash
mkdir -p runtime/io/stbtt/vendor
curl -fsSL https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h \
  -o runtime/io/stbtt/vendor/stb_truetype.h
# Capture the version banner for the NOTICE / ADR (e.g. "v1.26").
grep -m1 'stb_truetype.*- v' runtime/io/stbtt/vendor/stb_truetype.h
```

Record the dual license verbatim from the bottom of the header into `runtime/io/stbtt/vendor/STB_TRUETYPE_LICENSE.txt` (the "LICENSE" section near EOF — "This software is available under 2 licenses -- choose whichever you prefer." MIT / Public Domain). This mirrors `runtime/io/wuffs/vendor/WUFFS_LICENSE.txt`.

- [ ] **Step 2: Fetch the Liberation font fixtures**

Liberation Fonts are OFL-1.1, metric-compatible with Times/Arial/Courier — clean SPDX id, simple NOTICE. Download the Regular faces from the upstream release and vendor only the three TTFs we test:

```bash
mkdir -p runtime/io/tests/fixtures/font
cd runtime/io/tests/fixtures/font
curl -fsSL -o liberation.tar.gz \
  https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz
tar -xzf liberation.tar.gz
cp liberation-fonts-ttf-2.1.5/LiberationSerif-Regular.ttf .
cp liberation-fonts-ttf-2.1.5/LiberationSans-Regular.ttf .
cp liberation-fonts-ttf-2.1.5/LiberationMono-Regular.ttf .
cp liberation-fonts-ttf-2.1.5/LICENSE OFL.txt
rm -rf liberation.tar.gz liberation-fonts-ttf-2.1.5
cd -
ls -1 runtime/io/tests/fixtures/font
```

Expected output: `LiberationMono-Regular.ttf`, `LiberationSans-Regular.ttf`, `LiberationSerif-Regular.ttf`, `OFL.txt`. (If the pinned release URL 404s, fetch the latest `liberation-fonts-ttf-*.tar.gz` from https://github.com/liberationfonts/liberation-fonts/releases and adjust the version; record the version used.)

- [ ] **Step 3: Document the adapter contract in the seam header**

In `runtime/extract/FontMetrics.hpp`, immediately above `using FontMetrics = ...` (the line `using FontMetrics = std::function<GlyphResult(const FontKey&)>;`), insert this comment block (comment-only — no code/ABI change):

```cpp
// ---------------------------------------------------------------------------
// Backend adapter contract (T-TEXT genericity proof, ADR-0025).
// A FontMetrics backend that wants to be swap-test-interchangeable MUST:
//   1. Read metrics UNSCALED (no hinting/grid-fitting): the advance is the raw
//      `hmtx` advance in font units.
//   2. Read `unitsPerEm` as the raw uint16 from the `head` table and return
//      advanceEm = float(advance) / float(unitsPerEm). (Do NOT derive em-size
//      from a float pixel-scale reciprocal — that inserts a rounding step and
//      breaks bit-exact parity across backends.)
//   3. Return makeFailed() when the codepoint maps to glyph index 0 (.notdef):
//      glyph 0 carries a valid advance, so without this rule a missing glyph
//      would read as Ready instead of Failed.
//   4. Set hasAtlasUv = false: the atlas/raster path is embedder-defined and
//      is not part of the cross-backend equality surface.
// v1 backends honor PLAIN style only; other styles return Failed.
// ---------------------------------------------------------------------------
```

- [ ] **Step 4: Verify the fixtures load (sanity, no build)**

Confirm the fonts are real TrueType (sfnt 0x00010000) so later tasks' head-table parsing is valid:

```bash
for f in runtime/io/tests/fixtures/font/Liberation*-Regular.ttf; do
  printf '%s: ' "$f"; xxd -l 4 -p "$f"; done
```

Expected: each prints `00010000` (TrueType outline sfnt).

- [ ] **Step 5: Commit**

```bash
git add runtime/io/stbtt/vendor runtime/io/tests/fixtures/font runtime/extract/FontMetrics.hpp
git commit -m "chore(fontmetrics): vendor stb_truetype + Liberation OFL fixtures; document adapter contract

Claude-Session: [redacted]"
```

---

### Task 2: Backend A — `x3d_stbtt` (stb_truetype) + per-backend test

**Files:**
- Create: `runtime/io/stbtt/StbttFontMetrics.hpp`
- Create: `runtime/io/stbtt/StbttFontMetrics.cpp`
- Create: `runtime/io/tests/font_metrics_tests.cpp`
- Modify: `CMakeLists.txt` (add `X3D_CPP_BUILD_STBTT` option + `x3d_stbtt` target + the `x3d_text_tests` binary, gated on `x3d_stbtt` for now)

**Interfaces:**
- Consumes: `x3d::runtime::extract::FontMetrics`, `FontKey`, `GlyphResult` from `FontMetrics.hpp`.
- Produces:
  - `namespace x3d::runtime::io::stbtt { using FontFaceMap = std::map<std::string,std::string>; x3d::runtime::extract::FontMetrics makeStbttFontMetrics(FontFaceMap faces); }` — `faces` maps a `family` string (e.g. `"SERIF"`) to a TTF file path; the returned callback honors PLAIN style only.

- [ ] **Step 1: Write the backend header**

`runtime/io/stbtt/StbttFontMetrics.hpp`:

```cpp
// runtime/io/stbtt/StbttFontMetrics.hpp — Backend A (stb_truetype) for the
// FontMetrics seam (T-TEXT genericity proof, ADR-0025). Part of the
// runtime/io/stbtt quarantine (x3d_stbtt target, default OFF). Core (x3d_cpp,
// sdk.hpp) MUST NEVER include this file; it is decoder-free (no stb_truetype.h),
// so consumers linking x3d_stbtt do not inherit stb's translation unit.
#ifndef X3D_RUNTIME_IO_STBTT_STBTT_FONT_METRICS_HPP
#define X3D_RUNTIME_IO_STBTT_STBTT_FONT_METRICS_HPP

#include "FontMetrics.hpp"  // x3d::runtime::extract::FontMetrics

#include <map>
#include <string>

namespace x3d::runtime::io::stbtt {

/// family -> TTF/OTF file path. v1 supports PLAIN style only.
using FontFaceMap = std::map<std::string, std::string>;

/// Returns a FontMetrics that reads glyph advances via stb_truetype.
///
/// On call with FontKey{family, style, codepoint}: looks up `family` in `faces`;
/// returns Failed if the family is unmapped, the style is not PLAIN, the file
/// cannot be read, or the codepoint maps to glyph 0 (.notdef). On success
/// returns Ready with advanceEm = rawAdvance / rawUnitsPerEm (unscaled), and
/// hasAtlasUv = false. See the adapter contract in FontMetrics.hpp.
x3d::runtime::extract::FontMetrics makeStbttFontMetrics(FontFaceMap faces);

}  // namespace x3d::runtime::io::stbtt

#endif  // X3D_RUNTIME_IO_STBTT_STBTT_FONT_METRICS_HPP
```

- [ ] **Step 2: Write the backend implementation**

`runtime/io/stbtt/StbttFontMetrics.cpp`:

```cpp
// runtime/io/stbtt/StbttFontMetrics.cpp — the ONE translation unit where
// stb_truetype meets the FontMetrics seam. STB_TRUETYPE_IMPLEMENTATION is
// defined here and nowhere else; stb_truetype.h is included PRIVATE (see CMake)
// so it never leaks. The factory returns a std::function exchanging only std
// types, so linking x3d_stbtt pulls in no stb headers.
#include "StbttFontMetrics.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace x3d::runtime::io::stbtt {
namespace {

bool readFile(const std::string& path, std::vector<unsigned char>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size <= 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()), size));
}

// One initialized face: the byte buffer (stbtt_fontinfo points into it, so it
// must outlive the info) + the parsed font info.
struct Face {
  std::vector<unsigned char> bytes;
  stbtt_fontinfo info{};
  bool ok = false;
};

// Lazily load + init the face for a family path; cache by path.
struct State {
  FontFaceMap faces;
  std::unordered_map<std::string, std::shared_ptr<Face>> cache;

  std::shared_ptr<Face> faceFor(const std::string& family) {
    const auto it = faces.find(family);
    if (it == faces.end()) return nullptr;
    const std::string& path = it->second;
    auto cached = cache.find(path);
    if (cached != cache.end()) return cached->second;

    auto face = std::make_shared<Face>();
    if (readFile(path, face->bytes)) {
      const int off = stbtt_GetFontOffsetForIndex(face->bytes.data(), 0);
      if (off >= 0 && stbtt_InitFont(&face->info, face->bytes.data(), off)) {
        face->ok = true;
      }
    }
    cache.emplace(path, face);
    return face;
  }
};

// Raw unitsPerEm: the uint16 at head + 18 (big-endian). info.head is the public
// byte offset of the 'head' table within info.data. Reading it directly (rather
// than 1/stbtt_ScaleForMappingEmToPixels) keeps the value an exact integer.
std::uint16_t unitsPerEm(const stbtt_fontinfo& info) {
  const unsigned char* p = info.data + info.head + 18;
  return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

}  // namespace

x3d::runtime::extract::FontMetrics makeStbttFontMetrics(FontFaceMap faces) {
  using x3d::runtime::extract::GlyphMetrics;
  using x3d::runtime::extract::GlyphResult;
  using x3d::runtime::extract::FontKey;

  auto state = std::make_shared<State>();
  state->faces = std::move(faces);

  return [state](const FontKey& key) -> GlyphResult {
    if (!key.style.empty() && key.style != "PLAIN") return GlyphResult::makeFailed();

    std::shared_ptr<Face> face = state->faceFor(key.family);
    if (!face || !face->ok) return GlyphResult::makeFailed();

    const int glyph = stbtt_FindGlyphIndex(&face->info, static_cast<int>(key.codepoint));
    if (glyph == 0) return GlyphResult::makeFailed();  // .notdef → Failed (rule 3)

    int advance = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&face->info, glyph, &advance, &lsb);  // unscaled (rule 1)

    const std::uint16_t upm = unitsPerEm(face->info);            // raw uint16 (rule 2)
    if (upm == 0) return GlyphResult::makeFailed();

    const float advanceEm = static_cast<float>(advance) / static_cast<float>(upm);
    return GlyphResult::makeReady(GlyphMetrics{advanceEm, false, 0.f, 0.f, 0.f, 0.f});
  };
}

}  // namespace x3d::runtime::io::stbtt
```

- [ ] **Step 3: Write the failing per-backend test**

`runtime/io/tests/font_metrics_tests.cpp`:

```cpp
// runtime/io/tests/font_metrics_tests.cpp — the FontMetrics seam genericity
// proof (ADR-0025), one grouped doctest binary.
//
//   per-backend: stb_truetype reads advances + rejects bad input
//   per-backend: FreeType reads advances + rejects bad input   (added Task 3)
//   swap-test:   stbtt vs FreeType return EXACT-equal advanceEm,
//                plus Failed-parity on absent codepoints         (added Task 4)
//
// ABI ISOLATION (the AssetResolver lesson, ADR-0023): this TU talks ONLY through
// the seam factories and includes NO backend headers' heavy deps. The factories
// exchange std types, so no heavy backend header can leak in.
#include "FontMetrics.hpp"
#include "StbttFontMetrics.hpp"
#include "doctest/doctest.h"

#include <map>
#include <string>

using namespace x3d::runtime::extract;

namespace {

std::string fontDir() { return std::string(FIXTURES_DIR); }

std::map<std::string, std::string> liberationFaces() {
  return {
      {"SERIF", fontDir() + "/LiberationSerif-Regular.ttf"},
      {"SANS", fontDir() + "/LiberationSans-Regular.ttf"},
      {"TYPEWRITER", fontDir() + "/LiberationMono-Regular.ttf"},
  };
}

// Codepoints present in Liberation's cmap, and one guaranteed absent.
constexpr std::uint32_t kPresent[] = {'A', 'i', 'M', ' ', '0'};
constexpr std::uint32_t kAbsentEmoji = 0x1F600;  // 😀 — not in Liberation.

}  // namespace

TEST_CASE("fontmetrics_backend_a_stbtt") {
  FontMetrics fm = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  CHECK((static_cast<bool>(fm)));

  for (std::uint32_t cp : kPresent) {
    CAPTURE(cp);
    const GlyphResult g = fm(FontKey{"SANS", "PLAIN", cp});
    CHECK((g.ready()));
    CHECK((g.metrics.advanceEm > 0.0f));
    CHECK((g.metrics.advanceEm < 2.0f));
    CHECK((g.metrics.hasAtlasUv == false));
  }

  CHECK((fm(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));   // .notdef → Failed
  CHECK((fm(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));        // family not in map
  CHECK((fm(FontKey{"SANS", "BOLD", 'A'}).failed()));            // style out of scope
}
```

- [ ] **Step 4: Add the CMake option, the `x3d_stbtt` target, and the test binary**

In `CMakeLists.txt`, after the existing `x3d_wuffs` block (ends near line 552, before the `x3d_texture_tests` block), add:

```cmake
# ---------------------------------------------------------------------------
# FontMetrics seam — Backend A (stb_truetype). ADR-0025 / U1.
# option() -> an isolated STATIC lib (x3d_stbtt) whose single TU is
# StbttFontMetrics.cpp; stb_truetype.h is included PRIVATE so it meets the seam
# in ONE TU and never leaks. Build: cmake --preset dev -DX3D_CPP_BUILD_STBTT=ON
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_STBTT "Build stb_truetype FontMetrics backend (OFF default)" OFF)

if(X3D_CPP_BUILD_STBTT)
    add_library(x3d_stbtt STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/StbttFontMetrics.cpp")
    target_link_libraries(x3d_stbtt PUBLIC x3d_cpp::x3d_cpp)
    target_include_directories(x3d_stbtt PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt")
    # stb_truetype.h is PRIVATE: only StbttFontMetrics.cpp needs it; the factory
    # returns a std::function exchanging std types (decoder-free header).
    target_include_directories(x3d_stbtt PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/stbtt/vendor")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(x3d_stbtt PRIVATE -w)  # stb is warning-noisy.
    endif()
endif()

# ---------------------------------------------------------------------------
# U3 — the FontMetrics genericity proof. One grouped doctest binary drives
# identical font fixtures through stb_truetype and FreeType and asserts
# EXACT-equal advanceEm + Failed-parity. Gated on both backends once Task 3
# lands FreeType; for now gated on x3d_stbtt so Backend A is testable alone.
# Run: ctest -R x3d_text   (hermetic + offline — local font files only).
# ---------------------------------------------------------------------------
if(TARGET x3d_stbtt AND X3D_CPP_BUILD_TESTS)
    enable_testing()
    add_executable(x3d_text_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support/doctest_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/font_metrics_tests.cpp")
    target_link_libraries(x3d_text_tests PRIVATE x3d_stbtt)
    target_include_directories(x3d_text_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/test_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/extract")
    target_compile_definitions(x3d_text_tests PRIVATE
        FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/tests/fixtures/font")
    add_test(NAME x3d_text_tests COMMAND x3d_text_tests)
    set_tests_properties(x3d_text_tests PROPERTIES TIMEOUT 120)
endif()
```

- [ ] **Step 5: Configure + build + run the test (expect PASS)**

```bash
cmake -S . -B build-font -G Ninja -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-font --target x3d_text_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

Expected: `fontmetrics_backend_a_stbtt` passes (present glyphs Ready with sane advanceEm; absent/unmapped/non-PLAIN Failed). If the build fails to find `stb_truetype.h`, recheck the PRIVATE include dir in the CMake block.

- [ ] **Step 6: Commit**

```bash
git add runtime/io/stbtt CMakeLists.txt runtime/io/tests/font_metrics_tests.cpp
git commit -m "feat(fontmetrics): backend A — x3d_stbtt (stb_truetype), unscaled advanceEm

Claude-Session: [redacted]"
```

---

### Task 3: Backend B — `x3d_freetype` (FreeType, via vcpkg) + per-backend test

**Files:**
- Create: `runtime/io/freetype/FreetypeFontMetrics.hpp`
- Create: `runtime/io/freetype/FreetypeFontMetrics.cpp`
- Modify: `CMakeLists.txt` (add `X3D_CPP_BUILD_FREETYPE` option + `x3d_freetype` target; flip `x3d_text_tests` gate to require BOTH backends and link both)
- Modify: `runtime/io/tests/font_metrics_tests.cpp` (add the Backend B per-backend case)

**Interfaces:**
- Consumes: `FontMetrics`, `FontKey`, `GlyphResult` from `FontMetrics.hpp`.
- Produces:
  - `namespace x3d::runtime::io::freetype { using FontFaceMap = std::map<std::string,std::string>; x3d::runtime::extract::FontMetrics makeFreetypeFontMetrics(FontFaceMap faces); }` — identical semantics + signature shape to the stbtt factory.

- [ ] **Step 1: Write the backend header**

`runtime/io/freetype/FreetypeFontMetrics.hpp`:

```cpp
// runtime/io/freetype/FreetypeFontMetrics.hpp — Backend B (FreeType) for the
// FontMetrics seam (T-TEXT genericity proof, ADR-0025). Part of the
// runtime/io/freetype quarantine (x3d_freetype target, default OFF). Core
// (x3d_cpp, sdk.hpp) MUST NEVER include this file; it is FreeType-free (no
// <ft2build.h>), so consumers linking x3d_freetype do not inherit FreeType.
#ifndef X3D_RUNTIME_IO_FREETYPE_FREETYPE_FONT_METRICS_HPP
#define X3D_RUNTIME_IO_FREETYPE_FREETYPE_FONT_METRICS_HPP

#include "FontMetrics.hpp"  // x3d::runtime::extract::FontMetrics

#include <map>
#include <string>

namespace x3d::runtime::io::freetype {

/// family -> TTF/OTF file path. v1 supports PLAIN style only.
using FontFaceMap = std::map<std::string, std::string>;

/// Returns a FontMetrics that reads glyph advances via FreeType. Same observable
/// contract as makeStbttFontMetrics: unscaled advance / raw unitsPerEm, glyph 0
/// → Failed, hasAtlasUv = false, PLAIN only. See the contract in FontMetrics.hpp.
x3d::runtime::extract::FontMetrics makeFreetypeFontMetrics(FontFaceMap faces);

}  // namespace x3d::runtime::io::freetype

#endif  // X3D_RUNTIME_IO_FREETYPE_FREETYPE_FONT_METRICS_HPP
```

- [ ] **Step 2: Write the backend implementation**

`runtime/io/freetype/FreetypeFontMetrics.cpp`. Uses `FT_LOAD_NO_SCALE` so the advance is in font units, and `face->units_per_EM` (the same `head` uint16 stb reads), so the divide is bit-identical:

```cpp
// runtime/io/freetype/FreetypeFontMetrics.cpp — the ONE translation unit where
// FreeType meets the FontMetrics seam. <ft2build.h> is included PRIVATE (see
// CMake) so it never leaks. The factory returns a std::function exchanging only
// std types, so linking x3d_freetype pulls in no FreeType headers.
#include "FreetypeFontMetrics.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

namespace x3d::runtime::io::freetype {
namespace {

// Owns the FT_Library + lazily-opened faces; tears them down on destruction.
struct State {
  FontFaceMap faces;
  FT_Library lib = nullptr;
  std::unordered_map<std::string, FT_Face> cache;  // path -> face (nullptr = failed)

  ~State() {
    for (auto& kv : cache)
      if (kv.second) FT_Done_Face(kv.second);
    if (lib) FT_Done_FreeType(lib);
  }

  FT_Face faceFor(const std::string& family) {
    const auto it = faces.find(family);
    if (it == faces.end()) return nullptr;
    const std::string& path = it->second;
    auto cached = cache.find(path);
    if (cached != cache.end()) return cached->second;

    FT_Face face = nullptr;
    if (FT_New_Face(lib, path.c_str(), 0, &face) != 0) face = nullptr;
    cache.emplace(path, face);
    return face;
  }
};

}  // namespace

x3d::runtime::extract::FontMetrics makeFreetypeFontMetrics(FontFaceMap faces) {
  using x3d::runtime::extract::GlyphMetrics;
  using x3d::runtime::extract::GlyphResult;
  using x3d::runtime::extract::FontKey;

  auto state = std::make_shared<State>();
  state->faces = std::move(faces);
  if (FT_Init_FreeType(&state->lib) != 0) state->lib = nullptr;

  return [state](const FontKey& key) -> GlyphResult {
    if (!state->lib) return GlyphResult::makeFailed();
    if (!key.style.empty() && key.style != "PLAIN") return GlyphResult::makeFailed();

    FT_Face face = state->faceFor(key.family);
    if (!face) return GlyphResult::makeFailed();

    const FT_UInt glyph = FT_Get_Char_Index(face, key.codepoint);
    if (glyph == 0) return GlyphResult::makeFailed();  // .notdef → Failed (rule 3)

    FT_Fixed advance = 0;  // font units when FT_LOAD_NO_SCALE is set (rule 1)
    if (FT_Get_Advance(face, glyph, FT_LOAD_NO_SCALE, &advance) != 0)
      return GlyphResult::makeFailed();

    const std::uint16_t upm = static_cast<std::uint16_t>(face->units_per_EM);  // raw (rule 2)
    if (upm == 0) return GlyphResult::makeFailed();

    const float advanceEm = static_cast<float>(advance) / static_cast<float>(upm);
    return GlyphResult::makeReady(GlyphMetrics{advanceEm, false, 0.f, 0.f, 0.f, 0.f});
  };
}

}  // namespace x3d::runtime::io::freetype
```

> Note on rule 2: with `FT_LOAD_NO_SCALE`, `FT_Get_Advance` returns the advance in font units as an `FT_Fixed` that holds the plain integer (no 16.16 scaling — the no-scale path bypasses fixed-point scaling). It equals stb's `stbtt_GetGlyphHMetrics` advance. If a build surfaces a 16.16-scaled value here, that is the ULP-gap signal to investigate before reaching for tolerance.

- [ ] **Step 3: Add the FreeType per-backend test case**

Append to `runtime/io/tests/font_metrics_tests.cpp` (after the stbtt case), and add `#include "FreetypeFontMetrics.hpp"` next to the existing `#include "StbttFontMetrics.hpp"`:

```cpp
TEST_CASE("fontmetrics_backend_b_freetype") {
  FontMetrics fm = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());
  CHECK((static_cast<bool>(fm)));

  for (std::uint32_t cp : kPresent) {
    CAPTURE(cp);
    const GlyphResult g = fm(FontKey{"SANS", "PLAIN", cp});
    CHECK((g.ready()));
    CHECK((g.metrics.advanceEm > 0.0f));
    CHECK((g.metrics.advanceEm < 2.0f));
    CHECK((g.metrics.hasAtlasUv == false));
  }

  CHECK((fm(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));
  CHECK((fm(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));
  CHECK((fm(FontKey{"SANS", "BOLD", 'A'}).failed()));
}
```

- [ ] **Step 4: Add the CMake option + `x3d_freetype` target; flip the test gate to both backends**

In `CMakeLists.txt`, add after the `x3d_stbtt` block:

```cmake
# ---------------------------------------------------------------------------
# FontMetrics seam — Backend B (FreeType). ADR-0025 / U2.
# FreeType is NOT single-header; it is provided via vcpkg (find_package). Its
# headers are PRIVATE to the one TU (FreetypeFontMetrics.cpp), so no FreeType
# header leaks to consumers. Build: -DX3D_CPP_BUILD_FREETYPE=ON with the vcpkg
# toolchain file (see the CI job / spec §6).
# ---------------------------------------------------------------------------
option(X3D_CPP_BUILD_FREETYPE "Build FreeType FontMetrics backend (OFF default)" OFF)

if(X3D_CPP_BUILD_FREETYPE)
    find_package(Freetype REQUIRED)
    add_library(x3d_freetype STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype/FreetypeFontMetrics.cpp")
    target_link_libraries(x3d_freetype PUBLIC x3d_cpp::x3d_cpp)
    target_link_libraries(x3d_freetype PRIVATE Freetype::Freetype)  # heavy dep PRIVATE
    target_include_directories(x3d_freetype PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/io/freetype")
endif()
```

Then change the `x3d_text_tests` gate from `if(TARGET x3d_stbtt AND X3D_CPP_BUILD_TESTS)` to require both backends and link both:

```cmake
if(TARGET x3d_stbtt AND TARGET x3d_freetype AND X3D_CPP_BUILD_TESTS)
```
```cmake
    target_link_libraries(x3d_text_tests PRIVATE x3d_stbtt x3d_freetype)
```

(Leave the `add_executable`, include dirs, `FIXTURES_DIR`, `add_test`, and TIMEOUT lines unchanged.)

- [ ] **Step 5: Configure with vcpkg + build + run (expect PASS)**

Bootstrap vcpkg if needed and install FreeType, then configure with the toolchain file:

```bash
[ -x "$HOME/vcpkg/vcpkg" ] || { git clone --depth 1 https://github.com/microsoft/vcpkg "$HOME/vcpkg" && "$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics; }
"$HOME/vcpkg/vcpkg" install freetype --triplet x64-linux
cmake -S . -B build-font -G Ninja \
  -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON -DX3D_CPP_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-linux
cmake --build build-font --target x3d_text_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

Expected: both `fontmetrics_backend_a_stbtt` and `fontmetrics_backend_b_freetype` pass.

- [ ] **Step 6: Commit**

```bash
git add runtime/io/freetype CMakeLists.txt runtime/io/tests/font_metrics_tests.cpp
git commit -m "feat(fontmetrics): backend B — x3d_freetype (FreeType via vcpkg)

Claude-Session: [redacted]"
```

---

### Task 4: The swap-test — exact-equal advanceEm + Failed-parity

**Files:**
- Modify: `runtime/io/tests/font_metrics_tests.cpp` (add the two swap cases)

**Interfaces:**
- Consumes: `makeStbttFontMetrics`, `makeFreetypeFontMetrics`, `liberationFaces()`, `kPresent`, `kAbsentEmoji` from Tasks 2–3.
- Produces: the genericity proof — `x3d_text_tests` asserts the two backends are observably interchangeable.

- [ ] **Step 1: Write the swap-test cases (the proof)**

Append to `runtime/io/tests/font_metrics_tests.cpp`:

```cpp
TEST_CASE("fontmetrics_swap_advance_exact_equal") {
  FontMetrics a = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  FontMetrics b = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());

  const char* families[] = {"SERIF", "SANS", "TYPEWRITER"};
  for (const char* fam : families) {
    for (std::uint32_t cp : kPresent) {
      CAPTURE(fam);
      CAPTURE(cp);
      const GlyphResult ga = a(FontKey{fam, "PLAIN", cp});
      const GlyphResult gb = b(FontKey{fam, "PLAIN", cp});
      CHECK((ga.ready()));
      CHECK((gb.ready()));
      // The proof: two independent libraries, bit-identical advanceEm.
      CHECK((ga.metrics.advanceEm == gb.metrics.advanceEm));
    }
  }
}

TEST_CASE("fontmetrics_swap_failed_parity") {
  FontMetrics a = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  FontMetrics b = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());

  // Absent codepoint → glyph 0 → Failed on BOTH (the .notdef contract rule).
  CHECK((a(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));
  CHECK((b(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));

  // Unmapped family → Failed on both.
  CHECK((a(FontKey{"NOPE", "PLAIN", 'A'}).failed()));
  CHECK((b(FontKey{"NOPE", "PLAIN", 'A'}).failed()));

  // Out-of-scope style → Failed on both (synthetic BOLD/ITALIC not in v1).
  CHECK((a(FontKey{"SANS", "BOLD", 'A'}).failed()));
  CHECK((b(FontKey{"SANS", "BOLD", 'A'}).failed()));
}
```

- [ ] **Step 2: Build + run (expect PASS)**

```bash
cmake --build build-font --target x3d_text_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

Expected: all four test cases pass, including `fontmetrics_swap_advance_exact_equal`.

**If the exact-equal CHECK fails** on a genuine last-ULP difference (not a logic bug): this is the documented fallback path. Replace `ga.metrics.advanceEm == gb.metrics.advanceEm` with `doctest::Approx(gb.metrics.advanceEm).epsilon(1e-6)`, capture the observed delta, and record the gap + chosen epsilon in ADR-0025 (Task 6). First confirm it is not a logic bug (e.g. FreeType returning a 16.16-scaled advance — see the Task 3 note) before accepting tolerance.

- [ ] **Step 3: Commit**

```bash
git add runtime/io/tests/font_metrics_tests.cpp
git commit -m "test(fontmetrics): swap-test — stbtt vs FreeType exact-equal advanceEm + Failed-parity

Claude-Session: [redacted]"
```

---

### Task 5: CI job — gate the swap-test (vcpkg FreeType + binary cache)

**Files:**
- Modify: `.github/workflows/ci.yml` (add a `fontmetrics-swap` job after `texture-swap`)

**Interfaces:**
- Consumes: the `x3d_text_tests` target + the `X3D_CPP_BUILD_STBTT`/`X3D_CPP_BUILD_FREETYPE` flags.
- Produces: a permanent PR gate proving decode-parity can never silently break.

- [ ] **Step 1: Add the CI job**

In `.github/workflows/ci.yml`, after the `texture-swap` job block (ends near line 262, before the heavy baseline matrix comment), add. This mirrors the `assetresolver-swap` vcpkg setup (bootstrap + binary cache) but installs only `freetype`, and scopes the build to the one target:

```yaml
  # FontMetrics seam (ADR-0025): two independent backends — stb_truetype
  # (vendored single header) and FreeType (vcpkg) — must return BIT-IDENTICAL
  # advanceEm for the same glyph, plus Failed-parity on absent codepoints. stb is
  # vendored; FreeType comes from vcpkg with a binary cache so the cold build is
  # paid once. On every PR, so a future change that breaks metric parity fails.
  fontmetrics-swap:
    name: FontMetrics seam swap-test
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install build tools
        run: sudo apt-get update && sudo apt-get install -y cmake ninja-build ccache g++ mold curl zip unzip tar
      - name: Locate or bootstrap vcpkg
        run: |
          if [ -n "$VCPKG_INSTALLATION_ROOT" ] && [ -x "$VCPKG_INSTALLATION_ROOT/vcpkg" ]; then
            echo "VCPKG_ROOT=$VCPKG_INSTALLATION_ROOT" >> "$GITHUB_ENV"
          else
            git clone --depth 1 https://github.com/microsoft/vcpkg "$HOME/vcpkg"
            "$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
            echo "VCPKG_ROOT=$HOME/vcpkg" >> "$GITHUB_ENV"
          fi
      - name: Cache vcpkg binary archives
        uses: actions/cache@v4
        with:
          path: ~/.cache/vcpkg/archives
          # Bump alongside the `vcpkg install` line below.
          key: vcpkg-${{ runner.os }}-freetype
          restore-keys: vcpkg-${{ runner.os }}-
      - name: Install FreeType via vcpkg
        run: '"$VCPKG_ROOT/vcpkg" install freetype --triplet x64-linux'
      - name: ccache cache
        uses: actions/cache@v4
        with:
          path: ~/.cache/ccache
          key: ccache-fontmetrics-${{ github.sha }}
          restore-keys: ccache-fontmetrics-
      - name: Configure (stbtt + FreeType backends ON)
        run: |
          cmake -S . -B build -G Ninja \
            -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON \
            -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
            -DVCPKG_TARGET_TRIPLET=x64-linux
      # Build only the FontMetrics test binary — the `cpp` job covers the full build.
      - name: Build (FontMetrics swap-test target only)
        run: cmake --build build --target x3d_text_tests
      - name: Swap-test (stb_truetype vs FreeType exact-equal advanceEm)
        run: ctest --test-dir build --output-on-failure -R x3d_text
```

- [ ] **Step 2: Lint the workflow YAML**

```bash
python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/ci.yml')); print('ci.yml: valid YAML')"
```

Expected: `ci.yml: valid YAML`.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "ci(fontmetrics): gate the stbtt-vs-FreeType swap-test (vcpkg + binary cache)

Claude-Session: [redacted]"
```

---

### Task 6: U4 — freeze interface STABLE + ADR-0025 + docs + NOTICE

**Files:**
- Modify: `include/x3d/sdk.hpp` (two sites: include-line + using-block comments)
- Create: `docs/wiki/decisions/0025-fontmetrics-second-backend-swap-test.md`
- Create: `docs/wiki/subsystems/system-font-metrics.md`
- Modify: `docs/wiki/seam-status.md` (flip the FontMetrics row + add the GREEN-row prose + CI bullet + front-matter)
- Modify: `docs/wiki/coverage.md` (add the ADR-0025 row + FontMetrics seam subsystem row; update the prose count)
- Modify: `mkdocs.yml` (nav entries for the subsystem page + the ADR)
- Modify: `NOTICE` (stb_truetype + Liberation fonts vendored rows; FreeType optional row)

**Interfaces:**
- Consumes: the proven swap-test (`x3d_text_tests` green in CI) as the evidence for promotion.
- Produces: the GREEN seam row + frozen `[STABLE]` interface + living docs.

- [ ] **Step 1: Freeze the interface in `include/x3d/sdk.hpp`**

Edit the include line (currently line 52):

```cpp
#include "FontMetrics.hpp"         // x3d::runtime::extract — FontMetrics seam     [EXPERIMENTAL]
```
to
```cpp
#include "FontMetrics.hpp"         // x3d::runtime::extract — FontMetrics seam     [STABLE] (proven generic: stb_truetype + FreeType; see ADR-0025)
```

In the seam using-block (around lines 147–154, currently under the `[EXPERIMENTAL]` banner at line 122), add a `[STABLE]` note above the `FontMetrics` usings mirroring the texture STABLE note at line 136:

```cpp
// FontMetrics seam (T-TEXT) — [STABLE] (proven generic: stb_truetype + FreeType,
// exact-equal advanceEm swap-test; see ADR-0025).
```

- [ ] **Step 2: Author ADR-0025**

Create `docs/wiki/decisions/0025-fontmetrics-second-backend-swap-test.md` from the ADR-0024 template (`docs/wiki/decisions/0024-textureresolver-second-backend-swap-test.md`). It MUST state: the binding rule; Backend A = stb_truetype (`runtime/io/stbtt/`, `X3D_CPP_BUILD_STBTT`), Backend B = FreeType via vcpkg (`runtime/io/freetype/`, `X3D_CPP_BUILD_FREETYPE`); the swap-test target `x3d_text_tests` + the Liberation OFL fixtures; the **equality criterion actually achieved** (exact bit-identical `advanceEm`, or — if Task 4 fell back — the epsilon and the measured gap); the **dropped cells** (synthetic BOLD/ITALIC out of scope — stb has no embolden/oblique; atlas-UV/Pending out of scope); the `.notdef → Failed` contract rule; the `fontmetrics-swap` CI job; the `[STABLE]` promotion; and the NOTICE rows. Status: Accepted.

- [ ] **Step 3: Create the subsystem page**

Create `docs/wiki/subsystems/system-font-metrics.md` from the `system-texture-decode.md` template: the seam contract, the two backends + paths/flags, the swap matrix (PLAIN-only exact-equal advanceEm), the adapter contract rules, and the CI gate. Use front-matter consistent with the other subsystem pages (title/summary/tags/related, citing ADR-0025 + `seam-status.md`).

- [ ] **Step 4: Flip the seam-status row**

In `docs/wiki/seam-status.md`, replace the FontMetrics row (line 39):

```markdown
| FontMetrics | EXPERIMENTAL | monospace stub | — pending | — | — |
```
with
```markdown
| **FontMetrics** | **STABLE** | stb_truetype (StbttFontMetrics) | FreeType (FreetypeFontMetrics) | `x3d_text_tests` ✓ | thesis-completion (no findings) |
```

Add a "GREEN row: FontMetrics" prose section modeled on the "GREEN row: TextureResolver" section (interface frozen note; Backend A/B paths; swap matrix; the CI-gate bullet naming `fontmetrics-swap`). Bump the `updated:` front-matter date to `2026-06-24` and add ADR-0025 to `related:`.

- [ ] **Step 5: Update coverage.md**

In `docs/wiki/coverage.md`, add a decision row mirroring the ADR-0024 row (line 108):

```markdown
| covered | `decisions/0025-fontmetrics-second-backend-swap-test.md` | The genericity-proof pattern applied to the FontMetrics seam: a second backend (FreeType) alongside stb_truetype + the CI-gated `x3d_text_tests` exact-equal advanceEm swap-test over Liberation fixtures (PLAIN style) | `2026-06-24-fontmetrics-seam-genericity-design.md` |
```

Add a subsystem `covered` row for `subsystems/system-font-metrics.md` (mirror the `system-texture-decode.md` row at line 63), and update the closing prose paragraph (line 145) to mention the FontMetrics seam going GREEN with ADR-0025.

- [ ] **Step 6: Add the mkdocs nav entries**

In `mkdocs.yml`, add the subsystem page under the subsystems nav group (next to `Texture Decode Seam` at line 89):

```yaml
          - Font Metrics Seam: subsystems/system-font-metrics.md
```

and the ADR under the decisions nav group (after the ADR-0024 entry):

```yaml
      - "ADR-0025: FontMetrics Second Backend + Swap-Test": decisions/0025-fontmetrics-second-backend-swap-test.md
```

- [ ] **Step 7: Update NOTICE**

In `NOTICE` section 2 (Vendored, after the wuffs row at line 50), add:

```
  stb_truetype.h    runtime/io/stbtt/vendor/stb_truetype.h   MIT OR Public Domain  (-DX3D_CPP_BUILD_STBTT=ON)
                    (see runtime/io/stbtt/vendor/STB_TRUETYPE_LICENSE.txt)
  Liberation Fonts  runtime/io/tests/fixtures/font/*.ttf      OFL-1.1  (test fixtures)
                    (see runtime/io/tests/fixtures/font/OFL.txt)
```

In NOTICE section 3 (Optional build-time deps, after the AWS SDK row), add:

```
  FreeType          freetype.org (via vcpkg)                 FTL OR GPL-2.0  (-DX3D_CPP_BUILD_FREETYPE=ON)
```

- [ ] **Step 8: Build the docs strict + run the drift check**

```bash
mise run docs-build      # strict: dead links / nav orphans fail
mise run docs-drift working
```

Expected: `docs-build` succeeds (the new subsystem page + ADR are reachable from nav and not orphaned; no dead links). Review the drift output; it is advisory.

- [ ] **Step 9: Refresh the RAG stores (symbols moved)**

```bash
mise run code-ingest
mise run docs-ingest
```

- [ ] **Step 10: Commit**

```bash
git add include/x3d/sdk.hpp docs/wiki NOTICE mkdocs.yml
git commit -m "docs(fontmetrics): freeze seam STABLE + ADR-0025 + subsystem page + NOTICE

Promotes FontMetrics [EXPERIMENTAL]->[STABLE], proven generic by the
stbtt-vs-FreeType exact-equal advanceEm swap-test (ADR-0025). Flips the
seam-status row GREEN and adds the subsystem/coverage/nav/NOTICE rows.

Claude-Session: [redacted]"
```

---

### Task 7: Final verification

**Files:** none (verification only).

- [ ] **Step 1: Full swap-test from a clean configure**

```bash
rm -rf build-font
cmake -S . -B build-font -G Ninja \
  -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON -DX3D_CPP_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-linux
cmake --build build-font --target x3d_text_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

Expected: all four cases pass (two per-backend + two swap).

- [ ] **Step 2: Confirm the default build still excludes the backends (no leak)**

```bash
cmake -S . -B build-default -G Ninja
grep -i "stbtt\|freetype\|x3d_text" build-default/CMakeCache.txt || echo "OK: backends OFF by default"
```

Expected: `X3D_CPP_BUILD_STBTT:BOOL=OFF` / `X3D_CPP_BUILD_FREETYPE:BOOL=OFF` (the targets are not built); confirms no core dependency on the backends.

- [ ] **Step 3: Confirm no core file includes a backend header**

```bash
grep -rn "StbttFontMetrics.hpp\|FreetypeFontMetrics.hpp\|stb_truetype.h\|ft2build.h" include/ runtime/extract/FontMetrics.hpp || echo "OK: no backend leak into core"
```

Expected: `OK: no backend leak into core`.

- [ ] **Step 4: Run the existing FontMetrics consumer tests (regression)**

```bash
cmake --build build-default --target x3d_extract_tests 2>/dev/null || true
ctest --test-dir build-default --output-on-failure -R "text|extract" || true
```

Expected: existing text/extract tests still pass (the seam header change was comment-only; `makeMonospaceStub` is untouched).

---

## Self-Review

**Spec coverage:**
- Spec §2.1 adapter rules → Task 1 Step 3 (documented) + implemented in Tasks 2–3. ✓
- Spec §3 swap matrix (PLAIN exact-equal advanceEm; absent→Failed; BOLD/ITALIC + atlas/Pending out-of-scope) → Task 4 + the per-backend Failed cases. ✓
- Spec §3.1 equality model (exact, tolerance fallback) → Task 4 Step 2 fallback note + ADR record in Task 6. ✓
- Spec §5 U1/U2 backends → Tasks 2–3; U3 swap-test → Task 4; U4 freeze/docs → Task 6. ✓
- Spec §6 build/CI (vcpkg FreeType + binary cache, dedicated job) → Task 5. ✓
- Spec §7 NOTICE rows → Task 6 Step 7. ✓
- Spec §4 scope (composer deferred) → not implemented (correct — out of scope). ✓
- Spec §9 DoD checklist → covered across Tasks 2–7. ✓

**Placeholder scan:** No TBD/TODO; every code/CMake/YAML step shows full content. The only deliberate "fill at implementation time" items are the ADR-0025 / subsystem-page prose (Task 6 Steps 2–3), which name the exact template file, required contents, and citations — appropriate for a docs authoring step, not a code placeholder.

**Type consistency:** Both factories use the same shapes — `FontFaceMap = std::map<std::string,std::string>`, `make{Stbtt,Freetype}FontMetrics(FontFaceMap)`, returning `x3d::runtime::extract::FontMetrics`. The test uses `liberationFaces()`, `kPresent`, `kAbsentEmoji`, `FontKey{family,style,codepoint}`, `GlyphResult::ready()/failed()`, `metrics.advanceEm`, `metrics.hasAtlasUv` — all consistent with `FontMetrics.hpp`. CMake target names `x3d_stbtt`/`x3d_freetype`/`x3d_text_tests` and flags `X3D_CPP_BUILD_STBTT`/`X3D_CPP_BUILD_FREETYPE` are consistent across Tasks 2–7.
