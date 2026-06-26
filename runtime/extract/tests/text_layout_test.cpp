// text_layout_test.cpp — Unit tests for TextLayout.hpp (§15 layout engine).
//
// Uses a monospaced stub for font metrics:
//   natural_advance = n_chars * size   (each glyph is exactly one 'em' wide)
//   ascender        = 0.8f * size
//   descender       = -0.2f * size
//
// Tests are purely geometric — they verify the computed baseline origins,
// lineBounds, textBounds and origin against analytically derived values.
//
// NOTE: cmake-integration is done in the Integrate phase. This file is
// syntax-checked with g++ -fsyntax-only; it is NOT linked/executed here.
#include "TextLayout.hpp"

#include "doctest/doctest.h"
#include <cmath>
#include <string>
#include <tuple>
#include <vector>

using namespace x3d::runtime::extract;

// ---------------------------------------------------------------------------
// Monospaced stub: advance = n_chars * size, ascender = 0.8*size, desc = -0.2*size
// ---------------------------------------------------------------------------
static FontMetricsCallback monoMetrics() {
    return [](const std::string& s, float sz) -> std::tuple<float, float, float> {
        float advance   = static_cast<float>(s.size()) * sz;
        float ascender  = 0.8f * sz;
        float descender = -0.2f * sz;
        return {advance, ascender, descender};
    };
}

// ---------------------------------------------------------------------------
// Tolerance for float comparisons.
// ---------------------------------------------------------------------------
static bool feq(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Helper: check bool with message (mirrors the rest of the test harness).
// ---------------------------------------------------------------------------
static void check(bool cond, const char* msg) {
    if (!cond) {
        // In the full harness this throws; here we use assert so the
        // syntax-check TU compiles and a linked binary asserts on failure.
        CHECK((cond && msg));
        (void)msg;
    }
}

// ===========================================================================
// Test 1: Single line, default params (horizontal, BEGIN/FIRST, size=1).
//
// With size=1, spacing=1, string "abc" (3 chars):
//   natural_advance = 3.0
//   ascender = 0.8, descender = -0.2
//   baselineStep = 1.0 * 1.0 = 1.0
//   minor FIRST → firstBaselineMinor = 0.0  (baseline at Y=0)
//   major BEGIN, leftToRight=T → baselineX = 0.0
//   → lineBaselineOrigins[0] = {0, 0}
//   lineBounds[0] = {3.0, 1.0}
//   textBounds = {3.0, 1.0}  (totalBlockExtent = 0*step + size = 1.0)
//   upper-left corner: left = 0, top = 0 + 0.8 = 0.8
//   origin = {0, 0.8, 0}
// ===========================================================================
static void test_single_line_defaults() {
    FontStyleParams fs; // all defaults
    TextParams t;
    t.strings = {"abc"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(r.lineBounds.size() == 1,      "single: lineBounds count");
    check(feq(r.lineBounds[0].width, 3.0f),  "single: lineBounds width");
    check(feq(r.lineBounds[0].height, 1.0f), "single: lineBounds height");
    check(feq(r.textBoundsX, 3.0f),      "single: textBoundsX");
    check(feq(r.textBoundsY, 1.0f),      "single: textBoundsY");
    check(feq(r.lineBaselineOrigins[0][0], 0.0f), "single: baselineX");
    check(feq(r.lineBaselineOrigins[0][1], 0.0f), "single: baselineY");
    // origin = upper-left = (0, ascender=0.8, 0)
    check(feq(r.originX, 0.0f),  "single: originX");
    check(feq(r.originY, 0.8f),  "single: originY");
    check(feq(r.originZ, 0.0f),  "single: originZ");
}

// ===========================================================================
// Test 2: Two lines, FIRST/FIRST minor (baseline of first line at Y=0).
//
// strings = {"ab", "cd"}, size=1, spacing=1.5, topToBottom=true
//   advance[0]=2, advance[1]=2
//   baselineStep = 1.5
//   minor FIRST → firstBaselineMinor = 0
//   line[0].baselineY = 0
//   line[1].baselineY = 0 + 1*(−1.5) = −1.5   (topToBottom=T → sign is −)
//   textBoundsY = (N-1)*step + size = 1*1.5 + 1.0 = 2.5
// ===========================================================================
static void test_two_lines_first_minor() {
    FontStyleParams fs;
    fs.spacing = 1.5f;
    TextParams t;
    t.strings = {"ab", "cd"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(r.lineBounds.size() == 2, "two FIRST: count");
    check(feq(r.lineBaselineOrigins[0][1], 0.0f),  "two FIRST: line0 baselineY");
    check(feq(r.lineBaselineOrigins[1][1], -1.5f), "two FIRST: line1 baselineY");
    check(feq(r.textBoundsY, 2.5f), "two FIRST: textBoundsY");
}

// ===========================================================================
// Test 3: Minor axis BEGIN, topToBottom=true.
//
// strings = {"a"}, size=1, spacing=1, minor="BEGIN", topToBottom=T
//   FIRST line baseline = +ascender = 0.8  (top of first line at Y=0)
//   baselineOrigins[0] = {0, 0.8}
// ===========================================================================
static void test_minor_begin_topToBottom() {
    FontStyleParams fs;
    fs.justifyMinor = "BEGIN";
    TextParams t;
    t.strings = {"a"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][1], 0.8f), "BEGIN/tTB: baseline Y");
    // origin upper-left: Y = baseline + ascender = 0.8 + 0.8 = 1.6
    check(feq(r.originY, 1.6f), "BEGIN/tTB: originY");
}

// ===========================================================================
// Test 4: Minor axis BEGIN, topToBottom=false.
//
// strings = {"a"}, size=1, topToBottom=F, minor="BEGIN"
//   firstBaselineMinor = -descender = 0.2  (bottom of first line at Y=0)
//   line[0].baselineY = 0.2
//   upper-left Y = 0.2 + 0.8 = 1.0
// ===========================================================================
static void test_minor_begin_bottomToTop() {
    FontStyleParams fs;
    fs.topToBottom = false;
    fs.justifyMinor = "BEGIN";
    TextParams t;
    t.strings = {"a"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][1], 0.2f), "BEGIN/bTT: baseline Y");
    check(feq(r.originY, 1.0f), "BEGIN/bTT: originY");
}

