#include "FieldValueIO.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <locale>
#include <sstream>

namespace x3d::codec {

std::string stripEnumQuotes(const std::string &wire) {
  if (wire.find('"') == std::string::npos)
    return wire;
  std::string out;
  out.reserve(wire.size());
  for (char c : wire)
    if (c != '"')
      out += c;
  return out;
}

std::string fmtFloat(float v) {
  if (v == static_cast<long long>(v) && std::fabs(v) < 1e15f) {
    return std::to_string(static_cast<long long>(v));
  }
  // Shortest round-trip form (to_chars general): emits the fewest digits that
  // parse back bit-identical, so an authored "0.9" serializes as "0.9", not
  // the precision(9) "0.899999976". Locale-free by definition ('.' always).
  // Same approach as canonFmtFloat in CanonicalXmlWriter (which is pinned
  // separately and intentionally NOT shared — X3DC14N owns its own contract).
  std::array<char, 32> buf;
  auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v,
                                 std::chars_format::general);
  if (ec == std::errc())
    return std::string(buf.data(), ptr);
  // Unreachable with a 32-byte buffer; keep the old path as a safe fallback.
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os.precision(9);
  os << v;
  return os.str();
}

std::string fmtDouble(double v) {
  if (v == static_cast<long long>(v) && std::fabs(v) < 1e15) {
    return std::to_string(static_cast<long long>(v));
  }
  std::array<char, 32> buf;
  auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v,
                                 std::chars_format::general);
  if (ec == std::errc())
    return std::string(buf.data(), ptr);
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os.precision(17);
  os << v;
  return os.str();
}

std::string fmtBool(bool b) { return b ? "true" : "false"; }

std::vector<std::string> tokenize(const std::string &s) {
  std::vector<std::string> out;
  std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' ||
                            s[i] == '\r' || s[i] == ','))
      ++i;
    std::size_t j = i;
    while (j < s.size() && s[j] != ' ' && s[j] != '\t' && s[j] != '\n' &&
           s[j] != '\r' && s[j] != ',')
      ++j;
    if (j > i)
      out.push_back(s.substr(i, j - i));
    i = j;
  }
  return out;
}

float parseFloat(const std::string &s) {
  const char *begin = s.c_str();
  const char *end = begin + s.size();
  // Skip leading whitespace (from_chars rejects it).
  while (begin < end &&
         (*begin == ' ' || *begin == '\t' || *begin == '\n' || *begin == '\r'))
    ++begin;
  float v = 0.0f;
  auto [ptr, ec] = std::from_chars(begin, end, v);
  if (ec != std::errc{})
    return 0.0f;
  return v;
}

double parseDouble(const std::string &s) {
  const char *begin = s.c_str();
  const char *end = begin + s.size();
  while (begin < end &&
         (*begin == ' ' || *begin == '\t' || *begin == '\n' || *begin == '\r'))
    ++begin;
  double v = 0.0;
  auto [ptr, ec] = std::from_chars(begin, end, v);
  if (ec != std::errc{})
    return 0.0;
  return v;
}

int parseInt(const std::string &s) {
  const char *begin = s.c_str();
  const char *end = begin + s.size();
  while (begin < end &&
         (*begin == ' ' || *begin == '\t' || *begin == '\n' || *begin == '\r'))
    ++begin;
  int v = 0;
  auto [ptr, ec] = std::from_chars(begin, end, v);
  if (ec != std::errc{})
    return 0;
  return v;
}

