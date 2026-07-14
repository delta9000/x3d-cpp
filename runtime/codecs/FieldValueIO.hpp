// FieldValueIO.hpp
// Node-agnostic conversion between an X3D field's std::any value and its X3D
// wire string, driven entirely by the reflected X3DFieldType tag.
//
// This is the heart of every codec: given a FieldInfo + a node, format the
// field's value as the text that goes in an XML attribute / JSON value /
// ClassicVRML field; and parse such text back into the std::any the FieldInfo's
// `set` thunk expects. No per-node code — the X3DFieldType switch covers all
// value types. SFNode/MFNode are handled by the structural codec, not here, and
// SFEnum/MFEnum are handled via the FieldInfo's string thunks (not std::any).
//
// Public declarations and matrix templates, namespace x3d::codec. Compiled
// definitions live in x3d_cpp_authoring_runtime.
#ifndef X3D_FIELD_VALUE_IO_HPP
#define X3D_FIELD_VALUE_IO_HPP

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <any>
#include <string>
#include <vector>

namespace x3d::codec {
using namespace x3d::core;

// AUD-D: SFEnum/MFEnum values arrive on the wire with MFString-style quoting
// (e.g. NavigationInfo type='"FLY" "EXAMINE"'), but the generated setEnumString
// tokenizes on whitespace and matches bare tokens — a quoted `"FLY"` never
// matches, so the whole field is silently dropped. Enum tokens are bare
// identifiers and never contain a double quote, so strip them before matching.
std::string stripEnumQuotes(const std::string &wire);

// ---------------------------------------------------------------------------
// Low-level scalar formatting. Floats/doubles use the shortest round-trippable
// representation so the writer's output re-parses to the same value.
// ---------------------------------------------------------------------------

std::string fmtFloat(float v);

std::string fmtDouble(double v);

std::string fmtBool(bool b);

// ---------------------------------------------------------------------------
// Tokenizer: split an X3D MF value into whitespace/comma-separated tokens.
// ---------------------------------------------------------------------------

std::vector<std::string> tokenize(const std::string &s);

// Locale-independent numeric parsing. std::from_chars always uses '.' as the
// decimal point (no locale involvement), unlike std::stof/std::stod which honor
// the global C++ locale's numpunct facet.
float parseFloat(const std::string &s);
double parseDouble(const std::string &s);
int parseInt(const std::string &s);

// MFString parsing: X3D wraps each string in double quotes, e.g.
// '"a" "b c" "d"'. Tokens are quoted strings; unquoted runs are taken whole.
// AUD-FVAL-EXT: also accept the bracketed `[ "a" "b" ]` form (the canonical
// ClassicVRML wire form for any MF value per ISO/IEC 19776-2); strip a single
// leading '[' and trailing ']' before tokenizing so parseValue is symmetric
// with the bracketed emission that the VRML/JSON writers produce.
std::vector<std::string> parseMFString(const std::string &s);

// MFString writing: quote each element, escaping embedded quotes/backslashes.
std::string fmtMFString(const std::vector<std::string> &v);

// ---------------------------------------------------------------------------
// Matrix formatting helpers. SFMatrix3f/3d/4f/4d store an N×N C array in
// row-major order (matrix[row][col]); the X3D wire form is the N*N elements
// listed row-major, space-separated (fieldTypes.md §5.3.9-5.3.12).
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// SFImage formatting. Wire form: `width height numComponents` then width*height
// integers, one per pixel; each pixel packs numComponents bytes as a single
// unsigned number, high byte first (fieldTypes.md §5.3.6). `data` holds the raw
// width*height*numComponents bytes in the same left-to-right/bottom-to-top
// order the wire form uses.
// ---------------------------------------------------------------------------

std::string fmtImage(const SFImage &img);

// Parse one integer token, supporting 0x.. hexadecimal or decimal.
unsigned long parseImagePixel(const std::string &tok);

SFImage parseImageFrom(const std::vector<std::string> &t, std::size_t &i);

SFImage parseImage(const std::string &s);

// ---------------------------------------------------------------------------
// formatValue: std::any (per X3DFieldType) -> X3D wire string.
//   Returns the attribute/field text. SFNode/MFNode return "" (structural,
//   handled by the caller). SFEnum/MFEnum are handled via the FieldInfo string
//   thunks by the caller, not here.
// ---------------------------------------------------------------------------

std::string formatValue(X3DFieldType type, const std::any &v);

// ---------------------------------------------------------------------------
// parseValue: X3D wire string -> std::any (per X3DFieldType), ready for a
//   FieldInfo `set` thunk. Returns an empty std::any for types handled
//   elsewhere (nodes/enums) or unsupported here.
// ---------------------------------------------------------------------------

std::any parseValue(X3DFieldType type, const std::string &s);

} // namespace x3d::codec

#endif // X3D_FIELD_VALUE_IO_HPP