// ===========================================================================
// Test 5: Major axis MIDDLE, leftToRight=true, single line.
//
// "abc" (advance=3), size=1, major MIDDLE, leftToRight=T
//   baselineX = −3/2 = −1.5
//   left edge = −1.5, right edge = −1.5+3 = +1.5
// ===========================================================================
static void test_major_middle_ltr() {
    FontStyleParams fs;
    fs.justifyMajor = "MIDDLE";
    TextParams t;
    t.strings = {"abc"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][0], -1.5f), "MIDDLE/ltr: baselineX");
    // upper-left X = −1.5
    check(feq(r.originX, -1.5f), "MIDDLE/ltr: originX");
}

// ===========================================================================
// Test 6: Major axis END, leftToRight=true.
//
// "abc" (advance=3), major END, leftToRight=T
//   baselineX = −3 (right edge at X=0, left at X=−3)
//   origin.X = −3
// ===========================================================================
static void test_major_end_ltr() {
    FontStyleParams fs;
    fs.justifyMajor = "END";
    TextParams t;
    t.strings = {"abc"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][0], -3.0f), "END/ltr: baselineX");
    check(feq(r.originX, -3.0f), "END/ltr: originX");
}

// ===========================================================================
// Test 7: length[] per-line stretch.
//
// "ab" (natural=2), length=[5], maxExtent=0 → target=5, global_scale=1
//   effective=5, lineBounds.width=5
// ===========================================================================
static void test_length_stretch() {
    FontStyleParams fs;
    TextParams t;
    t.strings = {"ab"};
    t.length  = {5.0f};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBounds[0].width, 5.0f), "stretch: lineBounds.width");
    check(feq(r.textBoundsX, 5.0f),         "stretch: textBoundsX");
}

// ===========================================================================
// Test 8: length[] per-line compress.
//
// "abcd" (natural=4), length=[2] → effective=2, lineBounds.width=2
// ===========================================================================
static void test_length_compress() {
    FontStyleParams fs;
    TextParams t;
    t.strings = {"abcd"};
    t.length  = {2.0f};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBounds[0].width, 2.0f), "compress: lineBounds.width");
}

// ===========================================================================
// Test 9: maxExtent compresses ALL lines.
//
// strings = {"aaaa", "bb"}, natural = {4, 2}, maxExtent=2
//   max_target=4 > 2 → global_scale = 2/4 = 0.5
//   effective = {2, 1}
// ===========================================================================
static void test_maxExtent_compress() {
    FontStyleParams fs;
    TextParams t;
    t.strings   = {"aaaa", "bb"};
    t.maxExtent = 2.0f;

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBounds[0].width, 2.0f), "maxExtent: line0 width");
    check(feq(r.lineBounds[1].width, 1.0f), "maxExtent: line1 width");
    check(feq(r.textBoundsX, 2.0f),         "maxExtent: textBoundsX");
}

