// X3DParse.hpp
// Umbrella + front door for the unified X3D parsing layer.
//
// Pulls together the front-end interface (X3DReader), encoding sniffing
// (Encoding), the shared node-agnostic build core (NodeBuilder) and lexer
// (VrmlTokenizer), and the concrete readers. As later workstreams land
// (ClassicVrmlReader, Vrml97Reader, JsonReader) their headers are added here
// and wired into makeReader().
//
// For WS-A the only concrete reader is the XmlReaderAdapter, which wraps the
// existing codecs/XmlReader. parseDocument()/parseFile() sniff and dispatch;
// encodings whose reader has not landed yet throw a clear std::runtime_error.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_PARSE_HPP
#define X3D_PARSE_HPP

#include "ClassicVrmlReader.hpp"
#include "Encoding.hpp"
#include "Inflate.hpp"
#include "JsonReader.hpp"
#include "NodeBuilder.hpp"
#include "PathConfine.hpp"
#include "Vrml97Dialect.hpp"
#include "Vrml97Reader.hpp"
#include "VrmlTokenizer.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DProtoResolver.hpp"
#include "../InlineExpand.hpp"
#include "X3DRangeValidate.hpp"
#include "X3DReader.hpp"
#include "XmlReaderAdapter.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace x3d::codec {

/// Strip a single leading UTF-8 BOM (EF BB BF) in place, if present. Only the
/// UTF-8 BOM is handled: it is the one whose bytes are valid leading XML text
/// once removed. UTF-16/UTF-32 BOMs (FF FE / FE FF / 00 00 FE FF) would require
/// transcoding the whole byte stream to UTF-8 and are intentionally left intact
/// here (the readers will then fail/sniff Unknown rather than be mis-stripped).
/// Stripping at the front door means every encoding/reader receives BOM-free
/// bytes; content sniffing is unaffected because sniff()/sniffByContent()
/// already skip a BOM internally (see Encoding.hpp skipBomAndSpace), so removing
/// it first is idempotent.
inline void stripUtf8Bom(std::string &s) {
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF)
    s.erase(0, 3);
}

/// Construct the reader for a known encoding, or null for Unknown. Throws
/// std::runtime_error for an encoding whose reader has not landed yet (so the
/// failure names the gap rather than silently dropping the document).
inline std::unique_ptr<X3DReader> makeReader(Encoding enc) {
  switch (enc) {
  case Encoding::XML:
    return std::make_unique<XmlReaderAdapter>();
  case Encoding::ClassicVRML:
    return std::make_unique<ClassicVrmlReader>();
  case Encoding::VRML97:
    return std::make_unique<Vrml97Reader>();
  case Encoding::JSON:
    return std::make_unique<JsonReader>();
  case Encoding::Unknown:
  default:
    return nullptr;
  }
}

// Forward declarations: parseDocument's default resolver is localFileProtoResolver,
// which calls parseFile, which calls parseDocument — a definition cycle broken by
// declaring the latter two ahead of parseDocument's signature.
inline runtime::X3DDocument parseFile(const std::string &path,
                                      const std::string &confineRoot = "");
namespace detail {
/// Confinement root for the default local resolvers (ADR-0038, SEC-3). The
/// outermost parseFile/parseDocument sets it (default: the top-level file's own
/// directory; a trusted tool may widen it); nested re-parses driven by the
/// resolvers inherit it, so `../` confinement is measured against one stable
/// root rather than narrowing per hop. Empty when parsing from memory with no
/// base — the confine helper then falls back to the per-call baseUrl.
inline std::string &activeConfineRoot() {
  static thread_local std::string r;
  return r;
}
} // namespace detail

inline std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl);
inline std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl);

