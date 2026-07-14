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
// Public declarations, namespace x3d::codec. Front-door implementations are
// compiled in x3d_cpp_runtime.
#ifndef X3D_PARSE_HPP
#define X3D_PARSE_HPP

#include "../InlineExpand.hpp"
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
/// already skip a BOM internally (see Encoding.hpp skipBomAndSpace), so
/// removing it first is idempotent.
void stripUtf8Bom(std::string &s);

/// Construct the reader for a known encoding, or null for Unknown. Throws
/// std::runtime_error for an encoding whose reader has not landed yet (so the
/// failure names the gap rather than silently dropping the document).
std::unique_ptr<X3DReader> makeReader(Encoding enc);

// Forward declarations: parseDocument's default resolver is
// localFileProtoResolver, which calls parseFile, which calls parseDocument — a
// definition cycle broken by declaring the latter two ahead of parseDocument's
// signature.
runtime::X3DDocument parseFile(const std::string &path,
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

std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl);
std::shared_ptr<runtime::Scene>
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
/// embedder can supply a custom Inline loader (network fetch, virtual FS,
/// etc.); the default falls back to the file-local resolver.
runtime::X3DDocument parseDocument(
    const std::string &text, Encoding hint = Encoding::Unknown,
    const std::string &baseUrl = "",
    const ProtoDeclarationResolver &resolver = localFileProtoResolver,
    const runtime::InlineResolver &inlineResolver = localFileInlineResolver,
    const std::string &confineRoot = "");

/// Default EXTERNPROTO resolver: resolves file-like urls relative to `baseUrl`,
/// parses the target, and returns its matching ProtoDeclaration. http/https/urn
/// urls are skipped (embedder-override territory). Lenient: a
/// missing/unparsable candidate is skipped rather than thrown. A thread_local
/// active-file stack bounds cross-file EXTERN cycles (A.x3d#P -> B.x3d#Q ->
/// A.x3d#P ...).
std::shared_ptr<runtime::ProtoDeclaration>
localFileProtoResolver(const std::vector<std::string> &urls,
                       const std::string &baseUrl);

/// Default Inline resolver: resolve file-like urls relative to `baseUrl`, parse
/// the target, return its Scene. http/https/urn skipped (embedder territory).
/// A thread_local active-file stack makes self-referential / indirect Inline
/// loops terminate (ISO 19775-1 §9.4.2: browsers shall not honor such loops).
std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl);

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
runtime::X3DDocument parseFile(const std::string &path,
                               const std::string &confineRoot);

} // namespace x3d::codec

#endif // X3D_PARSE_HPP
