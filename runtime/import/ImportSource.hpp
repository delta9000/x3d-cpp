// ImportSource.hpp
// The mesh-import backend seam: an ImportSource reads one file into an
// ImportScene; a BackendRegistry picks the source for an input path.
#ifndef X3D_RUNTIME_IMPORT_IMPORT_SOURCE_HPP
#define X3D_RUNTIME_IMPORT_IMPORT_SOURCE_HPP

#include "ImportScene.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace x3d::runtime::importer {

/// Reads one file. Throws std::runtime_error with a readable message when the
/// file cannot be read or uses something the backend must refuse (for
/// example a required glTF extension it does not implement).
struct ImportSource {
  virtual ImportScene load(const std::string &path) = 0;
  virtual ~ImportSource() = default;
};

/// One selectable backend. `priority` returns a positive score for an input it
/// can read and <= 0 to decline; the highest score wins.
struct ImportBackend {
  std::string name;
  std::vector<std::string> extensions; ///< lower case with the dot, for help text
  std::function<int(const std::string &path)> priority;
  std::function<std::unique_ptr<ImportSource>()> make;
};

/// Lower-cased extension of `path` including the dot ("" when none).
inline std::string lowerExtension(const std::string &path) {
  const auto slash = path.find_last_of("/\\");
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    return "";
  std::string e = path.substr(dot);
  std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return e;
}

/// True when `path` ends in one of `exts` (lower case, with the dot).
inline bool hasExtension(const std::string &path,
                         std::initializer_list<const char *> exts) {
  const std::string e = lowerExtension(path);
  for (const char *x : exts)
    if (e == x) return true;
  return false;
}

class BackendRegistry {
public:
  void add(ImportBackend b) { backends_.push_back(std::move(b)); }

  /// Highest positive priority for `path`; the first registered wins ties.
  /// nullptr when no backend claims it.
  const ImportBackend *select(const std::string &path) const {
    const ImportBackend *best = nullptr;
    int bestP = 0;
    for (const auto &b : backends_) {
      const int p = b.priority ? b.priority(path) : 0;
      if (p > bestP) {
        bestP = p;
        best = &b;
      }
    }
    return best;
  }

  /// Exact name match, ignoring priority.
  const ImportBackend *byName(const std::string &name) const {
    for (const auto &b : backends_)
      if (b.name == name) return &b;
    return nullptr;
  }

  const std::vector<ImportBackend> &backends() const { return backends_; }

private:
  std::vector<ImportBackend> backends_;
};

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_IMPORT_SOURCE_HPP