/// Parse a document from in-memory text. If `hint` is Unknown the encoding is
/// resolved by content sniffing. After the document is built (and range
/// warnings collected) any captured PROTO/EXTERNPROTO instances are expanded in
/// place via `resolver`, with relative EXTERN urls resolved against `baseUrl`
/// (the directory of the source file when called through parseFile). Throws
/// std::runtime_error when the encoding cannot be determined (or its reader has
/// not landed yet); PROTO expansion itself is lenient (diagnostics land in
/// doc.protoWarnings). `inlineResolver` mirrors the proto resolver seam: an
/// embedder can supply a custom Inline loader (network fetch, virtual FS, etc.);
/// the default falls back to the file-local resolver.
inline runtime::X3DDocument
parseDocument(const std::string &text, Encoding hint = Encoding::Unknown,
              const std::string &baseUrl = "",
              const ProtoDeclarationResolver &resolver = localFileProtoResolver,
              const runtime::InlineResolver &inlineResolver = localFileInlineResolver,
              const std::string &confineRoot = "") {
  // Establish the SEC-3 confinement root (ADR-0038) for the default resolvers on
  // the outermost entry; nested re-parses inherit it. When reached via parseFile
  // the root is already set (so this is a no-op); a direct parseDocument call
  // from a tool handling a trusted content tree passes the tree root here.
  const bool outermostConfine = detail::activeConfineRoot().empty();
  struct ConfineRootScope {
    bool owns;
    ~ConfineRootScope() {
      if (owns)
        detail::activeConfineRoot().clear();
    }
  } confineScope{outermostConfine};
  if (outermostConfine && !(confineRoot.empty() && baseUrl.empty()))
    detail::activeConfineRoot() =
        !confineRoot.empty() ? confineRoot : baseUrl;
  // Strip a leading UTF-8 BOM up front so every reader sees BOM-free bytes. The
  // bundled XmlLite byte-level parser does not skip a BOM (it expects '<' or
  // ASCII space at the head), so a BOM-prefixed XML document would otherwise be
  // rejected with "expected root element". Sniffing already ignores a BOM, so
  // doing this first is harmless and idempotent.
  std::string body = text;
  stripUtf8Bom(body);
  Encoding enc = hint;
  if (enc == Encoding::Unknown)
    enc = sniffByContent(body);
  std::unique_ptr<X3DReader> reader = makeReader(enc);
  if (!reader)
    throw std::runtime_error(
        "parseDocument: could not determine X3D encoding from content");
  runtime::X3DDocument doc = reader->readDocument(body);
  // Surface out-of-range values the lenient readers kept (structured channel;
  // parseFile flows through here, so this is the single collection site).
  for (const auto &root : doc.scene.rootNodes)
    if (root)
      collectRangeWarnings(*root, doc.rangeWarnings);
  // Expand captured PROTO/EXTERNPROTO instances after range-warning collection,
  // splicing primaries into their parent slots. Local PROTOs always expand;
  // EXTERN instances resolve through `resolver` (default: sibling-file lookup).
  runtime::expandScene(doc.scene, resolver, baseUrl, doc.protoWarnings);
  // Expand load=TRUE Inlines: resolve each url to a sub-scene and splice it in.
  // Mirrors the PROTO pass above; nested Inlines recurse via parseFile.
  runtime::expandInlines(doc.scene, inlineResolver, baseUrl,
                         doc.inlineWarnings);
  return doc;
}

/// Default EXTERNPROTO resolver: resolves file-like urls relative to `baseUrl`,
/// parses the target, and returns its matching ProtoDeclaration. http/https/urn
/// urls are skipped (embedder-override territory). Lenient: a missing/unparsable
/// candidate is skipped rather than thrown. A thread_local active-file stack
/// bounds cross-file EXTERN cycles (A.x3d#P -> B.x3d#Q -> A.x3d#P ...).
inline std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u, frag;
    if (auto h = url.find('#'); h != std::string::npos) {
      frag = url.substr(h + 1);
      url = url.substr(0, h);
    }
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue; // embedder override territory; default resolver stays local
    // SEC-3: confine the resolved path to the source file's directory subtree
    // (reject absolute urls and `../` escapes) — the default local resolver
    // never reads arbitrary files. SEC-4: the returned path is canonical, so it
    // is a stable cycle-guard key that spelling variants cannot alias past.
    auto confined =
        confineLocalIncludePath(url, baseUrl, detail::activeConfineRoot());
    if (!confined)
      continue; // absolute / escaping / unresolvable -> skip this candidate
    const std::string path = std::move(*confined);
    if (std::find(activeFiles.begin(), activeFiles.end(), path) !=
        activeFiles.end())
      continue; // cycle: this file is already being resolved up the stack
    activeFiles.push_back(path);
    std::shared_ptr<runtime::ProtoDeclaration> found;
    try {
      runtime::X3DDocument sub = parseFile(path);
      if (!frag.empty())
        found = sub.scene.findProto(frag);
      else if (!sub.scene.protoDeclarations.empty())
        found = sub.scene.protoDeclarations.front();
    } catch (const std::exception &) {
      // lenient: fall through to the next candidate url
    }
    activeFiles.pop_back();
    if (found)
      return found;
  }
  return nullptr;
}

