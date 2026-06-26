// X3DEventGraph.hpp
// The ROUTE table: forward fan-out map from a source field endpoint to its sinks.
#ifndef X3D_RUNTIME_EVENT_GRAPH_HPP
#define X3D_RUNTIME_EVENT_GRAPH_HPP

#include "X3DFieldAddress.hpp"
#include "X3DNode.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

/// Resolve inputOutput field aliases per ISO/IEC 19775-1 §4.4.2.2.
/// An inputOutput field named `xxx` can be referred to as `set_xxx` (input side)
/// or `xxx_changed` (output side).  Returns the base name when the alias maps
/// to an inputOutput field, otherwise returns `name` unchanged.
inline std::string resolveFieldAlias(const X3DNode *node,
                                     const std::string &name) {
  if (!node) return name;

  // Fast path: exact match already exists.
  for (const auto &f : node->fields()) {
    if (f.x3dName == name) return name;
  }

  // set_xxx -> xxx (input side alias)
  if (name.rfind("set_", 0) == 0) {
    std::string base = name.substr(4);
    for (const auto &f : node->fields()) {
      if (f.x3dName == base && f.access == AccessType::InputOutput) return base;
    }
  }

  // xxx_changed -> xxx (output side alias)
  if (name.size() > 8 &&
      name.compare(name.size() - 8, 8, "_changed") == 0) {
    std::string base = name.substr(0, name.size() - 8);
    for (const auto &f : node->fields()) {
      if (f.x3dName == base && f.access == AccessType::InputOutput) return base;
    }
  }

  return name;
}

/**
 * @brief Directed routing graph of event ROUTEs, keyed by source field.
 * @details Each source `(node, field)` maps to the ordered list of sink
 *          endpoints it routes to (fan-out). Forward propagation only needs the
 *          source->sinks direction. Duplicate routes are ignored (X3D treats a
 *          repeated identical ROUTE as a no-op).
 */
class EventGraph {
public:
  void addRoute(const FieldAddress &from, const FieldAddress &to) {
    FieldAddress normFrom{from.node, resolveFieldAlias(from.node, from.field)};
    FieldAddress normTo{to.node, resolveFieldAlias(to.node, to.field)};
    auto &sinks = routes_[normFrom];
    for (const auto &existing : sinks) {
      if (existing == normTo) {
        return; // duplicate ROUTE: ignore
      }
    }
    sinks.push_back(normTo);
  }

  /**
   * @brief Remove the ROUTE from `from` to `to`, if present (else a no-op).
   * @details Mirrors addRoute for dynamic rerouting (X3D 19775-1 §4.3.7,
   *          Scripting/SAI deleteRoute). When the last sink for a source is
   *          removed the source key is erased so routeCount stays exact.
   */
  void removeRoute(const FieldAddress &from, const FieldAddress &to) {
    FieldAddress normFrom{from.node, resolveFieldAlias(from.node, from.field)};
    FieldAddress normTo{to.node, resolveFieldAlias(to.node, to.field)};
    auto it = routes_.find(normFrom);
    if (it == routes_.end()) {
      return;
    }
    auto &sinks = it->second;
    for (auto i = sinks.begin(); i != sinks.end(); ++i) {
      if (*i == normTo) {
        sinks.erase(i);
        break;
      }
    }
    if (sinks.empty()) {
      routes_.erase(it);
    }
  }

  /**
   * @brief Sinks routed from a source endpoint, or an empty list if none.
   */
  const std::vector<FieldAddress> &sinks(const FieldAddress &from) const {
    static const std::vector<FieldAddress> empty;
    FieldAddress norm{from.node, resolveFieldAlias(from.node, from.field)};
    auto it = routes_.find(norm);
    return it == routes_.end() ? empty : it->second;
  }

  std::size_t routeCount() const {
    std::size_t n = 0;
    for (const auto &kv : routes_) {
      n += kv.second.size();
    }
    return n;
  }

  /**
   * @brief Remove all routes from the graph.
   * @details Call this before the nodes referenced by existing routes are
   *          destroyed, or when switching to a new scene, to avoid dangling
   *          raw pointers in the internal route table.
   */
  void clear() { routes_.clear(); }

private:
  std::unordered_map<FieldAddress, std::vector<FieldAddress>> routes_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_EVENT_GRAPH_HPP
