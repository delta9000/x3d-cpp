// Encoding.hpp
// X3D encoding identification for the unified parsing front-end.
//
// Two independent sniffers and a combiner let a caller hand over a path, the
// file bytes, or both, and get back which of the four X3D encodings the data
// is. Content sniffing wins over extension when it yields a confident answer
// (so mislabeled / extensionless files and `.wrl` files that are really X3D
// ClassicVRML still classify correctly); otherwise the extension is the
// fallback. gzip is only *detected* here — the caller must inflate first.
//
// Header-only, namespace x3d::codec. No dependencies beyond the standard
// library; the readers themselves live in sibling headers.
#ifndef X3D_PARSE_ENCODING_HPP
#define X3D_PARSE_ENCODING_HPP

#include <cctype>
#include <string>
#include <string_view>

namespace x3d::codec {

/// Identifies an X3D encoding for sniffing and reader dispatch.
enum class Encoding { Unknown, XML, ClassicVRML, VRML97, JSON };

// ---------------------------------------------------------------------------
// Helpers.
// ---------------------------------------------------------------------------

/// Lowercase a copy of an ASCII string_view (used for extension matching).
inline std::string toLowerAscii(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s)
    out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

/// Skip a leading UTF-8 BOM and ASCII whitespace; return the remaining view.
inline std::string_view skipBomAndSpace(std::string_view s) {
  // UTF-8 BOM: EF BB BF.
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF)
    s.remove_prefix(3);
  std::size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0)
    ++i;
  s.remove_prefix(i);
  return s;
}

// ---------------------------------------------------------------------------
// Extension sniffing.
// ---------------------------------------------------------------------------

/// Map a file path's extension to an encoding (case-insensitive):
/// `.x3d`->XML, `.x3dv`->ClassicVRML, `.wrl`->VRML97, `.json`->JSON.
/// A `.gz`/`.gzip` suffix is stripped first (e.g. `foo.wrl.gz` -> VRML97).
/// Returns Encoding::Unknown for anything else.
inline Encoding sniffByExtension(std::string_view path) {
  std::string lower = toLowerAscii(path);
  // Strip a trailing gzip suffix so a compressed file classifies by its real
  // extension (the caller still has to inflate the bytes).
  auto endsWith = [&](std::string_view suf) {
    return lower.size() >= suf.size() &&
           lower.compare(lower.size() - suf.size(), suf.size(), suf) == 0;
  };
  if (endsWith(".gz"))
    lower.resize(lower.size() - 3);
  else if (endsWith(".gzip"))
    lower.resize(lower.size() - 5);

  auto ext = [&](std::string_view suf) {
    return lower.size() >= suf.size() &&
           lower.compare(lower.size() - suf.size(), suf.size(), suf) == 0;
  };
  if (ext(".x3dv") || ext(".x3dvz"))
    return Encoding::ClassicVRML;
  if (ext(".x3d") || ext(".x3dz"))
    return Encoding::XML;
  if (ext(".wrl") || ext(".vrml"))
    return Encoding::VRML97;
  if (ext(".json"))
    return Encoding::JSON;
  return Encoding::Unknown;
}

// ---------------------------------------------------------------------------
// Content sniffing.
// ---------------------------------------------------------------------------

/// Returns true if `bytes` begins with the gzip magic (0x1f 0x8b). The front
/// end does not inflate; this lets a caller detect a compressed payload.
inline bool isGzip(std::string_view bytes) {
  return bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0x1f &&
         static_cast<unsigned char>(bytes[1]) == 0x8b;
}

/// Classify by inspecting the leading bytes (after BOM/whitespace). See the
/// table in the parsing-frontend spec. gzip payloads return Unknown (the
/// caller must inflate first; use isGzip() to detect them).
inline Encoding sniffByContent(std::string_view raw) {
  if (isGzip(raw))
    return Encoding::Unknown; // compressed: caller inflates, then re-sniffs
  std::string_view s = skipBomAndSpace(raw);
  if (s.empty())
    return Encoding::Unknown;

  auto startsWith = [&](std::string_view p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
  };

  // VRML / ClassicVRML headers begin with '#'.
  if (startsWith("#VRML V2.0"))
    return Encoding::VRML97;
  if (startsWith("#VRML V1.0"))
    return Encoding::VRML97; // best-effort; readers may warn
  if (startsWith("#X3D V3") || startsWith("#X3D V4"))
    return Encoding::ClassicVRML;
  if (startsWith("#VRML")) // unknown VRML flavor: treat as VRML97 best-effort
    return Encoding::VRML97;

  // XML.
  if (startsWith("<?xml") || startsWith("<X3D") ||
      startsWith("<!DOCTYPE X3D") || startsWith("<!DOCTYPE x3d"))
    return Encoding::XML;

  // JSON: first non-ws char is '{' and the document mentions the "X3D" key.
  if (s.front() == '{' && raw.find("\"X3D\"") != std::string_view::npos)
    return Encoding::JSON;

  return Encoding::Unknown;
}

// ---------------------------------------------------------------------------
// Combined sniffing.
// ---------------------------------------------------------------------------

/// Combine extension and content sniffing. Content wins whenever it yields a
/// confident (non-Unknown) answer; otherwise the extension is used. This makes
/// a mislabeled file (e.g. an X3D ClassicVRML payload named `.wrl`) classify by
/// what it actually contains.
inline Encoding sniff(std::string_view path, std::string_view bytes) {
  Encoding byContent = sniffByContent(bytes);
  if (byContent != Encoding::Unknown)
    return byContent;
  return sniffByExtension(path);
}

} // namespace x3d::codec

#endif // X3D_PARSE_ENCODING_HPP
