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