std::vector<std::string> parseMFString(const std::string &s) {
  std::string buf = s;
  // Strip a single outer pair of brackets, allowing surrounding whitespace.
  std::size_t lo = 0;
  while (lo < buf.size() && (buf[lo] == ' ' || buf[lo] == '\t' ||
                             buf[lo] == '\n' || buf[lo] == '\r'))
    ++lo;
  std::size_t hi = buf.size();
  while (hi > lo && (buf[hi - 1] == ' ' || buf[hi - 1] == '\t' ||
                     buf[hi - 1] == '\n' || buf[hi - 1] == '\r'))
    --hi;
  if (lo < hi && buf[lo] == '[' && buf[hi - 1] == ']') {
    buf = buf.substr(lo + 1, hi - lo - 2);
  }
  std::vector<std::string> out;
  std::size_t i = 0;
  while (i < buf.size()) {
    while (i < buf.size() &&
           (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' ||
            buf[i] == '\r' || buf[i] == ','))
      ++i;
    if (i >= buf.size())
      break;
    if (buf[i] == '"') {
      ++i;
      std::string cur;
      while (i < buf.size() && buf[i] != '"') {
        if (buf[i] == '\\' && i + 1 < buf.size() &&
            (buf[i + 1] == '"' || buf[i + 1] == '\\')) {
          // ISO 19776 defines exactly two escapes: \" and \\. Any other \x
          // keeps its backslash (ENC-MFSTRING-READ: c:\new\tex must not
          // become c:newtex).
          cur += buf[i + 1];
          i += 2;
        } else {
          cur += buf[i++];
        }
      }
      if (i < buf.size())
        ++i; // closing quote
      out.push_back(cur);
    } else {
      std::size_t j = i;
      while (j < buf.size() && buf[j] != ' ' && buf[j] != '\t' &&
             buf[j] != '\n' && buf[j] != '\r')
        ++j;
      out.push_back(buf.substr(i, j - i));
      i = j;
    }
  }
  return out;
}

std::string fmtMFString(const std::vector<std::string> &v) {
  std::string out;
  for (std::size_t i = 0; i < v.size(); ++i) {
    if (i)
      out += ' ';
    out += '"';
    for (char c : v[i]) {
      if (c == '"' || c == '\\')
        out += '\\';
      out += c;
    }
    out += '"';
  }
  return out;
}

std::string fmtImage(const SFImage &img) {
  std::string out = std::to_string(img.width) + " " +
                    std::to_string(img.height) + " " +
                    std::to_string(img.numComponents);
  const int nc = img.numComponents;
  const std::size_t pixels =
      (nc > 0) ? (img.data.size() / static_cast<std::size_t>(nc)) : 0;
  for (std::size_t p = 0; p < pixels; ++p) {
    unsigned long packed = 0;
    for (int b = 0; b < nc; ++b) {
      packed = (packed << 8) | static_cast<unsigned long>(img.data[p * nc + b]);
    }
    out += ' ';
    out += std::to_string(packed);
  }
  return out;
}

unsigned long parseImagePixel(const std::string &tok) {
  const char *begin = tok.c_str();
  const char *end = begin + tok.size();
  int base = 10;
  if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
    begin += 2;
    base = 16;
  }
  unsigned long v = 0;
  std::from_chars(begin, end, v, base);
  return v;
}

SFImage parseImageFrom(const std::vector<std::string> &t, std::size_t &i) {
  SFImage img{0, 0, 0, {}};
  if (i < t.size())
    img.width = parseInt(t[i++]);
  if (i < t.size())
    img.height = parseInt(t[i++]);
  if (i < t.size())
    img.numComponents = parseInt(t[i++]);
  // SEC-2: X3D §5.3.6 caps numComponents at [0,4]. A hostile token can claim a
  // huge (or negative) count; clamping BEFORE the unpack loop bounds the output
  // byte count and keeps the shift amount in range (the old `8 * b` overflowed
  // a signed int for large b, UB). Width/height only ever consume one pixel
  // token apiece, so the pixel loop is already bounded by the token stream.
  const int nc = std::clamp(img.numComponents, 0, 4);
  img.numComponents = nc;
  const std::size_t pixels = static_cast<std::size_t>(img.width) *
                             static_cast<std::size_t>(img.height);
  for (std::size_t p = 0; p < pixels && i < t.size(); ++p) {
    unsigned long packed = parseImagePixel(t[i++]);
    for (int b = nc - 1; b >= 0; --b) {
      img.data.push_back(static_cast<unsigned char>(
          (packed >> (8u * static_cast<unsigned>(b))) & 0xFFu));
    }
  }
  return img;
}

SFImage parseImage(const std::string &s) {
  auto t = tokenize(s);
  std::size_t i = 0;
  return parseImageFrom(t, i);
}

