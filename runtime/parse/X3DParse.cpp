#include "X3DParse.hpp"

#include "ClassicVrmlReader.hpp"
#include "Inflate.hpp"
#include "JsonReader.hpp"
#include "PathConfine.hpp"
#include "Vrml97Reader.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DRangeValidate.hpp"
#include "XmlReaderAdapter.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace x3d::codec {

void stripUtf8Bom(std::string &s) {
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF)
    s.erase(0, 3);
}

std::unique_ptr<X3DReader> makeReader(Encoding enc) {
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

runtime::X3DDocument
parseDocument(const std::string &text, Encoding hint,
              const std::string &baseUrl,
              const ProtoDeclarationResolver &resolver,
              const runtime::InlineResolver &inlineResolver,
              const std::string &confineRoot) {
  // Establish the SEC-3 confinement root (ADR-0038) for the default resolvers
  // on the outermost entry; nested re-parses inherit it. When reached via
  // parseFile the root is already set (so this is a no-op); a direct
  // parseDocument call from a tool handling a trusted content tree passes the
  // tree root here.
  const bool outermostConfine = detail::activeConfineRoot().empty();
  struct ConfineRootScope {
    bool owns;
    ~ConfineRootScope() {
      if (owns)
        detail::activeConfineRoot().clear();
    }
  } confineScope{outermostConfine};
  if (outermostConfine && !(confineRoot.empty() && baseUrl.empty()))
    detail::activeConfineRoot() = !confineRoot.empty() ? confineRoot : baseUrl;
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

std::shared_ptr<runtime::ProtoDeclaration>
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

std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl) {
  static thread_local std::vector<std::string> activeFiles;
  for (const std::string &u : urls) {
    std::string url = u;
    if (auto h = url.find('#'); h != std::string::npos)
      url = url.substr(0, h);
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
      runtime::X3DDocument sub =
          parseFile(path); // recurses (expands sub-Inlines)
      result = std::make_shared<runtime::Scene>(std::move(sub.scene));
    } catch (const std::exception &) {
      // lenient: try the next candidate url
    }
    activeFiles.pop_back();
    if (result)
      return result;
  }
  return nullptr;
}

runtime::X3DDocument parseFile(const std::string &path,
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
  // per-file default) unless a trusted caller widened it. `.` when base is
  // empty (a bare filename) so the root is never the empty "fall back to
  // baseUrl" key.
  if (outermostConfine)
    detail::activeConfineRoot() =
        !confineRoot.empty() ? confineRoot
                             : (base.empty() ? std::string(".") : base);
  return parseDocument(bytes, enc, base);
}

} // namespace x3d::codec