// ===========================================================================
// Test 10: maxExtent does NOT stretch (natural < maxExtent).
//
// strings = {"ab"}, natural=2, maxExtent=10 → global_scale=1, effective=2
// ===========================================================================
static void test_maxExtent_no_stretch() {
    FontStyleParams fs;
    TextParams t;
    t.strings   = {"ab"};
    t.maxExtent = 10.0f;

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBounds[0].width, 2.0f), "no-stretch: width unchanged");
}

// ===========================================================================
// Test 11: Minor axis MIDDLE, two lines.
//
// strings = {"a", "b"}, size=1, spacing=1, minor=MIDDLE, topToBottom=T
// MIDDLE centres the glyph BLOCK on Y=0 (§15.2.2.3), NOT the baselines.
//   firstBaselineMinor = ((N-1)*step - ascender[0] - descender[N-1]) / 2
//                      = (1*1 - 0.8 - (-0.2)) / 2 = 0.4/2 = 0.2
//   line[0].Y = 0.2   (top of line 0 = 0.2 + 0.8 =  1.0)
//   line[1].Y = 0.2 − 1.0 = −0.8  (bottom of line 1 = −0.8 + (−0.2) = −1.0)
//   block spans [−1.0, 1.0] → centred on 0  ✓
// ===========================================================================
static void test_minor_middle() {
    FontStyleParams fs;
    fs.justifyMinor = "MIDDLE";
    TextParams t;
    t.strings = {"a", "b"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][1], 0.2f), "MIDDLE: line0 Y");
    check(feq(r.lineBaselineOrigins[1][1], -0.8f), "MIDDLE: line1 Y");
}

// ===========================================================================
// Test 12: Minor axis END, topToBottom=true, two lines.
//
// strings = {"a", "b"}, size=1, spacing=1, minor=END, topToBottom=T
//   totalBlockExtent = 2
//   firstBaselineMinor = (N-1)*step - descender[N-1]
//                       = 1*1 - (-0.2) = 1.2
//   line[0].Y = 1.2
//   line[1].Y = 1.2 − 1.0 = 0.2
//   bottom of last line = line[1].Y + descender[1] = 0.2 + (-0.2) = 0.0  ✓
// ===========================================================================
static void test_minor_end_topToBottom() {
    FontStyleParams fs;
    fs.justifyMinor = "END";
    TextParams t;
    t.strings = {"a", "b"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][1], 1.2f), "END/tTB: line0 Y");
    check(feq(r.lineBaselineOrigins[1][1], 0.2f), "END/tTB: line1 Y");
    // bottom of last line = 0.2 + (-0.2) = 0.0
    float lastBottom = r.lineBaselineOrigins[1][1] - 0.2f; // descender = -0.2
    check(feq(lastBottom, 0.0f), "END/tTB: last line bottom at 0");
}

// ===========================================================================
// Test 13: vertical text (horizontal=false), single column, BEGIN/FIRST.
//
// fs.horizontal=false, leftToRight=T, topToBottom=T, major=BEGIN, minor=FIRST
// strings = {"ab"}, size=1, spacing=1
//   For vertical text: major axis = Y, minor axis = X.
//   natural_advance = 2*1 = 2 (2 characters advancing in Y)
//   effective = 2
//   minor FIRST for vertical = BEGIN (special case only for horizontal)
//     → leftToRight=T → firstBaselineMinor = ascender = 0.8
//   major BEGIN, topToBottom=T → baselineY = 0 (first glyph at Y=0)
//   column[0] baseline = {0.8, 0}
// ===========================================================================
static void test_vertical_single() {
    FontStyleParams fs;
    fs.horizontal = false;
    TextParams t;
    t.strings = {"ab"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(r.lineBounds.size() == 1, "vert: count");
    // vertical: lineBounds.width = size (column width), height = effective
    check(feq(r.lineBounds[0].width,  1.0f), "vert: lineBounds width = size");
    check(feq(r.lineBounds[0].height, 2.0f), "vert: lineBounds height = advance");
    // baseline X = firstBaselineMinor = 0.8
    check(feq(r.lineBaselineOrigins[0][0], 0.8f), "vert: baseline X");
    // baseline Y = 0 (major BEGIN/FIRST)
    check(feq(r.lineBaselineOrigins[0][1], 0.0f), "vert: baseline Y");
}

// ===========================================================================
// Test 14: length[] with missing entries treated as 0 (unconstrained).
//
// strings = {"ab", "cde"}, length=[3] — length[1] is missing → 0 → unconstrained
//   effective[0] = 3, effective[1] = 3 (natural=3, no constraint)
// ===========================================================================
static void test_length_missing_entries() {
    FontStyleParams fs;
    TextParams t;
    t.strings = {"ab", "cde"};
    t.length  = {3.0f}; // only one entry; second treated as 0

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBounds[0].width, 3.0f), "missing len: line0 constrained to 3");
    check(feq(r.lineBounds[1].width, 3.0f), "missing len: line1 natural=3");
}

