// stb_truetype FontMetrics adapter. Font library headers stay private.
// See ADR-0025.
#ifndef X3D_RUNTIME_IO_STBTT_STBTT_FONT_METRICS_HPP
#define X3D_RUNTIME_IO_STBTT_STBTT_FONT_METRICS_HPP

#include "FontMetrics.hpp"  // x3d::runtime::extract::FontMetrics

#include <map>
#include <string>

namespace x3d::runtime::io::stbtt {

/// family -> TTF/OTF file path. Supports PLAIN style only.
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
