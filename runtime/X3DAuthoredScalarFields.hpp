#ifndef X3D_RUNTIME_AUTHORED_SCALAR_FIELDS_HPP
#define X3D_RUNTIME_AUTHORED_SCALAR_FIELDS_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {

// Field presence belongs to a node's shared identity, not its address: a
// temporary node may be destroyed and a later node may reuse that address.
// Weak keys keep the marks queryable while the graph owns the node without
// extending the graph's lifetime.
class AuthoredScalarFields {
public:
  void record(const std::shared_ptr<nodes::X3DNode> &node,
              const std::string &field) {
    if (node && !field.empty())
      fields_[node].insert(field);
  }

  bool contains(const std::shared_ptr<nodes::X3DNode> &node,
                const std::string &field) const {
    if (!node) return false;
    const auto it = fields_.find(node);
    return it != fields_.end() && it->second.count(field) != 0;
  }

  void copyClonesTo(
      const std::unordered_map<const nodes::X3DNode *,
                               std::shared_ptr<nodes::X3DNode>> &clones,
      AuthoredScalarFields &destination) const {
    for (const auto &[weakNode, names] : fields_) {
      const auto source = weakNode.lock();
      if (!source) continue;
      const auto clone = clones.find(source.get());
      if (clone == clones.end()) continue;
      for (const auto &field : names)
        destination.record(clone->second, field);
    }
  }

private:
  std::map<std::weak_ptr<nodes::X3DNode>, std::unordered_set<std::string>,
           std::owner_less<std::weak_ptr<nodes::X3DNode>>> fields_;
};

} // namespace x3d::runtime

#endif