// ===========================================================================
// Test 15: empty strings → returns empty result without crashing.
// ===========================================================================
static void test_empty_strings() {
    FontStyleParams fs;
    TextParams t;
    t.strings = {};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(r.lineBounds.empty(),              "empty: no lineBounds");
    check(r.lineBaselineOrigins.empty(),     "empty: no origins");
    check(feq(r.textBoundsX, 0.0f),         "empty: textBoundsX=0");
    check(feq(r.textBoundsY, 0.0f),         "empty: textBoundsY=0");
}

// ===========================================================================
// Test 16: leftToRight=false, major MIDDLE.
//
// "abc" (advance=3), leftToRight=F, major MIDDLE
//   advances in −X; baselineX = +eff/2 = 1.5
//   glyph extends from +1.5 to +1.5−3 = −1.5 → centred about X=0 ✓
// ===========================================================================
static void test_major_middle_rtl() {
    FontStyleParams fs;
    fs.leftToRight  = false;
    fs.justifyMajor = "MIDDLE";
    TextParams t;
    t.strings = {"abc"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][0], 1.5f), "RTL MIDDLE: baselineX");
}

// ===========================================================================
// Test 17: leftToRight=false, major END.
//
// "abc" (advance=3), leftToRight=F, major END
//   advances in −X; END → left edge of string at X=0 → baselineX = +3
// ===========================================================================
static void test_major_end_rtl() {
    FontStyleParams fs;
    fs.leftToRight  = false;
    fs.justifyMajor = "END";
    TextParams t;
    t.strings = {"abc"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.lineBaselineOrigins[0][0], 3.0f), "RTL END: baselineX");
}

// ===========================================================================
// Test 18: textBoundsY for three lines with non-unit spacing.
//
// N=3, size=2, spacing=1.5 → baselineStep=3
//   totalBlockExtent = (3-1)*3 + 2 = 8
// ===========================================================================
static void test_textBoundsY_three_lines() {
    FontStyleParams fs;
    fs.size    = 2.0f;
    fs.spacing = 1.5f;
    TextParams t;
    t.strings = {"a", "b", "c"};

    auto r = computeTextLayout(fs, t, monoMetrics());

    check(feq(r.textBoundsY, 8.0f), "3-line: textBoundsY=8");
    // line spacing: step=3
    check(feq(r.lineBaselineOrigins[1][1] - r.lineBaselineOrigins[0][1], -3.0f),
          "3-line: step between line0 and line1");
    check(feq(r.lineBaselineOrigins[2][1] - r.lineBaselineOrigins[1][1], -3.0f),
          "3-line: step between line1 and line2");
}

// ===========================================================================
// Test 19: FIRST minor justify = special case for horizontal=TRUE.
//
// The first line's baseline sits exactly at Y=0. Confirm this is independent
// of topToBottom direction.
// ===========================================================================
static void test_first_minor_baseline_at_zero_both_directions() {
    for (bool tTB : {true, false}) {
        FontStyleParams fs;
        fs.topToBottom  = tTB;
        fs.justifyMinor = "FIRST"; // explicit FIRST
        TextParams t;
        t.strings = {"x"};

        auto r = computeTextLayout(fs, t, monoMetrics());
        check(feq(r.lineBaselineOrigins[0][1], 0.0f),
              "FIRST special case: baseline at Y=0 regardless of topToBottom");
    }
}

// ---------------------------------------------------------------------------
// Entry point.
// ---------------------------------------------------------------------------
TEST_CASE("text_layout_test") {
    test_single_line_defaults();
    test_two_lines_first_minor();
    test_minor_begin_topToBottom();
    test_minor_begin_bottomToTop();
    test_major_middle_ltr();
    test_major_end_ltr();
    test_length_stretch();
    test_length_compress();
    test_maxExtent_compress();
    test_maxExtent_no_stretch();
    test_minor_middle();
    test_minor_end_topToBottom();
    test_vertical_single();
    test_length_missing_entries();
    test_empty_strings();
    test_major_middle_rtl();
    test_major_end_rtl();
    test_textBoundsY_three_lines();
    test_first_minor_baseline_at_zero_both_directions();
    return;
}