/// Default Inline resolver: resolve file-like urls relative to `baseUrl`, parse
/// the target, return its Scene. http/https/urn skipped (embedder territory).
/// A thread_local active-file stack makes self-referential / indirect Inline
/// loops terminate (ISO 19775-1 §9.4.2: browsers shall not honor such loops).
inline std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u;
    if (auto h = url.find('#'); h != std::string::npos) url = url.substr(0, h);
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("urn:", 0) == 0)
      continue;
    std::string path = url;
    if (!baseUrl.empty() && !path.empty() && path.front() != '/')
      path = baseUrl + "/" + path;
    if (std::find(activeFiles.begin(), activeFiles.end(), path) !=
        activeFiles.end())
      continue; // cycle: this file is already being resolved up the stack
    activeFiles.push_back(path);
    std::shared_ptr<runtime::Scene> result;
    try {
      runtime::X3DDocument sub = parseFile(path); // recurses (expands sub-Inlines)
      result = std::make_shared<runtime::Scene>(std::move(sub.scene));
    } catch (const std::exception &) {
      // lenient: try the next candidate url
    }
    activeFiles.pop_back();
    if (result) return result;
  }
  return nullptr;
}

/// Read a file, sniff its encoding from path + content (content wins when
/// confident), and parse it. gzip-compressed input (gzip magic 0x1f 0x8b) is
/// inflated in memory before sniffing, so a `.wrl.gz` / `.x3d.gz` / `.x3dv.gz`
/// payload parses transparently. Throws std::runtime_error if the file cannot
/// be opened, the gzip stream is corrupt, or its encoding cannot be determined.
///
/// `confineRoot` bounds where the default Inline/EXTERNPROTO resolvers may read
/// (SEC-3, ADR-0038). Empty (the default) confines includes to this file's own
/// directory — a tight, secure default that blocks `../` cross-directory reads.
/// A tool parsing a TRUSTED content tree passes the tree root so that `../`
/// references which stay within it resolve; absolute paths and escapes above
/// the root are rejected either way. Only the OUTERMOST parseFile establishes
/// the root; nested re-parses driven by the resolvers inherit it.
inline runtime::X3DDocument parseFile(const std::string &path,
                                      const std::string &confineRoot) {
  // Establish the confinement root on the outermost call; inner resolver-driven
  // re-parses (activeConfineRoot already set) inherit it and restore nothing.
  const bool outermostConfine = detail::activeConfineRoot().empty();
  struct ConfineRootScope {
    bool owns;
    ~ConfineRootScope() {
      if (owns)
        detail::activeConfineRoot().clear();
    }
  } confineScope{outermostConfine};

  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("parseFile: cannot open file: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string bytes = ss.str();
  // Inflate first: sniffByExtension already strips .gz/.gzip and the inflated
  // bytes content-sniff correctly, so no further branching is needed below.
  if (isGzip(bytes))
    bytes = inflateGzip(bytes);
  // Strip a leading UTF-8 BOM before sniffing/dispatch (see stripUtf8Bom). This
  // covers every encoding at the front door; parseDocument also strips, so this
  // is belt-and-suspenders and keeps sniff(path, bytes) BOM-insensitive.
  stripUtf8Bom(bytes);
  Encoding enc = sniff(path, bytes);
  if (enc == Encoding::Unknown)
    throw std::runtime_error("parseFile: could not determine X3D encoding: " +
                             path);
  // Derive the base directory so relative EXTERNPROTO urls resolve against the
  // source file's location rather than the process cwd.
  std::string base;
  if (auto slash = path.find_last_of("/\\"); slash != std::string::npos)
    base = path.substr(0, slash);
  // Default the confinement root to this top-level file's directory (secure
  // per-file default) unless a trusted caller widened it. `.` when base is empty
  // (a bare filename) so the root is never the empty "fall back to baseUrl" key.
  if (outermostConfine)
    detail::activeConfineRoot() =
        !confineRoot.empty() ? confineRoot : (base.empty() ? std::string(".") : base);
  return parseDocument(bytes, enc, base);
}

} // namespace x3d::codec

#endif // X3D_PARSE_HPP