std::string formatValue(X3DFieldType type, const std::any &v) {
  switch (type) {
  case X3DFieldType::SFBool:
    return fmtBool(std::any_cast<SFBool>(v));
  case X3DFieldType::SFInt32:
    return std::to_string(std::any_cast<int>(v));
  case X3DFieldType::SFFloat:
    return fmtFloat(std::any_cast<float>(v));
  case X3DFieldType::SFDouble:
  case X3DFieldType::SFTime:
    return fmtDouble(std::any_cast<double>(v));
  case X3DFieldType::SFString:
    return std::any_cast<std::string>(v);
  case X3DFieldType::SFColor: {
    auto c = std::any_cast<SFColor>(v);
    return fmtFloat(c.r) + " " + fmtFloat(c.g) + " " + fmtFloat(c.b);
  }
  case X3DFieldType::SFColorRGBA: {
    auto c = std::any_cast<SFColorRGBA>(v);
    return fmtFloat(c.r) + " " + fmtFloat(c.g) + " " + fmtFloat(c.b) + " " +
           fmtFloat(c.a);
  }
  case X3DFieldType::SFVec2f: {
    auto p = std::any_cast<SFVec2f>(v);
    return fmtFloat(p.x) + " " + fmtFloat(p.y);
  }
  case X3DFieldType::SFVec2d: {
    auto p = std::any_cast<SFVec2d>(v);
    return fmtDouble(p.x) + " " + fmtDouble(p.y);
  }
  case X3DFieldType::SFVec3f: {
    auto p = std::any_cast<SFVec3f>(v);
    return fmtFloat(p.x) + " " + fmtFloat(p.y) + " " + fmtFloat(p.z);
  }
  case X3DFieldType::SFVec3d: {
    auto p = std::any_cast<SFVec3d>(v);
    return fmtDouble(p.x) + " " + fmtDouble(p.y) + " " + fmtDouble(p.z);
  }
  case X3DFieldType::SFVec4f: {
    auto p = std::any_cast<SFVec4f>(v);
    return fmtFloat(p.x) + " " + fmtFloat(p.y) + " " + fmtFloat(p.z) + " " +
           fmtFloat(p.w);
  }
  case X3DFieldType::SFVec4d: {
    auto p = std::any_cast<SFVec4d>(v);
    return fmtDouble(p.x) + " " + fmtDouble(p.y) + " " + fmtDouble(p.z) + " " +
           fmtDouble(p.w);
  }
  case X3DFieldType::SFRotation: {
    auto r = std::any_cast<SFRotation>(v);
    return fmtFloat(r.x) + " " + fmtFloat(r.y) + " " + fmtFloat(r.z) + " " +
           fmtFloat(r.angle);
  }
  case X3DFieldType::MFBool: {
    const auto &vec = std::any_cast<std::vector<bool>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtBool(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFInt32: {
    const auto &vec = std::any_cast<std::vector<int>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += std::to_string(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFFloat: {
    const auto &vec = std::any_cast<std::vector<float>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFTime: {
    const auto &vec = std::any_cast<std::vector<double>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtDouble(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFString:
    return fmtMFString(std::any_cast<std::vector<std::string>>(v));
  case X3DFieldType::MFColor: {
    const auto &vec = std::any_cast<std::vector<SFColor>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].r) + " " + fmtFloat(vec[i].g) + " " +
             fmtFloat(vec[i].b);
    }
    return out;
  }
  case X3DFieldType::MFColorRGBA: {
    const auto &vec = std::any_cast<std::vector<SFColorRGBA>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].r) + " " + fmtFloat(vec[i].g) + " " +
             fmtFloat(vec[i].b) + " " + fmtFloat(vec[i].a);
    }
    return out;
  }
  case X3DFieldType::MFVec2f: {
    const auto &vec = std::any_cast<std::vector<SFVec2f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].x) + " " + fmtFloat(vec[i].y);
    }
    return out;
  }
  case X3DFieldType::MFVec3f: {
    const auto &vec = std::any_cast<std::vector<SFVec3f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].x) + " " + fmtFloat(vec[i].y) + " " +
             fmtFloat(vec[i].z);
    }
    return out;
  }
  case X3DFieldType::MFVec2d: {
    const auto &vec = std::any_cast<std::vector<SFVec2d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtDouble(vec[i].x) + " " + fmtDouble(vec[i].y);
    }
    return out;
  }
  case X3DFieldType::MFVec3d: {
    const auto &vec = std::any_cast<std::vector<SFVec3d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtDouble(vec[i].x) + " " + fmtDouble(vec[i].y) + " " +
             fmtDouble(vec[i].z);
    }
    return out;
  }
  case X3DFieldType::MFVec4f: {
    const auto &vec = std::any_cast<std::vector<SFVec4f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].x) + " " + fmtFloat(vec[i].y) + " " +
             fmtFloat(vec[i].z) + " " + fmtFloat(vec[i].w);
    }
    return out;
  }
  case X3DFieldType::MFVec4d: {
    const auto &vec = std::any_cast<std::vector<SFVec4d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtDouble(vec[i].x) + " " + fmtDouble(vec[i].y) + " " +
             fmtDouble(vec[i].z) + " " + fmtDouble(vec[i].w);
    }
    return out;
  }
  case X3DFieldType::MFRotation: {
    const auto &vec = std::any_cast<std::vector<SFRotation>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtFloat(vec[i].x) + " " + fmtFloat(vec[i].y) + " " +
             fmtFloat(vec[i].z) + " " + fmtFloat(vec[i].angle);
    }
    return out;
  }
  case X3DFieldType::SFMatrix3f:
    return fmtMatrixF<SFMatrix3f, 3>(std::any_cast<SFMatrix3f>(v));
  case X3DFieldType::SFMatrix4f:
    return fmtMatrixF<SFMatrix4f, 4>(std::any_cast<SFMatrix4f>(v));
  case X3DFieldType::SFMatrix3d:
    return fmtMatrixD<SFMatrix3d, 3>(std::any_cast<SFMatrix3d>(v));
  case X3DFieldType::SFMatrix4d:
    return fmtMatrixD<SFMatrix4d, 4>(std::any_cast<SFMatrix4d>(v));
  case X3DFieldType::MFMatrix3f: {
    const auto &vec = std::any_cast<std::vector<SFMatrix3f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtMatrixF<SFMatrix3f, 3>(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFMatrix4f: {
    const auto &vec = std::any_cast<std::vector<SFMatrix4f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtMatrixF<SFMatrix4f, 4>(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFMatrix3d: {
    const auto &vec = std::any_cast<std::vector<SFMatrix3d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtMatrixD<SFMatrix3d, 3>(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFMatrix4d: {
    const auto &vec = std::any_cast<std::vector<SFMatrix4d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtMatrixD<SFMatrix4d, 4>(vec[i]);
    }
    return out;
  }
  case X3DFieldType::SFImage:
    return fmtImage(std::any_cast<SFImage>(v));
  case X3DFieldType::MFImage: {
    const auto &vec = std::any_cast<std::vector<SFImage>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtImage(vec[i]);
    }
    return out;
  }
  // Node fields are structural; enum fields use the string thunks.
  default:
    return "";
  }
}

std::any parseValue(X3DFieldType type, const std::string &s) {
  switch (type) {
  case X3DFieldType::SFBool:
    return std::any(SFBool(s == "true" || s == "TRUE" || s == "1"));
  case X3DFieldType::SFInt32:
    return std::any(parseInt(s));
  case X3DFieldType::SFFloat:
    return std::any(parseFloat(s));
  case X3DFieldType::SFDouble:
  case X3DFieldType::SFTime:
    return std::any(parseDouble(s));
  case X3DFieldType::SFString:
    return std::any(s);
  case X3DFieldType::SFColor: {
    auto t = tokenize(s);
    SFColor c{0, 0, 0};
    if (t.size() > 0)
      c.r = parseFloat(t[0]);
    if (t.size() > 1)
      c.g = parseFloat(t[1]);
    if (t.size() > 2)
      c.b = parseFloat(t[2]);
    return std::any(c);
  }
  case X3DFieldType::SFColorRGBA: {
    auto t = tokenize(s);
    SFColorRGBA c{0, 0, 0, 0};
    if (t.size() > 0)
      c.r = parseFloat(t[0]);
    if (t.size() > 1)
      c.g = parseFloat(t[1]);
    if (t.size() > 2)
      c.b = parseFloat(t[2]);
    if (t.size() > 3)
      c.a = parseFloat(t[3]);
    return std::any(c);
  }
  case X3DFieldType::SFVec2f: {
    auto t = tokenize(s);
    SFVec2f p{0, 0};
    if (t.size() > 0)
      p.x = parseFloat(t[0]);
    if (t.size() > 1)
      p.y = parseFloat(t[1]);
    return std::any(p);
  }
  case X3DFieldType::SFVec2d: {
    auto t = tokenize(s);
    SFVec2d p{0, 0};
    if (t.size() > 0)
      p.x = parseDouble(t[0]);
    if (t.size() > 1)
      p.y = parseDouble(t[1]);
    return std::any(p);
  }
  case X3DFieldType::SFVec3f: {
    auto t = tokenize(s);
    SFVec3f p{0, 0, 0};
    if (t.size() > 0)
      p.x = parseFloat(t[0]);
    if (t.size() > 1)
      p.y = parseFloat(t[1]);
    if (t.size() > 2)
      p.z = parseFloat(t[2]);
    return std::any(p);
  }
  case X3DFieldType::SFVec3d: {
    auto t = tokenize(s);
    SFVec3d p{0, 0, 0};
    if (t.size() > 0)
      p.x = parseDouble(t[0]);
    if (t.size() > 1)
      p.y = parseDouble(t[1]);
    if (t.size() > 2)
      p.z = parseDouble(t[2]);
    return std::any(p);
  }
  case X3DFieldType::SFVec4f: {
    auto t = tokenize(s);
    SFVec4f p{0, 0, 0, 0};
    if (t.size() > 0)
      p.x = parseFloat(t[0]);
    if (t.size() > 1)
      p.y = parseFloat(t[1]);
    if (t.size() > 2)
      p.z = parseFloat(t[2]);
    if (t.size() > 3)
      p.w = parseFloat(t[3]);
    return std::any(p);
  }
  case X3DFieldType::SFVec4d: {
    auto t = tokenize(s);
    SFVec4d p{0, 0, 0, 0};
    if (t.size() > 0)
      p.x = parseDouble(t[0]);
    if (t.size() > 1)
      p.y = parseDouble(t[1]);
    if (t.size() > 2)
      p.z = parseDouble(t[2]);
    if (t.size() > 3)
      p.w = parseDouble(t[3]);
    return std::any(p);
  }
  case X3DFieldType::SFRotation: {
    auto t = tokenize(s);
    SFRotation r{0, 0, 1, 0};
    if (t.size() > 0)
      r.x = parseFloat(t[0]);
    if (t.size() > 1)
      r.y = parseFloat(t[1]);
    if (t.size() > 2)
      r.z = parseFloat(t[2]);
    if (t.size() > 3)
      r.angle = parseFloat(t[3]);
    return std::any(r);
  }
  case X3DFieldType::MFBool: {
    auto t = tokenize(s);
    std::vector<bool> vec;
    for (auto &x : t)
      vec.push_back(x == "true" || x == "TRUE" || x == "1");
    return std::any(vec);
  }
  case X3DFieldType::MFInt32: {
    auto t = tokenize(s);
    std::vector<int> vec;
    for (auto &x : t)
      vec.push_back(parseInt(x));
    return std::any(vec);
  }
  case X3DFieldType::MFFloat: {
    auto t = tokenize(s);
    std::vector<float> vec;
    for (auto &x : t)
      vec.push_back(parseFloat(x));
    return std::any(vec);
  }
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFTime: {
    auto t = tokenize(s);
    std::vector<double> vec;
    for (auto &x : t)
      vec.push_back(parseDouble(x));
    return std::any(vec);
  }
  case X3DFieldType::MFString:
    return std::any(parseMFString(s));
  case X3DFieldType::MFColor: {
    auto t = tokenize(s);
    std::vector<SFColor> vec;
    for (std::size_t i = 0; i + 2 < t.size() + 1 && i + 3 <= t.size(); i += 3)
      vec.push_back(
          {parseFloat(t[i]), parseFloat(t[i + 1]), parseFloat(t[i + 2])});
    return std::any(vec);
  }
  case X3DFieldType::MFColorRGBA: {
    auto t = tokenize(s);
    std::vector<SFColorRGBA> vec;
    for (std::size_t i = 0; i + 4 <= t.size(); i += 4)
      vec.push_back({parseFloat(t[i]), parseFloat(t[i + 1]),
                     parseFloat(t[i + 2]), parseFloat(t[i + 3])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec2f: {
    auto t = tokenize(s);
    std::vector<SFVec2f> vec;
    for (std::size_t i = 0; i + 2 <= t.size(); i += 2)
      vec.push_back({parseFloat(t[i]), parseFloat(t[i + 1])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec2d: {
    auto t = tokenize(s);
    std::vector<SFVec2d> vec;
    for (std::size_t i = 0; i + 2 <= t.size(); i += 2)
      vec.push_back({parseDouble(t[i]), parseDouble(t[i + 1])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec3f: {
    auto t = tokenize(s);
    std::vector<SFVec3f> vec;
    for (std::size_t i = 0; i + 3 <= t.size(); i += 3)
      vec.push_back(
          {parseFloat(t[i]), parseFloat(t[i + 1]), parseFloat(t[i + 2])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec3d: {
    auto t = tokenize(s);
    std::vector<SFVec3d> vec;
    for (std::size_t i = 0; i + 3 <= t.size(); i += 3)
      vec.push_back(
          {parseDouble(t[i]), parseDouble(t[i + 1]), parseDouble(t[i + 2])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec4f: {
    auto t = tokenize(s);
    std::vector<SFVec4f> vec;
    for (std::size_t i = 0; i + 4 <= t.size(); i += 4)
      vec.push_back({parseFloat(t[i]), parseFloat(t[i + 1]),
                     parseFloat(t[i + 2]), parseFloat(t[i + 3])});
    return std::any(vec);
  }
  case X3DFieldType::MFVec4d: {
    auto t = tokenize(s);
    std::vector<SFVec4d> vec;
    for (std::size_t i = 0; i + 4 <= t.size(); i += 4)
      vec.push_back({parseDouble(t[i]), parseDouble(t[i + 1]),
                     parseDouble(t[i + 2]), parseDouble(t[i + 3])});
    return std::any(vec);
  }
  case X3DFieldType::MFRotation: {
    auto t = tokenize(s);
    std::vector<SFRotation> vec;
    for (std::size_t i = 0; i + 4 <= t.size(); i += 4)
      vec.push_back({parseFloat(t[i]), parseFloat(t[i + 1]),
                     parseFloat(t[i + 2]), parseFloat(t[i + 3])});
    return std::any(vec);
  }
  case X3DFieldType::SFMatrix3f:
    return std::any(parseMatrixF<SFMatrix3f, 3>(s));
  case X3DFieldType::SFMatrix4f:
    return std::any(parseMatrixF<SFMatrix4f, 4>(s));
  case X3DFieldType::SFMatrix3d:
    return std::any(parseMatrixD<SFMatrix3d, 3>(s));
  case X3DFieldType::SFMatrix4d:
    return std::any(parseMatrixD<SFMatrix4d, 4>(s));
  case X3DFieldType::MFMatrix3f: {
    auto t = tokenize(s);
    std::vector<SFMatrix3f> vec;
    for (std::size_t i = 0; i + 9 <= t.size(); i += 9) {
      SFMatrix3f m{};
      for (int k = 0; k < 9; ++k)
        m.matrix[k / 3][k % 3] = parseFloat(t[i + k]);
      vec.push_back(m);
    }
    return std::any(vec);
  }
  case X3DFieldType::MFMatrix4f: {
    auto t = tokenize(s);
    std::vector<SFMatrix4f> vec;
    for (std::size_t i = 0; i + 16 <= t.size(); i += 16) {
      SFMatrix4f m{};
      for (int k = 0; k < 16; ++k)
        m.matrix[k / 4][k % 4] = parseFloat(t[i + k]);
      vec.push_back(m);
    }
    return std::any(vec);
  }
  case X3DFieldType::MFMatrix3d: {
    auto t = tokenize(s);
    std::vector<SFMatrix3d> vec;
    for (std::size_t i = 0; i + 9 <= t.size(); i += 9) {
      SFMatrix3d m{};
      for (int k = 0; k < 9; ++k)
        m.matrix[k / 3][k % 3] = parseDouble(t[i + k]);
      vec.push_back(m);
    }
    return std::any(vec);
  }
  case X3DFieldType::MFMatrix4d: {
    auto t = tokenize(s);
    std::vector<SFMatrix4d> vec;
    for (std::size_t i = 0; i + 16 <= t.size(); i += 16) {
      SFMatrix4d m{};
      for (int k = 0; k < 16; ++k)
        m.matrix[k / 4][k % 4] = parseDouble(t[i + k]);
      vec.push_back(m);
    }
    return std::any(vec);
  }
  case X3DFieldType::SFImage:
    return std::any(parseImage(s));
  case X3DFieldType::MFImage: {
    auto t = tokenize(s);
    std::vector<SFImage> vec;
    std::size_t i = 0;
    while (i < t.size())
      vec.push_back(parseImageFrom(t, i));
    return std::any(vec);
  }
  default:
    return std::any{};
  }
}

} // namespace x3d::codec
