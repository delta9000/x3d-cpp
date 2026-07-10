#ifndef X3D_ASSET_IMPORT_BACKEND_REGISTRY_HPP
#define X3D_ASSET_IMPORT_BACKEND_REGISTRY_HPP
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "import_source.hpp"

namespace x3d::asset_import {

// One selectable import backend. `priority` inspects the WHOLE input string
// (so a "fixture:" prefix and a ".glb" extension resolve through one path); it
// returns a positive score when it can handle the input, <=0 to decline.
struct ImportBackend {
  std::string name;
  std::function<int(const std::string& input)> priority;
  std::function<std::unique_ptr<ImportSource>()> make;
};

// Resolves which backend loads a given input. Pure (holds only its vector) so
// the selection/override/decline logic is unit-testable with fake backends,
// independent of any concrete backend's build gate.
class BackendRegistry {
public:
  void add(ImportBackend b) { backends_.push_back(std::move(b)); }

  // Highest positive priority for `input`; first-registered wins ties.
  // nullptr when no backend claims the input.
  const ImportBackend* select(const std::string& input) const {
    const ImportBackend* best = nullptr;
    int bestP = 0;
    for (const auto& b : backends_) {
      int p = b.priority(input);
      if (p > bestP) {
        bestP = p;
        best = &b;
      }
    }
    return best;
  }

  // Exact name match, ignoring priority (the --backend override path).
  const ImportBackend* byName(const std::string& name) const {
    for (const auto& b : backends_)
      if (b.name == name) return &b;
    return nullptr;
  }

  std::vector<std::string> names() const {
    std::vector<std::string> out;
    for (const auto& b : backends_) out.push_back(b.name);
    return out;
  }

private:
  std::vector<ImportBackend> backends_;
};

} // namespace x3d::asset_import
#endif
