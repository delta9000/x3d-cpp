// Encoding detection and dispatch for X3D documents and files.
#ifndef X3D_PARSE_HPP
#define X3D_PARSE_HPP

#include "../InlineResolver.hpp"
#include "Encoding.hpp"
#include "X3DProtoResolver.hpp"
#include "X3DReader.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace x3d::codec {

/// Shared by all synchronous nested parseFile/parseDocument calls, including
/// custom resolvers. Counts the top-level document in depth. Input bytes include
/// every attempted file read and memory document; gzip additionally charges its
/// decoded bytes. Limits bound input work, not the resulting scene's heap size.
/// Only the outermost call chooses limits; each independent call resets them.
/// Exhaustion throws ParseResourceLimitError, never a recoverable URL warning.
struct ParseLimits {
  std::size_t maxDocumentDepth = 64;
  std::size_t maxInputBytes = 256u * 1024u * 1024u;
};

/// Construct the reader for a known encoding, or null for Unknown.
std::unique_ptr<X3DReader> makeReader(Encoding enc);

namespace detail {
/// Confinement root for the default local resolvers (ADR-0038). The
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
/// the target, return its Scene. Like localFileProtoResolver above, resolved
/// paths are confined to the SEC-3 confinement root (absolute urls and `../`
/// escapes above the root are rejected, and the canonical path doubles as the
/// cycle-guard key). http/https/urn skipped (embedder territory). A
/// thread_local active-file stack makes self-referential / indirect Inline
/// loops terminate (ISO 19775-1 §9.4.2: browsers shall not honor such loops).
std::shared_ptr<runtime::Scene>
localFileInlineResolver(const std::vector<std::string> &urls,
                        const std::string &baseUrl);

/// Parse a document from in-memory text. If `hint` is Unknown the encoding is
/// resolved by content sniffing. After the document is built (and range
/// warnings collected) any captured PROTO/EXTERNPROTO instances are expanded in
/// place via `resolver`, with relative EXTERN urls resolved against `baseUrl`
/// (the directory of the source file when called through parseFile). Throws
/// std::runtime_error when the encoding cannot be determined; PROTO expansion itself is lenient (diagnostics land in
/// doc.protoWarnings). `inlineResolver` mirrors the proto resolver seam: an
/// embedder can supply a custom Inline loader (network fetch, virtual FS,
/// etc.); the default falls back to the file-local resolver. Recursive custom
/// loaders must pass both intended callbacks on every nested parse; use
/// AssetDocumentResolvers.hpp's paired assetResolversFrom for one asset policy
/// across both kinds of load, without an implicit local-file fallback.
runtime::X3DDocument parseDocument(
    const std::string &text, Encoding hint = Encoding::Unknown,
    const std::string &baseUrl = "",
    const ProtoDeclarationResolver &resolver = localFileProtoResolver,
    const runtime::InlineResolver &inlineResolver = localFileInlineResolver,
    const std::string &confineRoot = "",
    const ParseLimits &limits = {});

/// A parser for scene text created at run time (the SAI
/// Browser.createX3DFromString / createVrmlFromString services; pass it to
/// ScriptSystem::setSceneFactory). Returns the root nodes of the parsed text.
/// A fragment is accepted: XML without an <X3D> element is wrapped in one, and
/// ClassicVRML without a header line gets `#X3D V4.0 utf8`. PROTOs declared in
/// the text are expanded; Inline and EXTERNPROTO urls are not fetched. The
/// text's ROUTEs, IMPORT/EXPORT and Script author fields are not carried over.
/// Throws std::runtime_error when the text yields no nodes.
std::function<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>(const std::string &)>
makeStringSceneFactory();

/// Read a file, sniff its encoding from path + content (content wins when
/// confident), and parse it. gzip-compressed input (gzip magic 0x1f 0x8b) is
/// inflated in memory before sniffing, so a `.wrl.gz` / `.x3d.gz` / `.x3dv.gz`
/// payload parses transparently. Throws std::runtime_error if the file cannot
/// be opened, the gzip stream is corrupt, or its encoding cannot be determined.
///
/// `confineRoot` bounds where the default Inline/EXTERNPROTO resolvers may read
/// (ADR-0038). Empty (the default) confines includes to this file's own
/// directory — a tight, secure default that blocks `../` cross-directory reads.
/// A tool parsing a trusted content tree passes the tree root so that `../`
/// references which stay within it resolve; absolute paths and escapes above
/// the root are rejected either way. Only the outermost parseFile establishes
/// the root; nested re-parses driven by the resolvers inherit it.
runtime::X3DDocument parseFile(const std::string &path,
                             const std::string &confineRoot = "",
                             const ParseLimits &limits = {});

} // namespace x3d::codec

#endif // X3D_PARSE_HPP
