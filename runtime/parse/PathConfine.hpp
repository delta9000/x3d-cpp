// Canonicalize include paths before checking confinement and cycle identity.
// Absolute URLs and paths outside the configured root are rejected. The default
// root is the top-level file's directory; callers may select a wider trusted tree.
// See docs/wiki/explanation/decisions/0038-local-resolver-path-confinement.md (ADR-0038).

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
