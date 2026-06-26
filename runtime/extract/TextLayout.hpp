// TextLayout.hpp — ISO/IEC 19775-1:2023 §15 Text layout engine (pure).
//
// namespace x3d::runtime::extract. Header-only. No scene/IO coupling; no X3D
// node types are referenced. Implements the complete §15.2.2.3 Direction and
// justification algorithm including:
//   - baseline_step = spacing × size
//   - horizontal / vertical axes with leftToRight / topToBottom direction flags
//   - justify[0] (major-axis, per-line) and justify[1] (minor-axis, whole block)
//     per Tables 15.2–15.5, including the FIRST special case (baseline at Y=0)
//   - Text.length[] per-line stretch/compress (§15.4.2)
//   - Text.maxExtent global compress-to-fit (§15.4.2)
//   - textBounds / lineBounds / origin output fields
//
// Font metrics are supplied via a caller-provided callback so this unit has zero
// IO dependency and is independently testable with a monospaced stub.
//
// References:
//   ISO/IEC 19775-1:2023 §15.2.2.3  Direction and justification
//   ISO/IEC 19775-1:2023 §15.4.1    FontStyle — size, spacing
//   ISO/IEC 19775-1:2023 §15.4.2    Text — length, maxExtent, bounds outputs
//   https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/text.html
#ifndef X3D_RUNTIME_EXTRACT_TEXTLAYOUT_HPP
#define X3D_RUNTIME_EXTRACT_TEXTLAYOUT_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// FontStyleParams — maps directly to X3D FontStyle node fields (§15.4.1).
// ---------------------------------------------------------------------------
struct FontStyleParams {
    float       size        = 1.0f; // nominal glyph height in local coords (> 0)
    float       spacing     = 1.0f; // line-spacing multiplier (>= 0)
    bool        horizontal  = true; // true => major axis X; false => major axis Y
    bool        leftToRight = true; // character-advance direction on major axis
    bool        topToBottom = true; // line-advance direction on minor axis

    // justify[0] = major-axis alignment per line; justify[1] = minor-axis alignment
    // for the whole block. "" is treated as the slot default:
    //   major default: "BEGIN"   minor default: "FIRST"
    std::string justifyMajor = "BEGIN";
    std::string justifyMinor = "FIRST";
};

// ---------------------------------------------------------------------------
// TextParams — maps to X3D Text node fields (§15.4.2).
// ---------------------------------------------------------------------------
struct TextParams {
    std::vector<std::string> strings;
    std::vector<float>       length;    // MFFloat [0,∞); 0 = unconstrained
    float                    maxExtent = 0.0f; // [0,∞); 0 = unconstrained
};

// ---------------------------------------------------------------------------
// Output types.
// ---------------------------------------------------------------------------

// 2D bounding box of one rendered line in local text space (Z=0 plane).
struct LineBounds {
    float width  = 0.0f;
    float height = 0.0f;
};

// Full layout result returned by computeTextLayout().
struct TextLayoutResult {
    // One entry per string in TextParams::strings.
    std::vector<LineBounds>          lineBounds;

    // 2D bounding box of all lines combined (local text space).
    float textBoundsX = 0.0f;
    float textBoundsY = 0.0f;

    // Upper-left corner of textBounds in local text space (Z = 0).
    // §15.4.2: "origin: SFVec3f — upper-left corner of textBounds"
    float originX = 0.0f;
    float originY = 0.0f;
    float originZ = 0.0f;

    // Per-line baseline start positions [X, Y] in local text coordinates.
    // These are the positions at which the first character of each line begins
    // on its baseline (Y for horizontal, X for vertical).
    std::vector<std::array<float, 2>> lineBaselineOrigins;
};

// ---------------------------------------------------------------------------
// FontMetrics callback type.
//
// Signature: (string s, float size) -> (natural_advance, ascender, descender)
//
//   natural_advance — total advance along the major axis for rendering s at
//                     the given nominal glyph height (size).
//   ascender        — distance from baseline to top of cap-height (>= 0).
//   descender       — distance from baseline to bottom of descenders (<= 0,
//                     stored as a negative number, e.g. -0.2 * size).
//
// A monospaced stub (e.g. advance = n_chars * size, ascender = 0.8*size,
// descender = -0.2*size) satisfies all unit tests without real font data.
// ---------------------------------------------------------------------------
using FontMetricsCallback =
    std::function<std::tuple<float, float, float>(const std::string&, float)>;

