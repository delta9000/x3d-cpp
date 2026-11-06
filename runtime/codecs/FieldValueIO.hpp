// Reflection-based conversion between field values and X3D wire strings.
// Structural codecs handle nodes; reflection string thunks handle enums.

#ifndef X3D_FIELD_VALUE_IO_HPP
#define X3D_FIELD_VALUE_IO_HPP

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <any>
#include <string>
#include <vector>

namespace x3d::codec {
using namespace x3d::core;

// Enum tokens are bare identifiers; strip MFString-style quotes before matching.
std::string stripEnumQuotes(const std::string &wire);
// Multiple-value wire types; node fields are handled by structural codecs.
bool isMF(X3DFieldType type);

// Low-level scalar formatting. Floats/doubles use the shortest round-trippable
// representation so the writer's output re-parses to the same value.

std::string fmtFloat(float v);

std::string fmtDouble(double v);

std::string fmtBool(bool b);
// Tokenizer: split an X3D MF value into whitespace/comma-separated tokens.

std::vector<std::string> tokenize(const std::string &s);

// Locale-independent numeric parsing. std::from_chars always uses '.' as the
// decimal point (no locale involvement), unlike std::stof/std::stod which honor
// the global C++ locale's numpunct facet.
float parseFloat(const std::string &s);
double parseDouble(const std::string &s);
// UNIT factors must be complete positive numbers; ordinary node field parsing
// remains lenient for compatibility with existing reader recovery behavior.
double parseUnitConversionFactor(const std::string &s);
int parseInt(const std::string &s);

// MFString parsing: X3D wraps each string in double quotes, e.g.
// '"a" "b c" "d"'. Tokens are quoted strings; unquoted runs are taken whole.
// Also accept the bracketed `[ "a" "b" ]` form (the canonical
// ClassicVRML wire form for any MF value per ISO/IEC 19776-2); strip a single
// leading '[' and trailing ']' before tokenizing so parseValue is symmetric
// with the bracketed emission that the VRML/JSON writers produce.
std::vector<std::string> parseMFString(const std::string &s);

// MFString writing: quote each element, escaping embedded quotes/backslashes.
std::string fmtMFString(const std::vector<std::string> &v);
// Matrix formatting helpers. SFMatrix3f/3d/4f/4d store an N×N C array in
// row-major order (matrix[row][col]); the X3D wire form is the N*N elements
// listed row-major, space-separated (fieldTypes.md §5.3.9-5.3.12).

template <typename M, int N> inline std::string fmtMatrixF(const M &m) {
  std::string out;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      if (r || c)
        out += ' ';
      out += fmtFloat(m.matrix[r][c]);
    }
  return out;
}

template <typename M, int N> inline std::string fmtMatrixD(const M &m) {
  std::string out;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      if (r || c)
        out += ' ';
      out += fmtDouble(m.matrix[r][c]);
    }
  return out;
}

template <typename M, int N> inline M parseMatrixF(const std::string &s) {
  auto t = tokenize(s);
  M m{};
  std::size_t i = 0;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c, ++i)
      m.matrix[r][c] = (i < t.size()) ? parseFloat(t[i]) : 0.0f;
  return m;
}

template <typename M, int N> inline M parseMatrixD(const std::string &s) {
  auto t = tokenize(s);
  M m{};
  std::size_t i = 0;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c, ++i)
      m.matrix[r][c] = (i < t.size()) ? parseDouble(t[i]) : 0.0;
  return m;
}
// SFImage formatting. Wire form: `width height numComponents` then width*height
// integers, one per pixel; each pixel packs numComponents bytes as a single
// unsigned number, high byte first (fieldTypes.md §5.3.6). `data` holds the raw
// width*height*numComponents bytes in the same left-to-right/bottom-to-top
// order the wire form uses.

std::string fmtImage(const SFImage &img);

// Parse one integer token, supporting 0x.. hexadecimal or decimal.
unsigned long parseImagePixel(const std::string &tok);

SFImage parseImageFrom(const std::vector<std::string> &t, std::size_t &i);

SFImage parseImage(const std::string &s);
// formatValue: std::any (per X3DFieldType) -> X3D wire string.
//   Returns the attribute/field text. SFNode/MFNode return "" (structural,
//   handled by the caller). SFEnum/MFEnum are handled via the FieldInfo string
//   thunks by the caller, not here.

std::string formatValue(X3DFieldType type, const std::any &v);
// parseValue: X3D wire string -> std::any (per X3DFieldType), ready for a
//   FieldInfo `set` thunk. Returns an empty std::any for types handled
//   elsewhere (nodes/enums) or unsupported here.

std::any parseValue(X3DFieldType type, const std::string &s);

} // namespace x3d::codec

#endif // X3D_FIELD_VALUE_IO_HPP
