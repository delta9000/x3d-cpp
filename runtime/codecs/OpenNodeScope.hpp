// Readers register a node's DEF before reading its body, so a USE inside the
// body can name an enclosing node. Attaching that node as a child, or as a node
// value of a PROTO instance nested in it, would make it contain itself: a
// shared_ptr cycle that outlives the document. Readers keep every DEF'd node
// whose body is still being read open on this thread and refuse to attach an
// open node.
#ifndef X3D_CODEC_OPEN_NODE_SCOPE_HPP
#define X3D_CODEC_OPEN_NODE_SCOPE_HPP

#include "X3DProto.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::codec {

inline std::unordered_set<const x3d::nodes::X3DNode *> &openNodes() {
  thread_local std::unordered_set<const x3d::nodes::X3DNode *> open;
  return open;
}

/// True while `node`'s own body is being read on this thread.
inline bool isOpenNode(const x3d::nodes::X3DNode *node) {
  return node && openNodes().count(node) != 0;
}

/// Marks `node` open for the lifetime of the scope. Null is a no-op, so a
/// reader can pass the node only when it has a DEF.
class OpenNodeScope {
public:
  explicit OpenNodeScope(const x3d::nodes::X3DNode *node)
      : node_(node && openNodes().insert(node).second ? node : nullptr) {}
  ~OpenNodeScope() {
    if (node_)
      openNodes().erase(node_);
  }
  OpenNodeScope(const OpenNodeScope &) = delete;
  OpenNodeScope &operator=(const OpenNodeScope &) = delete;

private:
  const x3d::nodes::X3DNode *node_;
};

/// Record the dropped USE of an enclosing node.
inline void noteEnclosingUse(std::vector<runtime::ReaderWarning> &out,
                             const x3d::nodes::X3DNode &node) {
  out.push_back({runtime::ReaderWarning::Kind::CyclicUse,
                 "USE '" + node.getDEF() + "' names an enclosing " +
                     node.nodeTypeName() +
                     " and was dropped (a node cannot contain itself)"});
}

/// Append `node` to a PROTO instance's node-typed field value unless it is
/// open (the instance sits inside it), which is recorded and dropped instead.
inline void pushNodeValue(std::vector<std::shared_ptr<x3d::nodes::X3DNode>> &values,
                          std::shared_ptr<x3d::nodes::X3DNode> node,
                          std::vector<runtime::ReaderWarning> &out) {
  if (!node)
    return;
  if (isOpenNode(node.get())) {
    noteEnclosingUse(out, *node);
    return;
  }
  values.push_back(std::move(node));
}

} // namespace x3d::codec

#endif // X3D_CODEC_OPEN_NODE_SCOPE_HPP