// ---------------------------------------------------------------------------
// Internal helpers (anonymous namespace keeps them translation-unit local even
// though this is a header-only implementation — each TU gets its own copy but
// they are identical and the compiler inlines/eliminates them).
// ---------------------------------------------------------------------------
namespace detail {

// Resolve "" justify values to their defaults per slot.
inline std::string resolveMajorJustify(const std::string& s) {
    return (s.empty()) ? "BEGIN" : s;
}
inline std::string resolveMinorJustify(const std::string& s) {
    return (s.empty()) ? "FIRST" : s;
}

// Clamp NaN/Inf-producing divisions: if denominator is near zero return 1.
inline float safeDivide(float num, float den) {
    if (std::fabs(den) < 1e-12f) return 1.0f;
    return num / den;
}

} // namespace detail

// ---------------------------------------------------------------------------
// computeTextLayout() — the complete §15 layout algorithm.
//
// ALGORITHM SUMMARY
// =================
// 1. Collect font metrics for every line via the callback.
// 2. Compute per-line target lengths (applying length[] then maxExtent).
// 3. Determine the minor-axis block offset (justify[1]).
// 4. Determine per-line major-axis offsets (justify[0]).
// 5. Emit lineBaselineOrigins, lineBounds, textBounds, origin.
//
// The Z coordinate is always 0 (Text lives in the Z=0 plane, §15.2.1.2).
// ---------------------------------------------------------------------------
inline TextLayoutResult computeTextLayout(
    const FontStyleParams& fs,
    const TextParams&      t,
    FontMetricsCallback    fontMetrics)
{
    TextLayoutResult result;

    const std::size_t N = t.strings.size();
    if (N == 0) return result;

    result.lineBounds.resize(N);
    result.lineBaselineOrigins.resize(N, {0.0f, 0.0f});

    // Resolve justify tokens.
    const std::string majorJ = detail::resolveMajorJustify(fs.justifyMajor);
    const std::string minorJ = detail::resolveMinorJustify(fs.justifyMinor);

    const float size    = (fs.size > 0.0f) ? fs.size : 1.0f;
    const float spacing = (fs.spacing >= 0.0f) ? fs.spacing : 1.0f;
    const float baselineStep = spacing * size; // §15.4.1 baseline distance

    // -----------------------------------------------------------------------
    // Step 1: collect per-line font metrics.
    // -----------------------------------------------------------------------
    struct LineMetrics {
        float naturalAdvance = 0.0f; // advance along the major axis
        float ascender       = 0.0f; // baseline → top (>= 0)
        float descender      = 0.0f; // baseline → bottom (<= 0)
    };
    std::vector<LineMetrics> metrics(N);
    for (std::size_t i = 0; i < N; ++i) {
        auto [adv, asc, desc] = fontMetrics(t.strings[i], size);
        metrics[i].naturalAdvance = adv;
        metrics[i].ascender       = asc;
        metrics[i].descender      = desc;
    }

    // -----------------------------------------------------------------------
    // Step 2: per-line target lengths (length[] + maxExtent). §15.4.2
    //
    //   target[i] = (length[i] > 0) ? length[i] : naturalAdvance[i]
    //   global_scale = 1.0 unless maxExtent > 0 and max(target) > maxExtent
    //   effective[i] = target[i] * global_scale
    // -----------------------------------------------------------------------
    std::vector<float> target(N);
    for (std::size_t i = 0; i < N; ++i) {
        float lenI = (i < t.length.size() && t.length[i] > 0.0f)
                         ? t.length[i]
                         : metrics[i].naturalAdvance;
        target[i] = lenI;
    }

    float maxTarget = 0.0f;
    for (float v : target) maxTarget = std::max(maxTarget, v);

    float globalScale = 1.0f;
    if (t.maxExtent > 0.0f && maxTarget > t.maxExtent) {
        globalScale = detail::safeDivide(t.maxExtent, maxTarget);
    }

    std::vector<float> effective(N);
    for (std::size_t i = 0; i < N; ++i)
        effective[i] = target[i] * globalScale;

    // Per-line glyph scale factors (applied on the major axis only).
    // line_scale_x / line_scale_y — used by callers rendering glyphs;
    // lineBounds[i] reports the effective extent regardless.
    std::vector<float> lineScaleMajor(N);
    for (std::size_t i = 0; i < N; ++i) {
        lineScaleMajor[i] = detail::safeDivide(effective[i],
                                                metrics[i].naturalAdvance);
    }

    // -----------------------------------------------------------------------
    // Step 3: minor-axis block offset (positions the whole block).
    //
    // horizontal = TRUE: minor axis is Y.
    //   FIRST  → block_y_offset = 0 (baseline of first line at Y=0) [special case]
    //   BEGIN  + topToBottom=T → block_y_offset = +ascender[0]        (top of first line at Y=0 or above)
    //   BEGIN  + topToBottom=F → block_y_offset = -descender[0]       (bottom of first line at Y=0)
    //   MIDDLE → block_y_offset = totalBlockExtent / 2
    //   END    + topToBottom=T → block_y_offset = totalBlockExtent - descender[N-1]  (bottom of last line at origin)
    //   END    + topToBottom=F → block_y_offset = -(totalBlockExtent - ascender[N-1])
    //
    // horizontal = FALSE: minor axis is X. (analogous; leftToRight governs sign.)
    //
    // totalBlockExtent = (N-1)*baselineStep + lineHeight_last
    //   where lineHeight_last = ascender[N-1] - descender[N-1]
    //
    // NOTE: the minor-axis offset is the Y (or X for vertical) coordinate of the
    // BASELINE of the first line.  Each subsequent line's baseline is stepped from
    // the first by ±baselineStep (sign determined by topToBottom / leftToRight).
    // -----------------------------------------------------------------------
    const float lineHeight = size; // nominal height per §15.4.1; used for bounds

    // total block extent in the minor axis direction (absolute, unsigned)
    // = (N-1) gaps of baselineStep + height of last line
    float totalBlockExtent = (N > 1 ? static_cast<float>(N - 1) * baselineStep : 0.0f)
                           + lineHeight;

    // minor-axis position of the FIRST line's baseline
    float firstBaselineMinor = 0.0f;

    if (fs.horizontal) {
        // Minor axis = Y.
        if (minorJ == "FIRST") {
            // §15.2.2.3 FIRST special case: first-line baseline at Y=0.
            firstBaselineMinor = 0.0f;
        } else if (minorJ == "BEGIN") {
            // Top of first line touches origin.
            if (fs.topToBottom) {
                // Lines go down; first baseline is above Y=0 by ascender.
                firstBaselineMinor = metrics[0].ascender;
            } else {
                // Lines go up; first baseline is below Y=0 by |descender|.
                firstBaselineMinor = -metrics[0].descender; // descender <= 0 => this >= 0
            }
        } else if (minorJ == "MIDDLE") {
            // Centre of the BLOCK at Y=0 (§15.2.2.3). The block's vertical extent
            // runs from the top line's ascender to the bottom line's descender;
            // centring the baselines (extent/2) ignores the ascender/descender
            // asymmetry and lifts the block by ~one ascender. Solve center==0 for
            // the first baseline: top line is line 0 when topToBottom, else N-1.
            if (fs.topToBottom) {
                firstBaselineMinor = (static_cast<float>(N - 1) * baselineStep
                                      - metrics[0].ascender
                                      - metrics[N - 1].descender) / 2.0f;
            } else {
                firstBaselineMinor = -(static_cast<float>(N - 1) * baselineStep
                                       + metrics[N - 1].ascender
                                       + metrics[0].descender) / 2.0f;
            }
        } else if (minorJ == "END") {
            if (fs.topToBottom) {
                // Bottom of last line at Y=0 (block entirely above Y=0).
                // Last baseline = -(N-1)*baselineStep  from first;
                // bottom of last line = last_baseline + descender[N-1]
                // We want last_baseline + descender[N-1] = 0, i.e.,
                // firstBaseline + (-(N-1)*baselineStep) + descender[N-1] = 0
                // => firstBaseline = (N-1)*baselineStep - descender[N-1]
                firstBaselineMinor = static_cast<float>(N - 1) * baselineStep
                                   - metrics[N - 1].descender;
            } else {
                // Top of last line at Y=0 (block entirely below Y=0).
                // Last baseline is (N-1)*baselineStep ABOVE first (topToBottom=F).
                // top of last line = last_baseline + ascender[N-1] = 0
                // firstBaseline - (N-1)*baselineStep + ascender[N-1] = 0
                // => firstBaseline = (N-1)*baselineStep - ascender[N-1]
                firstBaselineMinor = static_cast<float>(N - 1) * baselineStep
                                   - metrics[N - 1].ascender;
            }
        }
    } else {
        // horizontal = FALSE: minor axis is X.
        // Symmetric to the horizontal case but left/right governs sign.
        if (minorJ == "FIRST") {
            // FIRST for vertical = identical to BEGIN (§15.2.2.3 special case
            // applies to horizontal=TRUE only).
            if (fs.leftToRight) {
                firstBaselineMinor = metrics[0].ascender; // left edge of first col at X=0
            } else {
                firstBaselineMinor = -metrics[0].descender;
            }
        } else if (minorJ == "BEGIN") {
            if (fs.leftToRight) {
                firstBaselineMinor = metrics[0].ascender;
            } else {
                firstBaselineMinor = -metrics[0].descender;
            }
        } else if (minorJ == "MIDDLE") {
            // Centre the column BLOCK on X=0 (same block-centring as the
            // horizontal axis; leftToRight plays topToBottom's role). The
            // leftmost column is line 0 when leftToRight, else N-1.
            if (fs.leftToRight) {
                firstBaselineMinor = -(static_cast<float>(N - 1) * baselineStep
                                       + metrics[N - 1].ascender
                                       + metrics[0].descender) / 2.0f;
            } else {
                firstBaselineMinor = (static_cast<float>(N - 1) * baselineStep
                                      - metrics[0].ascender
                                      - metrics[N - 1].descender) / 2.0f;
            }
        } else if (minorJ == "END") {
            if (fs.leftToRight) {
                firstBaselineMinor = static_cast<float>(N - 1) * baselineStep
                                   - metrics[N - 1].descender;
            } else {
                firstBaselineMinor = static_cast<float>(N - 1) * baselineStep
                                   - metrics[N - 1].ascender;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Step 4: per-line baseline positions.
    //
    // horizontal = TRUE:
    //   baseline.Y[i] = firstBaselineMinor + i * (topToBottom ? -baselineStep : +baselineStep)
    //   baseline.X[i] from majorJ: FIRST/BEGIN→0, MIDDLE→∓eff/2, END→∓eff
    //
    // horizontal = FALSE:
    //   baseline.X[i] = firstBaselineMinor + i * (leftToRight ? +baselineStep : -baselineStep)
    //   baseline.Y[i] from majorJ: FIRST/BEGIN→0, MIDDLE→±eff/2, END→±eff
    // -----------------------------------------------------------------------
    for (std::size_t i = 0; i < N; ++i) {
        float eff = effective[i];
        float baselineX = 0.0f;
        float baselineY = 0.0f;

        if (fs.horizontal) {
            // Minor axis offset (Y coordinate of baseline).
            float minorSign = fs.topToBottom ? -1.0f : +1.0f;
            baselineY = firstBaselineMinor + static_cast<float>(i) * minorSign * baselineStep;

            // Major axis offset (X coordinate of first glyph).
            // Tables 15.2: characters advance in +X if leftToRight=T.
            if (majorJ == "FIRST" || majorJ == "BEGIN") {
                baselineX = 0.0f; // line starts at X=0 regardless of leftToRight
            } else if (majorJ == "MIDDLE") {
                // Centre line about X=0.
                if (fs.leftToRight) {
                    baselineX = -eff / 2.0f; // left edge is at -eff/2
                } else {
                    baselineX = eff / 2.0f;  // advances in -X; first glyph at +eff/2
                }
            } else if (majorJ == "END") {
                if (fs.leftToRight) {
                    baselineX = -eff; // right edge at X=0 → left edge at -eff
                } else {
                    baselineX = eff;  // advances in -X; right edge at X=0
                }
            }
        } else {
            // horizontal = FALSE: major axis is Y, minor axis is X.
            // Minor axis offset (X coordinate of baseline).
            float minorSign = fs.leftToRight ? +1.0f : -1.0f;
            baselineX = firstBaselineMinor + static_cast<float>(i) * minorSign * baselineStep;

            // Major axis offset (Y coordinate of first glyph).
            // Tables 15.3: characters advance in -Y if topToBottom=T.
            if (majorJ == "FIRST" || majorJ == "BEGIN") {
                baselineY = 0.0f;
            } else if (majorJ == "MIDDLE") {
                if (fs.topToBottom) {
                    baselineY = eff / 2.0f;  // advances in -Y; first glyph at +eff/2
                } else {
                    baselineY = -eff / 2.0f;
                }
            } else if (majorJ == "END") {
                if (fs.topToBottom) {
                    baselineY = eff;   // advances in -Y; far end at Y=0
                } else {
                    baselineY = -eff;
                }
            }
        }

        result.lineBaselineOrigins[i] = {baselineX, baselineY};
    }

    // -----------------------------------------------------------------------
    // Step 5: lineBounds, textBounds, origin.
    //
    // §15.4.2: lineBounds[i] = (width, height) 2D bbox of one rendered line.
    //   horizontal style: width = effective[i] (major extent)
    //                     height = size (minor extent = nominal glyph height)
    //   vertical style:   width = size (minor extent = column width ≈ glyph height)
    //                     height = effective[i] (major extent)
    //
    // textBounds: overall 2D bbox of all lines combined.
    //   horizontal: textBoundsX = max(effective[i])
    //               textBoundsY = totalBlockExtent
    //   vertical:   textBoundsX = totalBlockExtent
    //               textBoundsY = max(effective[i])
    //
    // origin: upper-left corner of textBounds in local text space (Z=0).
    //   The "upper-left" is the corner with the MOST NEGATIVE X and MOST
    //   POSITIVE Y (standard text convention matching §15 and the spec's
    //   S-increases-right / T-increases-up texture orientation).
    // -----------------------------------------------------------------------
    float maxEffective = 0.0f;
    for (std::size_t i = 0; i < N; ++i) {
        maxEffective = std::max(maxEffective, effective[i]);

        if (fs.horizontal) {
            result.lineBounds[i].width  = effective[i];
            result.lineBounds[i].height = size;
        } else {
            result.lineBounds[i].width  = size;
            result.lineBounds[i].height = effective[i];
        }
    }

    if (fs.horizontal) {
        result.textBoundsX = maxEffective;
        result.textBoundsY = totalBlockExtent;
    } else {
        result.textBoundsX = totalBlockExtent;
        result.textBoundsY = maxEffective;
    }

    // Compute upper-left corner of the textBounds bbox.
    // We need the minimum X and maximum Y over all line corners.
    float minX =  std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();

    for (std::size_t i = 0; i < N; ++i) {
        float bx = result.lineBaselineOrigins[i][0];
        float by = result.lineBaselineOrigins[i][1];

        if (fs.horizontal) {
            // The line bbox top-left corner:
            //   X: leftmost = bx for leftToRight; for !leftToRight bx is rightmost,
            //      so leftmost = bx - effective[i]
            //   Y: top = by + ascender
            float lineLeft = fs.leftToRight ? bx : bx - effective[i];
            float lineTop  = by + metrics[i].ascender;
            minX = std::min(minX, lineLeft);
            maxY = std::max(maxY, lineTop);
        } else {
            // vertical: X minor-axis advances; Y is major axis.
            // Column top: by is major-axis start.
            //   Y top: topToBottom=T → by is top; topToBottom=F → by - effective[i] is top
            float colTop  = fs.topToBottom ? by : by - effective[i];
            // X left edge: bx is the baseline X.
            //   For vertical text the "left" of the column is bx - ascender
            //   (the ascender describes the half-width to the left of the centre axis).
            float colLeft = bx - metrics[i].ascender;
            minX = std::min(minX, colLeft);
            maxY = std::max(maxY, colTop);
        }
    }

    // Guard against the pathological empty-strings case.
    if (!std::isfinite(minX)) minX = 0.0f;
    if (!std::isfinite(maxY)) maxY = 0.0f;

    result.originX = minX;
    result.originY = maxY;
    result.originZ = 0.0f;

    return result;
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_TEXTLAYOUT_HPP
