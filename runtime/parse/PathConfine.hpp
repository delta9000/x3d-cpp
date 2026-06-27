// PathConfine.hpp
// SEC-3 / SEC-4: confine a file-like Inline/EXTERNPROTO url to the directory
// subtree of the file currently being parsed, and canonicalize it.
//
// The default local resolvers (localFileProtoResolver / localFileInlineResolver
// in X3DParse.hpp) used to build a path with `baseUrl + "/" + url` and read it
// verbatim — so an absolute url (`/etc/passwd`) or a traversal (`../../etc/
// passwd`) escaped the scene directory and read arbitrary files (SEC-3). They
// also keyed their cycle guard on that raw string, so two spellings of one file
// (`./a.x3d` vs `a.x3d`) aliased past the guard (SEC-4).
//
// confineLocalIncludePath() fixes both at the source: it resolves `url` against
// `baseDir`, rejects absolute urls and any result that escapes `confineRoot`,
// and returns the canonical path otherwise. The canonical form de-aliases
// spellings, so it is also a stable cycle-guard key.
//
// The confinement root is configurable (ADR-0038): the default parse path roots
// it at the top-level file's own directory (a tight, secure default — `../`
// cross-directory refs are blocked), while a tool/embedder parsing a trusted
// content tree passes a broader root so `../` refs that stay within the tree
// resolve. Either way absolute urls and escapes above the root are rejected;
// non-filesystem sources (network/VFS) are the resolver-seam's job.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_PATH_CONFINE_HPP
#define X3D_PATH_CONFINE_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace x3d::codec {

// Resolve `url` (fragment already stripped, not http/https/urn) against `baseDir`
// (the directory of the file being parsed) and confine the canonical result to
// `confineRoot`'s subtree. Returns the canonical filesystem path on success, or
// std::nullopt if the url is absolute, escapes the root, or cannot be resolved.
inline std::optional<std::string>
confineLocalIncludePath(const std::string &url, const std::string &baseDir,
                        const std::string &confineRoot) {
  namespace fs = std::filesystem;
  std::error_code ec;

  if (url.empty())
    return std::nullopt;
  // The default local resolver never reads by absolute path.
  if (fs::path(url).is_absolute())
    return std::nullopt;

  fs::path base = baseDir.empty() ? fs::current_path(ec) : fs::path(baseDir);
  if (ec)
    return std::nullopt;
  // The confinement boundary; falls back to `base` when no explicit root is set
  // (a direct parseDocument with only a baseUrl → tight per-source confinement).
  fs::path root =
      confineRoot.empty() ? base : fs::path(confineRoot);
  root = fs::weakly_canonical(root, ec);
  if (ec)
    return std::nullopt;

  // weakly_canonical resolves `.`/`..` and de-aliases spellings even when the
  // target file does not exist (it canonicalizes the existing prefix and
  // lexically-normalizes the rest). Resolve the url against the source dir.
  fs::path cand = fs::weakly_canonical(fs::weakly_canonical(base, ec) / fs::path(url), ec);
  if (ec)
    return std::nullopt;

  // Confine: `cand` must be `root` itself or strictly under it. lexically_relative
  // returns "" when unrelated, or a path whose first component is ".." on escape.
  fs::path rel = cand.lexically_relative(root);
  if (rel.empty() || *rel.begin() == "..")
    return std::nullopt;

  return cand.string();
}

} // namespace x3d::codec

#endif // X3D_PATH_CONFINE_HPP
